/*
 * The team implemented a segregated free list in which each element in the segrated free list
 * belongs to a fixed class set. The classes in the segregated list range from a block size of 4 to 4096.
 * Each class index in the segregated list points to the header of the doubly linked list.
 * 
 * High level view of functions
 *
 * mm_init : initializes the all the CLASSSIZE (i.e. 16) elements in segregated list 
 *           to 0 (NULL).
 *
 * mm_free : adds the freed element to the free list in the appropriate class so the element 
 * can be used for next fetches.
 *
 * mm_malloc: allocates the memory. If the blk is in the free list it uses the blk from the 
 *           free list else extends the heap.
 *
 * mm_realloc: reallocates the memory without losing the original data in the memory blk.
 *
 * mm_check: checks the consistency of the heap.
 *
 * find_fit: uses LIFO to get the block from the free list
 *
 * coalesce: a function that combines memory to decrease the effect of fragmentation
 *
 */

 /**********************************************************************
 * Design:
 * Segregated Free list 
 *
 * GLOBAL variables
 *----------------
 *
 * void* free_list: This is a ptr to the header of the structure free list
 *                  in the heap.
 *
 * void * heap_listp: This is ptr to the start of the the heap.
 *
 * DIAGRAM
 *--------
 *
 * ---------------------------------------------------------------------
 * 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15
 *----------------------------------------------------------------------
 * \
 *  \
 *   *
 * DOUBLY LINKED LIST ELEMENT (structure)
 * -------------------------------------------------------
 * Boundary Tag | Successor | Predecessor | ... | Boundary Tag
 * -------------------------------------------------------
 *
 **********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "***",
    /* First member's full name */
    "***",
    /* First member's email address */
    "***",
    /* Second member's full name (leave blank if none) */
    "***",
    /* Second member's email address (leave blank if none) */
    "***"
};

/*************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
*************************************************************************/
#define WSIZE       sizeof(void *)            /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<7)      /* initial heap size (bytes) */

#define MAX(x,y) ((x) > (y)?(x) :(y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

void* heap_listp = NULL;


/* SattarSaad CODE */
#define DEBUG_PRINT() for (i = 0; i < (mem_heapsize() / WSIZE); i++)\
		printf("%d ", GET(mem_heap_lo() + (i * WSIZE))); \
		printf("\n")

#define DEBUG_FREE() for (x = 0; x < CLASSIZE - 1; x++)\
		printf("%p ", GET(free_list + (x * WSIZE))); \
		printf("\n")
		
#define CLASSIZE 16

/* Define all the classes */
#define class_1 1
#define class_2 2
#define class_3 4
#define class_4 8
#define class_5 16
#define class_6 32
#define class_7 64
#define class_8 128
#define class_9 256		
#define class_10 512
#define class_11 1024		
#define class_12 2048		
#define class_13 4096		
#define class_14 8192
//#define class_15 Anything greater than 8192		

//For printing out information
#define DEBUG 0

//Holds pointer to the beginning of the free list
void* free_list = NULL;

/**********************************************************
 * Helper Function: compute_class
 * Takes in a word size and returns the block size in class
 * current classes are powers of 2.
 **********************************************************/
size_t compute_class(size_t words, int flag)
{

	words = (words >> 1) | words;
	words = (words >> 2) | words;
	words = (words >> 4) | words;
	words = (words >> 8) | words;
	words = (words >> 16) | words;
	words = (words >> 32) | words;
	if(flag)
		words = words >> 1;
		
	return ++words;
}

/**********************************************************
 * Helper Function: compute_index
 * Takes in a class size (block size power of 2) and 
 * returns the class index.
 **********************************************************/
int compute_index(size_t class)
{
	switch(class)
	{
		case class_1: return 0;
		case class_2: return 1;
		case class_3: return 2;
		case class_4: return 3;
		case class_5: return 4;
		case class_6: return 5;
		case class_7: return 6;
		case class_8: return 7;
		case class_9: return 8;
		case class_10: return 9;
		case class_11: return 10;
		case class_12: return 11;
		case class_13: return 12;
		case class_14: return 13;
		default:
			return 14;
	}
}

