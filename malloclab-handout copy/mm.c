/*
    malloc allocator based on single explicit double-linked lists 
    LIFO (last in first out) principle for free an allocated block

    Free block
      31                     3  2  1  0 
     -----------------------------------
    | s  s  s  s  ... s  s  s  0  0  a/f|
     ----------------------------------- 
    |     pointing to predecessor       |
     ----------------------------------- 
    |     pointing to successor         |
     ----------------------------------- 
    |             padding               |
     ----------------------------------- 
    | s  s  s  s  ... s  s  s  0  0  a/f|

    Allocated block
      31                     3  2  1  0 
     -----------------------------------
    | s  s  s  s  ... s  s  s  0  0  a/f|
     ----------------------------------- 
    |             payload               |
     ----------------------------------- 
    |             padding               |
     ----------------------------------- 
    | s  s  s  s  ... s  s  s  0  0  a/f|

    Prologue block
    | hdr(8:a) | head_listp | succ | ftr(8:a) |
    Epilogue block
    | hdr(8:a) | pred | end_listp | ftr(8:a) |
        Regular block
    | hdr(8:a) |( pred | payload (| padding) | succ |) ftr(8:a) |
    Therefore, the smallest block size is 4-word
    
*/
#include <stdio.h>
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
#define checkheap() mm_checkheap()

#define WSIZE       4       /* word size (bytes) */
#define DSIZE       8       /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<12) /* initial heap size (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(size_t *)(p))
#define PUT(p, val)     (*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     ((GET(p)) & ~0x7)
#define GET_ALLOC(p)    ((GET(p)) & 0x1)

/* Given an allocated-block ptr bp, compute address of its header, footer, payload in pred block, and payload in succ block */
#define AL_HDRP(bp)        ((char *)(bp) - WSIZE)
#define AL_FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define AL_NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define AL_PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given a free-block ptr bp, compute address of its header, footer, payload in pred block, and payload in succ block */
#define HDRP(bp)        ((char *)(bp) - 3*WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - 4*WSIZE)
#define PREDP(bp)       ((char *)(bp) - DSIZE)
#define SUCCP(bp)       ((char *)(bp) - WSIZE)

#define PRED_BLKP(bp)  ((char *)(*(size_t *)PREDP(head_listp)))
#define SUCC_BLKP(bp)   ((char *)(*(size_t *)SUCCP(head_listp)))

/* Given a free-block ptr bp, compute address-adjacent blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)  ((char *)(bp) - (GET_SIZE((char *)(bp) - 4 * WSIZE)) + 4 * WSIZE)

/* global variable*/
static char *head_listp, *end_listp, *payload;

/* function protoptypes for internal helper routines */
static void *extend_heap(size_t words);
static void place_link(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *address_coalesce(void *bp);
static void checkblock(void *bp);
void mm_checkheap();

int mm_init(void) {
    // initalize empty heap
    if ((payload = mem_sbrk(8 * WSIZE)) == NULL) {
        return -1;
    }

    head_listp = payload + 3 * WSIZE;
    end_listp = payload + 7 * WSIZE;
    PUT(payload, PACK(4 * WSIZE,0));                        // prologue header
    PUT(payload + 1 * WSIZE, 0);                     // prologue pred address
    PUT(payload + 2 * WSIZE, (size_t)(payload + 7 * WSIZE));      // prologue succ address
    PUT(payload + 3 * WSIZE, PACK(4 * WSIZE,0));            // prologue footer

    PUT(payload + 4 * WSIZE, PACK(4 * WSIZE,0));            // epilogue header
    PUT(payload + 5 * WSIZE, (size_t)(payload +3 * WSIZE));       // epilogue pred address
    PUT(payload + 6 * WSIZE, 0);                     // epologue succ address                     
    PUT(payload + 7 * WSIZE, PACK(4 * WSIZE,0));            // epilogue footer

    // Extend the empty heap with a free block of CHUNKSIZE bytes 
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    checkheap();
    return 0;
}

static void *extend_heap(size_t words) {
    // last in first out
    char *bp, *old;
    size_t size;

    // Allocate an even number of words of words to maintain alignment
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((int)(bp = mem_sbrk(size)) == -1)
        return NULL;

    bp = bp + 3 * WSIZE; // the address for payload/padding bytes

    // Initialize free block header and footer
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    // store the old succ addres in prologue
    old = SUCC_BLKP(head_listp);

    // update the succ address in prologue
    PUT(SUCCP(head_listp), (size_t)bp);

    // linking this free block
    PUT(PREDP(bp), (size_t)head_listp);
    PUT(SUCCP(bp), (size_t)old);

    PUT(PREDP(old), (size_t)bp);

    return address_coalesce(bp);
}

void mm_free(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    // update the allocation status

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size,0));

    // store the old succ addres in prologue
    char *old = SUCC_BLKP(head_listp);

    // update the succ address in prologue
    PUT(SUCCP(head_listp), (size_t)bp);

    // linking this free block
    PUT(PREDP(bp), (size_t)head_listp);
    PUT(SUCCP(bp), (size_t)old);

    PUT(PREDP(old), (size_t)bp);
    // looking at the address-adjacent block to see whether they are also free
    // if so combine them
    address_coalesce(bp);
    mm_checkheap();
}

