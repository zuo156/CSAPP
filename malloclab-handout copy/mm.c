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
    where 8 is 8 bytes
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
#define checkheap(lineno) mm_checkheap(lineno)

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
static char *head_listp, *end_listp, *payload;

/* function protoptypes for internal helper routines */
static void *extend_heap(size_t words);
static void place_link(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *address_coalesce(void *bp);
static void checkblock(void *bp);
static void mm_checkheap(int lineno);

int mm_init(void) {
    // initalize empty heap
    if ((payload = mem_sbrk(9 * WSIZE)) == NULL) {
        return -1;
    }

    PUT(payload, 0);                                 // alignment padding
    head_listp = payload + 4 * WSIZE;
    end_listp = payload + 8 * WSIZE;
    PUT(payload + 1 * WSIZE, PACK(4 * WSIZE,0));                        // prologue header
    PUT(payload + 2 * WSIZE, 0);                     // prologue pred address
    PUT(payload + 3 * WSIZE, (size_t)(payload + 8 * WSIZE));      // prologue succ address
    PUT(payload + 4 * WSIZE, PACK(4 * WSIZE,0));            // prologue footer

    PUT(payload + 5 * WSIZE, PACK(4 * WSIZE,0));            // epilogue header
    PUT(payload + 6 * WSIZE, (size_t)(payload + 4 * WSIZE));       // epilogue pred address
    PUT(payload + 7 * WSIZE, 0);                     // epologue succ address                     
    PUT(payload + 8 * WSIZE, PACK(4 * WSIZE,0));            // epilogue footer

    // Extend the empty heap with a free block of CHUNKSIZE bytes 
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    checkheap(__LINE__);
    return 0;
}

static void *extend_heap(size_t words) {
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

    // store the old succ addres in prologue
    old = (char *)SUCC_VAL(head_listp);

    // update the succ address in prologue
    PUT(SUCCP(head_listp), (size_t)bp);

    // linking this free block
    PUT(PREDP(bp), (size_t)head_listp);
    PUT(SUCCP(bp), (size_t)old);

    PUT(PREDP(old), (size_t)bp);

    return address_coalesce(bp);
}

void mm_free(void *bp) {
    bp = bp + DSIZE; 
    size_t size = GET_SIZE(HDRP(bp));
    // update the allocation status

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size,0));

    // store the old succ addres in prologue
    char *old = (char *)SUCC_VAL(head_listp);

    // update the succ address in prologue
    PUT(SUCCP(head_listp), (size_t)bp);

    // linking this free block
    PUT(PREDP(bp), (size_t)head_listp);
    PUT(SUCCP(bp), (size_t)old);

    PUT(PREDP(old), (size_t)bp);
    // looking at the address-adjacent block to see whether they are also free
    // if so combine them
    address_coalesce(bp);
    checkheap(__LINE__);
}

