#include "quin_huffman.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tools.h"
// #define DEBUG_OUTPUT
int quinary_node_cmp(const void *node_1, const void *node_2) {
    quin_huffman_node_t *const *node_1_ptr = node_1;
    quin_huffman_node_t *const *node_2_ptr = node_2;
    if ((*node_1_ptr)->p > (*node_2_ptr)->p) {
        return -1;
    }
    if ((*node_1_ptr)->p < (*node_2_ptr)->p) {
        return 1;
    }
    return 0;
}

int quinary_huffman_pack(FILE *in, FILE *out) {
    uint64_t cnt[256] = {0};
    unsigned char value;
    while (fread(&value, sizeof(value), 1, in) == sizeof(value)) {
        ++cnt[value];
    }
    long fsize = ftell(in);
    rewind(in);

    // Quinary huffman needs node counts satisfying c % 4 == 1.
    // 257 % 4 == 1, so at most there will be 257 nodes.
    quin_huffman_node_t *huffman_nodes[257] = {0};
    uint64_t node_count = 0;
    for (int i = 0; i != 256; ++i) {
        if (cnt[i]) {
            huffman_nodes[node_count] = malloc(sizeof(quin_huffman_node_t));
            huffman_nodes[node_count]->c = (unsigned char)i;
            huffman_nodes[node_count]->p = cnt[i];
            huffman_nodes[node_count]->type = NODE_CHAR_QUIN;
            huffman_nodes[node_count]->left = NULL;
            huffman_nodes[node_count]->middle1 = NULL;
            huffman_nodes[node_count]->middle2 = NULL;
            huffman_nodes[node_count]->middle3 = NULL;
            huffman_nodes[node_count]->right = NULL;

            ++node_count;
        }
    }
    unsigned char dummy_count = 0;
    for (dummy_count = 0; (node_count + dummy_count) % 4 != 1; ++dummy_count)
        ;
    // For every needed dummy.
    for (unsigned char i = 0; i != dummy_count; ++i) {
        // Add a dummy node.
        huffman_nodes[node_count] = malloc(sizeof(quin_huffman_node_t));
        huffman_nodes[node_count]->c = 0x00U;
        huffman_nodes[node_count]->p = 0;
        huffman_nodes[node_count]->type = NODE_DUMMY_QUIN;
        huffman_nodes[node_count]->left = NULL;
        huffman_nodes[node_count]->middle1 = NULL;
        huffman_nodes[node_count]->middle2 = NULL;
        huffman_nodes[node_count]->middle3 = NULL;
        huffman_nodes[node_count]->right = NULL;

        ++node_count;
    }
    qsort(huffman_nodes, node_count, sizeof(quin_huffman_node_t *),
          quinary_node_cmp);
    while (node_count != 1) {
        // reduce three nodes.
        quin_huffman_node_t *node1 = huffman_nodes[--node_count];
        quin_huffman_node_t *node2 = huffman_nodes[--node_count];
        quin_huffman_node_t *node3 = huffman_nodes[--node_count];
        quin_huffman_node_t *node4 = huffman_nodes[--node_count];
        quin_huffman_node_t *node5 = huffman_nodes[--node_count];
        quin_huffman_node_t *reduced_node = malloc(sizeof(quin_huffman_node_t));
        reduced_node->type = NODE_REDUCED_QUIN;
        reduced_node->left = node1;
        reduced_node->middle1 = node2;
        reduced_node->middle2 = node3;
        reduced_node->middle3 = node4;
        reduced_node->right = node5;
        reduced_node->p = node1->p + node2->p + node3->p + node4->p + node5->p;
        huffman_nodes[node_count++] = reduced_node;
        // sort again.
        qsort(huffman_nodes, node_count, sizeof(quin_huffman_node_t *),
              quinary_node_cmp);
    }

    // We don't need dummy node in dict (never appeared).
    quin_huffman_char_t dict[256] = {0};
    quin_huffman_char_t init_code = {0, {{0}}};
    quin_huffman_head_dummy_list_t dummy_list = {0, {{0}}};

    // Save dummy code. We need all dummy codes to rebuild the tree.
    // Or the tree will generate forever and raise SIGSEGV.
    quinary_huffman_tree_walk(huffman_nodes[0], init_code, dict, &dummy_list);
    quinary_huffman_tree_clean(huffman_nodes[0]);
#ifdef DEBUG_OUTPUT
    for (int i = 0; i != 256; ++i) {
        if (dict[i].bit_length != 0) {
            printf("code for: 0x%x (%c): ", i, i);
            for (int j = (int)dict[i].bit_length - 1; j >= 0; --j) {
                printf("%d", dict[i].qbits[j].bit);
            }
            printf("\n");
        }
    }
#endif

    uint64_t dict_count = 0;
    quin_huffman_head_dict_t head_dict[256];
    for (unsigned int i = 0; i != 256; ++i) {
        if (dict[i].bit_length != 0) {
            head_dict[dict_count].byte = (unsigned char)i;
            head_dict[dict_count].code = dict[i];
            ++dict_count;
        }
    }

    // Write magic head.
    fwrite(&quin_huffman_head_magic, sizeof(quin_huffman_head_magic), 1, out);
    quin_huffman_head_t head = {
        .data_size = (uint64_t)fsize, .dict_count = dict_count, .reserved = {0}};
    // Write head.
    fwrite(&head, sizeof(head), 1, out);
    // Write dict.
    fwrite(head_dict, sizeof(quin_huffman_head_dict_t), dict_count, out);
    // Write dummy list.
    fwrite(&dummy_list, sizeof(dummy_list), 1, out);
    unsigned char byte = 0;
    unsigned short bit_count = 0;
    while (fread(&value, sizeof(value), 1, in) == sizeof(value)) {
#ifdef DEBUG_OUTPUT
        printf("\n0x%x (%c) -> ", value, value);
#endif
        int len = (int)dict[value].bit_length;
        for (int j = len - 1; j >= 0; --j) {
            unsigned char three_bits =
                quinary_bit_to_three_low_bits(dict[value].qbits[j]);
#ifdef DEBUG_OUTPUT
                printf("%d", three_bits);
#endif
            for (int k = 0; k != 3; ++k) {
                byte = set_bit(byte, bit_count, three_bits & 0x01U);
                ++bit_count;
                three_bits >>= 1;

                if (bit_count == 8) {
#ifdef DEBUG_OUTPUT
                    printf(" [written byte: 0x%02x] ", byte);
#endif
                    fwrite(&byte, sizeof(char), 1, out);
                    bit_count = 0;
                    byte = 0;
                }
            }
        }
    }

#ifdef DEBUG_OUTPUT
    printf("\n");
#endif
    // write last byte.
    if (bit_count != 0) {
#ifdef DEBUG_OUTPUT
        printf(" [written last byte: 0x%02x]\n", byte);
#endif
        fwrite(&byte, sizeof(char), 1, out);
        bit_count = 0;
        byte = 0;
    }

    return 0;
}

