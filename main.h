#pragma once

typedef enum 
{
	can_message = 0,
	/*
	* uint8_t - packet type
	* uint32_t - time sec
	* uint16_t = time mili sec
	* uint32_t - can id
	* uint8_t - data len
	* uint8_t[] - data
	*/


} packet_type_e;