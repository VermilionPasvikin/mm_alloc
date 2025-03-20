#include <stdlib.h>
#include <stdio.h>

#define MAX_HEAP (1 << 20)

static char * mem_heap; //指向内存模型的第一个字节
static char * mem_brk; //指向内存模型的最后一个字节加1
static char * mem_max_addr; //最高的合法堆地址加一

//这个文件里的两个函数用c语言提供的malloc()函数向进程堆申请一片内存，然后在这片内存内部模拟内存分配器的运行

void mem_init(void)
{
    if((mem_heap = (char *)malloc(MAX_HEAP)) == NULL)
    {
        fprintf(stderr,"ERROR: mem_init failed, memory allocation failed.");
        exit(EXIT_FAILURE);
    }
    mem_brk = (char *)mem_heap;
    mem_max_addr = (char *)mem_heap + MAX_HEAP;
}

void * mem_sbrk(int increment)
{
    char * old_brk = mem_brk;
    if (increment < 0 || ((mem_brk + increment) > mem_max_addr))
    {
        fprintf(stderr,"ERROR: mem_sbrk failed. Run out memory");
        return (void *)-1;
    }
    
    mem_brk += increment;
    return old_brk;
}