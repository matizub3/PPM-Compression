/**************************************************************
 *
 *                     uarray2.c
 *
 *     Assignment: iii
 *     Authors:  Mateusz, Annica
 *     Date:     02/03/25
 *
 *     uarray2.c TODO
 *
 **************************************************************/

 #include "uarray2.h"
 #include "mem.h"
 #include "assert.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <stdbool.h>
 
 #define T UArray2_T
 
 /* 
  * struct to hold uarray2 data including width, height, 
  * element size, and a UArray_T with elements 
  */
 struct T {
         int width;
         int height;
         int elementSize;
         UArray_T elements;
 };
 
 
 /********** UArray2_new ********
  *
  * Allocates space for and creates a UArray2 (2d array), which stores  
  * height, width, size of each element, and the elements themselves.
  *
  * Parameters:
  *      int width:            desired width of new UArray2
  *      int height:           desired height of new UArray2
  *      int elementSize:      number of bytes to allocate for each element
  * Return: 
  *      Pointer to created UArray2
  * Expects: 
  *      - elementSize to be greater than 0
  *      - height and width to be greater than or equal to 0
  * Notes: 
  *      - throws CRE if elementSize <= 0
  *      - throws CRE if height or width < 0
  *
  ************************/
 T UArray2_new(int width, int height, int elementSize) 
 {
         assert(width >= 0 && height >= 0 && elementSize >= 0);
         
         T new_uarray2 = ALLOC(sizeof(*new_uarray2));
 
         new_uarray2->width = width;
         new_uarray2->height = height;
         new_uarray2->elementSize = elementSize;
         new_uarray2->elements = UArray_new(width * height, elementSize);
 
         return new_uarray2;
 
 }
 
 
 /********** UArray2_free ********
  *
  * Frees allocated memory for provided UArray2
  *
  * Parameters:
  *      T *uarray2:    Double pointer to UArray2 to free
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
 void UArray2_free(T *uarray2) 
 {
         /* ensure that uarray2 and value of uarray2 are not NULL */
         assert(uarray2 != NULL);
         assert(*uarray2 != NULL);
 
         /* free contents of uarray2 using UArray_free */
         UArray_free(&((*uarray2)->elements)); 
 
         /* free and set uarray2 to NULL */
         FREE(*uarray2); 
         *uarray2 = NULL; 
 }
 
 
 /********** UArray2_width ********
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
 int UArray2_width(T uarray2) 
 {
         assert(uarray2 != NULL);
         return uarray2->width;
 }
 
 
 /********** UArray2_height ********
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
 int UArray2_height(T uarray2) 
 {
         assert(uarray2 != NULL);
         return uarray2->height;
 }
 
 
 /********** UArray2_size ********
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
 int UArray2_size(T uarray2)
 {
         assert(uarray2 != NULL);
         return uarray2->elementSize;
 }
 
 
 /********** Uarray2_at ********
  *
  * Returns pointer to element stored at specified row and column index
  *
  * Parameters:
  *      T uarray:       UArray2 of desired element
  *      int row:        row index of element to retrieve
  *      int col:        column index of element to retrieve
  * Return: 
  *      void pointer to element in specified UArray2 at specifed index
  * Expects: 
  *      - uarray2 to be a valid UArray2 structure
  *      - col and row must be valid coordinates within the height 
  *        and width of UArray2
  * Notes: 
  *      - Throws CRE if row, col out of bounds of the UArray2
  *      - Throws CRE if uarray is NULL
  *
  ************************/
 void *UArray2_at(T uarray2, int col, int row)
 {        
         /* make sure uarray2 is not NULL and col, row within bounds */
         assert(uarray2 != NULL);
         assert(col >= 0 && col < uarray2->width);
         assert(row >= 0 && row < uarray2->height);
 
         /* convert 2d index to 1d index */
         int index = row * uarray2->width + col;
         
         return UArray_at(uarray2->elements, index);
 }
 
 
 /********** UArray2_map_row_major ********
  *
  * Calls an apply function on every element in a specified UArray2 where
  * column indices vary more rapidly than row indices.
  *
  * Parameters:
  *      T uarray2:      pointer to specified UArray2
  *      apply():        A well-defined apply function
  *              int col:   column index of element
  *              int row:   row index of element
  *              T uarray2: pointer to UArray2 structure containing elements
  *              void *cl:  context parameter of apply function
  * Return: 
  *      none
  * Expects: 
  *      - uarray2 to be a valid UArray2 structure
  *      - well-defined apply function
  * Notes: 
  *      - Throws CRE if uarray2 pointer is NULL
  *      - Throws CRE if apply function is NULL
  *
  ************************/
 void UArray2_map_row_major(T uarray2,
                            void apply(int col, int row, T uarray2,
                                       void *elem, void *cl),
                            void *cl)
 {
         /* make sure parameters are not NULL */
         assert(uarray2 != NULL);
         assert(apply != NULL);
 
         /* loop through rows first */
         for (int row = 0; row < uarray2->height; row++) {
                 /* loop through cols second */
                 for (int col = 0; col < uarray2->width; col++) {
                         void *elem = UArray2_at(uarray2, col, row);
                         apply(col, row, uarray2, elem, cl);
                 }
         }
 }
 
 
 /********** UArray2_map_col_major ********
  *
  * Calls an apply function on every element in a specified UArray2 where
  * row indices vary more rapidly than columns.
  *
  * Parameters:
  *      T uarray2:      pointer to specified UArray2
  *      apply():        A well-defined apply function
  *              int col:   column index of element
  *              int row:   row index of element
  *              T uarray2: pointer to UArray2 structure containing elements
  *              void *cl:  context parameter of apply function
  * Return: 
  *      none
  * Expects: 
  *      - uarray2 to be a valid UArray2 structure
  *      - well-defined apply function
  * Notes: 
  *      - Throws CRE if uarray2 pointer is NULL
  *      - Throws CRE if apply function is NULL
  *
  ************************/
 void UArray2_map_col_major(T uarray2,
                            void apply(int col, int row, T uarray2,
                                       void *elem, void *cl),
                            void *cl)
 {
         /* make sure parameters are good */
         assert(uarray2 != NULL);
         assert(apply != NULL);
 
         /* loop through cols first */
         for (int col = 0; col < uarray2->width; col++) { 
                 /* loop through rows second */
                 for (int row = 0; row < uarray2->height; row++) { 
                         void *elem = UArray2_at(uarray2, col, row);
                         apply(col, row, uarray2, elem, cl);
                 }
         }
 }
 
 #undef T