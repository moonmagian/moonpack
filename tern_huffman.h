#ifndef COMPRESS_STEP_H
#define COMPRESS_STEP_H
#include <stdlib.h>
#include <stdio.h>
enum type { NODE_REDUCED, NODE_CHAR };
typedef struct tern_huffman_node {
    int type;
    unsigned char c;
    size_t p;
    struct tern_huffman_node *left;
    struct tern_huffman_node *middle;
    struct tern_huffman_node *right;
} tern_huffman_node_t;

typedef struct ternary_bit {
    unsigned char bit;
} ternary_bit_t;
inline unsigned char ternary_bit_to_two_low_bits(ternary_bit_t tbit) {
    if (tbit.bit <= 2) {
        return tbit.bit;
    } else {
        return 0xff;
    }
}
inline ternary_bit_t two_low_bits_to_ternary(unsigned char bits) {
    ternary_bit_t result;
    switch (bits & 0x03) {
    case 0:
        result.bit = 0;
        return result;
    case 1:
        result.bit = 1;
        return result;
    case 2:
        result.bit = 2;
        return result;
    default:
        result.bit = 2;
        return result;
    }
}

typedef struct tern_huffman_char {
    u_int16_t bit_length;

    // For worst condition, a byte will be encoded into 256 tbits.
    // Arrays in structs are automatically deep copied.
    ternary_bit_t tbits[256];
} tern_huffman_char_t;
typedef struct tern_huffman_head {
    size_t data_size;
    size_t dict_count;
    char reserved[8];
} tern_huffman_head_t;
typedef struct tern_huffman_head_dict {
    unsigned char byte;
    tern_huffman_char_t code;
} tern_huffman_head_dict_t;

// Magic head: MPKT (moonpack binary).
static const u_int32_t tern_huffman_head_magic = 0x544b504dUL;

int ternary_node_cmp(const void *node_1, const void *node_2);
int ternary_huffman_pack(FILE *in, FILE *out);
int ternary_huffman_get_data_size(FILE *in, size_t *out);
int ternary_huffman_unpack(FILE *in, FILE *out);
void ternary_huffman_tree_walk(tern_huffman_node_t *const root,
                               tern_huffman_char_t code,
                               tern_huffman_char_t out[257]);
void ternary_huffman_tree_clean(tern_huffman_node_t *const root);
void ternary_huffman_tree_build(tern_huffman_node_t *const root,
                                tern_huffman_char_t code,
                                tern_huffman_head_dict_t *dict, size_t n);
#endif // COMPRESS_STEP_H
