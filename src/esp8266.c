#include <stdio.h>
#include "esp8266.h"
#include "secret-password.h"

#define ESP8266_RST_PORT	GPIOA
#define ESP8266_RST_PIN		GPIO_PIN_4

#define ESP8266_CH_PD_PORT		GPIOA
#define ESP8266_CH_PD_PIN		GPIO_PIN_5

#define ESP8266_GPIO0_PORT	GPIOA
#define ESP8266_GPIO0_PIN	GPIO_PIN_6

#define ESP8266_GPIO2_PORT	GPIOA
#define ESP8266_GPIO2_PIN	GPIO_PIN_7

#define AT_RESET_CMD		"AT+RST\r\n\0"
#define AT_GMR			"AT+GMR\r\n\0"
#define AT_CWMODE_1		"AT+CWMODE=1\r\n\0"
#define AT_CWLAP		"AT+CWLAP\r\n\0"
#define AT_PING_GOOGLE		"AT+PING=\"www.google.com\"\r\n\0"
#define AT_CIPMODE_0		"AT+CIPMODE=0\r\n\0"
#define AT_CIPMUX_1		"AT+CIPMUX=1\r\n\0"
#define AT_CIPSERVER		"AT+CIPSERVER=1,80\r\n\0"
#define AT_CIPCLIENT		"AT+CIPSERVER=0\r\n\0"
#define AT_CIPSTO		"AT+CIPSTO=7000\r\n\0"
#define AT_CIFSR		"AT+CIFSR\r\n\0"
#define AT_CIPSEND		"AT+CIPSEND=\0"
#define AT_CLOSE_SOCKET		"AT+CIPCLOSE=\0"
#define AT_CIPCLOSE             "AT+CIPCLOSE\r\n"
#define AT_UART_CUR		"AT+UART_CUR=230400,8,1,0,0\r\n\0"

#define CHNL_STATE_OPENED	8
#define CHNL_STATE_CLOSED	4
#define CHNL_STATE_TRANSMIT	2

#define HELP_BUF_SIZE 32
#define HELP_BUF_SIZE_2 64

__IO uint8_t esp8266_second_flag = 0;

struct channel_data {
	char buf[5][HELP_BUF_SIZE];
	int8_t state[5];
	uint8_t reset;
};

static struct channel_data chn_data;
static uint8_t do_it = 0;

static __IO int flag_tmp = 0;

static void ClearChannel(const uint8_t id, const uint8_t state)
{
	chn_data.state[id] &= ~state;
}


static void SetChannel(const uint8_t id, const uint8_t state)
{
	chn_data.state[id] |= state;
}

static int8_t CheckChannel(const uint8_t id, const uint8_t state)
{
	return chn_data.state[id] & state;
}

int8_t esp8266_ScanForFile(char *file, uint8_t *id)
{
	do_it = 1;
	if (flag_tmp) {
	/*
		TM_ILI9341_DrawFilledRectangle(10, 90, 239, 120, ILI9341_COLOR_BLACK);
		TM_ILI9341_Puts(10, 100, buf_tmp, &TM_Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
	*/
		flag_tmp = 0;
	}
	for (size_t i = 0; i < 5; i++) {
		if (CheckChannel(i, CHNL_STATE_TRANSMIT)) {
			/* + 1 to ommit '/' sign. */
			strncpy(file, chn_data.buf[i] + 1, HELP_BUF_SIZE - 1);
			*id = i;
			return 0;
		}
	}
	return -EINVAL;
}

int8_t esp8266_GetReply(const char *command, const char *delimiter,
			char *output, unsigned int delay, uint16_t multiplier)
{
	int8_t ret;
	uint16_t cnt = 0;
	do {
		ret = buffer_SearchGetLabel(&UART2_receive_buffer, command,
					    delimiter, output);
		if (ret)
			delay_ms(delay);
	} while (((ret == -EBUSY) || (ret == -EINVAL)) && (++cnt < multiplier));
	return ret;
}

int8_t esp8266_Send(const char *data, size_t data_size)
{
	int8_t ret;
	ret = buffer_set_text(&UART2_transmit_buffer, data, data_size);
	if (!ret)
		UART_2_transmit();
	return ret;
}

static void SetChannelTransmit(char *buf, uint8_t id)
{
	memset(chn_data.buf[id], 0, HELP_BUF_SIZE);
	strncpy(chn_data.buf[id], buf, HELP_BUF_SIZE - 1);
	SetChannel(id, CHNL_STATE_TRANSMIT);
}

/**
 * @brief Pins initalization.
 *
 * It sets pins, to which esp8266 is connected.
 */
