
#include "simple_buffer.h"


void buffer_init(struct simple_buffer *buf)
{
	memset(buf, 0, sizeof(struct simple_buffer));
}

static size_t LengthOfString(const char * string) {
	size_t ret = 0;
	char sign;
	while ((sign = *string++) != '\0') {
		if ((sign != '\n') && (sign != '\r'))
			ret++;
	}
	return ret;
}

static inline size_t buffer_IncrementCounter(size_t counter)
{
	return (counter + 1) % BUF_MEM_SIZE;
}

int8_t buffer_get_byte(struct simple_buffer* buf, uint8_t *byte)
{
	if (buf->tail == buf->head)
		return -ENOMEM;
	*byte = buf->memory[buf->tail];
	buf->tail = buffer_IncrementCounter(buf->tail);
	return 0;
}

static inline uint8_t buffer_CheckLockFlag(struct simple_buffer *buf) 
{
	return buf->lock & 1;
}


static inline int8_t buffer_PushBuf(struct simple_buffer *buf, uint8_t byte)
{
	size_t temp;
	uint8_t acc = 0;
	temp = buffer_IncrementCounter(buf->head);
	if (buf->tail == temp) {
		buffer_get_byte(buf, &acc);
	}
	buf->memory[buf->head] = byte;
	buf->head = temp;
	return 0;
}

/*RVO??*/
static inline int8_t buffer_ClearTempBuf(struct simple_buffer *buf) 
{
	size_t cnt = buf->lock >> 1;
	int8_t ret = 0;
	for (size_t i = 0; i < cnt; i++) {
		ret = buffer_PushBuf(buf, buf->temp[i]);
		if (ret)
			break;
	}
	buf->lock = 0;
	memset(buf->temp, 0, TEMP_MEM_SIZE);
	return ret;
}

static inline int8_t buffer_PushTempBuf(struct simple_buffer *buf, 
					uint8_t byte)
{
	uint8_t head;
	head = buf->lock >> 1;
	if (head == TEMP_MEM_SIZE)
		return -ENOMEM;
	buf->temp[head++] = byte;
	buf->lock = (head << 1) | 1;
	return 0;
}

static inline void buffer_SetLock(struct simple_buffer *buf) 
{
	buf->lock |= 1;
}

static inline void buffer_ClearLock(struct simple_buffer *buf)
{
	buf->lock &= ~1;
}	

static inline int8_t buffer_IsLocked(struct simple_buffer *buf)
{
	return buf->lock & 1;
}


int8_t buffer_set_byte(struct simple_buffer* buf, uint8_t byte)
{
	int8_t ret = 0;
	if (buf->ignore) {
		buf->ignore--;
		return 0;
	} else if (buffer_CheckLockFlag(buf)) {
		ret = buffer_PushTempBuf(buf, byte);
		return ret;
	} else {
		ret = buffer_ClearTempBuf(buf);
		if (ret)
			return ret;
		ret = buffer_PushBuf(buf, byte);
		return ret;
	}
}	

/*ret equals to what if not = 0?*/
int8_t buffer_set_text(struct simple_buffer *buf, const char *text, 
		       size_t data_size) 
{
	int8_t ret = 0;
	if (buf->head >= buf->tail) {
		if (data_size >= BUF_MEM_SIZE - (buf->head - buf->tail)) 
			return -ENOMEM;
	} else if (buf-> head < buf->tail) {
		if (data_size >= BUF_MEM_SIZE - (buf->tail - buf->head))
			return -ENOMEM;
	}
	for (size_t i = 0; i < data_size; i++) {
		ret = buffer_set_byte(buf, text[i]);
		if (ret)
			break;
	}
	return ret;
}

int8_t buffer_IsFull(struct simple_buffer *buf) 
{
	size_t temp = buffer_IncrementCounter(buf->head);
	if (buf->tail == temp)
		return -ENOMEM;
	else
		return 0;
}

int8_t buffer_IsEmpty(struct simple_buffer *buf)
{
	if (buf->tail == buf->head)
		return 0;
	return
		-EINVAL;
}

static int8_t buffer_MoveTailToLabel(struct simple_buffer *buf, const char *label)
{
	uint8_t byte;
	int8_t ret;
	size_t cnt = 0;
	const size_t LENGTH = LengthOfString(label);
	size_t tail_old = buf->tail;
	
	while (buf->tail != buf->head) {
		ret = buffer_get_byte(buf, &byte);
		if (ret == -ENOMEM) {
			buf->tail = tail_old;
			return -EBUSY;
		}
		if (label[cnt] == byte) {
			if (cnt == LENGTH - 1)
				return 0;
			else 
				cnt++;
		} else {
			cnt = 0;
		}
	}	
	buf->tail = tail_old;
	return -EINVAL;
}

static int8_t buffer_CopyToNearestWord(struct simple_buffer *buf, char *output,
				const char *word)
{
	uint8_t byte = 0;
	const size_t len = LengthOfString(word);
	size_t cnt = 0;
	int8_t ret;
	size_t tail_old = buf->tail;
	while(!(ret = buffer_get_byte(buf, &byte))) {
		*output = byte;
		if (*output == word[len - 1]) {
			for (cnt = 0; cnt < len; cnt++) {
				if (*(output - cnt) != word[len - 1 - cnt])
					break;
			}
			if (cnt == len) {
				for (int i = len - 1; i >= 0; i--)
					*output-- = '\0';
				return 0;
			}
		}
		output++;
	}
	buf->tail = tail_old;
	if (ret == -ENOMEM)
		ret = -EBUSY;
	return ret;
}


int8_t buffer_SearchGetLabel(struct simple_buffer *buf, const char *label, 
			     const char *limiter, char *output)
{	
	int8_t ret;
	size_t tail_old;
	if (buffer_IsLocked(buf))
		return -EBUSY;
	buffer_SetLock(buf);
	tail_old = buf->tail;
	ret = buffer_MoveTailToLabel(buf, label);
	if (ret) {
		buffer_ClearLock(buf);
		return ret;
	}
	if (output)
		ret = buffer_CopyToNearestWord(buf, output, limiter);
	else
		ret = buffer_MoveTailToLabel(buf, limiter);

	if (ret)
		buf->tail = tail_old;
	buffer_ClearLock(buf);
	return ret;
}

void buffer_Reset(struct simple_buffer *buf) 
{
	memset(buf, 0, sizeof(struct simple_buffer));
}

inline void buffer_SetIgnore(struct simple_buffer *buf, size_t ignore)
{
	buf->ignore = ignore;
}
