#include "Dispatcher.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#define Max 520

static void* epollInit();
static int epollAdd(struct Channel* channel, struct EventLoop* evloop);
static int epollRemove(struct Channel* channel, struct EventLoop* evloop);
static int epollModify(struct Channel* channel, struct EventLoop* evloop);
static int epolldispatch(struct EventLoop* evloop, int timeout);
static int epollclear(struct EventLoop* evloop);
static int epollCtrl(struct Channel* channel, struct EventLoop* evloop,int op);
struct Dispatcher epollDispatcher = {
	epollInit,
	epollAdd,
	epollRemove,
	epollModify,
	epolldispatch,
	epollclear
};
//和epoll相关的数据块
struct epollData {
	int epfd;//红黑树根节点
	struct epoll_event* events;//相关的检测事件,可作为传出参数，获取epollwait得到的就绪事件
};
//初始化epoll需要的数据块
static void* epollInit() {
	struct epollData* edata = (struct epollData*)malloc(sizeof(struct epollData));
	edata->epfd = epoll_create(10);
	if (edata->epfd == -1) {
		perror("epoll_create");
		exit(0);//服务器进程退出
	}
	edata->events = (struct epoll_event*)calloc(Max,sizeof(struct epoll_event));
	return edata;
}
//添加删除更改内部有完全一样的部分，我们进行封装
static int epollCtrl(struct Channel* channel, struct EventLoop* evloop,int op) {
	struct epollData* edata = (struct epollData*)evloop->dispatcherData;
	struct epoll_event ev;
	ev.data.fd = channel->fd;
	int tevent = 0;
	if (channel->event & ReadEvent) {
		//读事件要监测
		tevent |= EPOLLIN;
	}
	if (channel->event & WriteEvent) {
		//写事件要监测
		tevent |= EPOLLOUT;
	}
	ev.events = tevent;
	int ret = epoll_ctl(edata->epfd, op, channel->fd, &ev);
	return ret;
}
//添加
//将文件描述符入epoll树并设置检测事件
//文件描述符，检测事件：channel。epoll树根节点，要内核检测的事件evloop里面的dispatcherData
//直白：dispatcherData，一个指针，在epoll这个模块指向epoll初始化的epolldata
static int epollAdd(struct Channel* channel, struct EventLoop* evloop) {
	/*struct epollData* edata = (struct epollData*)evloop->dispatcherData;
	struct epoll_event ev;
	ev.data.fd = channel->fd;
	int tevent = 0;
	if (channel->event & ReadEvent) {
		//读事件要监测
		tevent |= EPOLLIN;
	}
	if (channel->event & WriteEvent) {
		//写事件要监测
		tevent |= EPOLLOUT;
	}
	ev.events = tevent;
	int ret = epoll_ctl(edata->epfd, EPOLL_CTL_ADD, channel->fd, &ev);*/
	int ret=epollCtrl(channel, evloop, EPOLL_CTL_ADD);
	if (ret == -1) {
		perror("epoll_ctl add");
		exit(0);
	}
	return ret;
}
//删除
static int epollRemove(struct Channel* channel, struct EventLoop* evloop) {
	/*struct epollData* edata = (struct epollData*)evloop->dispatcherData;
	struct epoll_event ev;
	ev.data.fd = channel->fd;
	int tevent = 0;
	if (channel->event & ReadEvent) {
		//读事件要监测
		tevent |= EPOLLIN;
	}
	if (channel->event & WriteEvent) {
		//写事件要监测
		tevent |= EPOLLOUT;
	}
	ev.events = tevent;
	int ret = epoll_ctl(edata->epfd, EPOLL_CTL_DEL, channel->fd, &ev);*/
	int ret = epollCtrl(channel, evloop, EPOLL_CTL_DEL);
	if (ret == -1) {
		perror("epoll_ctl delete");
		exit(0);
	}
	return ret;
}
//更改
static int epollModify(struct Channel* channel, struct EventLoop* evloop) {
	/*struct epollData* edata = (struct epollData*)evloop->dispatcherData;
	struct epoll_event ev;
	ev.data.fd = channel->fd;
	int tevent = 0;
	if (channel->event & ReadEvent) {
		//读事件要监测
		tevent |= EPOLLIN;
	}
	if (channel->event & WriteEvent) {
		//写事件要监测
		tevent |= EPOLLOUT;
	}
	ev.events = tevent;
	int ret = epoll_ctl(edata->epfd, EPOLL_CTL_MOD, channel->fd, &ev);*/
	int ret = epollCtrl(channel, evloop, EPOLL_CTL_MOD);
	if (ret == -1) {
		perror("epoll_ctl delete");
		exit(0);
	}
	return ret;
}
//事件监测
static int epolldispatch(struct EventLoop* evloop, int timeout) {
	struct epollData* edata = (struct epollData*)evloop->dispatcherData;
	int num = epoll_wait(edata->epfd, edata->events, Max, timeout * 1000);
	for (int i = 0; i < num; i++) {
		int fd = edata->events[i].data.fd;
		int event = edata->events[i].events;
		if (event & EPOLLERR || event & EPOLLHUP) {
			//对方断开连接
			//epollRemove(channel,evloop)
			continue;
		}
		if (event & ReadEvent) {
			//读事件就绪，开始读相关操作
		}
		if (event & WriteEvent) {
			//写事件就绪，开始写相关事件
		}
	}
	return 0;
}
//清空数据（关闭fd或者释放内存）
static int epollclear(struct EventLoop* evloop) {
	struct epollData* edata = (struct epollData*)evloop->dispatcherData;
	free(edata->events);
	close(edata->epfd);
	free(edata);
	return 0;
}