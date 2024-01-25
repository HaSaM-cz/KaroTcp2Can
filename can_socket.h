#pragma once

#include <sys/socket.h>
//#include <netinet/in.h> // Needed to use struct sockaddr_in
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include <mutex>
#include <vector>
#include <thread>
#include <functional>

///////////////////////////////////////////////////////////////////////////////////////////////////
class CanSocket
{
public:
	CanSocket();
	~CanSocket();
	void Send(struct can_frame *frame);
	void Run();
private:
	void RunThread();

	virtual void Receive(struct timeval* tv, struct canfd_frame* frame) = 0;

private:
	int s;
	struct sockaddr_can addr;
	std::thread can_thread;
};

///////////////////////////////////////////////////////////////////////////////////////////////////