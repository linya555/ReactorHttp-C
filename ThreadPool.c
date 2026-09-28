#include "ThreadPool.h"
#include <assert.h>
#include <stdlib.h>

struct ThreaddPool* threadPoolInit(struct EventLoop* mainLoop, int threadNum) {
	struct ThreaddPool* threadPool = (struct ThreaddPool*)malloc(sizeof(struct ThreaddPool));
	threadPool->isStart = false;
	threadPool->index = 0;
	threadPool->threadNum = threadNum;
	threadPool->mainLoop = mainLoop;
	threadPool->workerThreads = (struct WorkerThread*)malloc(threadNum * sizeof(struct WorkerThread));
	return threadPool;
}
void threadPoolRun(struct ThreaddPool* pool) {
	//如果线程池已经开启了或者线程池不存在直接结束
	assert(pool && !pool->isStart);
	//必须是主线程开启线程池
	if (pool->mainLoop->threadID != pthread_self()) {
		exit(0);
	}
	pool->isStart = true;
	if (pool->threadNum > 0) {
		for (int i = 0; i < pool->threadNum; i++) {
			workerThreadInit(&pool->workerThreads[i], i);
			workerThreadRun(&pool->workerThreads[i]);
		}
	}
	
}
struct EventLoop* takeWorkerEventLoop(struct ThreaddPool* pool) {
	//线程池必须开启
	assert(pool->isStart);
	//必须是主线程操作
	if (pool->mainLoop->threadID != pthread_self()) {
		exit(0);
	}
	//从线程池找一个子线程，取出反应堆实例
	struct EventLoop* evLoop;
	if (pool->threadNum > 0) {
		 evLoop= pool->workerThreads[pool->index].evLoop;
	}
	else {
		evLoop = pool->mainLoop;
	}
	return evLoop;
}