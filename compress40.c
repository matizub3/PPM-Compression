/**************************************************************
*
*                     compress40.c
*
*       Assignment: arith
*       Authors:    Mateusz, Annica
*       Date:       03/07/25
*
*       compress40.c implements functions for compressing/decompressing
*       a PPM image using the lossy compression algorithm. 
*
**************************************************************/
#include "compress40.h"
#include "bitpack.h"
#include "assert.h"
#include "pnm.h"
#include "a2plain.h"
#include "a2methods.h"
#include "arith40.h"
#include <math.h>

#define A2 A2Methods_UArray2

/* Constants */
#define BLOCKSIZE 2
#define BLOCKAREA (BLOCKSIZE * BLOCKSIZE)
#define NUM_CODEWORD_ELEMENTS 6
#define DECOMPRESSION_IMAGE_DENOMINATOR 255

/********** ComponentVideo **********
 *
 * struct to hold a pixel's component video color space
 *
 * Contains:
 *      float y
 *          represents luma (brightness) value in the range [0, 1]
 *
 *      float pb
 *          represents the blue color difference chroma component
 *
 *      float pr
 *          represents the red color difference chroma component
 *
 ************************/
 typedef struct ComponentVideo{
        float y;
        float pb;
        float pr;
} ComponentVideo;


/********** Block_Pixel_Info **********
 *
 * struct to holds all data for processing a 2×2 pixel block during compression 
 * and decompression.
 *
 * Contains:
 *      ComponentVideo compvidArr[4]
 *          array of component video values (Y, Pb, Pr) for each pixel in block
 *
 *      float discreteCosineArr[4]
 *          array storing the DCT coefficients computed from blocks luma values
 *
 *      int quantized_abcd[4]
 *          array storing the quantized DCT coefficients (a, b, c, d) for block
 *
 *      unsigned pb_chromaIndex
 *          quantized index for the average Pb value of the block
 *
 *      unsigned pr_chromaIndex
 *          quantized index for the average Pr value of the block
 *
 *      float pb_mean
 *          average Pb value computed from the block
 *
 *      float pr_mean
 *          average Pr value computed from the block
 *
 ************************/
typedef struct Block_Pixel_Info {
        ComponentVideo compvidArr[4];
        float discreteCosineArr[4];
        int quantized_abcd[4];
        unsigned pb_chromaIndex;
        unsigned pr_chromaIndex;
        float pb_mean;
        float pr_mean;

} Block_Pixel_Info;


/********** CodeWord_Element **********
 *
 * struct to hold single field within a 32-bit codeword for image compression
 *
 * Contains:
 *      uint64_t value
 *          unasigned 64-bit integer value to store in the field
 *
 *      bool isSigned
 *          represents a signed value - true for signed, false for unsigned
 *
 *      unsigned width
 *          unsigned number of bits allocated for the field
 *
 *      unsigned lsb
 *          the position (unsigned bit index) of the least-significant bit of 
 *          the field in the codeword.
 *
 ************************/
typedef struct CodeWord_Element {
        uint64_t value;
        bool isSigned;
        unsigned width;
        unsigned lsb;
} CodeWord_Element;


/********** CodeWord Templates **********
 *
 * define bitfield templates for a 32-bit codeword
 *
 * these templates initialize a CodeWord_Element for a, b, c, d, Pb, and Pr:
 *      .value    = the value to store in the field
 *      .isSigned = a signed value - true for signed, false for unsigned
 *      .width    = the number of bits allocated for the field
 *      .lsb      = the position of the least-significant bit of the field
 *
 ************************/
#define CODEWORD_A  { .value = 0, .isSigned = false, .width = 9, .lsb = 23 }
#define CODEWORD_B  { .value = 0, .isSigned = true,  .width = 5, .lsb = 18 }
#define CODEWORD_C  { .value = 0, .isSigned = true,  .width = 5, .lsb = 13 }
#define CODEWORD_D  { .value = 0, .isSigned = true,  .width = 5, .lsb = 8  }
#define CODEWORD_PB { .value = 0, .isSigned = false, .width = 4, .lsb = 4  }
#define CODEWORD_PR { .value = 0, .isSigned = false, .width = 4, .lsb = 0  }

                        

