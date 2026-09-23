//反应堆模型
#pragma once
#include "Dispatcher.h"
#include "Channel.h"
#include "ChannelMap.h"
#include<pthread.h>
//声明，可以直接使用在别的.c定义好的这些结构体，防止重复定义
extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;
// 处理该节点中的channel的方式
enum ElemType { ADD, DELETE, MODIFY };
//定义任务队列节点
struct ChannelElement {
	int type;//进行什么任务操作，添加，删除。。。
	struct Channel* channel;
	struct ChannelElement* next;//指向下一个元素指针
};

struct EventLoop {
	struct Dispatcher* dispatcher;
	//万能指针成员变量
	//用来保存多路复用数据epoll时存fd，epoll_event 数组；poll 就存 pollfd 数组。
	void* dispatcherData;	
	bool isQuit;//是否开启
	//任务队列头尾节点
	struct ChannelElement* head;
	struct ChannelElement* tail;
	//map
	struct ChannelMap* channelMap;
	//线程相关
	pthread_t threadID;
	char* threadName;
	pthread_mutex_t mutex;
	int socketPair[2];  // 自己定义的间谍文件描述符，用于当子线程在poll epoll select阻塞时，解除阻塞
};
//主线程eventloop
struct EventLoop* eventLoopInit();
//子线程eventloop
struct EventLoop* eventLoopInitEx(char* threadName);
//启动反应堆模型
int EventLoopRun(struct EventLoop* eventLoop);
//相关事件就绪行为函数
//通过三大io复用的dispatcher操作得到就绪fd
//通过fd找到channelmap里面的channel
//通过传入的就绪的读/写状态进行相应的回调函数
int EventActive(int fd, struct EventLoop* eventLoop, int event);
//给任务队列添加节点
int EventLoopAddTask(struct Channel* channel, struct EventLoop* evLoop,int type);
//唤醒子线程函数，通过socketpair，socketpair0给socketpair1传数据，让socketpair1读就绪
void takeWakeup(struct EventLoop* evLoop);
int readLocalMessage(void* arg);