void quinary_huffman_tree_walk(quin_huffman_node_t *const root,
                               quin_huffman_char_t code,
                               quin_huffman_char_t out[256],
                               quin_huffman_head_dummy_list_t *dummy_list) {
    if (root == NULL)
        return;
    // Dummy node needs not to be in dict.
    if (root->type == NODE_CHAR_QUIN) {
        out[root->c] = code;

    } else if (root->type == NODE_DUMMY_QUIN) {
        dummy_list->codes[dummy_list->count++] = code;
    } else {
        for (int i = code.bit_length; i >= 1; --i) {
            code.qbits[i] = code.qbits[i - 1];
        }
        quin_huffman_char_t left_code = code;
        left_code.bit_length += 1;
        left_code.qbits[0].bit = 0;
        quin_huffman_char_t middle1_code = code;
        middle1_code.bit_length += 1;
        middle1_code.qbits[0].bit = 1;
        quin_huffman_char_t middle2_code = code;
        middle2_code.bit_length += 1;
        middle2_code.qbits[0].bit = 2;
        quin_huffman_char_t middle3_code = code;
        middle3_code.bit_length += 1;
        middle3_code.qbits[0].bit = 3;
        quin_huffman_char_t right_code = code;
        right_code.bit_length += 1;
        right_code.qbits[0].bit = 4;
        quinary_huffman_tree_walk(root->left, left_code, out, dummy_list);
        quinary_huffman_tree_walk(root->middle1, middle1_code, out, dummy_list);
        quinary_huffman_tree_walk(root->middle2, middle2_code, out, dummy_list);
        quinary_huffman_tree_walk(root->middle3, middle3_code, out, dummy_list);
        quinary_huffman_tree_walk(root->right, right_code, out, dummy_list);
    }
}

