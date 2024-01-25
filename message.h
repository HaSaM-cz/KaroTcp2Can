#pragma once

#include <ctime>
#include <vector>
#include <bits/stdint-uintn.h>

#include "properties.h"

#include "main.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
enum
{
	FirstPacketLongMsg_Mask = 0x08000000,
	Response_Mask = 0x01000000,
	LongMsg_Mask = 0x10000000,
	SourceAddr_Mask = 0x00FF0000,
	DestAddr_Mask = 0x0000FF00,
	Command_Mask = 0x000000FF
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class Message
{
public:
	Message();
	Message(struct timeval* tv, struct canfd_frame* frame);
	bool AddNextFrame(struct timeval* tv, struct canfd_frame* frame);
	void Dump(const char* prefix = "");
	std::vector<uint8_t> ToTcpOld(packet_type_e packet_type);
	std::vector<uint8_t> ToTcpNew(packet_type_e packet_type);

public:
	uint8_t get_SRC() { return (uint8_t)((can_id >> 16) & 0xFF); }
	uint8_t get_DEST() { return (uint8_t)((can_id >> 8) & 0xFF); }
	uint8_t get_CMD() { return (uint8_t)(can_id & 0xFF); }

	void set_SRC(uint8_t val)  { can_id &= 0xFF00FFFF; can_id |= (val << 16); }
	void set_DEST(uint8_t val) { can_id &= 0xFFFF00FF; can_id |= (val << 8); }
	void set_CMD(uint8_t val)  { can_id &= 0xFFFFFF00; can_id |= val; }

public:
	static canid_t CanIdFromFrame(struct canfd_frame* frame);

public:
	struct timeval time;
	uint8_t len;
	canid_t can_id;
	std::vector<uint8_t> data;
};