/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 **********************************************************/
int mm_init(void)
 {
   if ((heap_listp = mem_sbrk((CLASSIZE + 6)*WSIZE)) == (void *)-1)
         return -1;
	
     PUT(heap_listp, 0);                         // alignment padding
     PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));   // prologue header
     PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));   // prologue footer
     heap_listp += DSIZE;
	 free_list = heap_listp + DSIZE;
	 PUT(free_list - WSIZE, PACK(((CLASSIZE + 2) * WSIZE), 1));
	 
	 int i;
	 for (i = 0; i < CLASSIZE; i++)
		  PUT(free_list + (i * WSIZE), 0);
		
	
	 PUT(free_list + (i * WSIZE), PACK(((CLASSIZE + 2) * WSIZE), 1));
	 PUT(heap_listp + ((CLASSIZE + 5) * WSIZE), PACK(0, 1));    // epilogue header

     return 0;
 }

 /**********************************************************
 * Helper Function: split
 * Takes in a pointer to a free block size and splits it
 * into required size and blk_size - required
 **********************************************************/
void split (void* bp, size_t asize)
{
  
#if DEBUG	
	printf("Entering Split!\n");
#endif 
	//Find the extra size not needed by malloc
	size_t Np_Size = GET_SIZE(HDRP(bp)) - asize;
	
	//Re-adjust the required size
	PUT(HDRP(bp), PACK(asize,1));
	PUT(FTRP(bp), PACK(asize,1));

	//Beginning reallocating the free space not needed by malloc
	void* Np = NEXT_BLKP(bp);

	//Re-adjust the free size
	PUT(HDRP(Np), PACK(Np_Size,0));
	PUT(FTRP(Np), PACK(Np_Size,0));
    
	//Get the free size in # of words
	//Used to calculate what class it belongs to
	size_t Np_words = Np_Size / WSIZE;
	
	//Find out the class
	size_t class = compute_class(Np_words, 1);
	
	//Figure out what range this class belongs to
	int i = compute_index(class);

	//Re-adjust the freelist pointers
	if(GET(free_list + (i * WSIZE)) != 0)
	{	
		//This class is not empty
		//Place free block on the top of the pointers
		PUT(Np, GET(free_list + (i * WSIZE)));
		PUT(GET(free_list + (i * WSIZE)) + WSIZE, Np);
	}	
	else
		//This is the only free block in the range
		PUT(Np, 0);

	//Since we're placing at the top of list make successor o
	PUT(Np + WSIZE,0);
	
	//Make sure our free_list points to the free block
	PUT(free_list + (i * WSIZE), Np);

#if DEBUG	
	printf("Exiting Split!\n");
#endif 
}

/**********************************************************
 * Helper Function: removeBlock
 * Takes in a pointer to a blk and removes it from free_list
 * Re-adjusts the pointers (predecessor and successor)
 **********************************************************/
 void removeBlock(void *bp){
	size_t class;
	int i;
	
	//If there is a successor re-adjust pointer
	//Pointer of Successor's Predecessor will be blks Predecessor 
	if(GET(bp) != 0)
		PUT(GET(bp) + WSIZE, GET(bp + WSIZE));
	
	//If there is a predecessor re-adjust pointer
	//Pointer of Predecessor's successor will be blks successor 
	if(GET(bp + WSIZE) != 0)
		PUT(GET(bp + WSIZE), GET(bp));
	else
	{
		//No predecessor (At the top of the linked list)
		//Figure out what linked list it belongs to
		//Re-adjust pointer of that class to point to successor
		class = compute_class(GET_SIZE(HDRP(bp)) / WSIZE, 1);
		i = compute_index(class);
		PUT(free_list + (i * WSIZE), GET(bp));
	}
}
/**********************************************************
 * coalesce
 * Covers the 4 cases discussed in the text:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 **********************************************************/
