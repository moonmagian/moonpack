#ifndef TERN_HUFFMAN_H
#define TERN_HUFFMAN_H
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
enum thuffman_type { NODE_REDUCED_TERN, NODE_CHAR_TERN, NODE_DUMMY_TERN };
typedef struct tern_huffman_node {
    int type;
    unsigned char c;
    uint64_t p;
    struct tern_huffman_node *left;
    struct tern_huffman_node *middle;
    struct tern_huffman_node *right;
} tern_huffman_node_t;

typedef struct ternary_bit {
    unsigned char bit;
} ternary_bit_t;
unsigned char ternary_bit_to_two_low_bits(ternary_bit_t tbit);
ternary_bit_t two_low_bits_to_ternary(unsigned char bits);

typedef struct tern_huffman_char {
    uint16_t bit_length;

    // For worst condition, a byte will be encoded into 256 tbits.
    // Arrays in structs are automatically deep copied.
    ternary_bit_t tbits[256];
} tern_huffman_char_t;
typedef struct tern_huffman_head {
    uint64_t data_size;
    uint64_t dict_count;
    char has_dummy;
    char reserved[8];
} tern_huffman_head_t;
typedef struct tern_huffman_head_dict {
    unsigned char byte;
    tern_huffman_char_t code;
} tern_huffman_head_dict_t;
// Magic head: MPKT (moonpack binary).
static const uint32_t tern_huffman_head_magic = 0x544b504dUL;

int ternary_node_cmp(const void *node_1, const void *node_2);
int ternary_huffman_pack(FILE *in, FILE *out);
int ternary_huffman_get_data_size(FILE *in, uint64_t *out);
int ternary_huffman_unpack(FILE *in, FILE *out);
void ternary_huffman_tree_walk(tern_huffman_node_t *const root,
                               tern_huffman_char_t code,
                               tern_huffman_char_t out[256],
                               tern_huffman_char_t *dummy_out);
void ternary_huffman_tree_clean(tern_huffman_node_t *const root);
void ternary_huffman_tree_build(tern_huffman_node_t *const root,
                                tern_huffman_char_t code,
                                tern_huffman_head_dict_t *dict, uint64_t n,
                                tern_huffman_char_t *dummy);
#endif // COMPRESS_STEP_H
