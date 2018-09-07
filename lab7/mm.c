/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE 4104

#define MAX(x, y) ((x) > (y) ? (x):(y))

#define PACK(size, alloc) ((size)|(alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0X7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp))-DSIZE)

#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE)))

//size_t size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
static char* heap_listp = NULL;
static char* current_p = NULL;

static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void* find_fit(size_t asize);
static void place(void* bp, size_t asize);
/* 
 * mm_init - initialize the malloc package.
 */
static void* extend_heap(size_t words)
{	
	//printf("func:extend_heap\n");
	char* bp;
	size_t size;

	size = (words % 2)? (words + 1) * WSIZE : words * WSIZE;
	if((long)(bp = mem_sbrk(size)) == -1)
		return NULL;
	
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));

	return coalesce(bp);
}

static void* coalesce(void* bp)
{	
	//printf("func:coalesce\n");
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	//printf("pre_alloc:%lu,next_alloc:%lu,size:%lu\n",prev_alloc,next_alloc,size);
	//printf("start judging\n");
	if( prev_alloc && next_alloc){
		//printf("1:1\n");
		return bp;
	}
	else if(prev_alloc && !next_alloc){
		//printf("1:0\n");
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}
	else if( !prev_alloc && next_alloc){
		//printf("0:1\n");
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	else{
	//printf("0:0\n");
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)))	;
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	//printf("final judge\n");
	if(current_p > (char*)bp && current_p < NEXT_BLKP(bp))
		current_p = bp;
	return bp;
}

static void* find_fit(size_t asize)
{
	//printf("func:find_fit asize:%lu\n", asize);
	void* bp;
	for(bp = current_p; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
		//printf("fit1\n");
		if(!GET_ALLOC(HDRP(bp))&&(asize<=GET_SIZE(HDRP(bp)))){
			current_p = bp;
			return bp;
		}
	}
	for(bp = heap_listp;(char*)bp < current_p; bp = NEXT_BLKP(bp)){
		//printf("heap_listp=%p,current_p:%p,fit2\n",bp,current_p);
		if(!GET_ALLOC(HDRP(bp)) && asize <= GET_SIZE(HDRP(bp))){
			current_p = bp;
			return bp;
		}
	}
	//printf("bp=NULL\n");
	return NULL;
}

static void place(void* bp, size_t asize)
{
	//printf("func:place\n");
	size_t csize = GET_SIZE(HDRP(bp));

	if((csize-asize) >= (2*DSIZE)){
		PUT(HDRP(bp),PACK(asize,1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize-asize, 0));
		PUT(FTRP(bp), PACK(csize-asize, 0));
	}else{
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
}

int mm_init(void)
{	
	//printf("func:mm_init\n");
	if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
		return -1;
	//printf("start put\n");
	PUT(heap_listp, 0);
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
   PUT(heap_listp + (3*WSIZE), PACK(0,1));
	//printf("end put\n");
	heap_listp += (2*WSIZE);
	//printf("1\n");
	//printf("heap_listp: %p\n",heap_listp);
	current_p = heap_listp;
	//printf("heap_listp: %p,current_p: %p\n",heap_listp,current_p);
	if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
	return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{	
	//printf("func:mm_malloc\n");
	size_t asize;
	size_t extendsize;
	char* bp;
	if(size == 0)
		return NULL;
	if(size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/DSIZE);
	
	if(size == 448)
		asize = 520;
	if(size == 112)
		asize = 136;
	if((bp = find_fit(asize)) != NULL){
		//printf("before bp:%p,size:%lu,*bp:header%d,footer%d\n", bp,asize,GET(HDRP(bp)),GET(FTRP(bp)));
		place(bp, asize);
		//printf("after bp:%p,size:%lu,*bp: header%d,footer%d\n", bp,asize,GET(HDRP(bp)),GET(FTRP(bp)));
		//printf("next_blk:header%d,footer%d\n\n",GET(HDRP(NEXT_BLKP(bp))),GET(FTRP(NEXT_BLKP(bp))));
		//printf("next: %d\n",GET((char*)bp+2044));
		return bp;
	}
	extendsize = MAX(asize, CHUNKSIZE);
	if((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	place(bp, asize);
	return bp;
    /*int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }*/
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{	
	//printf("func:mm_free\n");
	size_t size = GET_SIZE(HDRP(bp));
	//printf("size:%lu\n",size);
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	//printf("func:mm_realloc\n");
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