/********** Function Prototypes **********/
static void applyCompress(int col, int row, A2 uarray2, void *elem, void *cl);
static void applyDecompress(int col, int row, A2 uarray2, void *elem, void *cl);

static void RGB_to_ComponentVideo(Pnm_rgb pixel, unsigned denominator, 
                                  ComponentVideo *compvid);
static void ComponentVideo_to_RGB(float pb_mean, float pr_mean, 
                                  ComponentVideo *compvid, unsigned denominator,
                                  Pnm_rgb pixel);

static void discrete_Cosine_Transform(Block_Pixel_Info *block);
static void inverse_discrete_Cosine_Transform(Block_Pixel_Info *block);

static void abcd_quantization(Block_Pixel_Info *block);
static void chroma_quantization(Block_Pixel_Info *block);

static uint64_t pack_codeword(CodeWord_Element elementArr[]);
static void print_codeword(uint64_t codeword);
static void read_codeword(FILE *input, uint64_t *codeword);
static void extract_bitpack(uint64_t codeword, CodeWord_Element *code_elems);


/********** trim_image **********
 *
 * Trims given PPM image so that its width and height are both even.
 * If the image's width or height is odd, the image is trimmed by removing
 * the last column and/or row
 *
 * Parameters:
 *      Pnm_ppm image - Pointer to the Pnm_ppm image to be trimmed
 *
 * Return:
 *      None
 *
 * Expects:
 *      image is non-null
 *      CRE if image is NULL
 *
 * Notes:
 *      Allocates a new pixel array with even values of width and height
 *      Copies pixels to new pixel array without the last column and/or row
 *      Frees the old pixel array
 ************************/
static void trim_image(Pnm_ppm image)
{
        
        /* assert image is not NULL */
        assert(image != NULL);


        /* calculate new even width */
        unsigned new_width;
        if (image->width % 2 != 0) {
                new_width = image->width - 1;
        } else {
                new_width = image->width;
        }       
        
        /* calculate new even height */
        unsigned new_height;
        if (image->height % 2 != 0) {
                new_height = image->height - 1;
        } else {
                new_height = image->height;
        }

        A2Methods_T methods = uarray2_methods_plain;
        A2Methods_UArray2 new_pixels = methods->new(new_width, new_height, 
                                                    sizeof(struct Pnm_rgb));

        /* copy image pixels to image of even new_width and new_height */
        for (unsigned row = 0; row < new_height; row++) {
            for (unsigned col = 0; col < new_width; col++) {

                Pnm_rgb old_pixel = (Pnm_rgb) methods->at(image->pixels, col, 
                                                          row);

                Pnm_rgb new_pixel = (Pnm_rgb) methods->at(new_pixels, col, row);
                *new_pixel = *old_pixel;

            }
        }

        /* Free the old pixel array and update the image */
        methods->free(&(image->pixels));
        image->pixels = new_pixels;
        image->width = new_width;
        image->height = new_height;
}


/********** compress40 **********
 *
 * Reads a PPM image from the given input file, compresses it using a 2x2 block
 * discrete cosine transform (DCT), quantization, and bit-packing. It then
 * writes the resulting compressed image codewords to stdout.
 *
 * Parameters:
 *      FILE *input - A pointer to an input steam containing a valid PPM
 *
 * Return:
 *      None (writes compressed codewords corresponding to each 2x2 block to
 *            stdout)
 *
 * Expects:
 *      input is non-null and points to a valid PPM image
 *      CRE if input is NULL
 *      CRE if input stream does not contain a valid PPM image
 *      
 *
 * Notes:
 *      Writes compressed header and codewords to stdout
 *      If dimensions of provided image are odd, the image is trimmed by 
 *      removing the last row and/or column 
 ************************/
