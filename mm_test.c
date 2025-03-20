#include <stdio.h>

#define WSIZE 4
#define DSIZE 8

#define GET(p) (*((unsigned int *)p)) //读取一个字中存放的值
#define PUT(p, val) ((*((unsigned int *)p)) = val) //在一个字中放入指定的值

#define GET_SIZE(p) (GET(p) & (~0x7)) //获取每个内存块头或脚中存储的内存块大小
#define GET_ALLOC(p) (GET(p) & (0x1)) //获取每个内存块头或脚中存储的分配情况（0为未分配，1为已分配）

extern int mm_init(void);
extern void mm_free(void *bp);
extern void * mm_malloc(size_t size);

int main() {
    mm_init();
    int * bp1 = mm_malloc(1024*sizeof(int));
    printf("%p\n", bp1);

    printf("%d\n", *((char *)bp1 - WSIZE));
    mm_free(bp1);
    printf("%d\n", *((char *)bp1 - WSIZE));

    int * bp2 = mm_malloc(5*sizeof(int));
    printf("%p", bp2);

    mm_free(bp2);

    return 0;
}