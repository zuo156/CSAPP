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
// #define checkheap(lineno) mm_checkheap(lineno)
#define checkheap(lineno)

#define NUMLIST     20
#define WSIZE       4       /* word size (bytes) */
#define DSIZE       8       /* doubleword size (bytes) */
// #define CHUNKSIZE   (1<<12) /* initial heap size 4096(bytes) */
#define CHUNKSIZE   (1<<20)

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
#define NEXTP(bp)  ((char *)(bp) + (GET_SIZE(HDRP((char *)(bp)))))
#define PREVP(bp)  ((char *)(bp) - (GET_SIZE(HDRP((char *)(bp)) - WSIZE)))

// #define NEXT_VAL(bp)  ((*((size_t *)NEXTP(bp))))
// #define PREV_VAL(bp)  ((*((size_t *)PREVP(bp))))

/* global variable*/
// char *list0, *list1, *list2, *list3, *end_list, *payload;
char *head_listp[NUMLIST+1], *payload;
int trace = 0;

/* function protoptypes for internal helper routines */
void *extend_heap(size_t words);
void place_link(void *bp, size_t asize);
void *find_fit(size_t asize);
void *address_coalesce(void *bp);
int size2list(int size);
void mm_checkheap(int lineno);
void checkblock(void *bp, int i);

int mm_init(void) {
    // initalize empty heap
    if ((payload = mem_sbrk((NUMLIST*4+5) * WSIZE)) == NULL) { 
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
    for (int i = 0; i < NUMLIST-1; i++) {
        if (size <= (1<<(i+1))) {
            return i;
        }
    }
    return NUMLIST-1;
}

void mm_free(void *bp) {
    checkheap(__LINE__);
    // printf("freeing pointer %p\n", bp);
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
    char *old;
    int index;
    // cache to avoid overwritten
    size_t pred_next = PRED_VAL(next);
    size_t succ_next = SUCC_VAL(next);

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
            PUT(SUCCP(PRED_VAL(bp)), succ_next);
            PUT(PREDP(succ_next), PRED_VAL(bp));
        }
        else if (next == (char *)PRED_VAL(bp)) {
            // isolating bp and next_block together
            PUT(SUCCP(pred_next), SUCC_VAL(bp));
            PUT(PREDP(SUCC_VAL(bp)), pred_next);
        }
        else {
            // isolating the bp
            PUT(SUCCP(PRED_VAL(bp)),(size_t)SUCC_VAL(bp));
            PUT(PREDP(SUCC_VAL(bp)),(size_t)PRED_VAL(bp));
            // isolating the next_block
            PUT(SUCCP(pred_next),(size_t)succ_next);
            PUT(PREDP(succ_next),(size_t)pred_next);
        }
        // move the merged free block to the begining of the specific list
        index = size2list(size);
        old = (char *)SUCC_VAL(head_listp[index]);
        PUT(SUCCP(head_listp[index]), (size_t)bp);
        PUT(PREDP(bp), (size_t)head_listp[index]);
        PUT(SUCCP(bp), (size_t)old);
        PUT(PREDP(old), (size_t)bp);
    }


    size_t pred_prev = PRED_VAL(prev);
    size_t succ_prev = SUCC_VAL(prev);    

    if (!prev_alloc) {      /* coalesce with prev block*/
	    size += GET_SIZE(HDRP(PREVP(bp)));
	    PUT(FTRP(bp), PACK(size, 0));
	    PUT(HDRP(prev), PACK(size, 0));
        /* 
        what if the succ of bp is the prev free-block 
        what if the pred of bp is the prev free-block
        */
        if (prev == (char *)SUCC_VAL(bp)) {
            PUT(SUCCP(PRED_VAL(bp)), succ_prev);
            PUT(PREDP(succ_prev), PRED_VAL(bp));
        }
        else if (prev == (char *)PRED_VAL(bp)) {
            PUT(SUCCP(pred_prev), SUCC_VAL(bp));
            PUT(PREDP(SUCC_VAL(bp)), pred_prev);
        }
        else {
            // isolating the bp_block
            PUT(SUCCP(PRED_VAL(bp)), (size_t)SUCC_VAL(bp));
            PUT(PREDP(SUCC_VAL(bp)), (size_t)PRED_VAL(bp));
            // isolating the prev_block
            PUT(SUCCP(pred_prev), (size_t)succ_prev);
            PUT(PREDP(succ_prev), (size_t)pred_prev);
        }
        bp = prev;
        // move the merged free block to the begining of the specific list
        index = size2list(size);
        old = (char *)SUCC_VAL(head_listp[index]);
        PUT(SUCCP(head_listp[index]), (size_t)bp);
        PUT(PREDP(bp), (size_t)head_listp[index]);
        PUT(SUCCP(bp), (size_t)old);
        PUT(PREDP(old), (size_t)bp);
    }

    return bp;
}

