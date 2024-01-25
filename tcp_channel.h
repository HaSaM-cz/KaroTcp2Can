#pragma once

#include <mutex>
#include <vector>
#include <functional>

///////////////////////////////////////////////////////////////////////////////////////////////////
class TcpChannel
{
public:
    TcpChannel(int _cannel_socket, std::function<void(std::vector<uint8_t>)> fn);
    ~TcpChannel();
    void Run();
    void Send(std::vector<uint8_t> v);

private:
    void RunThread();

public:
    std::function<void(std::vector<uint8_t>)> rx_function;
    std::thread tcp_chammel_thread;

private:
    std::mutex s_mutex;
    int channel_socket;
};