static void *address_coalesce(void *bp) {

    char *prev = PREV_BLKP(bp);
    char *next = NEXT_BLKP(bp);

    size_t next_alloc;
    if (next > *(char *)mem_heap_lo) {
        next_alloc = 1;
    }
    else {
        next_alloc = GET_ALLOC(HDRP(next));
    }

    size_t prev_alloc = GET_ALLOC(FTRP(prev));
    size_t size = GET_ALLOC(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
	    return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
	    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
	    PUT(HDRP(bp), PACK(size, 0));
	    PUT(FTRP(bp), PACK(size,0));
        // reusing the predp and succp of bp
        // isolating the next_block
        PUT(SUCCP(PRED_BLKP(next)),(size_t)SUCC_BLKP(next));
        PUT(PREDP(SUCC_BLKP(next)),(size_t)PRED_BLKP(next));
	    return(bp);
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
	    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
	    PUT(FTRP(bp), PACK(size, 0));
	    PUT(HDRP(prev), PACK(size, 0));
        // moving predp and succp of bp to prev
        PUT(SUCCP(prev), (size_t)SUCC_BLKP(bp));
        PUT(PREDP(prev), (size_t)PRED_BLKP(bp));
        // isolating the prev_block
        PUT(SUCCP(PRED_BLKP(prev)), (size_t)SUCC_BLKP(prev));
        PUT(PREDP(SUCC_BLKP(prev)), (size_t)PRED_BLKP(prev));
	    return(prev);
    }

    else {       /* Case 4 */
        if (prev == end_listp) {
            // if prev_block is epilogue
            // the same as Case 2

            size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(bp), PACK(size,0));
            // reusing the predp and succp of bp
            // isolating the next_block
            PUT(SUCCP(PRED_BLKP(next)), (size_t)SUCC_BLKP(next));
            PUT(PREDP(SUCC_BLKP(next)), (size_t)PRED_BLKP(next));
            return(bp);
        }
        else {
            size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));

            PUT(HDRP(prev), PACK(size, 0));
            PUT(FTRP(next), PACK(size, 0));

            // moving predp and succp of bp to prev
            PUT(SUCCP(prev), (size_t)SUCC_BLKP(bp));
            PUT(PREDP(prev), (size_t)PRED_BLKP(bp));

            // isolating the prev_block
            PUT(SUCCP(PRED_BLKP(prev)), (size_t)SUCC_BLKP(prev));
            PUT(PREDP(SUCC_BLKP(prev)), (size_t)PRED_BLKP(prev));

            // isolating the next_block
            // it's impossible that next is prelogue or epilogue
            // because all regular blocks are behind the prelogue and epilogue 
            PUT(SUCCP(PRED_BLKP(next)), (size_t)SUCC_BLKP(next));
            PUT(PREDP(SUCC_BLKP(next)), (size_t)PRED_BLKP(next));
            return(prev);
        }
        
    }
}

void *mm_malloc(size_t size) {
    size_t asize;   // adjusted block size
    size_t extendsize; // amount to extend heap if no fit
    char *bp;

    /* Ignore suprious request */
    if (size <=0 )
        return NULL;
    
    // Adjust block size to include header and footer and to satisify alignment reqs.
    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    }
    else {
        asize = DSIZE * ((size+(DSIZE)+(DSIZE-1)) / DSIZE);
    }

    // Search the free list for a fit (first-fit)
    if ((bp = find_fit(asize)) != NULL) {
        place_link(bp, asize);
        return bp;
    }

    // No fit found. Get more memory and place the block
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
        return NULL;
    }
    place_link(bp, asize);
    mm_checkheap();
    return bp;
}

void *mm_realloc(void *ptr, size_t size) {
    return NULL;
}

static void place_link(void *bp, size_t asize) {
    size_t bsize = GET_SIZE(HDRP(bp));
    char *new_bp;
    if ((bsize - asize) >= 2 * DSIZE) {
        // allocated block
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        // free block
        new_bp = NEXT_BLKP(bp);
        PUT(HDRP(new_bp), PACK(bsize - asize, 0));
        PUT(FTRP(new_bp), PACK(bsize - asize, 0));
        // relinking
        PUT(PREDP(new_bp), (size_t)PRED_BLKP(bp));
        PUT(SUCCP(new_bp), (size_t)SUCC_BLKP(bp));
    }
    else {
        // allocated block
        PUT(HDRP(bp), PACK(bsize, 1));
        PUT(FTRP(bp), PACK(bsize, 1));
        // relinking or isolating
        PUT(SUCCP(PRED_BLKP(bp)), (size_t)SUCC_BLKP(bp));
        PUT(PREDP(SUCC_BLKP(bp)), (size_t)PRED_BLKP(bp));
    }
}

