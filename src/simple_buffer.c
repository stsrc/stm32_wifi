
#include "simple_buffer.h"


void buffer_init(struct simple_buffer *buf)
{
	memset(buf, 0, sizeof(struct simple_buffer));
}

static size_t LengthOfString(const char * string, int8_t ignore_rn) {
	size_t ret = 0;
	char sign;
	while ((sign = *string++) != '\0') {
		if (!ignore_rn || ((sign != '\n') && (sign != '\r')))
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

int8_t buffer_set_byte(struct simple_buffer* buf, uint8_t byte)
{
	int8_t ret = 0;

	ret = buffer_PushBuf(buf, byte);
	return ret;
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

static int8_t buffer_MoveTailToLabel(struct simple_buffer *buf, const char *label, int8_t ignore_rn)
{
	uint8_t byte;
	int8_t ret;
	size_t cnt = 0;
	const size_t LENGTH = LengthOfString(label, ignore_rn);
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
				const char *word, int8_t ignore_rn)
{
	uint8_t byte = 0;
	const size_t len = LengthOfString(word, ignore_rn);
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
			     const char *limiter, char *output, int8_t ignore_rn)
{
	int8_t ret;
	size_t tail_old;
	tail_old = buf->tail;
	ret = buffer_MoveTailToLabel(buf, label, ignore_rn);
	if (ret) {
		return ret;
	}
	if (output)
		ret = buffer_CopyToNearestWord(buf, output, limiter, ignore_rn);
	else
		ret = buffer_MoveTailToLabel(buf, limiter, ignore_rn);

	if (ret)
		buf->tail = tail_old;
	return ret;
}

void buffer_Reset(struct simple_buffer *buf)
{
	memset(buf, 0, sizeof(struct simple_buffer));
}
