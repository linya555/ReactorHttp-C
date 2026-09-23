#include "Dispatcher.h"
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#define Max 1024

static void* PollInit();
static int PollAdd(struct Channel* channel, struct EventLoop* evloop);
static int PollRemove(struct Channel* channel, struct EventLoop* evloop);
static int PollModify(struct Channel* channel, struct EventLoop* evloop);
static int Polldispatch(struct EventLoop* evloop, int timeout);
static int Pollclear(struct EventLoop* evloop);
struct Dispatcher PollDispatcher = {
	PollInit,
	PollAdd,
	PollRemove,
	PollModify,
	Polldispatch,
	Pollclear
};
//和poll相关的数据块
struct PollData {
	struct pollfd* pfds[Max];//poll函数第一个参数
	int maxfd;//相当于poll函数第二个参数，委托内核检查的数组元素个数
};
//初始化
static void* PollInit() {
	struct PollData* pdata = (struct PollData*)malloc(sizeof(struct PollData));
	pdata->maxfd = 0;
	for (int i = 0; i < Max; i++) {
		pdata->pfds[i]->fd = -1;//-1为无效文件描述符
		pdata->pfds[i]->revents = 0;//内核传出的就绪的事件
		pdata->pfds[i]->events = 0;//用户传入的要检测的事件
	}
	return pdata;
}
//添加
static int PollAdd(struct Channel* channel, struct EventLoop* evloop) {
	struct PollData* pdata = (struct PollData*)malloc(sizeof(struct PollData));
	int tevent = 0;
	if (channel->event & ReadEvent) {
		//读事件要监测
		tevent |= POLLIN;
	}
	if (channel->event & WriteEvent) {
		//写事件要监测
		tevent |= POLLOUT;
	}
	int i = 0;
	for (; i < Max; i++) {
		if (pdata->pfds[i]->fd == -1) {
			pdata->pfds[i]->fd = channel->fd;
			pdata->pfds[i]->events = tevent;
			//pdata->maxfd++;不能这么写，数组里面未使用文件描述符是动态变化的
			//有可能原来1 1 1 -1，因为调用删除函数，变成1 -1 1 ，就不能每次循环都pdata->maxfd++
			pdata->maxfd = i > pdata->maxfd ? i : pdata->maxfd;
			break;
		}
	}
	if (i >= Max) {
		return -1;
	}
	return 0;
}
//删除
static int PollRemove(struct Channel* channel, struct EventLoop* evloop) {
	struct PollData* pdata = (struct PollData*)malloc(sizeof(struct PollData));
	int i = 0;
	for (; i < Max; i++) {
		if (pdata->pfds[i]->fd == channel->fd) {
			pdata->pfds[i]->fd =-1;
			pdata->pfds[i]->events = 0;
			pdata->pfds[i]->revents = 0;
			break;
		}
	}
	if (i >= Max) {
		return -1;
	}
	return 0;
}
//更改
static int PollModify(struct Channel* channel, struct EventLoop* evloop) {
	struct PollData* pdata = (struct PollData*)malloc(sizeof(struct PollData));
	int tevent = 0;
	if (channel->event & ReadEvent) {
		//读事件要监测
		tevent |= POLLIN;
	}
	if (channel->event & WriteEvent) {
		//写事件要监测
		tevent |= POLLOUT;
	}
	int i = 0;
	for (; i < Max; i++) {
		if (pdata->pfds[i]->fd == channel->fd) {
			pdata->pfds[i]->events = tevent;
			break;
		}
	}
	if (i >= Max) {
		return -1;
	}
	return 0;
}
//事件监测
static int Polldispatch(struct EventLoop* evloop, int timeout) {
	struct PollData* pdata = (struct PollData*)malloc(sizeof(struct PollData));
	//mafd相当于内核要遍历的数组元素下标，而第二个参数是要遍历的个数，eg：下标为0遍历1个
	int count = poll(pdata->pfds, pdata->maxfd + 1, timeout * 1000);
	if (count == -1) {
		perror("poll");
		exit(0);
	}
	int i = 0;
	for (; i <= pdata->maxfd; i++) {
		if (pdata->pfds[i]->fd == -1) {
			continue;
		}
		if (pdata->pfds[i]->revents & POLLIN) {
			//读事件就绪，开始读相关操作
			EventActive(pdata->pfds[i]->fd, evloop, pdata->pfds[i]->revents);
		}
		if (pdata->pfds[i]->revents & POLLOUT) {
			//写事件就绪，开始写相关事件
			EventActive(pdata->pfds[i]->fd, evloop, pdata->pfds[i]->revents);
		}
	}
	
	return 0;
}
//清空数据（关闭fd或者释放内存）
static int Pollclear(struct EventLoop* evloop) {
	struct PollData* pdata = (struct PollData*)malloc(sizeof(struct PollData));
	free(pdata);
	return 0;
}