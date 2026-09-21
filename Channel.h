#pragma once
#include <stdbool.h>
//定义Channel结构体
typedef int (*handleFunc)(void* arg);
//定义文件描述符读写事件
enum FDEvent {
	TimeOut = 0x01,
	ReadEvent = 0x02,
	WriteEvent = 0x04,
};
struct Channel {
	int fd;//文件描述符
	int event;//事件
	handleFunc readCallback;//读操作回调函数
	handleFunc writeCallback;//写操作回调函数
	void* arg;//回调函数参数

};
//初始化一个channel
struct Channel* initChannel(int fd, int event, handleFunc readCallback, handleFunc writeCallback, void* arg);
//修改fd写事件状态(检测/不检测)
void writeEventEnable(struct Channel* channel, bool flag);
//写事件状态不需要一直监听，缓冲区绝大多数时候都是有空的
//判断是否需要检测文件描述符的写事件
bool iswriteEventEnable(struct Channel* channel);