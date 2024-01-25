#include <stdio.h>
#include <string.h>
#include <linux/can.h>

#include "main.h"
#include "message.h"
#include "util.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
Message::Message()
{
	time.tv_sec = 0;
	time.tv_usec = 0;
	can_id = 0;
	len = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Message::Message(struct timeval* tv, struct canfd_frame* frame)
{
	if (frame->can_id & FirstPacketLongMsg_Mask)
	{
		// Dlouhá zprava
		time = *tv;
		can_id = frame->can_id & ~FirstPacketLongMsg_Mask;
		len = frame->data[1];
		data.assign(frame->data + 4, frame->data + 8);
	}
	else
	{
		// Jednoducha zprava
		time = *tv;
		can_id = frame->can_id;
		len = frame->len;
		data.assign(frame->data, frame->data + len);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Message::AddNextFrame(struct timeval* tv, struct canfd_frame* frame)
{
	time = *tv;
	data.insert(data.end(), frame->data, frame->data + frame->len);
	return (data.size() >= len);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<uint8_t> Message::ToTcpOld(packet_type_e packet_type)
{
	std::vector<uint8_t> packet_data;

	if (packet_type == can_message)
	{
		std::vector<uint8_t> vc{ 0,4,5 };

		/*
		0 Start of Heading
		1 SRC
		2 CRC8
		3 CMD
		4 data len
		5-n data
		(n+1)-(n+2) CRC16	(MSB first)
		(n+3) End of Transmission
		*/
		packet_data.push_back(0x01);			// Start of Heading
		packet_data.push_back(this->get_SRC()); // SRC
		packet_data.push_back(GetCRC8({ this->get_SRC(), this->get_CMD(), this->len }));	// CRC8
		packet_data.push_back(this->get_CMD()); // CMD
		packet_data.push_back(this->len);		// data len
		if (len > 0)
			packet_data.insert(packet_data.end(), data.begin(), data.end()); // data
		uint16_t crc16 = GetCRC16(packet_data, 1);
		uint8_t *p = (uint8_t*)&crc16;
		packet_data.insert(packet_data.end(), p, p + 2); // CRC16 
		packet_data.push_back(0x04);			// End of Transmission
	}

	return packet_data;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<uint8_t> Message::ToTcpNew(packet_type_e packet_type)
{
	std::vector<uint8_t> packet_data;

	if (packet_type == can_message)
	{
		/*
		* uint8_t - packet type
		* uint32_t - time sec
		* uint16_t = time mili sec
		* uint32_t - can id
		* uint8_t - data len
		* uint8_t[] - data
		*/
		packet_data.push_back((uint8_t)packet_type);
		uint32_t sec = (uint32_t)time.tv_sec;
		uint8_t* p = (uint8_t*)&sec;
		packet_data.insert(packet_data.end(), p, p + 4);
		uint16_t msec = (uint16_t)(time.tv_usec / 1000);
		p = (uint8_t*)&msec;
		packet_data.insert(packet_data.end(), p, p + 2);
		p = (uint8_t*)&can_id;
		packet_data.insert(packet_data.end(), p, p + 4);
		packet_data.push_back(len);
		if (len > 0)
			packet_data.insert(packet_data.end(), data.begin(), data.end());
	}

	return packet_data;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void Message::Dump(const char* prefix)
{
	char buffer[1024];
	sprintf(buffer, "%s <0x%8.8X> [%d]", prefix, can_id, len);
	for (int i = 0; i < len; i++)
		sprintf(&buffer[strlen(buffer)], " %2.2X", data[i]);
	printf("%s\n", buffer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
canid_t Message::CanIdFromFrame(struct canfd_frame* frame)
{
	return frame->can_id & ~FirstPacketLongMsg_Mask & CAN_ERR_MASK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////