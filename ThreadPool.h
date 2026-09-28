#pragma once
#include<stdio.h>
#include "WorkerThread.h"
struct ThreaddPool {
	bool isStart;//线程池是否开启
	int threadNum;//子线程个数
	int index;//用于数组中访问元素的下标
	struct WorkerThread* workerThreads;//存放WorkerThread的数组
	//主线程的反应堆模型
	//只用于接收客户端和服务器的通信
	//当threadNum为0时，用于和客户端通信
	//属于应急反应堆
	struct EventLoop* mainLoop;
};
//初始化线程池
struct ThreaddPool* threadPoolInit(struct EventLoop* mainLoop, int threadNum);
//线程池启动(主线程操作)
//给每一个子线程初始化并让每一个子线程启动
void threadPoolRun(struct ThreaddPool* pool);
//从线程池找一个子线程，取出反应堆实例，用于主线程往该反应堆里面添加任务
struct EventLoop* takeWorkerEventLoop(struct ThreaddPool* pool);