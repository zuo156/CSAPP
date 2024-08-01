#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "mm.h"
#include "memlib.h"


/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Ying Zuo",
    /* First member's email address */
    "yingzuo86@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

// begin macros
// Basic constant and macros

#define NUMLIST     4
#define WSIZE       4       /* word size (bytes) */
#define DSIZE       8       /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<12) /* initial heap size 4096(bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(size_t *)(p))
#define PUT(p, val)     (*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     ((GET(p)) & ~0x7)
#define GET_ALLOC(p)    ((GET(p)) & 0x1)

// /* Given an allocated-block ptr bp, compute address of its header, footer, payload in pred block, and payload in succ block */
// #define AL_HDRP(bp)        ((char *)(bp) - WSIZE)
// #define AL_FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// #define AL_NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(AL_HDRP(bp)))
// #define AL_PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(AL_HDRP(bp) - WSIZE))

/* Given a free-block ptr bp, compute address of its header, footer, payload in pred block, and payload in succ block */
#define HDRP(bp)        ((char *)(bp) - 3*WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - 4*WSIZE)
#define PREDP(bp)       ((char *)(bp) - DSIZE)
#define SUCCP(bp)       ((char *)(bp) - WSIZE)

#define PRED_VAL(bp)   ((*((size_t *)PREDP(bp))))
#define SUCC_VAL(bp)   ((*((size_t *)SUCCP(bp))))

/* Given a free-block ptr bp, compute address-adjacent blocks */
#define NEXTP(bp)  ((char *)(bp) + GET_SIZE(HDRP((char *)(bp))))
#define PREVP(bp)  ((char *)(bp) - (GET_SIZE(HDRP((char *)(bp)) - WSIZE)))

// #define NEXT_VAL(bp)  ((*((size_t *)NEXTP(bp))))
// #define PREV_VAL(bp)  ((*((size_t *)PREVP(bp))))

/* global variable*/
// char *list0, *list1, *list2, *list3, *end_list, *payload;
char *head_listp[NUMLIST+1], *payload;
/*  list0: 0-128
    list1: 128-512
    list2: 512-1024
    list3: 1024-
*/

/* function protoptypes for internal helper routines */
void *extend_heap(size_t words);
void place_link(void *bp, size_t asize);
void *find_fit(size_t asize);
void *address_coalesce(void *bp);
void checkblock(void *bp);
int size2list(int size);


