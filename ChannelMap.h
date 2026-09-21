#pragma once
#include <stdbool.h>
//定义结构体
struct ChannelMap {
	int size;//数组元素总个数
	//list是一个数组，每个元素都是一个指针，指向channel结构体
	struct Channel** list;
};
//初始化
struct ChannelMap* initChannelMap(int size);
//清空map
void channelMapClear(struct ChannelMap* map);
//对map进行扩容(第三个参数是单个槽位占用的字节大小)
bool makeMapRoom(struct ChannelMap* map, int newsize, int nuitsize);