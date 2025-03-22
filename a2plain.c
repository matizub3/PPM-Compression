/**************************************************************
*
*                     a2plain.c
*
*     Assignment: locality
*     Authors:  TJ, Annica
*     Date:     02/19/25
*
*     a2plain.c defines private functions for A2 for UArray2 application
*     using the UArray2 interface, allowing for creation, access, and iteration.
*     The implementation organizes and exports UArray2 functions into the 
*     A2Methods_T interface for dynamic use.
*
*     Functions include new, new_with_blocksize, a2free, width, height, size,
*     blocksize, at, apply_small, small_map_row_major, map_row_major,
*     small_map_col_major, and map_col_major
*
**************************************************************/
#include <string.h>
#include <a2plain.h>
#include "uarray2.h"
#include "assert.h"

/************************************************/
/* Define a private version of each function in */
/* A2Methods_T that we implement.               */
/************************************************/

/********** new ********
*
* Allocates space for and creates a UArray2 (2d array), which stores
* height, width, size of each element, and the elements themselves.
*
* Parameters:
*      int width:       desired width of new UArray2
*      int height:      desired height of new UArray2
*      int size:        number of bytes to allocate for each element
* Return:
*      Pointer to created UArray2
* Expects:
*      - elementSize to be greater than 0
*      - height and width to be greater than or equal to 0
* Notes:
*      - throws CRE if elementSize <= 0
*      - throws CRE if height or width < 0
*      - must be freed by the user using the provided a2free function
*
************************/
static A2Methods_UArray2 new(int width, int height, int size)
{
        return UArray2_new(width, height, size);
}


/********** new_with_blocksize ********
*
* Allocates space for and creates a UArray2 (2d array), which stores
* height, width, size of each element, and the elements themselves.
*
* Parameters:
*      int width:       column index of element
*      int height:      row index of element
*      int size:        number of bytes to allocate for each element
*      int blocksize:   desired side length of a block, not used
* Return:
*      Pointer to created UArray2
* Expects:
*      - elementSize to be greater than 0
*
* Notes:
*      - throws CRE if size <= 0
*      - throws CRE if blocksize <= 0
*      - throws CRE if height or width < 0
*      - must be freed by the user using the provided a2free function
*
************************/
static A2Methods_UArray2 new_with_blocksize(int width, int height, int size,
                                            int blocksize)
{
        assert(blocksize > 0);
        return new(width, height, size);
}


/********** a2free ********
*
* Frees allocated memory for provided UArray2
*
* Parameters:
*      T *uarray2p:    Double pointer to UArray2 to free
* Return:
*      none
* Expects:
*      - *uarray2 to point to a valid UArray2 structure
*      - *uarray2 to not be NULL
* Notes:
*      - Throws CRE if uarray2 address is NULL
*      - Throws CRE if *uarray2 is NULL
*      - Sets uarray2 address to NULL after freeing
*
************************/
static void a2free(A2Methods_UArray2 *uarray2p)
{
        UArray2_free((UArray2_T *) uarray2p);
}


/********** width ********
*
* Gets width of provided UArray2
*
* Parameters:
*      T uarray2:    Pointer to desired UArray2 to get width
* Return:
*      integer containing width of specified UArray2
* Expects:
*      - uarray2 to be a valid UArray2 structure
* Notes:
*      - Throws CRE if uarray2 is NULL
*
************************/
static int width(A2Methods_UArray2 uarray2)
{
        return UArray2_width(uarray2);
}


/********** height ********
*
* Gets height of provided UArray2
*
* Parameters:
*      T uarray2:    Pointer to desired UArray2 to get height
* Return:
*      integer containing height of specified UArray2
* Expects:
*      - uarray2 to be a valid UArray2 structure
* Notes:
*      - Throws CRE if uarray2 is NULL
*
************************/
static int height(A2Methods_UArray2 uarray2)
{
        return UArray2_height(uarray2);
}


