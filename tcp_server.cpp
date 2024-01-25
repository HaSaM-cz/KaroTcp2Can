#include "tcp_server.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
TcpServer::TcpServer(uint16_t port, std::function<void(std::vector<uint8_t>)> fn)
{
    rx_fn = fn;

    int param = 1;      // Parametr for setsockopt(), maybe we can change it to bool.
    //char ip_str[INET_ADDRSTRLEN] = { '\0' };

    if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw "Can't create a socket!\n";

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &param, sizeof(param)) < 0)
        throw "setsockopt() error!\n";

    if (bind(listen_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        throw "bind() error!\n";

    if (listen(listen_socket, 5) < 0)
        throw "listen() error!\n";

    //FD_ZERO(&connections);
    //FD_SET(listen_socket, &connections);
    //max_socket = listen_socket;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TcpServer::~TcpServer()
{
    /*for (int i = 0; i <= max_socket; i++)
    {
        if (i == listen_socket)
        {
            shutdown(i, SHUT_RDWR);
            close(i);
            FD_CLR(i, &connections);
        }
        if (FD_ISSET(i, &connections))
        {
            shutdown(i, SHUT_RDWR);
            close(i);
            FD_CLR(i, &connections);
        }
    }*/
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void TcpServer::Run()
{
    printf("TcpServer::Run entry\n");
    tcp_thread = std::thread(&TcpServer::RunThread, this);
    printf("TcpServer::Run exit\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void TcpServer::RunThread()
{
    printf("TcpServer::RunThread entry\n");
    int new_socket;
    struct sockaddr_in cli_addr;
    socklen_t addrlen = sizeof(cli_addr);

    while (true)
    {
        if ((new_socket = accept(listen_socket, (struct sockaddr*)&cli_addr, &addrlen)) < 0)
            fprintf(stderr, "error in accept\n");

        TcpChannel* tcp_channel = new TcpChannel(new_socket, rx_fn);
        tcp_channel->Run();

        std::shared_ptr<TcpChannel> sh_ptr(tcp_channel);
        tcp_channels.push_back(sh_ptr);


        fprintf(stdout, "accept OK\n");
    }

    printf("TcpServer::RunThread entry\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void TcpServer::Send(std::vector<uint8_t> &v)
{
    //printf("TcpServer::Send entry\n");

    //printf("tcp_channels %d\n", tcp_channels.size());

    auto it = std::begin(tcp_channels);
    while (it != std::end(tcp_channels))
    {
        if (!(*it)->tcp_chammel_thread.joinable())
        {
            it = tcp_channels.erase(it);
        }
        else
        {
            (*it)->Send(v);
            it++;
        }
    }

    //printf("TcpServer::Send exit\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////