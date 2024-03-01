#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#include <iostream>
#include <array>
//#include <atomic>
//#include <condition_variable>
#include <thread>
#include <chrono>
#include <string>
#include <string_view>
//#include <future>
//#include <vector>
//#include <algorithm>
//#include <csignal>
#include <fmt/format.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "tcp_server.h"
#include "can_socket.h"
#include "can_hasam.h"
#include "util.h"


uint16_t default_tcp_port = 10000;
uint8_t default_can_address = 0x08;

namespace chrono = std::chrono;

void tcp_rx_fn(std::shared_ptr<Message> message);
void can_rx_fn(std::shared_ptr<Message> message);

std::shared_ptr<TcpServer> tcp_server;
std::shared_ptr<CanHasam> can_socket;

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char const* argv[])
{
	fprintf(stderr, "version = %s\n", "1.0.2");

	tcp_server = std::make_shared<TcpServer>(default_tcp_port, tcp_rx_fn);
	tcp_server->Run();

	can_socket = std::make_shared<CanHasam>(can_rx_fn);
	can_socket->Run();

	while (true)
		std::this_thread::sleep_for(chrono::milliseconds(1000));

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Zprava z TCP socketu. Pošleme na CAN.
///////////////////////////////////////////////////////////////////////////////////////////////////
void tcp_rx_fn(std::shared_ptr<Message> message)
{
	if (message->len <= 8)
	{
		struct can_frame frame;
		frame.can_id = message->can_id | 0x80000000;
		frame.can_dlc = message->len;
		memcpy(frame.data, message->data.data(), message->len);
		can_socket->Send(&frame);
	}
	else
	{
		struct can_frame frame;
		uint16_t crc = GetCRC16(message->data, 0);

		frame.can_id = message->can_id | 0x80000000 | FirstPacketLongMsg_Mask | LongMsg_Mask;
		frame.can_dlc = 8;
		frame.data[0] = 0;
		frame.data[1] = (uint8_t)message->data.size();
		frame.data[2] = (uint8_t)(crc >> 8);
		frame.data[3] = (uint8_t)(crc & 0xFF);
		memcpy(&frame.data[4], message->data.data(), 4);
		can_socket->Send(&frame);

		frame.can_id &= ~FirstPacketLongMsg_Mask;

		for (uint32_t i = 4; i < (uint32_t)message->data.size(); i += 8)
		{
			frame.can_dlc = (uint8_t)((i + 8) < message->data.size() ? 8 : message->data.size() - i);
			memcpy(frame.data, &message->data.data()[i], frame.can_dlc);
			can_socket->Send(&frame);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Zprava z CAN strany. Pošleme na TCP socket.
///////////////////////////////////////////////////////////////////////////////////////////////////
void can_rx_fn(std::shared_ptr<Message> message)
{
	tcp_server->Send(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