int quinary_huffman_get_data_size(FILE *in, uint64_t *out) {
    uint32_t magic;
    fread(&magic, sizeof(uint32_t), 1, in);
    if (magic != quin_huffman_head_magic) {
        return -1;
    }
    quin_huffman_head_t head;
    fread(&head, sizeof(quin_huffman_head_t), 1, in);
    *out = head.data_size;
    rewind(in);
    return 0;
}

int quinary_huffman_unpack(FILE *in, FILE *out) {

    uint32_t magic;
    fread(&magic, sizeof(uint32_t), 1, in);
    if (magic != quin_huffman_head_magic) {
        return -1;
    }
    quin_huffman_head_t head;
    fread(&head, sizeof(quin_huffman_head_t), 1, in);
    quin_huffman_head_dict_t *dict =
        malloc(sizeof(quin_huffman_head_dict_t) * head.dict_count);
    fread(dict, sizeof(quin_huffman_head_dict_t), head.dict_count, in);
#ifdef DEBUG_OUTPUT
    for (uint64_t i = 0; i != head.dict_count; ++i) {
        printf("in-file code for: 0x%x (%c): ", dict[i].byte, dict[i].byte);
        for (int j = (int)dict[i].code.bit_length - 1; j >= 0; --j) {
            printf("%d", dict[i].code.qbits[j].bit);
        }
        printf("\n");
    }
#endif
    quin_huffman_head_dummy_list_t dummy_list;
    fread(&dummy_list, sizeof(dummy_list), 1, in);

    // Restore the huffman tree.
    quin_huffman_node_t *root = malloc(sizeof(quin_huffman_node_t));
    quin_huffman_char_t icode = {0, {{0}}};
    quinary_huffman_tree_build(root, icode, dict, head.dict_count, &dummy_list);

#ifdef DEBUG_OUTPUT
    quin_huffman_char_t debug_dict[256] = {0};
    quin_huffman_head_dummy_list_t debug_dummy_list = {0, {{0}}};
    quinary_huffman_tree_walk(root, icode, debug_dict, &debug_dummy_list);
    for (int i = 0; i != 256; ++i) {
        if (debug_dict[i].bit_length != 0) {
            printf("restored code for: 0x%x (%c): ", i, i);
            for (int j = (int)debug_dict[i].bit_length - 1; j >= 0; --j) {
                printf("%d", debug_dict[i].qbits[j].bit);
            }
            printf("\n");
        }
    }
#endif
    uint64_t decoded_byte_count = 0;
    unsigned char byte = 0;
    quin_huffman_node_t *position = root;
    // Walk a step when every three bits are read into tbit_temp.
    unsigned char bit_count = 0;
    unsigned char qbit_temp = 0;
    while (decoded_byte_count != head.data_size) {
        fread(&byte, sizeof(byte), 1, in);
        for (int i = 0; i != 8; ++i, byte >>= 1) {
            // Walk a step.
            qbit_temp = set_bit(qbit_temp, bit_count, byte & 0x01U);
            ++bit_count;
            if (bit_count == 3) {
                quinary_bit_t qbit =
                    three_low_bits_to_quinary(qbit_temp & 0x07U);
                if (qbit.bit == 0) {
                    position = position->left;
                } else if (qbit.bit == 1) {
                    position = position->middle1;
                } else if (qbit.bit == 2) {
                    position = position->middle2;
                } else if (qbit.bit == 3) {
                    position = position->middle3;
                } else {
                    position = position->right;
                }
                // error, free tree and exit.
                if (position == NULL) {
                    quinary_huffman_tree_clean(root);
                    return -1;
                }
                // decode it.
                if (position->type == NODE_CHAR_QUIN) {
                    fwrite(&position->c, sizeof(unsigned char), 1, out);
                    ++decoded_byte_count;
                    position = root;
                    if (decoded_byte_count == head.data_size) {
                        // jump out of multiple loops.
                        goto END_LOOP;
                    }
                }
                // clear bc and temp bit.
                bit_count = 0;
                qbit_temp = 0;
            }
        }
    }
END_LOOP:
    // free tree.
    quinary_huffman_tree_clean(root);
    return 0;
}