extern void compress40(FILE *input) 
{
        assert(input != NULL);
        Pnm_ppm image = Pnm_ppmread(input, uarray2_methods_plain);
        
        /* trim image if necessary  */
        if ((image->width % 2 != 0) || (image->height % 2 != 0)) {
                trim_image(image);
        }
            

        /* print header of compressed image */
        printf("COMP40 Compressed image format 2\n%u %u\n", 
                image->width, image->height);

        /* compress image */
        uarray2_methods_plain->map_row_major(image->pixels, applyCompress, 
                                             image);

        /* free image */
        Pnm_ppmfree(&image);
}


/********** decompress40 ******************************************************
 *
 * Reads a compressed image from the given input stream decompresses it by 
 * reading each 32-bit codeword, unpacking the fields, applying the inverse DCT, 
 * and converting from component video back to RGB. The decompressed PPM image
 * is written to stdout.
 *
 * Parameters:
 *      FILE *input - A non-null pointer to an open compressed image file.
 *
 * Return:
 *      None
 *
 * Expects:
 *      input is non-null and its header matches the expected format:
 *              COMP40 Compressed image format 2
 *
 * Notes:
 *      side effect - writes decompressed PPM image to stdout
 *      Will CRE if input is NULL or the header is wrong format.
 *****************************************************************************/
extern void decompress40(FILE *input) 
{
        assert(input != NULL);

        /* parse the header of compressed image */
        unsigned height, width;
        int read = fscanf(input, "COMP40 Compressed image format 2\n%u %u", 
                          &width, &height);
        assert(read == 2);
        int c = getc(input);
        assert(c == '\n');

        /* create a new Pnm_ppm struct */
        int denominator = DECOMPRESSION_IMAGE_DENOMINATOR;
        A2Methods_T methods = uarray2_methods_plain;
        A2 array = methods->new(width, height, sizeof(struct Pnm_rgb));

        struct Pnm_ppm pixmap = { .width = width, .height = height
                                , .denominator = denominator, .pixels = array
                                , .methods = methods
                                };

        Pnm_ppm image = &pixmap;

        /* decompress image */
        uarray2_methods_plain->map_row_major(image->pixels, applyDecompress, 
                                             input);

        /* print decompressed image to output */
        Pnm_ppmwrite(stdout, image);

        /* free image */
        uarray2_methods_plain->free(&(image->pixels));
}


/******************************************************************************
 * 
 *     APPLY HELPER FUNCTIONS FOR COMPRESSION AND DECOMPRESSION
 *
 *****************************************************************************/


/********** applyCompress **********
 *
 * Compress a 2x2 block from the input image. For first pixel in every block it:
 *    - Converts the block from RGB to component video
 *    - Computes the DCT coefficients
 *    - Quantizes the coefficients and chroma values
 *    - Packs these values into a codeword using the Bitpack interface
 *    - Prints the codeword to stdout
 *
 * Parameters:
 *      int col         - The column index of the current pixel
 *      int row         - The row index of the current pixel
 *      A2 uarray2      - The 2D array of Pnm_rgb pixels
 *      void *elem      - Pointer to current element in image array
 *      void *cl        - Pointer to the Pnm_ppm image structure
 *
 * Return:
 *      None
 *
 * Expects:
 *      cl and elem are non-null.
 *
 * Notes:
 *      side effect - prints the codeword for the block to stdout
 *      Only processes the first pixel in each 2x2 block.
 ************************/
static void applyCompress(int col, int row, A2 uarray2, void *elem, void *cl) 
{
        (void)elem;
        assert(cl != NULL);
        assert(elem != NULL);
        
        Pnm_ppm image = (Pnm_ppm)cl;

        /* 2 x 2 block is out of bounds of image */ 
        if (col + BLOCKSIZE - 1 >= (int) image->width 
           || row + BLOCKSIZE - 1 >= (int) image->height) {
                return;
        }

        /* only look at first pixel in block */
        if (col % BLOCKSIZE != 0 || row % BLOCKSIZE != 0) {
                return;
        }

        Block_Pixel_Info block;
        
        /* loop through each pixel in the current 2x2 block */
        for (int block_i = 0; block_i < BLOCKSIZE * BLOCKSIZE; block_i++) {    
                int block_col = col + block_i % BLOCKSIZE;      
                int block_row = row + block_i / BLOCKSIZE;               
                Pnm_rgb pixel = uarray2_methods_plain->at(uarray2, 
                                                          block_col, 
                                                          block_row);

                /* convert RGB block to component video */
                RGB_to_ComponentVideo(pixel, image->denominator, 
                                      &block.compvidArr[block_i]);
        }

        /* apply discrete cosine transform */
        discrete_Cosine_Transform(&block);

        /* apply quantization of a, b, c, d values */
        abcd_quantization(&block);

        /* apply quantization of chroma */
        chroma_quantization(&block);
        
        /* assign computed values to codeword elements */
        CodeWord_Element code_elems[NUM_CODEWORD_ELEMENTS] = { CODEWORD_A, 
                                                               CODEWORD_B,
                                                               CODEWORD_C,
                                                               CODEWORD_D,
                                                               CODEWORD_PB,
                                                               CODEWORD_PR };
        code_elems[0].value = block.quantized_abcd[0];
        code_elems[1].value = block.quantized_abcd[1];
        code_elems[2].value = block.quantized_abcd[2];
        code_elems[3].value = block.quantized_abcd[3];
        code_elems[4].value = block.pb_chromaIndex;
        code_elems[5].value = block.pr_chromaIndex;
        
        /* pack codeword with compressed image components */
        uint64_t bitpacked_codeword = pack_codeword(code_elems);
        
        /* print codeword corresponding to current 2x2 block */
        print_codeword(bitpacked_codeword);       
}


/********** applyDecompress **********
 *
 * Processes a 2x2 block in decompressed image. For first pixel in block it:
 *    - Reads a 32-bit codeword from the input
 *    - Unpacks the quantized values
 *    - Applies the inverse DCT
 *    - Converts the component video values back to RGB values
 *
 * Parameters:
 *      int col         - The column index of the current pixel
 *      int row         - The row index of the current pixel
 *      A2 uarray2      - uarray2 of Pnm_rgb pixels for the decompressed image
 *      void *elem      - Pointer to current element in image array
 *      void *cl        - Pointer to the input FILE with codewords
 *
 * Return:
 *      None
 *
 * Expects:
 *      cl is non-null and points to a valid input stream with codewords
 *      Throws CRE is cl
 *
 * Notes:
 *      writes decompressed pixel values into the uarray2
 *      Only processes the first pixel in each 2x2 block.
 ************************/
static void applyDecompress(int col, int row, A2 uarray2, void *elem, void *cl) 
{       
        (void)elem;
        (void)col;
        (void)row;
         
        assert(cl != NULL);
        FILE *input = (FILE *)cl;

        /* only look at first pixel in block */
        if (col % BLOCKSIZE != 0 || row % BLOCKSIZE != 0) {
                return;
        }

        /* 2 x 2 block is out of bounds of image */ 
        assert(col + BLOCKSIZE - 1 < uarray2_methods_plain->width(uarray2) &&
               row + BLOCKSIZE - 1 < uarray2_methods_plain->height(uarray2));

        /* read codewords from input */
        uint64_t codeword;
        read_codeword(input, &codeword);

        /* unpack codewords */
        CodeWord_Element code_elems[NUM_CODEWORD_ELEMENTS] = { CODEWORD_A, 
                                                               CODEWORD_B,
                                                               CODEWORD_C,
                                                               CODEWORD_D,
                                                               CODEWORD_PB,
                                                               CODEWORD_PR };
        extract_bitpack(codeword, code_elems);
            
        /* create block to store extracted codeword values */
        Block_Pixel_Info block;
        block.quantized_abcd[0] = code_elems[0].value;
        block.quantized_abcd[1] = code_elems[1].value;
        block.quantized_abcd[2] = code_elems[2].value;
        block.quantized_abcd[3] = code_elems[3].value;
        block.pb_chromaIndex = code_elems[4].value;
        block.pr_chromaIndex = code_elems[5].value;
        
        /* get index of chroma */
        block.pb_mean = Arith40_chroma_of_index(block.pb_chromaIndex);
        block.pr_mean = Arith40_chroma_of_index(block.pr_chromaIndex);

        /* apply inverse discrete cosine transform */
        inverse_discrete_Cosine_Transform(&block);

        /* convert each pixel back to RGB values */
        for (int block_i = 0; block_i < BLOCKSIZE * BLOCKSIZE; block_i++) {  
                int block_col = col + block_i % BLOCKSIZE;      
                int block_row = row + block_i / BLOCKSIZE;               
                Pnm_rgb pixel = uarray2_methods_plain->at(uarray2, 
                                                        block_col, 
                                                        block_row);

                /* convert current pixel from component video to RGB */
                ComponentVideo_to_RGB(block.pb_mean, block.pr_mean, 
                                &block.compvidArr[block_i], 
                                255u, pixel);
        }
}

/******************************************************************************
 * 
 *     COMPRESSING HELPER FUNCTIONS
 *
 *****************************************************************************/



/********** RGB_to_ComponentVideo **********
 *
 * Converts an RGB pixel to its component video representation (Y, Pb, Pr)
 *
 * Parameters:
 *      Pnm_rgb pixel         - The source RGB pixel.
 *      unsigned denominator  - The image denominator (max color value).
 *      ComponentVideo *compvid - Pointer to struct to store Y, Pb, and Pr
 *
 * Return:
 *      None
 *
 * Expects:
 *      pixel and compvid are non-null
 *
 * Notes:
 *      side effect - stores Y, Pb, and Pr in ComponentVideo struct by reference
 *      None
 ************************/

static void RGB_to_ComponentVideo(Pnm_rgb pixel, unsigned denominator, 
                                  ComponentVideo *compvid)
{
        /* read the pixel RGB values from the Pnm_rgb pixel struct */
        float r = (float) pixel->red / denominator;
        float g = (float) pixel->green / denominator;
        float b = (float) pixel->blue / denominator;

        /* calculate the Y, Pb, and Pr values with linear transformation */
        float y  =  0.299    * r + 0.587    * g + 0.114    * b;
        float pb = -0.168736 * r - 0.331264 * g + 0.5      * b;
        float pr =  0.5      * r - 0.418688 * g - 0.081312 * b;

        /* store the component video values in ComponentVideo struct*/
        compvid->y = y;
        compvid->pb = pb;
        compvid->pr = pr;
}



/********** discrete_Cosine_Transform **********
 *
 * Applies a discrete cosine transform (DCT) on a 2x2 block, computing the 
 * coefficients a, b, c, and d from the luminance (Y) values
 *
 * Parameters:
 *     Block_Pixel_Info *block - Pointer to block_pixel_info struct containing 
 *                               component video values for each pixel in block
 *
 * Return:
 *     None
 *
 * Expects:
 *     block is non-null and its compvidArr[] contains valid Y values.
 *
 * Notes:
 *     side effect - calculates and updates  a, b, c, d values for 2x2 block
 *     Uses the following formulas
 *          a = (Y1 + Y2 + Y3 + Y4) / 4
 *          b = (Y4 + Y3 - Y2 - Y1) / 4
 *          c = (Y4 - Y3 + Y2 - Y1) / 4
 *          d = (Y4 - Y3 - Y2 + Y1) / 4
 ************************/
static void discrete_Cosine_Transform(Block_Pixel_Info *block) 
{
        /* extract Y values from the component video struct */
        float y1 = block->compvidArr[0].y;
        float y2 = block->compvidArr[1].y;
        float y3 = block->compvidArr[2].y;
        float y4 = block->compvidArr[3].y;

        /* calculate DCT coefficients */
        block->discreteCosineArr[0] = (y4 + y3 + y2 + y1) / (float) BLOCKAREA;
        block->discreteCosineArr[1] = (y4 + y3 - y2 - y1) / (float) BLOCKAREA;
        block->discreteCosineArr[2] = (y4 - y3 + y2 - y1) / (float) BLOCKAREA;
        block->discreteCosineArr[3] = (y4 - y3 - y2 + y1) / (float) BLOCKAREA;
}
    

