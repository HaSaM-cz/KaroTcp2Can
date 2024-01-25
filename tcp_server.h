#pragma once
#ifndef TCP_SERVER
#define TCP_SERVER
#include <netinet/in.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <thread>
#include <vector>
#include <functional>

#include "tcp_channel.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
class TcpServer
{
public:
    TcpServer(uint16_t port, std::function<void(std::vector<uint8_t>)> fn);
    ~TcpServer();
    void Run();
    void Send(std::vector<uint8_t> &v);
protected:

    void RunThread();

    std::function<void(std::vector<uint8_t>)> rx_fn;
    std::thread tcp_thread;
    std::vector<std::shared_ptr<TcpChannel>> tcp_channels;

    int listen_socket;                  // Listening socket
    struct sockaddr_in serv_addr;       // Structure with information about socket (adress, port etc.)
    //fd_set connections;                 // Set of clients
    //int max_socket;                     // Number of maximum existing descriptor
    //int current_socket;                 // Number of the processed descriptor
};

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif