#include "tern_huffman.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tools.h"
#define DEBUG_OUTPUT
int ternary_node_cmp(const void *node_1, const void *node_2) {
    tern_huffman_node_t *const *node_1_ptr = node_1;
    tern_huffman_node_t *const *node_2_ptr = node_2;
    if ((*node_1_ptr)->p > (*node_2_ptr)->p) {
        return -1;
    }
    if ((*node_1_ptr)->p < (*node_2_ptr)->p) {
        return 1;
    }
    return 0;
}

int ternary_huffman_pack(FILE *in, FILE *out) {
    size_t cnt[256] = {0};
    unsigned char value;
    while (fread(&value, sizeof(value), 1, in) == sizeof(value)) {
        ++cnt[value];
    }
    long fsize = ftell(in);
    rewind(in);

    // Trinary huffman needs odd node counts.
    // If there will be 256 nodes,
    // a dummy node should be added.
    tern_huffman_node_t *huffman_nodes[257] = {0};
    size_t node_count = 0;
    for (int i = 0; i != 256; ++i) {
        if (cnt[i]) {
            huffman_nodes[node_count] = malloc(sizeof(tern_huffman_node_t));
            huffman_nodes[node_count]->c = (unsigned char)i;
            huffman_nodes[node_count]->p = cnt[i];
            huffman_nodes[node_count]->type = NODE_CHAR_TERN;
            huffman_nodes[node_count]->left = NULL;
            huffman_nodes[node_count]->middle = NULL;
            huffman_nodes[node_count]->right = NULL;

            ++node_count;
        }
    }
    char has_dummy = node_count % 2 != 1;
    // For even node count.
    if (has_dummy) {
        // Add a dummy node.
        huffman_nodes[node_count] = malloc(sizeof(tern_huffman_node_t));
        huffman_nodes[node_count]->c = 0x00U;
        huffman_nodes[node_count]->p = 0;
        huffman_nodes[node_count]->type = NODE_DUMMY_TERN;
        huffman_nodes[node_count]->left = NULL;
        huffman_nodes[node_count]->middle = NULL;
        huffman_nodes[node_count]->right = NULL;

        ++node_count;
    }
    qsort(huffman_nodes, node_count, sizeof(tern_huffman_node_t *),
          ternary_node_cmp);
    while (node_count != 1) {
        // reduce three nodes.
        tern_huffman_node_t *node1 = huffman_nodes[--node_count];
        tern_huffman_node_t *node2 = huffman_nodes[--node_count];
        tern_huffman_node_t *node3 = huffman_nodes[--node_count];
        tern_huffman_node_t *reduced_node = malloc(sizeof(tern_huffman_node_t));
        reduced_node->type = NODE_REDUCED_TERN;
        reduced_node->left = node1;
        reduced_node->middle = node2;
        reduced_node->right = node3;
        reduced_node->p = node1->p + node2->p + node3->p;
        huffman_nodes[node_count++] = reduced_node;
        // sort again.
        qsort(huffman_nodes, node_count, sizeof(tern_huffman_node_t *),
              ternary_node_cmp);
    }

    // We don't need dummy node in dict (never appeared).
    tern_huffman_char_t dict[256] = {0};
    tern_huffman_char_t init_code = {0, {{0}}};
    tern_huffman_char_t dummy_code;

    // Save dummy code. We need all dummy codes to rebuild the tree.
    // Or the tree will generate forever and raise SIGSEGV.
    ternary_huffman_tree_walk(huffman_nodes[0], init_code, dict, &dummy_code);
    ternary_huffman_tree_clean(huffman_nodes[0]);
#ifdef DEBUG_OUTPUT
    for (int i = 0; i != 256; ++i) {
        if (dict[i].bit_length != 0) {
            printf("code for: 0x%x (%c): ", i, i);
            for (int j = (int)dict[i].bit_length - 1; j >= 0; --j) {
                printf("%d", dict[i].tbits[j].bit);
            }
            printf("\n");
        }
    }
#endif

    size_t dict_count = 0;
    tern_huffman_head_dict_t head_dict[256];
    for (unsigned int i = 0; i != 256; ++i) {
        if (dict[i].bit_length != 0) {
            head_dict[dict_count].byte = (unsigned char)i;
            head_dict[dict_count].code = dict[i];
            ++dict_count;
        }
    }

    // Write magic head.
    fwrite(&tern_huffman_head_magic, sizeof(tern_huffman_head_magic), 1, out);
    tern_huffman_head_t head = {.data_size = (size_t)fsize,
                                .dict_count = dict_count,
                                .has_dummy = has_dummy,
                                .reserved = {0}};
    // Write head.
    fwrite(&head, sizeof(head), 1, out);
    // Write dict.
    fwrite(head_dict, sizeof(tern_huffman_head_dict_t), dict_count, out);
    if (has_dummy) {
        fwrite(&dummy_code, sizeof(dummy_code), 1, out);
    }
    unsigned char byte = 0;
    unsigned short bit_count = 0;
    while (fread(&value, sizeof(value), 1, in) == sizeof(value)) {
#ifdef DEBUG_OUTPUT_D
        printf("\n0x%x (%c) -> ", value, value);
#endif
        int len = (int)dict[value].bit_length;
        for (int j = len - 1; j >= 0; --j) {
            unsigned char two_bits =
                ternary_bit_to_two_low_bits(dict[value].tbits[j]);
            byte = set_bit(byte, bit_count, two_bits & 0x01U);
            ++bit_count;
            two_bits >>= 1;
            byte = set_bit(byte, bit_count, two_bits & 0x01U);
            ++bit_count;

#ifdef DEBUG_OUTPUT_D
            printf("%d", two_bits);
#endif
            if (bit_count == 8) {
#ifdef DEBUG_OUTPUT_D
                printf(" [written byte: 0x%02x] ", byte);
#endif
                fwrite(&byte, sizeof(char), 1, out);
                bit_count = 0;
                byte = 0;
            }
        }
    }

#ifdef DEBUG_OUTPUT_D
    printf("\n");
#endif
    // write last byte.
    if (bit_count != 0) {
#ifdef DEBUG_OUTPUT_D
        printf(" [written last byte: 0x%02x]\n", byte);
#endif
        fwrite(&byte, sizeof(char), 1, out);
        bit_count = 0;
        byte = 0;
    }

    return 0;
}

