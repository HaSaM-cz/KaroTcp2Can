#include <thread>
#include <netinet/in.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "tcp_channel.h"
#include "message.h"
#include "util.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
TcpChannel::TcpChannel(int _channel_socket, std::function<void(std::shared_ptr<Message>)> fn)
{
	printf("TcpChannel constructor\n");
	rx_fn = fn;
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
// Vlákno pro pøijem na TCP kanálu
///////////////////////////////////////////////////////////////////////////////////////////////////
void TcpChannel::RunThread()
{
	uint8_t buffer[1024];

	printf("TcpChannel::RunThread entry\n");

	while (true)
	{
		//printf("loop\n");
		ssize_t valread = read(channel_socket, buffer, sizeof(buffer));
		if (valread <= 0)
			break;
		// terminator at the end
		//printf("%s\n", buffer);
		for (int i = 0; i < valread; i++)
			Parse(buffer[i]);
	}

	tcp_chammel_thread.detach();

	s_mutex.lock();
	close(channel_socket);
	channel_socket = -1;
	s_mutex.unlock();

	printf("TcpChannel::RunThread exit\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Parsování TCP dat
///////////////////////////////////////////////////////////////////////////////////////////////////
void TcpChannel::Parse(uint8_t b)
{
    const uint8_t SOH = 0x01;
    const uint8_t EOT = 0x04;
    const uint8_t SOH_X = 0xF1;
    const uint8_t EOT_X = 0xF4;
    const uint8_t SUBST = 0xF0;

    enum ReceiveRS {
        SL_ST_NO_RECEIVING,
        SL_ST_RECEIVING_DEST,
        SL_ST_RECEIVING_CRC8,
        SL_ST_RECEIVING_CMD,
        SL_ST_RECEIVING_DLEN,
        SL_ST_RECEIVING_DTA,
        SL_ST_RECEIVING_CRC_H,
        SL_ST_RECEIVING_CRC_L,
        SL_ST_FINALIZE,
        SL_ST_RECEIVING_DTA_X,
    };

    typedef struct
    {
        uint8_t RS_Dest;                  //Destination
        uint8_t RS_CRC_8;                 //CRC8 header
        uint8_t RS_CMD;                   //Command
        uint8_t RS_DLEN;                  //delka Dat v bytech
        std::vector<uint8_t> RS_DTA;      //data	
        uint8_t RS_crc_H;                 //cyclic redundancy check
        uint8_t RS_crc_L;                 //cyclic redundancy check
    }s_RS_receive;

    static uint8_t subst = 0;
    static uint8_t data_counter;
    static s_RS_receive RS_receive;
	static ReceiveRS receive_state = SL_ST_NO_RECEIVING;
    std::vector<uint8_t> buf_head(3);

	switch (receive_state)
	{
        case SL_ST_NO_RECEIVING:
            if (b == SOH)
            {
                RS_receive.RS_DTA.clear();
                receive_state = SL_ST_RECEIVING_DEST;
            }
            else if (b == SOH_X)
            {
                subst = 0;
                RS_receive.RS_DTA.clear();
                receive_state = SL_ST_RECEIVING_DTA_X;
            }
            break;
        case SL_ST_RECEIVING_DEST:
            RS_receive.RS_Dest = b;
            receive_state = SL_ST_RECEIVING_CRC8;
            break;
        case SL_ST_RECEIVING_CRC8:
            RS_receive.RS_CRC_8 = b;
            receive_state = SL_ST_RECEIVING_CMD;
            break;
        case SL_ST_RECEIVING_CMD:
            RS_receive.RS_CMD = b;
            receive_state = SL_ST_RECEIVING_DLEN;
            break;
        case SL_ST_RECEIVING_DLEN:
            RS_receive.RS_DLEN = b;
            data_counter = b;
            buf_head[0] = RS_receive.RS_Dest;
            buf_head[1] = RS_receive.RS_CMD;
            buf_head[2] = RS_receive.RS_DLEN;
            if (GetCRC8(buf_head) == RS_receive.RS_CRC_8)
                receive_state = (RS_receive.RS_DLEN > 0 ? SL_ST_RECEIVING_DTA : SL_ST_RECEIVING_CRC_H);
            else
                receive_state = SL_ST_NO_RECEIVING;
            break;
        case SL_ST_RECEIVING_DTA:
            RS_receive.RS_DTA.push_back(b);
            data_counter--;
            if (data_counter == 0)
                receive_state = SL_ST_RECEIVING_CRC_H;
            break;
        case SL_ST_RECEIVING_CRC_H:
            RS_receive.RS_crc_H = b;
            receive_state = SL_ST_RECEIVING_CRC_L;
            break;
        case SL_ST_RECEIVING_CRC_L:
            RS_receive.RS_crc_L = b;
            receive_state = SL_ST_FINALIZE;
            break;
        case SL_ST_FINALIZE:
            if (b == EOT)
            {
                std::shared_ptr<Message> message_sh_ptr(new Message(old_can_id, RS_receive.RS_Dest, RS_receive.RS_CMD, RS_receive.RS_DTA));
                //message_sh_ptr->Dump();
                rx_fn(message_sh_ptr);
                receive_state = SL_ST_NO_RECEIVING;
            }
            else
            {
                receive_state = SL_ST_NO_RECEIVING;
            }
            break;
        case SL_ST_RECEIVING_DTA_X:
            if (b != EOT_X)
            {
                if (b == SUBST)
                {
                    subst = SUBST;
                }
                else
                {
                    RS_receive.RS_DTA.push_back(b | subst);
                    subst = 0;
                }
            }
            else
            {
                // FINALIZE X
                ProcessNewMessage(RS_receive.RS_DTA);
                receive_state = SL_ST_NO_RECEIVING;
            }
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Zpracování data (nový zpùsob komunikace)
///////////////////////////////////////////////////////////////////////////////////////////////////
void TcpChannel::ProcessNewMessage(std::vector<uint8_t> data)
{
    packet_can_message_t* p_packet_can_message;
    size_t data_len = data.size();
    if (data_len >= 1)
    {
        switch (data[0])
        {
        case set_extended:
            SetExtended(data_len >= 2 && data[1] != 0);
            break;

        case can_message:
            p_packet_can_message = (packet_can_message_t*)data.data();
            std::shared_ptr<Message> message_sh_ptr(new Message());
            message_sh_ptr->can_id = p_packet_can_message->can_id;
            message_sh_ptr->len = p_packet_can_message->data_len;
            message_sh_ptr->data = std::vector<uint8_t>(&p_packet_can_message->data[0], &p_packet_can_message->data[p_packet_can_message->data_len]);
            //message_sh_ptr->Dump();
            rx_fn(message_sh_ptr);
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void TcpChannel::SetExtended(bool b)
{
    extended_comm = b;
    printf("Switch to extended protocol %d\n", extended_comm);
    std::vector<uint8_t> resp_data;
    resp_data.push_back(0xF1);
    resp_data.push_back(set_extended);
    resp_data.push_back(2);
    resp_data.push_back(0xF4);
    s_mutex.lock();
    if (channel_socket > 0)
        send(channel_socket, resp_data.data(), resp_data.size(), 0);
    s_mutex.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Odeslání do otevøeneho TCP kanálu
///////////////////////////////////////////////////////////////////////////////////////////////////
void TcpChannel::Send(std::shared_ptr<Message> message)
{
	if (extended_comm)
    {
        std::vector<uint8_t> v = message->ToTcpNew(can_message);
        if (v.size() > 0)
        {
            v = PrepareTcpData(v);
            s_mutex.lock();
            if (channel_socket > 0)
                send(channel_socket, v.data(), v.size(), 0);
            s_mutex.unlock();
        }
    }
    else
    {
		if (message->get_DEST() == old_can_id/* || message->get_DEST() >= 0xFE*/)
		{
			std::vector<uint8_t> v = message->ToTcpOld(can_message);
			if (v.size() > 0)
			{
				s_mutex.lock();
				if (channel_socket > 0)
					send(channel_socket, v.data(), v.size(), 0);
				s_mutex.unlock();
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////