int mm_init(void) {
    // initalize empty heap
    if ((payload = mem_sbrk(21 * WSIZE)) == NULL) { 
        return -1;
    }

    PUT(payload, 0);                                 // alignment padding

    head_listp[0] = payload + 4 * WSIZE;
    PUT(payload + 1 * WSIZE, PACK(4 * WSIZE,0));                        // header
    PUT(payload + 2 * WSIZE, 0);                                        // pred address
    // PUT(payload + 3 * WSIZE, (size_t)(payload + 8 * WSIZE));            // succ address
    PUT(payload + 4 * WSIZE, PACK(4 * WSIZE,0));   

    for (int i = 1; i < NUMLIST; i++) {
        head_listp[i] = payload + 4*(i+1) * WSIZE;
        PUT(head_listp[i] - 3 * WSIZE, PACK(4 * WSIZE,0));               // header
        PUT(head_listp[i-1] - 1 * WSIZE, (size_t)head_listp[i]);   // succ address of the previous head_listp
        PUT(head_listp[i] - 2 * WSIZE, (size_t)head_listp[i-1]);   // pred address            
        PUT(head_listp[i], PACK(4 * WSIZE,0));                           // footer 
    }

    head_listp[NUMLIST] = payload + 4*(NUMLIST+1) * WSIZE;
    PUT(head_listp[NUMLIST] - 3 * WSIZE, PACK(4 * WSIZE,0));               // header
    PUT(head_listp[NUMLIST] - 2 * WSIZE, (size_t)head_listp[NUMLIST-1]);   // pred address                        // pred address
    PUT(head_listp[NUMLIST] - 1 * WSIZE, 0);                               // succ address
    PUT(head_listp[NUMLIST], PACK(4 * WSIZE,0));                           // footer 

    PUT(head_listp[NUMLIST-1] - 1 * WSIZE, (size_t)head_listp[NUMLIST]);

    // Extend the empty heap with a free block of CHUNKSIZE bytes 
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

void *extend_heap(size_t words) {
    // last in first out
    char *bp, *old;
    size_t size;

    // Allocate an even number of words of words to maintain alignment
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;   
    // not always double-word because the choice of CHUNKSIZE
    // but the size from the mm_malloc is always double-word
    if ((int)(bp = mem_sbrk(size)) == -1)
        return NULL;

    bp = bp + 3 * WSIZE; // the address for payload/padding bytes

    // Initialize free block header and footer
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    // store the old head_listp[3] succ addres in prologue
    old = (char *)SUCC_VAL(head_listp[NUMLIST-1]);

    // update the succ address in prologue
    PUT(SUCCP(head_listp[NUMLIST-1]), (size_t)bp);

    // linking this free block
    PUT(PREDP(bp), (size_t)head_listp[NUMLIST-1]);
    PUT(SUCCP(bp), (size_t)old);

    PUT(PREDP(old), (size_t)bp);

    return address_coalesce(bp);
}

int size2list(int size) {
    if (size <= 128){
        return 0;
    } else if (size <= 512) {
        return 1;
    }
    else if (size <= 1024) {
        return 2;
    } else {
        return 3;
    }
}

void mm_free(void *bp) {
    bp = bp + DSIZE; 
    size_t size = GET_SIZE(HDRP(bp));
    // update the allocation status

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size,0));

    int index = size2list(size);
    // store the old succ addres in prologue
    char *old = (char *)SUCC_VAL(head_listp[index]);

    // update the succ address in prologue
    PUT(SUCCP(head_listp[index]), (size_t)bp);

    // linking this free block
    PUT(PREDP(bp), (size_t)head_listp[index]);
    PUT(SUCCP(bp), (size_t)old);

    PUT(PREDP(old), (size_t)bp);
    // looking at the address-adjacent block to see whether they are also free
    // if so combine them
    address_coalesce(bp);
}

void *address_coalesce(void *bp) {
    char *prev = PREVP(bp);
    char *next = NEXTP(bp);

    size_t next_alloc;
    if (next > (char *)mem_heap_hi()) {      // if next is end of the heap
        next_alloc = 1;
    }
    else {
        next_alloc = GET_ALLOC(HDRP(next));
    }

    size_t prev_alloc;
    if (prev == head_listp[NUMLIST]) {                  // if prev is the epilogue
        prev_alloc = 1;
    }
    else{
        prev_alloc = GET_ALLOC(FTRP(prev));
    }

    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* no coalesce */
        return bp;
    }

    if (!next_alloc) {      /* coalesce with next block */
	    size += GET_SIZE(HDRP(NEXTP(bp)));
	    PUT(HDRP(bp), PACK(size, 0));
	    PUT(FTRP(bp), PACK(size,0));
        /*
        what if the succ of bp is the next free-block?
        or what is the pred of bp is the next free-block?
        */
       
        if (next == (char *)SUCC_VAL(bp)) {
            // isolating bp and next_block together
            PUT(SUCCP(PRED_VAL(bp)), SUCC_VAL(next));
            PUT(PREDP(SUCC_VAL(next)), PRED_VAL(bp));
        }
        else if (next == (char *)PRED_VAL(bp)) {
            // isolating bp and next_block together
            PUT(SUCCP(PRED_VAL(next)), SUCC_VAL(bp));
            PUT(PREDP(SUCC_VAL(bp)), PRED_VAL(next));
        }
        else {
            // isolating the bp
            PUT(SUCCP(PRED_VAL(bp)),(size_t)SUCC_VAL(bp));
            PUT(PREDP(SUCC_VAL(bp)),(size_t)PRED_VAL(bp));
            // isolating the next_block
            PUT(SUCCP(PRED_VAL(next)),(size_t)SUCC_VAL(next));
            PUT(PREDP(SUCC_VAL(next)),(size_t)PRED_VAL(next));
        }
    }

    if (!prev_alloc) {      /* coalesce with prev block*/
	    size += GET_SIZE(HDRP(PREVP(bp)));
	    PUT(FTRP(bp), PACK(size, 0));
	    PUT(HDRP(prev), PACK(size, 0));
        /* 
        what if the succ of bp is the prev free-block 
        what if the pred of bp is the prev free-block
        */
        if (prev == (char *)SUCC_VAL(bp)) {
            PUT(SUCCP(PRED_VAL(bp)), SUCC_VAL(prev));
            PUT(PREDP(SUCC_VAL(prev)), PRED_VAL(bp));
        }
        else if (prev == (char *)PRED_VAL(bp)) {
            PUT(SUCCP(PRED_VAL(prev)), SUCC_VAL(bp));
            PUT(PREDP(SUCC_VAL(bp)), PRED_VAL(prev));
        }
        else {
            // isolating the bp_block
            PUT(SUCCP(PRED_VAL(bp)), (size_t)SUCC_VAL(bp));
            PUT(PREDP(SUCC_VAL(bp)), (size_t)PRED_VAL(bp));
            // isolating the prev_block
            PUT(SUCCP(PRED_VAL(prev)),(size_t)SUCC_VAL(prev));
            PUT(PREDP(SUCC_VAL(prev)),(size_t)PRED_VAL(prev));
        }
        bp = prev;
    }
    // move the merged free block to the begining of the specific list
    int index = size2list(size);
    char *old = (char *)SUCC_VAL(head_listp[index]);
    PUT(SUCCP(head_listp[index]), (size_t)bp);
    PUT(PREDP(bp), (size_t)head_listp[index]);
    PUT(SUCCP(bp), (size_t)old);
    PUT(PREDP(old), (size_t)bp);

    return bp;
}

