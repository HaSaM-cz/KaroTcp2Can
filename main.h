#pragma once

#include <stdint.h>

extern uint16_t default_tcp_port;
extern uint8_t default_can_address;


typedef enum
{
	can_message = 0,
	/*
	* uint8_t - packet type = 0
	* uint32_t - time sec
	* uint16_t - time mili sec
	* uint32_t - can id
	* uint8_t - data len
	* uint8_t[] - data
	*/

	set_extended = 2,
	/*
	* uint8_t - packet type = 2
	* uint8_t - 0 = old_communication, 2 = new_communication
	*/

} packet_type_e;

typedef struct __attribute__((__packed__))
{
	uint8_t packet_type;
	uint32_t time_sec;
	uint16_t time_mili_sec;
	uint32_t can_id;
	uint8_t data_len;
	uint8_t data[1024];
} packet_can_message_t;