#include "GPS.h"
#include "UART.h"

int gps_get_data(char *data, size_t data_size) {
	int ret;
	uint16_t cnt = 0;
	uint16_t multiplier = 10;
	const int delay = 10;

	if (data_size < 64)
		return -1;

	do {
		ret = buffer_SearchGetLabel(&UART1_receive_buffer, "$GP",
					    "\r\n\0", data); //TODO data size problems
		if (ret)
			delay_ms(delay);
	} while (((ret == -EBUSY) || (ret == -EINVAL)) && (++cnt < multiplier));
	if (cnt == multiplier)
		return -1;

	return 0;
}