#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <linux/net_tstamp.h>
#include "can_socket.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CanSocket::CanSocket()
{
	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) 
	{
	}

	struct ifreq ifr;
	const char* ifname = "can0";

	strcpy(ifr.ifr_name, ifname);
	ioctl(s, SIOCGIFINDEX, &ifr);

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;


	/*const int timestamping_flags = (SOF_TIMESTAMPING_SOFTWARE |
		SOF_TIMESTAMPING_RX_SOFTWARE |
		SOF_TIMESTAMPING_RAW_HARDWARE);

	if (setsockopt(s, SOL_SOCKET, SO_TIMESTAMPING, &timestamping_flags, sizeof(timestamping_flags)) < 0) 
	{
		perror("setsockopt SO_TIMESTAMPING is not supported by your Linux kernel");
	//	return 1;
	}*/

	const int timestamp_on = 1;
	if (setsockopt(s, SOL_SOCKET, SO_TIMESTAMP, &timestamp_on, sizeof(timestamp_on)) < 0) 
	{
	//	perror("setsockopt SO_TIMESTAMP");
	//	return 1;
	}

	if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == -1)
	{
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CanSocket::~CanSocket()
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CanSocket::Send(struct can_frame *frame)
{
	//printf("CanSocket::Send entry\n");
	write(s, frame, sizeof(struct can_frame));
	//printf("CanSocket::Send exit\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CanSocket::Run()
{
	printf("CanSocket::Run entry\n");
	can_thread = std::thread(&CanSocket::RunThread, this);
	printf("CanSocket::Run exit\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CanSocket::RunThread()
{
	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) +
		CMSG_SPACE(3 * sizeof(struct timespec)) +
		CMSG_SPACE(sizeof(__u32))];

	struct msghdr msg;
	struct iovec iov;
	struct canfd_frame frame;
	struct cmsghdr* cmsg;
	struct timeval tv;

	iov.iov_base = &frame;
	msg.msg_name = &addr;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &ctrlmsg;

	while (true)
	{
		iov.iov_len = sizeof(frame);
		msg.msg_namelen = sizeof(addr);
		msg.msg_controllen = sizeof(ctrlmsg);
		msg.msg_flags = 0;

		long int nbytes = recvmsg(s, &msg, 0);

		//printf("recvmsg %d\n", nbytes);

		if (nbytes < 0)
		{
			fprintf(stderr, "can0: error %d\n", errno);
			if ((errno == ENETDOWN))
				fprintf(stderr, "can0: interface down\n");

			continue;
		}

		if ((size_t)nbytes == CAN_MTU)
			/*maxdlen = CAN_MAX_DLEN*/;
		else if ((size_t)nbytes == CANFD_MTU)
			/*maxdlen = CANFD_MAX_DLEN */;
		else
			fprintf(stderr, "read: incomplete CAN frame\n");


		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg && (cmsg->cmsg_level == SOL_SOCKET); cmsg = CMSG_NXTHDR(&msg, cmsg))
		{
			if (cmsg->cmsg_type == SO_TIMESTAMP) 
			{
				memcpy(&tv, CMSG_DATA(cmsg), sizeof(tv));
			}
			/*else if (cmsg->cmsg_type == SO_TIMESTAMPING)
			{
				struct timespec* stamp = (struct timespec*)CMSG_DATA(cmsg);

				// stamp[0] is the software timestamp
				// stamp[1] is deprecated
				// stamp[2] is the raw hardware timestamp
				// See chapter 2.1.2 Receive timestamps in
				// linux/Documentation/networking/timestamping.txt

				tv.tv_sec = stamp[2].tv_sec;
				tv.tv_usec = stamp[2].tv_nsec / 1000;
			}*/
			else if (cmsg->cmsg_type == SO_RXQ_OVFL) 
			{
				__u32 dropcnt;
				memcpy(&dropcnt, CMSG_DATA(cmsg), sizeof(__u32));
				fprintf(stderr, "dropcnt %ld\n", dropcnt);
			}
		}

		if (frame.can_id & CAN_RTR_FLAG)
			fprintf(stderr, "CAN Retry flag ?\n");
		if (frame.can_id & CAN_ERR_FLAG)
			fprintf(stderr, "CAN Err flag ?\n");
		if (frame.can_id & CAN_EFF_FLAG)
			Receive(&tv, &frame); // OK mame zpravu
		else
			fprintf(stderr, "Zprava z kratkym identifikatorem ?\n");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////