void *mm_malloc(size_t size) {
    // the input size is byte, not the number of the words
    size_t asize;   // adjusted block size
    size_t extendsize; // amount to extend heap if no fit
    char *bp;

    /* Ignore suprious request */
    if (size <= 0)
        return NULL;
    
    // Adjust block size to include header and footer and to satisify alignment reqs.
    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    }
    else {
        asize = DSIZE * ((size + DSIZE + (DSIZE-1)) / DSIZE);
    }

    // Search the free list for a fit (first-fit)
    if ((bp = find_fit(asize)) != NULL) {
        place_link(bp, asize);
        return bp - DSIZE; // because allocated block doesn't need pred and succ
    }

    // No fit found. Get more memory and place the block
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
        return NULL;
    }
    place_link(bp, asize);
    return bp - DSIZE;
}

void *mm_realloc(void *ptr, size_t size) {
    return NULL;
}

void place_link(void *bp, size_t asize) {
    // bp is the situation in freed block
    size_t bsize = GET_SIZE(HDRP(bp));
    char *new_bp;
    if ((bsize - asize) >= 2 * DSIZE) {
        // isolating bp_block
        PUT(SUCCP(PRED_VAL(bp)), (size_t)SUCC_VAL(bp));
        PUT(PREDP(SUCC_VAL(bp)), (size_t)PRED_VAL(bp));
        // allocated block
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        // free block
        new_bp = (char *)NEXTP(bp);
        PUT(HDRP(new_bp), PACK(bsize - asize, 0));
        PUT(FTRP(new_bp), PACK(bsize - asize, 0));

        // move the merged free block to the begining of the specific list
        int index = size2list(bsize-asize);
        char *old = (char *)SUCC_VAL(head_listp[index]);
        PUT(SUCCP(head_listp[index]), (size_t)new_bp);
        PUT(PREDP(new_bp), (size_t)head_listp[index]);
        PUT(SUCCP(new_bp), (size_t)old);
        PUT(PREDP(old), (size_t)new_bp);
    }
    else {
        // isolating bp_block
        PUT(SUCCP(PRED_VAL(bp)), (size_t)SUCC_VAL(bp));
        PUT(PREDP(SUCC_VAL(bp)), (size_t)PRED_VAL(bp));
        // allocated block
        PUT(HDRP(bp), PACK(bsize, 1));
        PUT(FTRP(bp), PACK(bsize, 1));
    }
}

// first fit
void *find_fit(size_t asize) { 
    void *bp, *start;
    start = head_listp[size2list(asize)];
    for (bp = (char *)SUCC_VAL(start); bp != head_listp[NUMLIST]; bp = (char *)SUCC_VAL(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL; /* no fit */
}