/********** clamp **********
 *
 * Clamps a floating-point value between a specified minimum and maximum value
 *
 * Parameters:
 *      float value - The value to be clamped
 *      float min   - The minimum allowed value
 *      float max   - The maximum allowed value
 *
 * Return:
 *      The clamped value:
 *          value if min <= value <= max,
 *          min if value < min,
 *          max if value > max
 *
 * Expects:
 *      min is less than or equal to max
 *
 * Notes:
 *     None
 ************************/
static inline float clamp(float value, float min, float max) 
{
        /* if value is less than min, return min */
        if (value < min) {
                return min;
        }

        /* if value is greater than max, return max */
        if (value > max) {
                return max;       
        }
        
        /* else, return value */
        return value;
}


/********** abcd_quantization **********
 *
 * Quantizes DCT coefficients a, b, c, d for a 2x2 block in the image
 * 
 *
 * Parameters:
 *      Block_Pixel_Info *block - Pointer to block whose DCT coefficients 
 *                                are to be quantized
 *
 * Return:
 *      None
 *
 * Expects:
 *      block is non-null and discreteCosineArr[] is computed
 *
 * Notes:
 *      side effect - Stores the quantized values in block->quantized_abcd[]
 *      Scales and rounds coefficient a (luminance) to 9 bits
 *      clamps b, c, and d to the range [-0.3, 0.3] before scaling to 5 bits
 ************************/
static void abcd_quantization(Block_Pixel_Info *block) 
{
        /* extract DCT values */
        float a = block->discreteCosineArr[0];
        float b = block->discreteCosineArr[1];
        float c = block->discreteCosineArr[2];
        float d = block->discreteCosineArr[3];

        /* clamp abcd */
        a = clamp(a,  0.0, 1.0);
        b = clamp(b, -0.3, 0.3);
        c = clamp(c, -0.3, 0.3);
        d = clamp(d, -0.3, 0.3);
        
        float a_scaling_factor = 511.0;
        float bcd_scaling_factor = 50.0;
        
        /* quantize abcd */
        unsigned quantized_a = (unsigned) round(a * a_scaling_factor);
        int quantized_b = (int) round(b * bcd_scaling_factor);
        int quantized_c = (int) round(c * bcd_scaling_factor);
        int quantized_d = (int) round(d * bcd_scaling_factor);


        /* store the quantized values in the block */
        block->quantized_abcd[0] = quantized_a;
        block->quantized_abcd[1] = quantized_b;
        block->quantized_abcd[2] = quantized_c;
        block->quantized_abcd[3] = quantized_d;
}

/********** chroma_quantization **********
 *
 * Computes and quantizes the average chroma (Pb and Pr) for a 2x2 block
 *
 * Parameters:
 *      Block_Pixel_Info *block - Pointer to block whose chroma values 
 *                                are to be quantized.
 *
 * Return:
 *      None
 *
 * Expects:
 *      block is non-null and compvidArr[] calculated component video values
 *
 * Notes:
 *      side effect - stores Pb, Pr means and quantized chroma indices in block
 ************************/
static void chroma_quantization(Block_Pixel_Info *block) 
{
        float pb_mean = 0.0;
        float pr_mean = 0.0;

        /* calculate pb, pr mean value*/
        for (unsigned pixel_index = 0; pixel_index < 4; pixel_index++) {
                pb_mean += block->compvidArr[pixel_index].pb;
                pr_mean += block->compvidArr[pixel_index].pr;
        }

        /* calculate mean values */
        pb_mean /= (float) BLOCKAREA;
        pr_mean /= (float) BLOCKAREA;

        /* store mean values in block passed by reference */
        block->pb_mean = pb_mean;
        block->pr_mean = pr_mean;

        /* get and store index of chroma corresponding to pb, pr means */
        block->pb_chromaIndex = Arith40_index_of_chroma(pb_mean);
        block->pr_chromaIndex = Arith40_index_of_chroma(pr_mean);
}


