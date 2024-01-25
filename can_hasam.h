#pragma once

#include <mutex>
#include <vector>
#include <map>
#include <thread>
#include <functional>

#include "can_socket.h"
#include "message.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
class CanHasam : public CanSocket
{
public:
	CanHasam(std::function<void(std::shared_ptr<Message>)> fn);

private:
	virtual void Receive(struct timeval *tv, struct canfd_frame* frame);
	std::unordered_map<canid_t, std::shared_ptr<Message>> long_msg_map;

private:
	std::function<void(std::shared_ptr<Message>)> rx_fn;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
