#include <string.h>

#include "can_hasam.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
CanHasam::CanHasam(std::function<void(std::shared_ptr<Message>)> fn)
{
	rx_fn = fn;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CanHasam::Receive(struct timeval *tv, struct canfd_frame* frame)
{
	canid_t canid = Message::CanIdFromFrame(frame);
	printf("%8.8X\n", canid);

	if (frame->can_id & FirstPacketLongMsg_Mask)
	{
		// Uvodn� ��st dlouh� zpr�vy
		if (auto search = long_msg_map.find(canid); search != long_msg_map.end())
		{
			fprintf(stderr, "Prisel zacatek dlouhe zpravy a predesla jests nebyla dokoncena (0x%8.8X)\n", canid);
		}
		std::shared_ptr<Message> message_sh_ptr(new Message(tv, frame));
		long_msg_map.insert_or_assign(canid, message_sh_ptr);
	}
	else if (frame->can_id & LongMsg_Mask)
	{
		// Dal�� ��st dlouh� zpr�vy
		if (auto search = long_msg_map.find(canid); search != long_msg_map.end())
		{ 
			if (search->second->AddNextFrame(tv, frame))
			{
				rx_fn(search->second);
				long_msg_map.extract(search);
			}
		}
		else
		{
			fprintf(stderr, "Nemame zacatek dlouhe zpravy (0x%8.8X)\n", canid);
		}
	}
	else
	{
		// Nejedn� se o dlouhou zpr�vu
		std::shared_ptr<Message> message_sh_ptr(new Message(tv, frame));
		rx_fn(message_sh_ptr);
	}
}