void ternary_huffman_tree_walk(tern_huffman_node_t *const root,
                               tern_huffman_char_t code,
                               tern_huffman_char_t out[256],
                               tern_huffman_char_t *dummy_code) {
    if (root == NULL)
        return;
    // Dummy node needs not to be in dict.
    if (root->type == NODE_CHAR_TERN) {
        out[root->c] = code;

    } else if (root->type == NODE_DUMMY_TERN) {
        *dummy_code = code;
    } else {
        for (int i = code.bit_length; i >= 1; --i) {
            code.tbits[i] = code.tbits[i - 1];
        }
        tern_huffman_char_t left_code = code;
        left_code.bit_length += 1;
        left_code.tbits[0].bit = 0;
        tern_huffman_char_t middle_code = code;
        middle_code.bit_length += 1;
        middle_code.tbits[0].bit = 1;
        tern_huffman_char_t right_code = code;
        right_code.bit_length += 1;
        right_code.tbits[0].bit = 2;
        ternary_huffman_tree_walk(root->left, left_code, out, dummy_code);
        ternary_huffman_tree_walk(root->middle, middle_code, out, dummy_code);
        ternary_huffman_tree_walk(root->right, right_code, out, dummy_code);
    }
}

int ternary_huffman_get_data_size(FILE *in, size_t *out) {
    u_int32_t magic;
    fread(&magic, sizeof(u_int32_t), 1, in);
    if (magic != tern_huffman_head_magic) {
        return -1;
    }
    tern_huffman_head_t head;
    fread(&head, sizeof(tern_huffman_head_t), 1, in);
    *out = head.data_size;
    rewind(in);
    return 0;
}

