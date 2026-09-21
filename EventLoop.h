#pragma once
#include "Dispatcher.h"
struct EventLoop {
	struct Dispatcher* dispatcher;
	//万能指针成员变量
	//用来保存多路复用数据epoll时存fd，epoll_event 数组；poll 就存 pollfd 数组。
	void* dispatcherData;	
};