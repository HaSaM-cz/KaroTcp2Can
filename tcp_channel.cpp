#include <thread>
#include <netinet/in.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "tcp_channel.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
TcpChannel::TcpChannel(int _channel_socket, std::function<void(std::vector<uint8_t>)> fn)
{
	printf("TcpChannel constructor\n");
	rx_function = fn;
    channel_socket = _channel_socket;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
TcpChannel::~TcpChannel()
{
	printf("TcpChannel destructor\n");
};

///////////////////////////////////////////////////////////////////////////////////////////////////
void TcpChannel::Run()
{
	printf("TcpChannel::Run entry\n");
	tcp_chammel_thread = std::thread(&TcpChannel::RunThread, this);
	printf("TcpChannel::Run exit\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void TcpChannel::RunThread()
{
	char buffer[1024];

	printf("TcpChannel::RunThread entry\n");

	while (true)
	{
		//printf("loop\n");
		ssize_t valread = read(channel_socket, buffer, sizeof(buffer));
		if (valread <= 0)
			break;
		// terminator at the end
		//printf("%s\n", buffer);
		
		std::vector<uint8_t> v(buffer, buffer + valread);
		rx_function(v);
	}

	tcp_chammel_thread.detach();

	s_mutex.lock();
	close(channel_socket);
	channel_socket = -1;
	s_mutex.unlock();

	printf("TcpChannel::RunThread exit\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void TcpChannel::Send(std::vector<uint8_t> v)
{
	//printf("TcpChannel::Send entry\n");
	s_mutex.lock();
	if(channel_socket > 0)
		send(channel_socket, v.data(), v.size(), 0);
	s_mutex.unlock();
	//printf("TcpChannel::Send exit\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////