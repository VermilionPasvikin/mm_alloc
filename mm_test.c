#include <stdio.h>

extern int mm_init(void);
extern void mm_free(void *bp);
extern void * mm_malloc(size_t size);

int main() {
    mm_init();
    int * bp1 = mm_malloc(1024*sizeof(int));

    printf("bp1:%p\n", bp1);
    mm_free(bp1);

    int * bp2 = mm_malloc(1025*sizeof(int));
    printf("bp2:%p\n",bp2);

    return 0;
} 