void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {       /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
		
		//Only Next block is free
		removeBlock(NEXT_BLKP(bp));
		
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
		return (bp);
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
       
		//Only Prev block is free
		removeBlock(PREV_BLKP(bp));
		
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return (PREV_BLKP(bp));
    }

    else {            /* Case 4 */
        
		//Both Next and Prev are free
		//Combine steps
		removeBlock(NEXT_BLKP(bp));
		removeBlock(PREV_BLKP(bp));
		
		size += GET_SIZE(HDRP(PREV_BLKP(bp)))  +
            GET_SIZE(FTRP(NEXT_BLKP(bp)))  ;
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        return (PREV_BLKP(bp));
    }
}

/**********************************************************
 * extend_heap
 * Extend the heap by "words" words, maintaining alignment
 * requirements of course. Free the former epilogue block
 * and reallocate its new header
 **********************************************************/
void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    /* Allocate an even number of words to maintain alignments */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;

	if ( (bp = mem_sbrk(size)) == (void *)-1 )
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));                // free block header
    PUT(FTRP(bp), PACK(size, 0));                // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));        // new epilogue header

    /* Coalesce if the previous block was free */
    return coalesce(bp);

}


/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/
void * find_fit(size_t asize)
{
#if DEBUG	
	printf("Entering Find Fit!\n");
#endif	
	void *bp = NULL;
	unsigned int* addr = 0;
	
	//Find how manys blocks there asking for
	//Compute the lowest range where memory can fit
	size_t words = asize / WSIZE;
	size_t class = compute_class(words, 1);
	
	int index_offset = compute_index(class);
	int i, x;
	
	//See if free memory that is significant exists
	for(i = index_offset; i < CLASSIZE - 1; i++)
	{
		//found space in that range
		if(GET(free_list + (i * WSIZE)) != 0)
		{
			addr = GET(free_list + (i * WSIZE));
			bp = (char *)addr;
			
			//increment until we find a valid space
			while(addr != 0)
			{
				//we found space in memory
				if(asize <= GET_SIZE(HDRP(bp)))
					break;
				//keep looking
				else{
					addr = GET(bp);
					bp = (char *)addr;
				}
			}
			//break out of loop in found memory
			if(addr != 0)
				break;
		}
	}


	//we have not found space in memory
	if(addr == 0)
		return NULL;

	//Found a valid blk
	//Re-adjust to take it out of the list
	if(GET(bp + WSIZE) != 0)
		PUT(GET(bp + WSIZE), GET(bp));
	 	
	if(GET(bp) != 0)
		PUT(GET(bp) + WSIZE, GET(bp + WSIZE));
	
	if(GET(free_list + (i * WSIZE)) == bp)
		PUT(free_list + (i * WSIZE), GET(bp));

	//If size is bigger than by required size + 4 *WSIZE then split blk
	if(asize + (4 * WSIZE) <= GET_SIZE(HDRP(bp)))
		split(bp, asize);
	
#if DEBUG	
	printf("Exiting Find Fit with memory found!\n");
#endif 
	return bp;

}

/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void place(void* bp, size_t asize)
{
  /* Get the current block size */
  size_t bsize = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(bsize, 1));
  PUT(FTRP(bp), PACK(bsize, 1));
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp)
{

#if DEBUG	
	printf("Entering Free!\n");
#endif 
	//User is trying to free invalid location
    if(bp == NULL){
      return;
    }
	
	//Extend the blk size to other already free neighbour blks 
	bp = coalesce(bp);
	
	//Calculate the free blk size and what range it will belong to
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));

	size_t class = compute_class(size / WSIZE, 1);
	
	int i = compute_index(class);
	
	
	if(GET(free_list + (i * WSIZE)) != 0){
		//If that range is not empty re-adjust pointers
		PUT(bp, GET(free_list + (i * WSIZE)));
		PUT(GET(free_list + (i * WSIZE)) + WSIZE, bp);
	}	
	else
		//Else set the free blks predecessor to null
		PUT(bp, 0);

	PUT(bp + WSIZE, 0);
	PUT(free_list + (i * WSIZE), bp);

	//mm_check();
#if DEBUG	
	printf("Exiting Free with and index addr %p and %p and %p!\n", bp, GET(bp), GET(bp + WSIZE));
#endif 
}


/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 * The decision of splitting the block, or not is determined
 *   in place(..)
 * If no block satisfies the request, the heap is extended
 **********************************************************/