static void esp8266_InitPins()
{
	GPIO_InitTypeDef init;
	init.Mode = GPIO_MODE_OUTPUT_PP;
	init.Pull = GPIO_PULLUP;
	init.Speed = GPIO_SPEED_FREQ_HIGH;
	init.Pin = ESP8266_RST_PIN | ESP8266_CH_PD_PIN | ESP8266_GPIO0_PIN | ESP8266_GPIO2_PIN;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	HAL_GPIO_Init(ESP8266_RST_PORT, &init);

	HAL_GPIO_WritePin(ESP8266_CH_PD_PORT, ESP8266_CH_PD_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(ESP8266_RST_PORT, ESP8266_RST_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(ESP8266_GPIO2_PORT, ESP8266_GPIO2_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(ESP8266_GPIO0_PORT, ESP8266_GPIO0_PIN, GPIO_PIN_SET);
}


/**
 * @brief hardware esp8266 reset.
 *
 * It toggles esp8266 reset pin to logic 0, holds for 1.5s, toggles to 1,
 * and holds for 1s.
 */
static void esp8266_HardReset()
{
	HAL_GPIO_WritePin(ESP8266_RST_PORT, ESP8266_RST_PIN, GPIO_PIN_RESET);
	delay_ms(5000);
	HAL_GPIO_WritePin(ESP8266_RST_PORT, ESP8266_RST_PIN, GPIO_PIN_SET);
	delay_ms(1000);
}

int8_t esp8266_WaitForOk(const char *command, unsigned int delay, uint8_t
			 multiplier)
{
	int8_t ret;
	char buf[BUF_MEM_SIZE];
	ret = esp8266_GetReply(command, "OK\0", buf, delay, multiplier);
	return ret;
}

int8_t esp8266_WaitForAck(const char *command, unsigned int delay,
			  uint8_t multiplier)
{
	int8_t ret;
	uint8_t cnt = 0;
	do {
		ret = esp8266_GetReply(command, "OK\0", NULL, 10, 0);
		if (!ret)
			return 0;

		if (ret) {
			ret = esp8266_GetReply(command, "FAIL\0", NULL, 10, 0);
			if (!ret)
				return 1;
		}

		if (ret) {
			ret = esp8266_GetReply(command, "ERROR\0", NULL, 10, 0);
			if (!ret)
				return 2;
		}

		if (ret)
			delay_ms(delay);

	} while ((ret == -EINVAL || ret == -EBUSY) && ++cnt < multiplier);
	return ret;
}

uint8_t esp8266_PingGoogle()
{
	esp8266_Send(AT_PING_GOOGLE, strlen(AT_PING_GOOGLE));
	return esp8266_WaitForOk(AT_PING_GOOGLE, 100, 100);
}

static int8_t esp8266_ConnectToWiFi()
{
	int ret;
	esp8266_Send(AT_CWMODE_1, strlen(AT_CWMODE_1));
	ret = esp8266_WaitForOk(AT_CWMODE_1, 100, 100);
	if (ret)
		return -1;
	esp8266_Send(WIFI_PASSWD_SECRET, strlen(WIFI_PASSWD_SECRET));
	ret = esp8266_WaitForOk(WIFI_PASSWD_SECRET, 100, 100);
	if (ret)
		return -2;

	return 0;
}

int8_t esp8266_SwitchToServer()
{
	int8_t ret;
	ret = esp8266_Send(AT_CIPSERVER, strlen(AT_CIPSERVER));
	if (ret)
		return -5;
	ret = esp8266_WaitForOk(AT_CIPSERVER, 100, 100);
	if (ret)
		return -6;

	return 0;
}

int8_t esp8266_SwitchToClient()
{
	int8_t ret;
	ret = esp8266_Send(AT_CIPCLIENT, strlen(AT_CIPCLIENT));
	if (ret)
		return -5;
	ret = esp8266_WaitForOk(AT_CIPCLIENT, 100, 100);
	if (ret)
		return -6;

	return 0;
}

int8_t esp8266_MakeAsServer()
{
	int8_t ret;
	ret = esp8266_Send(AT_CIPMODE_0, strlen(AT_CIPMODE_0));
	if (ret)
		return -1;
	ret = esp8266_WaitForOk(AT_CIPMODE_0, 100, 100);
	if (ret)
		return -2;
	ret = esp8266_Send(AT_CIPMUX_1, strlen(AT_CIPMUX_1));
	if (ret)
		return -3;
	ret = esp8266_WaitForOk(AT_CIPMUX_1, 100, 100);
	if (ret)
		return -4;
	ret = esp8266_Send(AT_CIPSERVER, strlen(AT_CIPSERVER));
	if (ret)
		return -5;
	ret = esp8266_WaitForOk(AT_CIPSERVER, 100, 100);
	if (ret)
		return -6;
	ret = esp8266_Send(AT_CIPSTO, strlen(AT_CIPSTO));
	if (ret)
		return -7;
	ret = esp8266_WaitForOk(AT_CIPSTO, 100, 100);
	if (ret)
		return -8;
	return 0;
}

int8_t esp8266_Init()
{
	int8_t ret;
	const char * const RST_CMD = AT_RESET_CMD;
	esp8266_InitPins();
	esp8266_HardReset();
	UART_2_init();
	buffer_Reset(&UART2_transmit_buffer);
	buffer_Reset(&UART2_receive_buffer);
	delay_ms(15000);
	esp8266_Send(RST_CMD, strlen(RST_CMD));
	delay_ms(15000);
	esp8266_Send(AT_GMR, strlen(AT_GMR));
	delay_ms(5000);
	esp8266_Send(AT_CWLAP, strlen(AT_CWLAP));
	delay_ms(5000);
	buffer_Reset(&UART2_receive_buffer);
	ret = esp8266_ConnectToWiFi();
	if (ret)
		return -2;
	delay_ms(10000);
	return 0;
}


int8_t esp8266_SendGetReply(const char *command, const char *delimiter,
			    char *output, unsigned int delay,
			    uint16_t multiplier)
{
	int8_t ret;
	if (buffer_IsFull(&UART2_transmit_buffer))
		return -ENOMEM;
	buffer_Reset(&UART2_receive_buffer);
	esp8266_Send(command, strlen(command));
	ret = esp8266_GetReply(command, delimiter, output, delay, multiplier);
	return ret;
}

static inline void ParseDate(uint8_t *day, uint8_t *month, uint16_t *year,
			     uint8_t *hour, uint8_t *minute, uint8_t *second,
			     char *buf)
{
	unsigned short temp_h, temp_min, temp_sec, temp_day, temp_year;
	char temp_buf[16];
	char temp[5];
	memset(temp, 0, 5);
	memset(temp_buf, 0, 16);
	sscanf(buf, "%s %hu %s %hu %hu:%hu:%hu", temp, &temp_day, temp_buf,
	       &temp_year, &temp_h, &temp_min, &temp_sec);
	*hour = (uint8_t)temp_h;
	*minute = (uint8_t)temp_min;
	*second = (uint8_t)temp_sec;
	*day = (uint8_t)temp_day;
	*year = temp_year;

	if (!strcmp(temp_buf, "Jan"))
		*month = 1;
	else if (!strcmp(temp_buf, "Feb"))
		*month = 2;
	else if (!strcmp(temp_buf, "Mar"))
		*month = 3;
	else if (!strcmp(temp_buf, "Apr"))
		*month = 4;
	else if (!strcmp(temp_buf, "May"))
		*month = 5;
	else if (!strcmp(temp_buf, "Jun"))
		*month = 6;
	else if (!strcmp(temp_buf, "Jul"))
		*month = 7;
	else if (!strcmp(temp_buf, "Aug"))
		*month = 8;
	else if (!strcmp(temp_buf, "Sep"))
		*month = 9;
	else if (!strcmp(temp_buf, "Oct"))
		*month = 10;
	else if (!strcmp(temp_buf, "Nov"))
		*month = 11;
	else if (!strcmp(temp_buf, "Dec"))
		*month = 12;
	else
		*month = 0;
}

int8_t esp8266_WriteATCIPSTART(const char *udp_tcp, const char *address,
			       const char *port)
{
	char buf[BUF_MEM_SIZE];
	int8_t ret = 0;

	const char *command = "AT+CIPSTART=\"";
	const char *first_separator = "\",\"";
	const char *second_separator = "\",";
	const char *third_separator = "\r\n";
	if (BUF_MEM_SIZE < strlen(command) +
			   strlen(first_separator) +
			   strlen(address) +
			   strlen(second_separator) +
			   strlen(port) +
			   strlen(third_separator) + 1) {
		return -1;
	}

	memset(buf, 0, BUF_MEM_SIZE);
	strcpy(buf, "AT+CIPSTART=\"");
	strcat(buf, udp_tcp);
	strcat(buf, first_separator);
	strcat(buf, address);
	strcat(buf, second_separator);
	strcat(buf, port);
	strcat(buf, third_separator);

	esp8266_Send(buf, strlen(buf));
	ret = esp8266_WaitForOk(buf, 100, 100);
	if (ret)
		return -1;

	return 0;
}

int8_t esp8266_WriteATCIPSEND_with_id(char *data, size_t data_size, uint8_t id)
{
	char temp[32];
	char temp_2[16];
	int ret;
	memset(temp, 0, 32);
	memset(temp_2, 0, 16);
	sprintf(temp_2, "%u,%u\r\n", (unsigned int)id, (unsigned int)data_size);
	strcpy(temp, AT_CIPSEND);
	strcat(temp, temp_2);

	if (!CheckChannel(id, CHNL_STATE_TRANSMIT))
		return -1;

	ret = esp8266_Send(temp, strlen(temp));
	if (ret)
		return -2;

	ret = esp8266_WaitForAck(temp, 100, 10);
	if (ret)
		return -3;

	ret = esp8266_Send(data, data_size);
	if (ret)
		return -4;

	return 0;
}

int8_t esp8266_WriteATCIPSEND(char *data, size_t data_size)
{
	char temp[32];
	char temp_2[16];
	int ret;
	memset(temp, 0, 32);
	memset(temp_2, 0, 16);
	sprintf(temp_2, "%u\r\n", (unsigned int)data_size);
	strcpy(temp, AT_CIPSEND);
	strcat(temp, temp_2);

	ret = esp8266_Send(temp, strlen(temp));
	if (ret)
		return -1;

	ret = esp8266_WaitForAck(temp, 100, 10);
	if (ret)
		return -2;

	ret = esp8266_Send(data, data_size);
	if (ret)
		return -3;

	return 0;
}

int8_t esp8266_WriteATCIPCLOSE()
{
	int ret;
	char buf[BUF_MEM_SIZE];
	memset(buf, 0, BUF_MEM_SIZE);
	strcpy(buf, AT_CIPCLOSE);

	ret = esp8266_Send(buf, strlen(buf));
	if (ret)
		return -1;

	ret = esp8266_WaitForAck(buf, 5, 200);

	if (ret)
		return -2;

	return 0;
}

int8_t esp8266_GetDate(uint8_t *day, uint8_t *month, uint16_t *year,
		       uint8_t *hour, uint8_t *minute, uint8_t *second)
{
	char *http_data = "HEAD / HTTP/1.1\r\n\r\n\0";
	int8_t ret = 0;
	char buf[BUF_MEM_SIZE];
	memset(buf, 0, BUF_MEM_SIZE);

	ret = esp8266_WriteATCIPSTART("TCP", "www.google.com", "80");
	if (ret)
		return -1;

	ret = esp8266_WriteATCIPSEND(http_data, strlen(http_data));
	if (ret)
		return -2;
	delay_ms(2000);
	ret = esp8266_WriteATCIPCLOSE();
	if (ret)
		return -3;

	ret = buffer_SearchGetLabel(&UART2_receive_buffer, "Date: \0", " GMT\0", buf);
	if (ret)
		return -4;
	ParseDate(day, month, year, hour, minute, second, buf);
	buffer_Reset(&UART2_receive_buffer);
	return 0;
}

int8_t esp8266_GetIp(char *buf)
{
	memset(buf, 0, BUF_MEM_SIZE);
	return esp8266_SendGetReply(AT_CIFSR, "OK\0", buf, 100, 10);
}

int8_t esp8266_CheckResetFlag()
{
	int8_t ret = chn_data.reset;
	if (ret)
		memset(&chn_data, 0, sizeof(struct channel_data));

	return ret;
}

void esp8266_ClearResetFlag()
{
		chn_data.reset = 0;
}

static void MoveInsert(char *buffer, size_t size, uint8_t new_byte)
{
	for(size_t i = 0; i < size - 1; i++)
		buffer[i] = buffer[i+1];
	buffer[size-1] = new_byte;
}

static int8_t CompareLastBytes(char *buffer, size_t size, const char *to_compare)
{
	size_t len = strlen(to_compare);
	size_t offset = size - len;
	return strncmp(buffer + offset, to_compare, len);
}

static void MoveToSign(char *buffer, size_t size, char sign)
{
	while(buffer[0] != sign)
		MoveInsert(buffer, size, '\0');
}

static int8_t esp8266_state0(const uint8_t data, char *buf,
			     const size_t buf_len, uint8_t *state)
{
	int8_t ret = 0;
	uint8_t id;
	const char *IPD = "+IPD,\0";
	const char *CLOSED = ",CLOSED\0";
	const char *CONNECT = ",CONNECT\0";

	switch(data) {

	case ',':
		ret = CompareLastBytes(buf, buf_len, IPD);
		if (!ret)
			*state = 1;
		break;

	case 'D':
		ret = CompareLastBytes(buf, buf_len, CLOSED);
		if (!ret) {
			id = *(buf + (buf_len - 1 - strlen(CLOSED)))
			     - 48;
			ClearChannel(id, CHNL_STATE_TRANSMIT);
		}
		break;

	case 'T':
		ret = CompareLastBytes(buf, buf_len, CONNECT);
		break;

	default:
		ret = -1;
	}

	return ret;
}

static int8_t esp8266_state1(char *buf, const size_t buf_len,
			     uint8_t *state)
{
	int8_t ret;
	uint16_t id, len, lentmp;
	char file[HELP_BUF_SIZE];


	ret = CompareLastBytes(buf, buf_len, "HTTP");
	if (ret)
		return -1;

	memset(buf + buf_len - strlen(" HTTP"), 0, strlen(" HTTP"));

	*state = 0;

	MoveToSign(buf, buf_len, '+');
	MoveToSign(buf, buf_len, 'I');
	MoveToSign(buf, buf_len, 'P');
	MoveToSign(buf, buf_len, 'D');
	MoveToSign(buf, buf_len, ',');
	memset(file, 0, HELP_BUF_SIZE);

	*buf = '\0';
	*(buf + 2) = '\0';

	id = atoi(buf + 1);
	len = atoi(buf + 3);

	MoveToSign(buf, buf_len, ':');
	MoveToSign(buf, buf_len, 'G');
	MoveToSign(buf, buf_len, 'E');
	MoveToSign(buf, buf_len, 'T');
	strncpy(file, buf + 2, strlen(buf + 2));

	if (id < 5)
		SetChannelTransmit(file, id);

	lentmp = strlen("+IPD,x,xxx: GET / HTTP") + strlen(file) + 35;

	if (len >= lentmp)
		len -= lentmp;

	buffer_SetIgnore(&UART2_receive_buffer, len);


//	memset(buf_tmp, 0, 100);
//	sprintf(buf_tmp, "id = %hu, len = %hu, strlen(buf + 2) = %u, file = %s, test = %u"
//		, id, len, strlen(buf + 2), file, strlen("test\0test"));
//	flag_tmp = 1;
	memset(buf, 0, buf_len);


	return 0;
}

static int8_t esp8266_CheckErrorsOnInput(const uint8_t data, uint8_t *state,
					 char *buf, const size_t len)
{
	static size_t test = 0;

	if ((data != '\r') && (data != '\n') && (data != '\0')) {
		if ((data < ' ') || (data > '~'))
			test++;
		else
			test = 0;
	}

	if (test == 3) {
		chn_data.reset = 1;
		test = 0;
		*state = 0;
		memset(buf, 0, len);
		return 1;
	}

	return 0;
}

static int8_t esp8266_CheckReset(uint8_t *state, char *buf, const size_t len)
{
	int8_t ret;
	ret = CompareLastBytes(buf, sizeof(buf), "rst cause");
	if (!ret) {
		chn_data.reset = 1;
		memset(buf, 0, len);
		*state = 0;
		return 1;
	}
	return 0;
}

void esp8266_CheckInput(uint8_t data)
{
	static uint8_t state = 0;
	static char buf[HELP_BUF_SIZE_2];

	int8_t ret;
	if (!do_it)
		return;

	MoveInsert(buf, sizeof(buf), data);

	/*
	 * Somehow sometimes I got some strange values (not ASCII signs!) on input.
	 * If it happens - reset
	 */
	ret = esp8266_CheckErrorsOnInput(data, &state, buf, sizeof(buf));
	if (ret)
		return;

	/*
	 * Sometimes esp8266 resets itself without reason.
	 * There is need to sense it and restart connection etc.
	 * Reset of wifi etc. applied
	 */
	ret = esp8266_CheckReset(&state, buf, sizeof(buf));
	if (ret)
		return;

	switch(state){
	case 0:
		ret = esp8266_state0(data, buf, sizeof(buf), &state);
		break;
	case 1:
		ret = esp8266_state1(buf, sizeof(buf), &state);
		break;
	default:
		state = 0;
		break;
	}
}

void esp8266_Update()
{
	int ret;
	char buf[BUF_MEM_SIZE];
	const char *update = "AT+CIUPDATE\r\n";
	ret = esp8266_SendGetReply(update, "OK", buf, 100, 5*600);
	(void)ret;
}