static void *address_coalesce(void *bp) {
    char *prev = PREVP(bp);
    char *next = NEXTP(bp);      

    size_t next_alloc;
    if (next > (char *)mem_heap_hi) {      // if next is end of the heap
        next_alloc = 1;
    }
    else {
        next_alloc = GET_ALLOC(HDRP(next));
    }
    size_t prev_alloc;
    if (prev == end_listp) {                  // if prev is the epilogue
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
            // update the succ of bp
            PUT(SUCCP(bp), SUCC_VAL(next));
            // update the pred of succ of next
            PUT(PREDP(SUCC_VAL(next)), (size_t)bp);
        }
        else if (next == (char *)PRED_VAL(bp)) {
            // update the pred of bp
            PUT(PREDP(bp), PRED_VAL(next));
            // update the succ of pred of next
            PUT(SUCCP(PRED_VAL(next)), (size_t)bp);

        }
        else {
            // reusing the predp and succp of bp
            // isolating the next_block
            PUT(SUCCP(PRED_VAL(next)),(size_t)SUCC_VAL(next));
            PUT(PREDP(SUCC_VAL(next)),(size_t)PRED_VAL(next));
        }
    }

    if (!prev_alloc) {      /* coalesce with prev block*/
	    size += GET_SIZE(HDRP(PREVP(bp)));
	    PUT(FTRP(bp), PACK(size, 0));
	    PUT(HDRP(prev), PACK(size, 0));
        // moving predp and succp of bp to prev
        PUT(SUCCP(prev), (size_t)SUCC_VAL(bp));
        PUT(PREDP(prev), (size_t)PRED_VAL(bp));
        /* 
        what if the succ of bp is the prev free-block 
        what if the pred of bp is the prev free-block
        */
        if (prev == (char *)SUCC_VAL(bp)) {
            // update the pred of prev
            PUT(PREDP(prev), PRED_VAL(bp));
            // update the succ of pred of bp
            PUT(SUCCP(PRED_VAL(bp)), prev);
        }
        else if (prev == (char *)PRED_VAL(bp)) {
            // update the succ of prev
            PUT(SUCCP(prev), SUCC_VAL(bp));
            // update the pred of succ of bp
            PUT(PREDP(SUCC_VAL(bp)), prev);
        }
        else {
            // isolating the prev_block
            PUT(SUCCP(PRED_VAL(prev)), (size_t)SUCC_VAL(prev));
            PUT(PREDP(SUCC_VAL(prev)), (size_t)PRED_VAL(prev));
        }
        bp = prev;
    }
    
    return bp;
    // else {       /* Case 4 */
    //     /* 
    //     what if the succ of the bp is the next 
    //     and if the pred of the bp is the prev
    //     */
    //     size += GET_SIZE(HDRP(PREVP(bp))) + GET_SIZE(FTRP(NEXTP(bp)));

    //     PUT(HDRP(prev), PACK(size, 0));
    //     PUT(FTRP(next), PACK(size, 0));

    //     // moving predp and succp of bp to prev
    //     PUT(SUCCP(prev), (size_t)SUCC_VAL(bp));
    //     PUT(PREDP(prev), (size_t)PRED_VAL(bp));

    //     // isolating the prev_block
    //     PUT(SUCCP(PRED_VAL(prev)), (size_t)SUCC_VAL(prev));
    //     PUT(PREDP(SUCC_VAL(prev)), (size_t)PRED_VAL(prev));

    //     // isolating the next_block
    //     // it's impossible that next is prelogue or epilogue
    //     // because all regular blocks are behind the prelogue and epilogue 
    //     PUT(SUCCP(PRED_VAL(next)), (size_t)SUCC_VAL(next));
    //     PUT(PREDP(SUCC_VAL(next)), (size_t)PRED_VAL(next));
    //     return(prev);
    // }
}

void *mm_malloc(size_t size) {
    // the input size is byte, not the number of the words
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
        // asize = DSIZE * ((size+(DSIZE)+(DSIZE-1)) / DSIZE);
        asize = DSIZE * ((size+(DSIZE-1)) / DSIZE);
    }

    // Search the free list for a fit (first-fit)
    if ((bp = find_fit(asize)) != NULL) {
        place_link(bp, asize);
        checkheap(__LINE__);
        return bp - DSIZE; // because allocated block doesn't need pred and succ
    }

    // No fit found. Get more memory and place the block
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
        return NULL;
    }
    place_link(bp, asize);
    checkheap(__LINE__);
    return bp - DSIZE;
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
        new_bp = (char *)NEXTP(bp);
        PUT(HDRP(new_bp), PACK(bsize - asize, 0));
        PUT(FTRP(new_bp), PACK(bsize - asize, 0));
        // relinking
        PUT(PREDP(new_bp), (size_t)PRED_VAL(bp));
        PUT(SUCCP(new_bp), (size_t)SUCC_VAL(bp));
        // update pred and succ to the new_bp
        PUT(SUCCP(PRED_VAL(bp)), (size_t)new_bp);
        PUT(PREDP(SUCC_VAL(bp)), (size_t)new_bp);
    }
    else {
        // allocated block
        PUT(HDRP(bp), PACK(bsize, 1));
        PUT(FTRP(bp), PACK(bsize, 1));
        // relinking or isolating
        PUT(SUCCP(PRED_VAL(bp)), (size_t)SUCC_VAL(bp));
        PUT(PREDP(SUCC_VAL(bp)), (size_t)PRED_VAL(bp));
        
    }
}