void quinary_huffman_tree_build(quin_huffman_node_t *const root,
                                quin_huffman_char_t code,
                                quin_huffman_head_dict_t *dict, uint64_t n,
                                quin_huffman_head_dummy_list_t *dummy_list) {

    for (uint64_t i = 0; i != n; ++i) {
        if (code.bit_length == dict[i].code.bit_length) {
            uint16_t bc = code.bit_length;
            uint16_t j = 0;
            for (j = 0;
                 j != bc && code.qbits[j].bit == dict[i].code.qbits[j].bit; ++j)
                ;
            if (j == bc) {
                root->p = 0;
                root->c = dict[i].byte;
                root->left = NULL;
                root->middle1 = NULL;
                root->middle2 = NULL;
                root->middle3 = NULL;
                root->right = NULL;
                root->type = NODE_CHAR_QUIN;
                return;
            }
        }
    }
    // check dummies.
    for (int i = 0; i != dummy_list->count; ++i) {
        quin_huffman_char_t *dummy = &dummy_list->codes[i];
        if (dummy != NULL && code.bit_length == dummy->bit_length) {
            uint16_t i;
            uint16_t bc = dummy->bit_length;
            for (i = 0; i != bc && code.qbits[i].bit == dummy->qbits[i].bit;
                 ++i)
                ;
            if (i == bc) {
                root->p = 0;
                root->c = 0;
                root->left = NULL;
                root->middle1 = NULL;
                root->middle2 = NULL;
                root->middle3 = NULL;
                root->right = NULL;
                root->type = NODE_DUMMY_QUIN;
                return;
            }
        }
    }
    for (int i = code.bit_length; i >= 1; --i) {
        code.qbits[i] = code.qbits[i - 1];
    }
    quin_huffman_char_t left_code = code;
    left_code.bit_length += 1;
    left_code.qbits[0].bit = 0;
    quin_huffman_char_t middle1_code = code;
    middle1_code.bit_length += 1;
    middle1_code.qbits[0].bit = 1;
    quin_huffman_char_t middle2_code = code;
    middle2_code.bit_length += 1;
    middle2_code.qbits[0].bit = 2;
    quin_huffman_char_t middle3_code = code;
    middle3_code.bit_length += 1;
    middle3_code.qbits[0].bit = 3;
    quin_huffman_char_t right_code = code;
    right_code.bit_length += 1;
    right_code.qbits[0].bit = 4;
    root->left = malloc(sizeof(quin_huffman_node_t));
    root->middle1 = malloc(sizeof(quin_huffman_node_t));
    root->middle2 = malloc(sizeof(quin_huffman_node_t));
    root->middle3 = malloc(sizeof(quin_huffman_node_t));
    root->right = malloc(sizeof(quin_huffman_node_t));
    root->type = NODE_REDUCED_QUIN;
    quinary_huffman_tree_build(root->left, left_code, dict, n, dummy_list);
    quinary_huffman_tree_build(root->middle1, middle1_code, dict, n,
                               dummy_list);
    quinary_huffman_tree_build(root->middle2, middle2_code, dict, n,
                               dummy_list);
    quinary_huffman_tree_build(root->middle3, middle3_code, dict, n,
                               dummy_list);
    quinary_huffman_tree_build(root->right, right_code, dict, n, dummy_list);
}

void quinary_huffman_tree_clean(quin_huffman_node_t *const root) {
    if (root != NULL) {
        quinary_huffman_tree_clean(root->left);
        quinary_huffman_tree_clean(root->middle1);
        quinary_huffman_tree_clean(root->middle2);
        quinary_huffman_tree_clean(root->middle3);
        quinary_huffman_tree_clean(root->right);
        free(root);
    }
}

unsigned char quinary_bit_to_three_low_bits(quinary_bit_t tbit) {
    if (tbit.bit <= 4) {
        return tbit.bit;
    } else {
        return 0xff;
    }
}

quinary_bit_t three_low_bits_to_quinary(unsigned char bits) {
    quinary_bit_t result;
    switch (bits & 0x07U) {
    case 0:
        result.bit = 0;
        return result;
    case 1:
        result.bit = 1;
        return result;
    case 2:
        result.bit = 2;
        return result;
    case 3:
        result.bit = 3;
        return result;
    case 4:
        result.bit = 4;
        return result;
    default:
        result.bit = 4;
        return result;
    }
}
