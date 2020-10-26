#ifndef QUIN_HUFFMAN_H
#define QUIN_HUFFMAN_H
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
enum qhuffman_type { NODE_REDUCED_QUIN, NODE_CHAR_QUIN, NODE_DUMMY_QUIN };
typedef struct quin_huffman_node {
    int type;
    unsigned char c;
    uint64_t p;
    struct quin_huffman_node *left;
    struct quin_huffman_node *middle1;
    struct quin_huffman_node *middle2;
    struct quin_huffman_node *middle3;
    struct quin_huffman_node *right;
} quin_huffman_node_t;

typedef struct quinary_bit {
    unsigned char bit;
} quinary_bit_t;
unsigned char quinary_bit_to_three_low_bits(quinary_bit_t tbit);
quinary_bit_t three_low_bits_to_quinary(unsigned char bits);

typedef struct quin_huffman_char {
    uint16_t bit_length;

    // For worst condition, a byte will be encoded into 256 tbits.
    // Arrays in structs are automatically deep copied.
    quinary_bit_t qbits[256];
} quin_huffman_char_t;
typedef struct quin_huffman_head {
    uint64_t data_size;
    uint64_t dict_count;
    char reserved[8];
} quin_huffman_head_t;
typedef struct quin_huffman_head_dict {
    unsigned char byte;
    quin_huffman_char_t code;
} quin_huffman_head_dict_t;
typedef struct quin_huffman_head_dummy_list {
    unsigned char count;
    quin_huffman_char_t codes[4];
} quin_huffman_head_dummy_list_t;
// Magic head: MPKQ (moonpack quinary).
static const uint32_t quin_huffman_head_magic = 0x514b504dUL;

int quinary_node_cmp(const void *node_1, const void *node_2);
int quinary_huffman_pack(FILE *in, FILE *out);
int quinary_huffman_get_data_size(FILE *in, uint64_t *out);
int quinary_huffman_unpack(FILE *in, FILE *out);
void quinary_huffman_tree_walk(quin_huffman_node_t *const root,
                               quin_huffman_char_t code,
                               quin_huffman_char_t out[256],
                               quin_huffman_head_dummy_list_t *dummy_out);
void quinary_huffman_tree_clean(quin_huffman_node_t *const root);
void quinary_huffman_tree_build(quin_huffman_node_t *const root,
                                quin_huffman_char_t code,
                                quin_huffman_head_dict_t *dict, uint64_t n,
                                quin_huffman_head_dummy_list_t *dummylist);
#endif // COMPRESS_STEP_H
