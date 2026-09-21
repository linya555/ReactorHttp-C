#include"Channel.h"
#include <stdlib.h>

struct Channel* initChannel(int fd, int event, handleFunc readCallback, handleFunc writeCallback, void* arg) {
	struct Channel* channel = (struct Channel*)malloc(sizeof(struct Channel));
	channel->fd = fd;
	channel->event = event;
	channel->readCallback = readCallback;
	channel->writeCallback = writeCallback;
	channel->arg = arg;
	return channel;
}
void writeEventEnable(struct Channel* channel, bool flag) {
	//ÐèÒª¼àÌý
	if (flag) {
		channel->event |= WriteEvent;
	}
	else {
		channel->event = channel->event & ~WriteEvent;
	}
}
bool iswriteEventEnable(struct Channel* channel) {
	return channel->event & WriteEvent;
}