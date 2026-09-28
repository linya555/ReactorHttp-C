#include"WorkerThread.h"
#include <stdio.h>

int workerThreadInit(struct WorkerThread* thread, int index) {
	thread->evLoop = NULL;
	thread->threadID = 0;
	sprintf(thread->name, "subthread-%d", index);
	pthread_mutex_init(&thread->mutex, NULL);
	pthread_cond_init(&thread->cond, NULL);
	return 0;
}
//子线程回调函树
//作用启动反应堆模型，实施任务的处理
void* subThreadRunning(void* arg) {
	struct WorkerThread* thread = (struct WorkerThread*)arg;
	//初始化反应堆
	pthread_mutex_lock(&thread->mutex);
	thread->evLoop = eventLoopInitEx(thread->name);
	pthread_cond_signal(&thread->cond);
	pthread_mutex_unlock(&thread->mutex);
	//启动反应堆
	EventLoopRun(thread->evLoop);
	return NULL;
}
void workerThreadRun(struct WorkerThread* thread) {
	//创建子线程
	pthread_create(&thread->threadID, NULL, subThreadRunning, thread);
	//此时需要主阻塞线程
	//原因：需要等子线程把初始化反应堆步骤执行完才能使主线程跳出workerThreadRun函数
	//否则，主线程跳出函数后可能会执行往子线程的evloop里面添加任务
	//如果此时子线程的evloop还没初始化，就会错乱
	//所以要加条件变量。等子线程反应堆初始化结束再唤醒主线程继续执行

	//访问共享资源加锁
	pthread_mutex_lock(&thread->mutex);
	//要while循环
	while (thread->evLoop == NULL) {
		pthread_cond_wait(&thread->cond, &thread->mutex);
	}
	pthread_mutex_unlock(&thread->mutex);
}