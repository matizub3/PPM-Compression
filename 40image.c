/**************************************************************
*
*                     40image.c
*
*       Assignment: arith
*       Authors:    Mateusz, Annica
*       Date:       03/07/25
*
*       40image.c is the main driver for the compress40 and decompress40
*       functions. It reads in command line arguments, handling user input 
*       and calls the appropriate function to compress or decompress the image. 
*
**************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "assert.h"
#include "compress40.h"

static void (*compress_or_decompress)(FILE *input) = compress40;

/********** main **********
* Main function that reads in command line arguments and calls the 
* appropriate function to compress or decompress the image
* 
* Parameters:
*      int argc - number of command line arguments
*      char *argv[] - array of command line arguments
* 
* Return:
*      int - EXIT_SUCCESS
* 
* Expects:
*      argc is non-negative
*      argv is non-null
* 
* Notes:
*      side effect - calls compress40 or decompress40 which writes to stdout
************************/

int main(int argc, char *argv[])
{
        int i;

        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-c") == 0) {
                        compress_or_decompress = compress40;
                } else if (strcmp(argv[i], "-d") == 0) {
                        compress_or_decompress = decompress40;
                } else if (*argv[i] == '-') {
                        fprintf(stderr, "%s: unknown option '%s'\n",
                                argv[0], argv[i]);
                        exit(1);
                } else if (argc - i > 2) {
                        fprintf(stderr, "Usage: %s -d [filename]\n"
                                "       %s -c [filename]\n",
                                argv[0], argv[0]);
                        exit(1);
                } else {
                        break;
                }
        }
        assert(argc - i <= 1);    /* at most one file on command line */
        if (i < argc) {
                FILE *fp = fopen(argv[i], "r");
                assert(fp != NULL);
                compress_or_decompress(fp);
                fclose(fp);
        } else {
                compress_or_decompress(stdin);
        }

        return EXIT_SUCCESS; 
}
