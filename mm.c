#include <stddef.h>
#include "memlib.h"
#include <stdio.h>

// 基础的常量与宏定义
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x > y) ? (x) : (y)) // 返回x与y中的最大值

#define PACK(size, isAlloc) (size | isAlloc) // 在一个块的头或脚中写入块大小与是否分配

#define GET(p) (*((unsigned int *)p))              // 读取一个字中存放的值
#define PUT(p, val) ((*((unsigned int *)p)) = val) // 在一个字中放入指定的值 

#define GET_SIZE(p) (GET(p) & (~0x7)) // 获取每个内存块头或脚中存储的内存块大小
#define GET_ALLOC(p) (GET(p) & (0x1)) // 获取每个内存块头或脚中存储的分配情况（0为未分配，1为已分配）

#define HDRP(bp) ((char *)bp - WSIZE)                      // 给定一个块指针bp，获得该内存块的头指针
#define FTRP(bp) ((char *)bp + GET_SIZE(HDRP(bp)) - DSIZE) // 给定一个块指针bp，获得该内存块的脚指针

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))           // 给定一个内存块的块指针bp，获取该内存块的下一个块的块指针
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)((bp) - DSIZE))) // 给定一个内存块的块指针bp，获取该内存块的前一个块的块指针

static char *heap_listp = NULL;

int mm_init(void);
static void *extend_heap(size_t words);
void mm_free(void *bp);
static void *coalesce(char *bp);
void *mm_malloc(size_t size);
static void *find_fit(size_t chars);
static void place(void *bp, size_t chars);

int mm_init(void)
{
    // 初始化堆：分配堆头的一个对齐填充块、序言块、和结尾块
    mem_init();
    if ((heap_listp = (char *)mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, PACK(0, 1)); // 对齐填充
    char * inti_ptr_0 = heap_listp; //这里不能直接递增heap_listp，我看了反汇编，heap_listp在加完WSIZE之后还会额外加一个0x10，似乎是静态数据区奇怪的对齐要求？
    char * inti_ptr_1 = inti_ptr_0 + WSIZE;
    char * inti_ptr_2 = inti_ptr_0 + 2 * WSIZE;
    char * inti_ptr_3 = inti_ptr_0 + 3 * WSIZE;
    PUT(inti_ptr_1, 9); // 序言块头
    PUT(inti_ptr_2, PACK(DSIZE, 1)); // 序言块脚
    PUT(inti_ptr_3, PACK(0, 1));     // 结尾块的头
    heap_listp = inti_ptr_0 + 4 * WSIZE; // 让heap_listp指向第一个普通块

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

static void *extend_heap(size_t words) // 以字为单位进行扩堆
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL; // 扩堆失败就返回NULL

    PUT(HDRP(bp), PACK(size, 0)); // 原先的结尾块在这时变成了新分配块的头
    PUT(FTRP(bp), PACK(size, 0));

    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 新的结尾块

    return coalesce(bp);
}

void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    coalesce(bp); // 立即合并空闲块
}

static void *coalesce(char *bp) // 立即合并空闲块策略
{
    /* char * next_blkp = NEXT_BLKP(bp);
    char * prev_blk_ftrp = bp - DSIZE;
    unsigned int prev_blk_sz = GET_SIZE(prev_blk_ftrp); 
    char * prev_blkp = bp - prev_blk_sz; */
    char * next_blkp = NEXT_BLKP(bp);
    char * prev_blkp = PREV_BLKP(bp);

    size_t prev_alloc = GET_ALLOC(HDRP(prev_blkp));
    size_t next_alloc = GET_ALLOC(HDRP(next_blkp));

    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) // 前后块都不是空闲
    {
        return bp;
    }

    if (prev_alloc && !next_alloc) // 后一个块是空闲块
    {
        size += GET_SIZE(HDRP(next_blkp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        return bp;
    }

    if (!prev_alloc && !next_alloc) // 前后块都是空闲块
    {
        size += (GET_SIZE(HDRP(prev_blkp)) + GET_SIZE(HDRP(next_blkp)));
        unsigned int pack = PACK(size, 0);
        PUT(HDRP(prev_blkp), pack);
        PUT(FTRP(prev_blkp), pack);

        return prev_blkp;
    }

    if (!prev_alloc && next_alloc) // 前一个块是空闲块
    {
        size += GET_SIZE(FTRP(prev_blkp));
        unsigned int pack = PACK(size, 0);
        PUT(HDRP(prev_blkp), pack);
        PUT(FTRP(prev_blkp), pack);
        
        return prev_blkp;
    }
}

void *mm_malloc(size_t size)
{
    size_t asize;
    char *bp;

    if (size <= DSIZE)
        asize = 16; // 2*DSIZE 如果请求的字节数小于双字，则分配一个四字的块（包含头和脚两个字、以及一个双字的块）
    else
        asize = DSIZE * ((size + 15 /*DSIZE + (DSIZE - 1)*/) / DSIZE); // 如果请求的字节数大于双字，则内存块大小向上舍入到最近的8的倍数

    if ((bp = find_fit(asize)) != NULL) // 找到了合适的空闲块
    {
        place(bp, asize);
        return bp;
    }

    // 如果没找到合适的空闲块，就申请更多的堆空间
    size_t extend_size;
    extend_size = MAX(asize / WSIZE, CHUNKSIZE / WSIZE);
    if ((bp = extend_heap(extend_size)) == NULL)
        return NULL; // 分配失败返回NULL
    place(bp, asize);
    return bp;
}

static void *find_fit(size_t chars) // 首次适配策略：返回查找到的第一个大小合适的内存块
{
    char *bp = heap_listp;

    while (1)
    {
        unsigned int is_alloc = GET_ALLOC(HDRP(bp));
        unsigned int size = GET_SIZE(HDRP(bp));
        if (is_alloc && (size == 0))
            return NULL; // 已分配且大小为0，说明取到结尾块了

        if (!is_alloc && (size >= chars))
            return bp; // 未分配且大小合适，查找命中
        else
            bp = NEXT_BLKP(bp); // 查找未命中，bp指针指向下一个块
    }
}

static void place(void *bp, size_t chars)
{
    char *hdrp = HDRP(bp);
    size_t size = GET_SIZE(hdrp);
    size_t differ = size - chars;

    if (differ < 16)
        chars = size; // 如果原先的空闲块大小比分配块长度大不到16字节，不切割块，直接分配这个空闲块

    unsigned int pack = PACK(chars, 1);
    PUT(hdrp, pack);
    PUT(FTRP(bp), pack);

    if (differ >= 16) // 如果原先的空闲块大小比分配块长度大至少16字节，说明需要切割块
    {
        unsigned int cutted_pack = PACK(differ, 0);
        char *next_blkp = NEXT_BLKP(bp);
        PUT(HDRP(next_blkp), cutted_pack);
        PUT(FTRP(next_blkp), cutted_pack);
    }
}