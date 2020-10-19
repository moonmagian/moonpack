#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bin_huffman.h"
#include "tern_huffman.h"
#include "quin_huffman.h"
enum opmode { BIN, TERN, QUIN };
enum op { PACK, UNPACK };
void print_help() {
    printf("moonpack - A simple huffman based compressing program.\n"
           "Usage: moonpack [-xbtq] -i infile -o outfile\n"
           "\t-i infile: input file.\n"
           "\t-o outfile: output file.\n"
           "\t-x: Decompress instead of compressing.\n"
           "\t-b: Use binary huffman encoding (default).\n"
           "\t-t: Use ternary huffman encoding.\n"
           "\t-q: Use quinary huffman encoding.\n"
           "\t-h: Display this help message.");
}
int main(int argc, char **argv) {
    int opmode = BIN;
    int op = PACK;
    char *inpath = 0;
    char *outpath = 0;
    int c;
    while ((c = getopt(argc, argv, "i:o:xbtqh")) != -1) {
        switch (c) {
        case 'i':
            inpath = optarg;
            break;
        case 'o':
            outpath = optarg;
            break;
        case 'x':
            op = UNPACK;
            break;
        case 'b':
            opmode = BIN;
            break;
        case 't':
            opmode = TERN;
            break;
        case 'q':
            opmode = QUIN;
            break;
        case 'h':
            print_help();
            return 0;
        case '?':
            if (optopt == 'i' || optopt == 'o') {
                // fprintf(stderr, "Option -%c requires an argument.\n",
                // optopt);
                fprintf(stderr, "Use moonpack -h to get some help.\n");
            } else {
                // fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                fprintf(stderr, "Use moonpack -h to get some help.\n");
            }
            return 1;
        default:
            abort();
        }
    }
    if (inpath == NULL) {

        fprintf(stderr, "Option -i is required.\n");
        fprintf(stderr, "Use moonpack -h to get some help.\n");
        return 1;
    }
    if (outpath == NULL) {

        fprintf(stderr, "Option -o is required.\n");
        fprintf(stderr, "Use moonpack -h to get some help.\n");
        return 1;
    }
    FILE *infile = fopen(inpath, "rb");
    if (infile == NULL) {
        fprintf(stderr, "Unable to open infile.\n");
        return 1;
    }
    FILE *outfile = fopen(outpath, "wb");
    if (outfile == NULL) {
        fprintf(stderr, "Unable to open outfile.\n");
        return 1;
    }
    int result = 0;
    if (op == PACK) {
        switch (opmode) {
        case BIN:
            binary_huffman_pack(infile, outfile);
            break;
        case TERN:
            ternary_huffman_pack(infile, outfile);
            break;
        case QUIN:
            quinary_huffman_pack(infile, outfile);
            break;
        }
    } else {
        switch (opmode) {
        case BIN:
            result = binary_huffman_unpack(infile, outfile);
            break;
        case TERN:
            result = ternary_huffman_unpack(infile, outfile);
            break;
        case QUIN:
            result = quinary_huffman_unpack(infile, outfile);
            break;
        }
    }
    if (result == -1) {
        switch (opmode) {
        case BIN:
            fprintf(stderr,
                    "Not a valid moonpack binary huffman compressed file.\n");
            break;
        case TERN:
            fprintf(stderr,
                    "Not a valid moonpack ternary huffman compressed file.\n");
            break;
        case QUIN:
            fprintf(stderr,
                    "Not a valid moonpack quirnary huffman compressed file.\n");
            break;
        }
    }
    fclose(infile);
    fclose(outfile);
    return 0;
}
