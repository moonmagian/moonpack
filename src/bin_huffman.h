#ifndef BIN_HUFFMAN_H
#define BIN_HUFFMAN_H
#include <stdlib.h>
#include <stdio.h>
enum bhuffman_type { NODE_REDUCED_BIN, NODE_CHAR_BIN };
typedef struct bin_huffman_node {
    int type;
    unsigned char c;
    size_t p;
    struct bin_huffman_node *left;
    struct bin_huffman_node *right;
} bin_huffman_node_t;

typedef struct bin_huffman_char {
    // Use u_int64_t to avoid padding. (No loss anyway)
    u_int64_t bit_length;
    // For worst condition, a byte will be encoded into 256 bits.
    // Arrays in structs are automatically deep copied.
    u_int64_t bits[4];
} bin_huffman_char_t;
typedef struct bin_huffman_head {
    size_t data_size;
    size_t dict_count;
    char reserved[8];
} bin_huffman_head_t;
typedef struct bin_huffman_head_dict {
    unsigned char byte;
    bin_huffman_char_t code;
} bin_huffman_head_dict_t;
// Magic head: MPKB (moonpack binary).
static const u_int32_t bin_huffman_head_magic = 0x424b504dUL;

int node_cmp(const void *node_1, const void *node_2);
int binary_huffman_pack(FILE *in, FILE *out);
int binary_huffman_get_data_size(FILE *in, size_t *out);
int binary_huffman_unpack(FILE *in, FILE *out);
void binary_huffman_tree_walk(bin_huffman_node_t *const root,
                              bin_huffman_char_t code,
                              bin_huffman_char_t out[256]);
void binary_huffman_tree_clean(bin_huffman_node_t *const root);
void binary_huffman_tree_build(bin_huffman_node_t *const root,
                               bin_huffman_char_t code,
                               bin_huffman_head_dict_t *dict, size_t n);
#endif // COMPRESS_STEP_H
