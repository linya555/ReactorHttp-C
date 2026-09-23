#include "EventLoop.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
//写数据
void takeWakeup(struct EventLoop* evLoop) {
	const char* msg = "hello world";
	write(evLoop->socketPair[0], msg, strlen(msg));
}
//读数据
int readLocalMessage(void* arg) {
	struct EventLoop* evLoop = (struct EventLoop*)arg;
	char buf[1024];
	read(evLoop->socketPair[1], buf, sizeof(buf));
	return 0;
}
struct EventLoop* eventLoopInit() {
	eventLoopInitEx(NULL);
}
struct EventLoop* eventLoopInitEx(char* threadName) {
	struct EventLoop* evloop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
	//如果我们选择了epoll
	evloop->dispatcher = &EpollDispatcher;
	evloop->dispatcherData = evloop->dispatcher->init();
	evloop->channelMap = initChannelMap(128);
	evloop->head = NULL;
	evloop->isQuit = false;
	pthread_mutex_init(&evloop->mutex, NULL);
	evloop->tail = NULL;
	strcpy(evloop->threadName, threadName == NULL ? "MainThread" : threadName);
	evloop->threadID = pthread_self();
	//建立socket【1】和【0】通信
	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evloop->socketPair);
	if (ret == -1) {
		perror("socketpair");
		exit(0);
	}
	//设置0为写端，1为读端
	//在反应堆eventloop初始化的时候就要把我们安插的眼线socketpair【1】这个文件描述添加到任务队列里面
	//并设置成add也就是添加到那三个io复用待检测集合（eg：文件描述符上epoll树）
	//（从一开始就视奸待检测集合，一阻塞，就发力）
	struct Channel* channel = initChannel(evloop->socketPair[1], ReadEvent, readLocalMessage,NULL, evloop);
	EventLoopAddTask(channel, evloop, ADD);
	return evloop;
}
int EventLoopRun(struct EventLoop* eventLoop) {
	asssert(eventLoop != NULL);//一旦等于，直接触发终止
	//取出事件分发和检测模型
	struct Dispatcher* dispatcher = eventLoop->dispatcher;
	//比较事件是否正常
	if (eventLoop->threadID != pthread_self()) {
		return -1;
	}
	//进行事件循环处理
	while (eventLoop->isQuit == true) {
		dispatcher->dispatch(eventLoop, 2);
	}
	return 0;
}
int EventActive(int fd, struct EventLoop* eventLoop, int event) {
	if (fd < 0 || eventLoop == NULL) {
		return -1;
	}
	//取出channel
	struct Channel* channel = eventLoop->channelMap->list[fd];
	assret(channel->fd == fd);
	if (event & ReadEvent) {
		channel->readCallback(channel->arg);
	}
	if (event & WriteEvent) {
		channel->writeCallback(channel->arg);
	}
	return 0;
}
int EventLoopAddTask(struct Channel* channel, struct EventLoop* evLoop, int type) {
	//加锁，线程同步
	pthread_mutex_lock(&evLoop->mutex);
	//分配一个节点内存
	struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
	node->channel = channel;
	node->next = NULL;
	node->type = type;
	//node入队
	//队列还没有节点
	if (evLoop->head == NULL) {
		evLoop->head = evLoop->tail = node;
	}
	else {
		evLoop->tail->next = node;
		evLoop->tail = node;
	}

	pthread_mutex_unlock(&evLoop->mutex);
	//开始处理节点
	/*
	* 细节:
	*   1. 对于链表节点的添加: 可能是当前线程也可能是其他线程(主线程)
	*       1). 修改fd的事件, 当前子线程发起, 当前子线程处理
	*       2). 添加新的fd, 添加任务节点的操作是由主线程发起的
	*   2. 不能让主线程处理任务队列, 需要由当前的子线程取处理
	*/
	if (pthread_self() == evLoop->threadID) {

	}
	else {
		// 主线程 -- 告诉子线程处理任务队列中的任务
	   // 1. 子线程在工作 2. 子线程被阻塞了:select, poll, epoll
		//一旦用这个函数，就会往socketpair0里面写数据，socketpair1就会成为读就绪
		takeWakeup(evLoop);
	}
}