int ternary_huffman_unpack(FILE *in, FILE *out) {

    u_int32_t magic;
    fread(&magic, sizeof(u_int32_t), 1, in);
    if (magic != tern_huffman_head_magic) {
        return -1;
    }
    tern_huffman_head_t head;
    fread(&head, sizeof(tern_huffman_head_t), 1, in);
    tern_huffman_head_dict_t *dict =
        malloc(sizeof(tern_huffman_head_dict_t) * head.dict_count);
    fread(dict, sizeof(tern_huffman_head_dict_t), head.dict_count, in);
#ifdef DEBUG_OUTPUT
    for (size_t i = 0; i != head.dict_count; ++i) {
            printf("in-file code for: 0x%x (%c): ", dict[i].byte, dict[i].byte);
            for (int j = (int)dict[i].code.bit_length - 1; j >= 0; --j) {
                printf("%d", dict[i].code.tbits[j].bit);
            }
            printf("\n");
    }
#endif
    tern_huffman_char_t dummy;
    if (head.has_dummy) {
        fread(&dummy, sizeof (dummy), 1, in);
    }
    // Restore the huffman tree.
    tern_huffman_node_t *root = malloc(sizeof(tern_huffman_node_t));
    tern_huffman_char_t icode = {0, {{0}}};
    if (head.has_dummy) {
    ternary_huffman_tree_build(root, icode, dict, head.dict_count, &dummy);
    } else {
    ternary_huffman_tree_build(root, icode, dict, head.dict_count, NULL);
    }

#ifdef DEBUG_OUTPUT
    tern_huffman_char_t debug_dict[256] = {0};
    tern_huffman_char_t debug_dummy = {0};
    ternary_huffman_tree_walk(root, icode, debug_dict, &debug_dummy);
    for (int i = 0; i != 256; ++i) {
        if (debug_dict[i].bit_length != 0) {
            printf("restored code for: 0x%x (%c): ", i, i);
            for (int j = (int)debug_dict[i].bit_length - 1; j >= 0; --j) {
                printf("%d", debug_dict[i].tbits[j].bit);
            }
            printf("\n");
        }
    }
#endif
    size_t decoded_byte_count = 0;
    unsigned char byte = 0;
    tern_huffman_node_t *position = root;
    while (decoded_byte_count != head.data_size) {
        fread(&byte, sizeof(byte), 1, in);
        for (int i = 0; i != 4; ++i, byte >>= 2) {
            // Walk a step.
            ternary_bit_t tbit = two_low_bits_to_ternary(byte & 0x03U);
            if (tbit.bit == 0) {
                position = position->left;
            } else if (tbit.bit == 1) {
                position = position->middle;
            } else {
                position = position->right;
            }
            // error, free tree and exit.
            if (position == NULL) {
                ternary_huffman_tree_clean(root);
                return -1;
            }
            // decode it.
            if (position->type == NODE_CHAR_TERN) {
                fwrite(&position->c, sizeof(unsigned char), 1, out);
                ++decoded_byte_count;
                position = root;
                if (decoded_byte_count == head.data_size) {
                    // jump out of multiple loops.
                    goto END_LOOP;
                }
            }
        }
    }
END_LOOP:
    // free tree.
    ternary_huffman_tree_clean(root);
    return 0;
}

void ternary_huffman_tree_build(tern_huffman_node_t *const root,
                                tern_huffman_char_t code,
                                tern_huffman_head_dict_t *dict, size_t n,
                                tern_huffman_char_t *dummy) {

    for (size_t i = 0; i != n; ++i) {
        if (code.bit_length == dict[i].code.bit_length) {
            u_int16_t bc = code.bit_length;
            u_int16_t j = 0;
            for (j = 0;
                 j != bc && code.tbits[j].bit == dict[i].code.tbits[j].bit; ++j)
                ;
            if (j == bc) {
                root->p = 0;
                root->c = dict[i].byte;
                root->left = NULL;
                root->middle = NULL;
                root->right = NULL;
                root->type = NODE_CHAR_TERN;
                return;
            }
        }
    }
    if (dummy != NULL && code.bit_length == dummy->bit_length) {
        u_int16_t i;
        u_int16_t bc = dummy->bit_length;
        for (i = 0; i != bc && code.tbits[i].bit == dummy->tbits[i].bit; ++i)
            ;
        if (i == bc) {
            root->p = 0;
            root->c = 0;
            root->left = NULL;
            root->middle = NULL;
            root->right = NULL;
            root->type = NODE_DUMMY_TERN;
            return;
        }
    }
    for (int i = code.bit_length; i >= 1; --i) {
        code.tbits[i] = code.tbits[i - 1];
    }
    tern_huffman_char_t left_code = code;
    left_code.bit_length += 1;
    left_code.tbits[0].bit = 0;
    tern_huffman_char_t middle_code = code;
    middle_code.bit_length += 1;
    middle_code.tbits[0].bit = 1;
    tern_huffman_char_t right_code = code;
    right_code.bit_length += 1;
    right_code.tbits[0].bit = 2;

    root->left = malloc(sizeof(tern_huffman_node_t));
    root->middle = malloc(sizeof(tern_huffman_node_t));
    root->right = malloc(sizeof(tern_huffman_node_t));
    root->type = NODE_REDUCED_TERN;
    ternary_huffman_tree_build(root->left, left_code, dict, n, dummy);
    ternary_huffman_tree_build(root->middle, middle_code, dict, n, dummy);
    ternary_huffman_tree_build(root->right, right_code, dict, n, dummy);
}

void ternary_huffman_tree_clean(tern_huffman_node_t *const root) {
    if (root != NULL) {
        ternary_huffman_tree_clean(root->left);
        ternary_huffman_tree_clean(root->middle);
        ternary_huffman_tree_clean(root->right);
        free(root);
    }
}

unsigned char ternary_bit_to_two_low_bits(ternary_bit_t tbit) {
    if (tbit.bit <= 2) {
        return tbit.bit;
    } else {
        return 0xff;
    }
}

ternary_bit_t two_low_bits_to_ternary(unsigned char bits) {
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
