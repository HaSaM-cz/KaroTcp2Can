#pragma once

#include <mutex>
#include <vector>
#include <functional>

#include "message.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
class TcpChannel
{
private:
    bool extended_comm = false;
    uint8_t old_can_id = default_can_address;

public:
    TcpChannel(int _cannel_socket, std::function<void(std::shared_ptr<Message>)> fn);
    ~TcpChannel();
    void Run();
    void Send(std::shared_ptr<Message> message);

private:
    void RunThread();
    void Parse(uint8_t b);
    void ProcessNewMessage(std::vector<uint8_t> data);
    void SetExtended(bool b);

public:
    std::function<void(std::shared_ptr<Message>)> rx_fn;
    std::thread tcp_chammel_thread;

private:
    std::mutex s_mutex;
    int channel_socket;
};

