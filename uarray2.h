/**************************************************************
 *
 *                     uarray2.h
 *
 *     Assignment: iii
 *     Authors:  Mateusz, Annica
 *     Date:     02/03/25
 *
 *     uarray2.h defines an interface for using a 2-dimensional Hanson UArray
 *
 **************************************************************/
 
 #ifndef __UARRAY2_H
 #define __UARRAY2_H
 
 #include "uarray.h"
 
 #define T UArray2_T
 
 typedef struct T *T;
 
 
 T UArray2_new(int width, int height, int elementSize);
 
 void UArray2_free(T *uarray2);
 
 int UArray2_width(T uarray2);
 int UArray2_height(T uarray2);
 int UArray2_size(T uarray2);  
 
 void *UArray2_at(T uarray2, int col, int row);
 
 void UArray2_map_row_major(T uarray2,
                            void apply(int col, int row, T uarray2,
                                       void *elem, void *cl),
                            void *cl);
 void UArray2_map_col_major(T uarray2,
                            void apply(int col, int row, T uarray2,
                                       void *elem, void *cl),
                            void *cl);
 
 #undef T
 #endif 