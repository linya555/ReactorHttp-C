#include "ChannelMap.h"
#include <stdlib.h>
#include <string.h>

struct ChannelMap* initChannelMap(int size) {
	//开辟一块ChannelMap内存
	struct ChannelMap* map = (struct ChannelMap*)malloc(sizeof(struct ChannelMap));
	map->size = size;
	//给map每个元素开辟内存（每个元素也是指针）
	map->list = (struct ChannelMap**)malloc(size * sizeof(struct Channel*));
	return map;
}
void channelMapClear(struct ChannelMap* map) {
	if (map != NULL) {
		//将list每个元素释放
		for (int i = 0; i < map->size; i++) {
			if (map->list[i] != NULL)
				free(map->list[i]);
		}
		//将list数组释放
		free(map->list);
		map->list = NULL;
	}
	map->size = 0;
}
bool makeMapRoom(struct ChannelMap* map, int newsize, int nuitsize) {
	//只有小于newsize才需要扩容
	if (map->size < newsize) {
		//一次性扩容两倍,当比newsize大时，开始一次性扩容
		int curSize = map->size;
		while (curSize < newsize) {
			curSize *= curSize;
		}
		//设置一个临时变量，reelloc后的地址可能不是原来的要覆盖
		struct Channel** temp = realloc(map->list, curSize * nuitsize);
		if (temp == NULL)
			return false;
		map->list = temp;
		//将新扩容出来的部分置为0
		memset(&map->list[map->size], 0, (curSize - map->size)*nuitsize);
		map->size = curSize;
	}
	return true;
}