void *mm_malloc(size_t size) {
    checkheap(__LINE__);
    // printf("malloc for size %d\n", size);ß
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

// void *mm_realloc(void *ptr, size_t size) {
//     char *bp;
//     size_t asize, old_size;

//     if (ptr == NULL) {
//         return mm_malloc(size);
//     }

//     if (size<=0) {
//         return NULL;
//     }

//     // Adjust block size to include header and footer and to satisity alignment reqs.
//     if (size <= DSIZE) {
//         asize = 2 * DSIZE;
//     }
//     else {
//         asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) /  DSIZE);
//     }

//     bp = ptr + DSIZE;
//     old_size = GET_SIZE(HDRP(bp));
    
//     // whether the next block is free?
//     char *next = NEXTP(bp);
//     size_t next_alloc;

//     if (next > (char *)mem_heap_hi()) {
//         next_alloc = 1;
//     }
//     else {
//         next_alloc = GET_ALLOC(HDRP(next));
//     }

//     // if it is free, update the old_size
//     if (!next_alloc) {
//         old_size += GET_SIZE(HDRP(next));
//     }

//     if ((int)(old_size - asize) >= 2 * DSIZE) {
//         // it can keep its location
//         // cache to avoid overwritten
//         size_t pred_next = PRED_VAL(next);
//         size_t succ_next = SUCC_VAL(next);
//         // reallocate block
//         PUT(HDRP(bp), PACK(asize, 1));
//         PUT(FTRP(bp), PACK(asize, 1));
//         // free block
//         char *free_bp = (char *)NEXTP(bp);
//         // header and footer size
//         PUT(HDRP(free_bp), PACK(old_size - asize, 0));
//         PUT(FTRP(free_bp), PACK(old_size - asize, 0));

//         if (!next_alloc) { // if next is free
//             // relinking
//             PUT(PREDP(free_bp), pred_next); 
//             PUT(SUCCP(free_bp), succ_next);
//             // update pred and succ to the free_bp
//             PUT(SUCCP(pred_next), (size_t)free_bp);
//             PUT(PREDP(succ_next), (size_t)free_bp);
//         }
//         else { // if next is allocated
//             // put the remain at the begining of the heap

//             // store the old succ addres in prologue
//             int index = size2list(old_size-asize);
//             char *old = (char *)SUCC_VAL(head_listp[index]);

//             // update the succ address in prologue
//             PUT(SUCCP(head_listp[index]), (size_t)free_bp);

//             // linking this free block
//             PUT(PREDP(free_bp), (size_t)head_listp[index]);
//             PUT(SUCCP(free_bp), (size_t)old);

//             PUT(PREDP(old), (size_t)free_bp);
//             address_coalesce(free_bp);
//         }
//         return bp - DSIZE;
//     }
//     else if ((int)(old_size - asize) >= 0) {
//         if (!next_alloc) { // if next is free
//             // update the size of block
//             PUT(HDRP(bp), PACK(old_size, 1));
//             PUT(FTRP(bp), PACK(old_size, 1));
//             // isolate the occupied freed block
//             PUT(SUCCP(PRED_VAL(next)), (size_t)SUCC_VAL(next));
//             PUT(PREDP(SUCC_VAL(next)), (size_t)PRED_VAL(next));
//             return ptr;
//         } else { // if next isn't free, keep origin size
//             return ptr;
//         }
//     }
//     else { 
//         // find a new block
//         char *new_bp = mm_malloc(size);
//         // copy the old content to the new block byte by byte
//         int length = GET_SIZE(HDRP(bp)) - 2 * WSIZE;
//         for(int i = 0; i < length; i++) {
//             PUT(new_bp + i, GET(ptr + i));
//         }
//         // free the old block
//         mm_free(ptr);
//         return new_bp;
//     }
// }

void *mm_realloc(void *ptr, size_t size) {
    char *bp;
    size_t asize, old_size;

    if (ptr == NULL) {
        return mm_malloc(size);
    }

    if (size <= 0) {
        mm_free(ptr);
        return NULL;
    }

    // Adjust block size to include overhead and alignment requirements
    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    }

    bp = ptr;
    old_size = GET_SIZE(HDRP(bp));
    
    // Check if next block is free and can be coalesced
    char *next = NEXTP(bp);
    size_t next_alloc;
    if (next > (char *)mem_heap_hi()) {
        next_alloc = 1;
    } else {
        next_alloc = GET_ALLOC(HDRP(next));
    }

    if (!next_alloc) {
        old_size += GET_SIZE(HDRP(next));
    }

    // If the current block with the next free block can fit the new size
    if ((int)(old_size - asize) >= 2 * DSIZE) {
        // Reallocate block
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        // Create a new free block with the remaining space
        char *free_bp = NEXTP(bp);
        size_t free_size = old_size - asize;
        PUT(HDRP(free_bp), PACK(free_size, 0));
        PUT(FTRP(free_bp), PACK(free_size, 0));

        // Insert the new free block into the free list
        insert_free_block(free_bp);

        return bp;
    } else if ((int)(old_size - asize) >= 0) {
        // If the block is large enough to satisfy the request without shrinking
        return ptr;
    } else {
        // Allocate a new block
        char *new_bp = mm_malloc(size);
        if (new_bp == NULL) {
            return NULL;
        }

        // Copy the old data to the new block
        size_t copy_size = GET_SIZE(HDRP(bp)) - DSIZE;
        memcpy(new_bp, ptr, copy_size);

        // Free the old block
        mm_free(ptr);

        return new_bp;
    }
}