// first fit
static void *find_fit(size_t asize) { 
    void *bp;

    for (bp = head_listp; SUCC_BLKP(bp) != end_listp; bp = SUCC_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL; /* no fit */
}

void mm_checkheap() {
    char *bp = head_listp + 5 * WSIZE;
    int cnt_free1 = 0;
    int cnt_free2 = 0;
    int state = 0; // help checking whether coalescing
    // go through in address order
    while (bp < *(char *)mem_heap_hi()) {
        if (GET_ALLOC(bp)) { // allocated
            state = 0;
        } 
        else { // free
            if (state == 1) {
                printf("Having contiguous free blocks that escaped coalescing at %p\n", bp);
            }
            cnt_free1 += 1;
            state = 1;
        }
        bp += GET_SIZE(bp);
    }
    cnt_free1 -= 2; // delete the epilogue and prologue

    // go through in linked-list order
    bp = head_listp;
    if ((GET_SIZE(HDRP(bp)) != 4*WSIZE) || !GET_ALLOC(HDRP(bp))) {
        printf("Bad prologue header\n");
    }
    if ((GET_SIZE(HDRP(bp)) != 4*WSIZE) || !GET_ALLOC(FTRP(bp))) {
        printf("Bad prologue footer\n");
    }
    if (PRED_BLKP(bp) != 0) {
        printf("Bad prologue predecessor\n");
    }
    if (SUCC_BLKP(bp) < head_listp + 4 * WSIZE) {
        printf("Bad prologue successor\n");
    }
    if ((size_t)bp % 8) {
        printf("Error: prologue %p is not doubleword aligned", bp);
    }

    for (bp = head_listp; SUCC_BLKP(bp) != end_listp; bp = SUCC_BLKP(bp)) {
        cnt_free2 += 1;
        checkblock(bp); 
        // check whether aligned
        // if 0->free, does it have header, pred, succ, footer?
        // if 1->allocated, does it have header and footer?
    }

    if ( cnt_free1 > cnt_free2 ) {
        printf("Might miss %i free blocks in explicit free list", cnt_free1 - cnt_free2);
    }
    if ( cnt_free1 < cnt_free2 ) {
        printf("Might duplicate %i free blocks in explicit free list", cnt_free2 - cnt_free1);
    }

    bp = end_listp;
    if ((GET_SIZE(HDRP(bp)) != 4*WSIZE) || !GET_ALLOC(HDRP(bp))) {
        printf("Bad epilogue header\n");
    }
    if ((GET_SIZE(HDRP(bp)) != 4*WSIZE) || !GET_ALLOC(FTRP(bp))) {
        printf("Bad epilogue footer\n");
    }
    if (SUCC_BLKP(bp) != 0) {
        printf("Bad epilogue predecessor\n");
    }
    if (!((PREV_BLKP(bp) == head_listp) || (PREV_BLKP(bp) >= end_listp + 4 * WSIZE))) {
        printf("Bad epilogue successor\n");
    }
    if ((size_t)bp % 8) {
        printf("Error: epilogue %p is not doubleword aligned", bp);
    }

}

static void checkblock(void *bp) {     // checkblock for the free list

/* Block level */
    // header and footer match
    if (!(GET(HDRP(bp)) == GET(FTRP(bp)))) {
        printf("Error: in a freed block starting at %p, header does not match footer\n", bp);
    }
    // check whether aligned
    if ((size_t)bp % 8) {
        printf("Error: %p is not doubleword aligned\n", bp);
    }

/* list level*/    
    // pred and succ are self-consistent?
    if (PRED_BLKP(SUCC_BLKP(bp)) != bp) {
        printf("Error: a freed block starting at %p is not self-consistent with its successor block\n", bp);
    }
    // free list contrains no allocated blocks
    if (GET(AL_HDRP(bp)) != GET(AL_FTRP(bp))) {
        printf("Error: %p is an allocated block\n", bp);
        return;
    }
    // pointing to a free block?
    if (GET_ALLOC(HDRP(SUCC_BLKP(bp))) == 1) {
        printf("Error: a freed block starting at %p points to an allocated block\n", bp);
    }
    
/* heap level*/
    // pointing to valid address?
    if ((GET(PREDP(bp)) < *(char *)mem_heap_lo()) || (GET(PREDP(bp)) > *(char *)mem_heap_hi())) {
        printf("Error: in a freed block starting at %p, invalid predecesor pointer\n", bp);
    }
    if ((GET(SUCCP(bp)) < *(char *)mem_heap_lo()) || (GET(SUCCP(bp)) > *(char *)mem_heap_hi())) {
        printf("Error: in a freed block starting at %p, invalid successor pointer\n", bp);
    }
       
}