/********** pack_codeword **********
 *
 * Packs an array of CodeWord_Element structures into a single 32-bit codeword
 *
 * Parameters:
 *      CodeWord_Element elementArr[] -  array of length NUM_CODEWORD_ELEMENTS
 *                                       containing the fields to be packed:
 *                                       (a, b, c, d, Pb index, Pr index)
 *
 * Return:
 *      A 32-bit codeword stored in a uint64_t with fields packed as specified
 *
 * Expects:
 *      elementArr contains Codeword_Elements correspoding to a, b, c, d,
 *      and Pb, Pr index values
 *
 * Notes:
 *      Uses Bitpack_newu or Bitpack_news depending on the field's signedness
 ************************/
static uint64_t pack_codeword(CodeWord_Element elementArr[])
{
        uint64_t new_word = 0;

        /* iterate over CodeWord_Element fields and pack into new_word */
        for (unsigned field_i = 0; field_i < NUM_CODEWORD_ELEMENTS; field_i++) {
                CodeWord_Element element = elementArr[field_i];
                uint64_t value = element.value;
                unsigned width = element.width;
                unsigned lsb = element.lsb;

                /* call right bitpacking function based on sign */
                if (element.isSigned) {
                        new_word = Bitpack_news(new_word, width, lsb, value);
                } else {
                        new_word = Bitpack_newu(new_word, width, lsb, value);
                }
        }
        
        /* return packed codeword */
        return new_word;
}


/********** print_codeword **********
 *
 * Prints a 32-bit codeword (stored in a uint64_t) to stdout in big-endian order
 *
 * Parameters:
 *      uint64_t codeword - The codeword to print
 *
 * Return:
 *      None
 *
 * Expects:
 *      The lower 32 bits of codeword contain the valid codeword
 *
 * Notes:
 *      side effect - writes codeword to stdout in big-endian order
 *      Iterates from most significant byte to least significant byte
 *      Codeword is divided into four 8-bit bytes, where each byte is extracted
 *      using Bitpack_getu
 ************************/
static void print_codeword(uint64_t codeword) 
{
        /* extract codeword in big-endian order */
        for (int byte_index = 3; byte_index >= 0; byte_index--) {
                uint64_t byte = Bitpack_getu(codeword, 8, byte_index * 8);
                putchar(byte);
        }
}


/******************************************************************************
 * 
 *     DECOMPRESSING HELPER FUNCTIONS
 *
 *****************************************************************************/




/********** read_codeword **********
 *
 * Reads a 32-bit codeword from input stream and reconstructs it
 * by reading four bytes, storing the result in the provided codeword address
 *
 * Parameters:
 *      FILE *input        - File pointer to file containing codewords from
 *                           decompressed image 
 *      uint64_t *codeword - Pointer to where read-in codeword will be stored
 *
 * Return:
 *      None
 *
 * Expects:
 *      input and codeword are non-null
 *
 * Notes:
 *      side effect - *codeword is set to the read in codeword
 *      Reads in codeword in big-endian order 
 *      Uses Bitpack_newu to insert each byte into its proper field
 ************************/
static void read_codeword(FILE *input, uint64_t *codeword) 
{
        /* assert codeword is not NULL */
        assert(codeword != NULL);

        *codeword = 0;

        /* read in codeword in big-endian order */
        for (int byte_index = 3; byte_index >= 0; byte_index--) {
                int curr_char = getc(input);
                assert(curr_char != EOF);

                uint64_t value = (uint64_t)curr_char;
                *codeword = Bitpack_newu(*codeword, 8, byte_index * 8, value);
        }
}


/********** extract_bitpack **********
 *
 * Extracts individual fields from a 32-bit codeword and stores them into array
 * of CodeWord_Element structures
 *
 * Parameters:
 *      uint64_t codeword            - 32-bit codeword packed with a, b, c, d
 *                                     Pb chroma index, Pr chroma index values
 *      CodeWord_Element *code_elems - Pointer to an array of CodeWord_Elements
 *                                     formatted to support each codeword value
 *
 * Return:
 *      None
 *
 * Expects:
 *      code_elems is not NULL and points to an array with NUM_CODEWORD_ELEMENTS
 *      elements
 *
 * Notes:
 *      side effect - code_elems array is updated with the extracted codeword 
 *      Assumes that the codeword is formatted with values in the order:
 *              a, b, c, d, Pb chroma index, Pr chroma index
 ************************/