// first fit
static void *find_fit(size_t asize) { 
    void *bp;

    for (bp = head_listp; bp != end_listp; bp = (char *)SUCC_VAL(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL; /* no fit */
}

void mm_checkheap(int lineno) {
    
    printf("checking heap at %d line\n", lineno);
    char *bp = head_listp + 5 * WSIZE; // the header of first block in address order after the epilogue
    int cnt_free1 = 0;
    int cnt_free2 = 0;
    int state = 0; // help checking whether coalescing
    // go through in address order
    while (bp < (char *)mem_heap_hi() && bp > head_listp) {
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

    // go through in linked-list order
    bp = head_listp;
    if ((GET_SIZE(HDRP(bp)) != 4*WSIZE) || GET_ALLOC(HDRP(bp))) {
        printf("Bad prologue header\n");
    }
    if ((GET_SIZE(HDRP(bp)) != 4*WSIZE) || GET_ALLOC(FTRP(bp))) {
        printf("Bad prologue footer\n");
    }
    if (PRED_VAL(bp) != 0) {
        printf("Bad prologue predecessor\n");
    }
    if ((char *)SUCC_VAL(bp) < end_listp) {
        printf("Bad prologue successor\n");
    }
    if ((size_t)bp % 8) {
        printf("Error: prologue %p is not double-word aligned\n", bp);
    }

    for (bp = (char *)SUCC_VAL(head_listp); bp != end_listp; bp = (char *)SUCC_VAL(bp)) {
        cnt_free2 += 1;
        checkblock(bp); 
    }

    if ( cnt_free1 > cnt_free2 ) {
        printf("Might miss %i free blocks in explicit free list\n", cnt_free1 - cnt_free2);
    }
    if ( cnt_free1 < cnt_free2 ) {
        printf("Might duplicate %i free blocks in explicit free list\n", cnt_free2 - cnt_free1);
    }

    bp = end_listp;
    if ((GET_SIZE(HDRP(bp)) != 4*WSIZE) || GET_ALLOC(HDRP(bp))) {
        printf("Bad epilogue header\n");
    }
    if ((GET_SIZE(HDRP(bp)) != 4*WSIZE) || GET_ALLOC(FTRP(bp))) {
        printf("Bad epilogue footer\n");
    }
    if ((char *)SUCC_VAL(bp) != 0) {
        printf("Bad epilogue successor\n");
    }
    if ((char *)PRED_VAL(bp) < head_listp) {
        printf("Bad epilogue predecessor\n");
    }
    if ((size_t)bp % 8) {
        printf("Error: epilogue %p is not double-word aligned\n", bp);
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
    if ((char *)PRED_VAL(SUCC_VAL(bp)) != bp) {
        printf("Error: a freed block starting at %p is not self-consistent with its successor block\n", bp);
    }
    // free list contrains no allocated blocks
    if (GET_ALLOC(HDRP(bp)) == 1) {
        printf("Error: %p is an allocated block\n", bp);
        return;
    }
    // pointing to a free block?
    // if (GET_ALLOC(HDRP(SUCC_VAL(bp))) == 1) {
    //     printf("Error: a freed block starting at %p points to an allocated block\n", bp);
    // }
    // it's redundant, same as previous one
    
/* heap level*/
    // pointing to valid address?
    if (((char *)PRED_VAL(bp) < (char *)mem_heap_lo()) || ((char *)PRED_VAL(bp) > (char *)mem_heap_hi())) {
        printf("Error: in a freed block starting at %p, invalid predecesor pointer\n", bp);
    }
    if (((char *)SUCC_VAL(bp) < (char *)mem_heap_lo()) || ((char *)SUCC_VAL(bp) > (char *)mem_heap_hi())) {
        printf("Error: in a freed block starting at %p, invalid successor pointer\n", bp);
    }
       
}