/********** size ********
*
* Gets size of elements stored in the UArray2
*
* Parameters:
*      T uarray2:    Pointer to desired UArray2 to get element size
* Return:
*      integer containing size of specified UArray2
* Expects:
*      - uarray2 to not be NULL
* Notes:
*      - Throws CRE if uarray2 is NULL
*
************************/
static int size(A2Methods_UArray2 uarray2)
{
        return UArray2_size(uarray2);
}


/********** blocksize ********
*
* Gets size of block stored in the UArray2
*
* Parameters:
*      T uarray2:    Pointer to desired UArray2 to get block size
* Return:
*      integer containing blockSize of specified UArray2
* Expects:
*      - uarray2 to not be NULL
* Notes:
*      - Throws CRE if uarray2 is NULL
*
************************/
static int blocksize(A2Methods_UArray2 uarray2)
{
        assert(uarray2 != NULL);
        return 1;
}


/********** at ********
*
* Returns pointer to element stored at specified row and column index
*
* Parameters:
*      T uarray2:    Pointer to desired UArray2 to get element
*      int i:        column index of element
*      int j:        row index of element
* Return:
*      Pointer to element at specified row and column index
* Expects:
*      - uarray2 to be a valid UArray2 structure
*      - i and j must be within the height and width of UArray2
* Notes:
*      - Throws CRE if i or j are out of bounds of the UArray2
*      - Throws CRE if uarray2 is NULL
*
************************/
static A2Methods_Object *at(A2Methods_UArray2 uarray2, int i, int j)
{
        return UArray2_at(uarray2, i, j);
}


/********** applyfun ********
*
* Function pointer to apply function for UArray2 map functions
*
* Parameters:
*      int i:        column index of element
*      int j:        row index of element
*      UArray2_T array2b:  pointer to array2b structure containing elements
*      void *elem:   pointer to element being mapped
*      void *cl:     context parameter of apply function
* Return:
*      none
* Expects:
*      - elem to exist
*      - cl function to exist
* Notes:
*      - Throws CRE if elem pointer is NULL
*      - Throws CRE if cl function is NULL
*
************************/
typedef void applyfun(int i, int j, UArray2_T array2b, void *elem, void *cl);

static void map_row_major(A2Methods_UArray2 uarray2,
                          A2Methods_applyfun apply,
                          void *cl)
{
        UArray2_map_row_major(uarray2, (applyfun *)apply, cl);
}

static void map_col_major(A2Methods_UArray2 uarray2,
                          A2Methods_applyfun apply,
                          void *cl)
{
        UArray2_map_col_major(uarray2, (applyfun *)apply, cl);
}

struct small_closure {
        A2Methods_smallapplyfun *apply; 
        void                    *cl;
};

static void apply_small(int i, int j, UArray2_T uarray2,
                        void *elem, void *vcl)
{
        struct small_closure *cl = vcl;
        (void)i;
        (void)j;
        (void)uarray2;
        cl->apply(elem, cl->cl);
}

static void small_map_row_major(A2Methods_UArray2        a2,
                                A2Methods_smallapplyfun  apply,
                                void *cl)
{
        struct small_closure mycl = { apply, cl };
        UArray2_map_row_major(a2, apply_small, &mycl);
}

static void small_map_col_major(A2Methods_UArray2        a2,
                                A2Methods_smallapplyfun  apply,
                                void *cl)
{
        struct small_closure mycl = { apply, cl };
        UArray2_map_col_major(a2, apply_small, &mycl);
}

/* struct to hold array2 methods defined above temporarily */
static struct A2Methods_T uarray2_methods_plain_struct = {
        new,
        new_with_blocksize,
        a2free,
        width,
        height,
        size,
        blocksize,
        at,
        map_row_major,
        map_col_major,
        NULL,                  // map_block_major
        map_row_major,         // map_default
        small_map_row_major,                
        small_map_col_major,                 
        NULL,                 // small_map_block_major
        small_map_row_major,  // small_map_default
};

// finally the payoff: here is the exported pointer to the struct

A2Methods_T uarray2_methods_plain = &uarray2_methods_plain_struct;
