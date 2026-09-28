#include "EventLoop.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <unistd.h>

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
		//原因
		eventLoopProcessTask(eventLoop);
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
		eventLoopProcessTask(evLoop);
	}
	else {
		// 主线程 -- 告诉子线程处理任务队列中的任务
	    // 1. 子线程在工作 2. 子线程被阻塞了:select, poll, epoll
		//一旦用这个函数，就会往socketpair0里面写数据，socketpair1就会成为读就绪
		takeWakeup(evLoop);
	}
}
int eventLoopProcessTask(struct EventLoop* evLoop) {
	pthread_mutex_lock(&evLoop->mutex);
	//取出头节点
	struct ChannelElement* head = evLoop->head;
	while (head != NULL) {
		struct ChannelElement* temp = head;
		if (head->type == ADD) {
			//增加
			eventLoopAdd(evLoop, head);
		}
		if (head->type == DELETE) {
			//删除
			eventLoopRemove(evLoop, head);
			//还需要把对应的channelmap中的channel释放掉，并关闭文件描述符
		}
		if (head->type == MODIFY) {
			//修改
			eventLoopModify(evLoop, head);
		}

		head = head->next;
		free(temp);
	}
	evLoop->head = evLoop->tail = NULL;
	pthread_mutex_lock(&evLoop->mutex);
}
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel) {
	struct ChannelMap* channelmap = evLoop->channelMap;
	int fd = channel->fd;
	int ret;
	//容量不够需要扩容
	if (fd >= channelmap->size) {
		if (makeMapRoom(channelmap, fd, sizeof(struct Channel*))==false) {
			return  -1;
		}
	}
	//找到fd对应的channelmap的数组位置，将channel放进去
	if (channelmap->list[fd] == NULL) {
		channelmap->list[fd] = channel;
		//并将fd加入对应的检测集合里面
		evLoop->dispatcher->add(channel, evLoop);
	}
	return ret;
}
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel) {
	struct ChannelMap* channelmap = evLoop->channelMap;
	int fd = channel->fd;
	//fd不在检测集合里面
	//因为每一个channelmap里面的元素都有在eventLoopAdd中被添加到检测集合里面
	if (fd >= channelmap->size||channelmap->list[fd]==NULL) {
		return -1;
	}
	int ret=evLoop->dispatcher->remove(channel, evLoop);
	return ret;
}
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel) {
	struct ChannelMap* channelmap = evLoop->channelMap;
	int fd = channel->fd;
	if (fd >= channelmap->size || channelmap->list[fd] == NULL) {
		return -1;
	}
	int ret = evLoop->dispatcher->modify(channel, evLoop);
	return ret;
}
int destroyChannel(struct EventLoop* evLoop, struct Channel* channel) {
	struct ChannelMap* channelmap = evLoop->channelMap;
	int fd = channel->fd;
	channelmap->list[fd] == NULL;
	close(fd);
	free(channel);
}