// Helper function to insert a block into the free list
void insert_free_block(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int index = size2list(size);
    char *succ = SUCC_VAL(head_listp[index]);

    // Insert bp at the beginning of the free list
    PUT(PREDP(bp), (size_t)head_listp[index]);
    PUT(SUCCP(bp), (size_t)succ);
    PUT(SUCCP(head_listp[index]), (size_t)bp);
    PUT(PREDP(succ), (size_t)bp);
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

void *find_fit(size_t asize) { 
    void *bp;
    void *start = head_listp[size2list(asize)];
    
    // Iterate over the free list starting from the successor of the head pointer
    for (bp = (char *)SUCC_VAL(start); bp != head_listp[NUMLIST]; bp = (char *)SUCC_VAL(bp)) {
        // Check if the block is free and of sufficient size
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            // Ensure bp is not one of the head pointers
            int is_head_pointer = 0;
            for (int i = 0; i < NUMLIST; i++) {
                if (bp == head_listp[i]) {
                    is_head_pointer = 1;
                    break;
                }
            }
            // If bp is not a head pointer, return it
            if (!is_head_pointer) {
                return bp;
            }
        }
    }

    // Return NULL if no fit is found
    return NULL;
}

void mm_checkheap(int lineno) {

    printf("checking heap at %d line of %d's command\n", lineno, trace);
    trace +=1;
    char *bp = head_listp[NUMLIST] + 4 * WSIZE;
    int cnt_free1 = 0;
    int cnt_free2 = 0;
    int state = 0; // help checking whether coalesing

    // go through in address order
    while (bp < (char *)mem_heap_hi() && bp > head_listp[NUMLIST]) {
        if (GET_ALLOC(HDRP(bp))) {
            state = 0;
        }
        else {
            if (state == 1) {
                printf("Having contiguous free block that escaped coalescing at %p\n", bp);
                exit(1);
            }
            cnt_free1 += 1;
            state = 1;
        }
        bp += GET_SIZE(HDRP(bp));
    }
    // go through the linked list
    for (int i = 0; i < NUMLIST; i ++) {
        for (bp = (char *)SUCC_VAL(head_listp[i]); bp != head_listp[i+1]; bp = (char *)SUCC_VAL(bp)) {
            cnt_free2 += 1;
            checkblock(bp, i);
        }
    }

    if ( cnt_free1 > cnt_free2 ) {
        printf("Might miss %i free blocks in explicit free list\n", cnt_free1 - cnt_free2);
    }
    if ( cnt_free1 < cnt_free2 ) {
        printf("Might duplicate %i free blocks in explicit free list\n", cnt_free2 - cnt_free1);
    }


}

void checkblock(void *bp, int i) {
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
        exit(1);
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
    if (size2list(GET_SIZE(HDRP(bp))) != i) {
        printf("Block at %p are not in its corresponding segregated list %d\n", bp, i);
    }

/* heap level*/
    // pointing to valid address?
    if (((char *)PRED_VAL(bp) < (char *)mem_heap_lo()) || ((char *)PRED_VAL(bp) > (char *)mem_heap_hi())) {
        printf("Error: in a freed block starting at %p, invalid predecesor pointer\n", bp);
    }
    if (((char *)SUCC_VAL(bp) < (char *)mem_heap_lo()) || ((char *)SUCC_VAL(bp) > (char *)mem_heap_hi())) {
        printf("Error: in a freed block starting at %p, invalid successor pointer\n", bp);
    }
}