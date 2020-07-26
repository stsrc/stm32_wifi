#ifndef _ESP8266_H_
#define _ESP8266_H_

#include <inttypes.h>
#include <stm32f1xx_hal_gpio.h>
#include "UART.h"
#include "stupid_delay.h"

/**
 * @addtogroup RFID_System libraries
 * @{
 */

/**
 * @defgroup esp8266
 * @brief esp8266 library
 * @{
 *
 * Uses UART interface with a circular buffer. It allows to speed up transmition,
 * and simplifies code.
 */


/**
 * @brief Initialization of esp8266
 *
 * Function sets esp8266 pins, turns on UART2,
 * changes speed of esp8266 and UART2, to achive higher transfer,
 * and connects to Wi-Fi. Password to Wi-Fi is in secret_password.h file.
 * @retval int8_t 0 on success, negative value on failure.
 */
int8_t esp8266_Init();

/**
 * @brief Function to get date from Internet by WWW.
 *
 * Function sends page request to google.com. It responds with time and date,
 * which is then parsed to variables in pointers.
 *
 * @param day pointer to day variable
 * @param month pointer to month variable
 * @param year pointer to year variable
 * @param ...  pointers to variable which function will update with actual
 *		values
 * @retval int8_t 0 on success, negative value on failure.
 */
int8_t esp8266_GetDate(uint8_t *day, uint8_t *month, uint16_t *year,
		       uint8_t *hour, uint8_t *minute, uint8_t *second);

/**
 * @brief Function to obtain present IP of esp8266.
 * @param buf buffer to which function will write IP and MAC.
 * @retval int8_t 0 on success, negative value on fail.
 */
int8_t esp8266_GetIp(char *buf);

/**
 * @brief creates TCP/IP server from esp8266.
 * @retval int8_t 0 on success, negative value otherwise.
 */
int8_t esp8266_MakeAsServer();

int8_t esp8266_SwitchToClient();

int8_t esp8266_SwitchToServer();

/**
 * @brief HTTP requests check.
 *
 * It scans internal structure, which holds file names and id of channels,
 * which requested data transfer. If there is such a channel and file,
 * this method returns
 * @param file buffer in which name of file to be send will be placed.
 * @param id id which is linked with file to be send.
 * @retval int8_t 0 if there is pending HTTP request, -EINVAL otherwise.
 */
int8_t esp8266_ScanForFile(char *file, uint8_t *id);

/**
 * @brief Reset condition check.
 *
 * If buffer contains string which is typical for reset state,
 * reset flag is set.
 * @retval int8_t true if esp8266 must be reset, false otherwise.
 */
int8_t esp8266_CheckResetFlag();

int8_t esp8266_WriteATCIPSTART(const char *udp_tcp, const char *address, const char *port);

/**
 * @brief AT+CIPSEND command transfer, used to send data.
 *
 * Function firstly sends AT+CIPSEND command, waits for acknowledge, then it
 * transmits data, and again waits for acknowledge from esp8266 chip.
 *
 * @param data payload of chunk to be written.
 * @param data_size data size
 * @param id channel to which data will be send.
 * @retval int8_t -1 if channel is not in transmit state (serious error),
 * -2 if send of command failed, -3 if wait for command result failed,
 *  -4 is send of data failed, 0 on success.
 */
int8_t esp8266_WriteATCIPSEND_with_id(char *data, size_t data_size, uint8_t id);

int8_t esp8266_WriteATCIPSEND(char *data, size_t data_size);

/**
 * @brief AT+CIPCLOSE command transfer, used to close channel.
 *
 * Used to close a channel.
 *
 * @retval int8_t -1 if send of AT+CIPCLOSE failed, -2 if waiting for command result
 * failed, 0 on success.
 */
int8_t esp8266_WriteATCIPCLOSE();

int8_t esp8266_SendUDPPacket(const char *address, const char *port, const char *data);
/**
 * @brief Function which waits for acknowledge to specific command.
 *
 * @param command command, that is waiting for.
 * @param delay delay in ms between searches of command in input buffer.
 * @param multiplier count of function repetition. First function searches
 * command, if there is no such command it waits for delay. Then multiplier
 * defines how many such a loops to make.
 *
 * @retval int8_t 0 if command ended with OK, 1 if command ended with FAIL, 2
 * if command ended wich ERROR, -EINVAL if there was no such command,
 * -EBUSY if there is i.e. half of command and nothing more.
 */
int8_t esp8266_WaitForAck(const char *command, unsigned int delay,
			  uint8_t multiplier);

/**
 * @brief Function to clear reset flag after Wi-Fi reset.
 */
void esp8266_ClearResetFlag();

void esp8266_CheckInput(uint8_t data);

uint8_t esp8266_PingGoogle();
#endif
