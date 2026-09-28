#pragma once
#include<pthread.h>
#include "EventLoop.h"
struct WorkerThread {
	pthread_t threadID;
	char name[24];
	struct EventLoop* evLoop;//每一个子线程都有一个反应堆模型
	pthread_mutex_t mutex;//互斥锁
	pthread_cond_t cond;//条件变量
};
//初始化子线程,外部传入一个已经分配好内存的thread，只需要初始化就好，index：线程编号
int workerThreadInit(struct WorkerThread* thread, int index);
//启动线程（让主线程创建子线程）
void workerThreadRun(struct WorkerThread* thread);