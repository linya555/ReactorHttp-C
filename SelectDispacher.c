#include "Dispatcher.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>

#define Max 1024

static void* SelectInit();
static int SelectAdd(struct Channel* channel, struct EventLoop* evloop);
static int SelectRemove(struct Channel* channel, struct EventLoop* evloop);
static int SelectModify(struct Channel* channel, struct EventLoop* evloop);
static int SelectDispatch(struct EventLoop* evloop, int timeout);
static int SelectClear(struct EventLoop* evloop);
static void SetFdset(struct Channel* channel, struct SelectData* sdata);
static void ClearFdset(struct Channel* channel, struct SelectData* sdata);
struct Dispatcher SelectDispatcher = {
	SelectInit,
	SelectAdd,
	SelectRemove,
	SelectModify,
	SelectDispatch,
	SelectClear
};
//和select相关的数据块
struct SelectData {
	fd_set readSet;//传入传出参数，select里面的读集合
	fd_set writeSet;//传入传出参数，select里面的写集合
};
//封装添加事件
static void SetFdset(struct Channel* channel, struct SelectData* sdata) {
	if (channel->event & ReadEvent) {
		//读事件要监测
		FD_SET(channel->fd, &sdata->readSet);
	}
	if (channel->event & WriteEvent) {
		//写事件要监测
		FD_SET(channel->fd, &sdata->writeSet);
	}
}
//封装删除事件
static void ClearFdset(struct Channel* channel, struct SelectData* sdata) {
	if (channel->event & ReadEvent) {
		FD_CLR(channel->fd, &sdata->readSet);
	}
	if (channel->event & WriteEvent) {
		FD_CLR(channel->fd, &sdata->writeSet);
	}
}
//初始化
static void* SelectInit() {
	struct SelectData* sdata = (struct SelectData*)malloc(sizeof(struct SelectData));
	FD_ZERO(&sdata->readSet);
	FD_ZERO(&sdata->writeSet);
	return sdata;
}
//添加
static int SelectAdd(struct Channel* channel, struct EventLoop* evloop) {
	struct SelectData* sdata = (struct SelectData*)malloc(sizeof(struct SelectData));
	/*if (channel->event & ReadEvent) {
		//读事件要监测
		FD_SET(channel->fd, &sdata->readSet);
	}
	if (channel->event & WriteEvent) {
		//写事件要监测
		FD_SET(channel->fd, &sdata->writeSet);
	}*/
	SetFdset(channel, sdata);
	return 0;
}
//删除
static int SelectRemove(struct Channel* channel, struct EventLoop* evloop) {
	struct SelectData* sdata = (struct SelectData*)malloc(sizeof(struct SelectData));
	/*if (channel->event & ReadEvent) {
		FD_CLR(channel->fd, &sdata->readSet);
	}
	if (channel->event & WriteEvent) {
		FD_CLR(channel->fd, &sdata->writeSet);
	}*/
	ClearFdset(channel, sdata);
	return 0;
}
//更改
static int SelectModify(struct Channel* channel, struct EventLoop* evloop) {
	struct SelectData* sdata = (struct SelectData*)malloc(sizeof(struct SelectData));
	SetFdset(channel, sdata);
	ClearFdset(channel, sdata);
	return 0;
}
//事件监测
static int SelectDispatch(struct EventLoop* evloop, int timeout) {
	struct SelectData* sdata = (struct SelectData*)malloc(sizeof(struct SelectData));
	struct timeval time;
	time.tv_sec = timeout;
	time.tv_usec = 0;
	//readset,writeset每次都会减少所以要保留原始值
	fd_set rdtemp = sdata->readSet;
	fd_set wttemp = sdata->writeSet;
	int count = select(Max, &rdtemp, &wttemp, NULL, &time);
	if (count == -1) {
		perror("select");
		exit(0);
	}
	for (int i = 0; i < Max; i++) {
		if (FD_ISSET(i, &rdtemp)) {

		}
		if (FD_ISSET(i, &wttemp)) {

		}
	}
	return 0;
}
//清空数据（关闭fd或者释放内存）
static int SelectClear(struct EventLoop* evloop) {
	struct SelectData* sdata = (struct SelectData*)malloc(sizeof(struct SelectData));
	free(sdata);
	return 0;
}