static void extract_bitpack(uint64_t codeword, CodeWord_Element *code_elems) 
{
        /* iterate over each field in the codeword */
        for (int field_i = 0; field_i < NUM_CODEWORD_ELEMENTS; field_i++) {
                /* extract value from codeword */
                CodeWord_Element *elem = &code_elems[field_i];

                /* call right bitpacking function based on sign */
                if (elem->isSigned) {
                        elem->value = Bitpack_gets(codeword, elem->width, 
                                                             elem->lsb);
                } else {
                        elem->value = Bitpack_getu(codeword, elem->width, 
                                                             elem->lsb);
                }
        }
}


/********** inverse_discrete_Cosine_Transform **********
 *
 * Applies the inverse discrete cosine transform to convert quantized DCT
 * coefficients back into luminance (Y) values for a 2x2 block
 *
 * Parameters:
 *      Block_Pixel_Info *block - Pointer to a block with quantized DCT 
 *                                coefficients
 *
 * Return:
 *      None
 *
 * Expects:
 *      block is non-null and quantized_abcd[] contains valid quantized values
 *
 * Notes:
 *      side effect - stores calculated Y values in block passed by reference
 *      Uses the formulas:
 *          Y1 = a - b - c + d
 *          Y2 = a - b + c - d
 *          Y3 = a + b - c - d
 *          Y4 = a + b + c + d
 ************************/
static void inverse_discrete_Cosine_Transform(Block_Pixel_Info *block) 
{
        /* set scaling factors for a, b, c, d values */
        float a_scaling_factor = 511.0;
        float bcd_scaling_factor = 50.0;
        
        /* extract quantized a, b, c, d values */
        float a = block->quantized_abcd[0] / a_scaling_factor;
        float b = block->quantized_abcd[1] / bcd_scaling_factor;
        float c = block->quantized_abcd[2] / bcd_scaling_factor;
        float d = block->quantized_abcd[3] / bcd_scaling_factor;
        
        /* calculate Y values */
        block->compvidArr[0].y = a - b - c + d;
        block->compvidArr[1].y = a - b + c - d;
        block->compvidArr[2].y = a + b - c - d;
        block->compvidArr[3].y = a + b + c + d;
}


/********** ComponentVideo_to_RGB **********
 *
 * Converts a component video pixel back to its RGB representation
 *
 * Parameters:
 *      float pb_mean           - Average Pb value for the block.
 *      float pr_mean           - Average Pr value for the block.
 *      ComponentVideo *compvid - Pointer to componentVideo data for 2x2 block 
 *      unsigned denominator    - Image denominator (max color value)
 *      Pnm_rgb pixel           - The destination RGB pixel
 *
 * Return:
 *      None
 *
 * Expects:
 *      compvid and pixel are non-null
 *      CRE if compvid is NULL
 *      CRE if pixel is NULL
 *
 * Notes:
 *      side effect - calculates & updates RGB pixel by reference
 ************************/
static void ComponentVideo_to_RGB(float pb_mean, float pr_mean, 
                                  ComponentVideo *compvid, unsigned denominator, 
                                  Pnm_rgb pixel) 
{
        /* assert compvid and pixel are not NULL */
        assert(compvid != NULL);
        assert(pixel != NULL);
        
        /* read luminance (Y) from ComponentVideo struct */
        float y  = compvid->y;
        float pb = pb_mean;
        float pr = pr_mean;

        /* apply calculation to get r, b, g values from component video */
        float r = 1.0 * y + 0.0      * pb + 1.402    * pr;
        float g = 1.0 * y - 0.344136 * pb - 0.714136 * pr;
        float b = 1.0 * y + 1.772    * pb + 0.0      * pr;

        /* update pixel with calculated r, g, b values */
        pixel->red   = (unsigned) clamp(r * denominator, 0, denominator);
        pixel->green = (unsigned) clamp(g * denominator, 0, denominator);
        pixel->blue  = (unsigned) clamp(b * denominator, 0, denominator);
}