void *mm_malloc(size_t size)
{

    size_t asize; /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char * bp;
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

	size_t words = size / WSIZE;
	
	//No blks found. Find if required blk lies on boundary of range
	//And is a small block
	//Added opt.
	if(words < class_5)
		size = class_5 * WSIZE;//found opt. pre-fetch
	if(words <= class_7 && compute_index(size) == 14)
		size = compute_class(size, 0);
		
    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);

	/* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
        return bp;
    }
	
    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
	
	if(asize + (4*WSIZE) <= GET_SIZE(HDRP(bp)))
			split(bp, asize); 
			
    place(bp, asize);

#if DEBUG	
	printf("Exiting Malloc with new allocated space!\n");
#endif 
    return bp;

}

/**********************************************************
 * mm_realloc
 * Implemented simply in terms of mm_malloc and mm_free
 *********************************************************/
void *mm_realloc(void *ptr, size_t size)
{
	size_t asize;
    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0){
      mm_free(ptr);
      return NULL;
    }
    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
      return (mm_malloc(size));

	 /* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
      asize = 2 * DSIZE;
	else
      asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);

	// Size is not less then old just return ptr
	if(asize <= GET_SIZE(HDRP(ptr)))
		return ptr;
	
	//If the next block is free and the total size is significate
	// combine the the blocks. Avoids having to copy data
	size_t newSize = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));;
	
	if(!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && newSize > size)
	{
		//take the free blk out of free list
		removeBlock(NEXT_BLKP(ptr));
	
		//Mark the blk as taken
		PUT(HDRP(ptr), PACK(newSize, 1));
		PUT(FTRP(ptr), PACK(newSize, 1));
    
		return ptr;
	}
		
	
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    /* Copy the old data. */
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/**********************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 *********************************************************/
int mm_check(void){
	void* bp;
	unsigned int* addr = 0;
	int i;
	int free_list_blks = 0, heap_free_blks = 0;
	
	// CASE 1: Make sure all blks in free list are marked free
	for(i = 0; i < CLASSIZE - 1; i++)
	{
		//Blks located in freelist
		if(GET(free_list + (i * WSIZE)) != 0)
		{
			addr = GET(free_list + (i * WSIZE));
			bp = (char *)addr;
			
			//loop through all the blks in that range
			while(addr != 0)
			{
				//If not marked as free return -1 error
				if(GET_ALLOC(bp))
					return -1;
				addr = GET(bp);
				bp = (char *)addr;
				
			}
		}
	}

	// CASE 2: CHECK THAT THE FREE_LIST BLKS ARE DWSIZE ALIGNED
	for(i = 0; i < CLASSIZE - 1; i++)
	{
		//Blks located in freelist
		if(GET(free_list + (i * WSIZE)) != 0)
		{
			addr = GET(free_list + (i * WSIZE));
			bp = (char *)addr;
			
			//loop through all the blks in that range
			while(addr != 0)
			{
				//If not marked as free return -1 error
				if(GET(bp) & 0xf != 0)
					return -1;
				addr = GET(bp);
				bp = (char *)addr;
				
			}
		}
	}

	// CASE 3: CHECK THAT NUMBER OF FREE BLKS IN HEAP AND IN FREE_LIST ARE SAME

	void* temp = heap_listp;

        while (temp != NULL)
	{
		if (!GET_ALLOC(temp))
			heap_free_blks++;
		temp = NEXT_BLKP(temp);
	}

	for(i = 0; i < CLASSIZE - 1; i++)
	{
		//Blks located in freelist
		if(GET(free_list + (i * WSIZE)) != 0)
		{
			addr = GET(free_list + (i * WSIZE));
			bp = (char *)addr;
			
			//loop through all the blks in that range
			while(addr != 0)
			{
				//If not marked as free return -1 error
				if(!GET_ALLOC(bp))
					free_list_blks++;
				addr = GET(bp);
				bp = (char *)addr;
				
			}
		}
	}
	
	if (heap_free_blks != free_list_blks)
	{
		return -1;
	}
	
	return 0;

}