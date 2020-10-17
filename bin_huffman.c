#include "bin_huffman.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tools.h"
//#define DEBUG_OUTPUT
int node_cmp(const void *node_1, const void *node_2) {
    bin_huffman_node_t *const *node_1_ptr = node_1;
    bin_huffman_node_t *const *node_2_ptr = node_2;
    if ((*node_1_ptr)->p > (*node_2_ptr)->p) {
        return -1;
    }
    if ((*node_1_ptr)->p < (*node_2_ptr)->p) {
        return 1;
    }
    return 0;
}

int binary_huffman_pack(FILE *in, FILE *out) {
    size_t cnt[256] = {0};
    unsigned char value;
    while (fread(&value, sizeof(value), 1, in) == sizeof(value)) {
        ++cnt[value];
    }
    long fsize = ftell(in);
    rewind(in);

    bin_huffman_node_t *huffman_nodes[256] = {0};
    size_t node_count = 0;
    for (int i = 0; i != 256; ++i) {
        if (cnt[i]) {
            huffman_nodes[node_count] = malloc(sizeof(bin_huffman_node_t));
            huffman_nodes[node_count]->c = (unsigned char)i;
            huffman_nodes[node_count]->p = cnt[i];
            huffman_nodes[node_count]->type = NODE_CHAR;
            huffman_nodes[node_count]->left = NULL;
            huffman_nodes[node_count]->right = NULL;

            ++node_count;
        }
    }
    qsort(huffman_nodes, node_count, sizeof(bin_huffman_node_t *), node_cmp);
    while (node_count != 1) {
        // reduce two nodes.
        bin_huffman_node_t *node1 = huffman_nodes[--node_count];
        bin_huffman_node_t *node2 = huffman_nodes[--node_count];
        bin_huffman_node_t *reduced_node = malloc(sizeof(bin_huffman_node_t));
        reduced_node->type = NODE_REDUCED;
        reduced_node->left = node1;
        reduced_node->right = node2;
        reduced_node->p = node1->p + node2->p;
        huffman_nodes[node_count++] = reduced_node;
        // sort again.
        qsort(huffman_nodes, node_count, sizeof(bin_huffman_node_t *),
              node_cmp);
    }
    bin_huffman_char_t dict[256] = {0};
    bin_huffman_char_t init_code = {0, {0}};
    binary_huffman_tree_walk(huffman_nodes[0], init_code, dict);
    binary_huffman_tree_clean(huffman_nodes[0]);
#ifdef DEBUG_OUTPUT
    for (int i = 0; i != 256; ++i) {
        if (dict[i].bit_length != 0) {
            printf("code for: 0x%x (%c): ", i, i);
            for (int j = (int)dict[i].bit_length - 1; j >= 0; --j) {
                printf(
                    "%c",
                    dict[i].bits[(unsigned int)j / (8U * sizeof(u_int64_t))] &
                            (0x01U
                             << ((unsigned int)j % (8U * sizeof(u_int64_t))))
                        ? '1'
                        : '0');
            }
            printf("\n");
        }
    }
#endif

    size_t dict_count = 0;
    bin_huffman_head_dict_t head_dict[256];
    for (unsigned int i = 0; i != 256; ++i) {
        if (dict[i].bit_length != 0) {
            head_dict[dict_count].byte = (unsigned char)i;
            head_dict[dict_count].code = dict[i];
            ++dict_count;
        }
    }

    // Write magic head.
    fwrite(&bin_huffman_head_magic, sizeof(bin_huffman_head_magic), 1, out);
    bin_huffman_head_t head = {
        .data_size = (size_t)fsize, .dict_count = dict_count, .reserved = {0}};
    // Write head.
    fwrite(&head, sizeof(head), 1, out);
    // Write dict.
    fwrite(head_dict, sizeof(bin_huffman_head_dict_t), dict_count, out);

    unsigned char byte = 0;
    unsigned short bit_count = 0;
    while (fread(&value, sizeof(value), 1, in) == sizeof(value)) {
#ifdef DEBUG_OUTPUT
        printf("\n0x%x (%c) -> ", value, value);
#endif
        int len = (int)dict[value].bit_length;
        for (int j = len - 1; j >= 0; --j) {
            unsigned int bit =
                dict[value].bits[(unsigned int)j / (8U * sizeof(u_int64_t))] &
                (0x01U << ((unsigned int)j % (8U * sizeof(u_int64_t))));
            byte = set_bit(byte, bit_count, bit);
            ++bit_count;

#ifdef DEBUG_OUTPUT
            printf("%c", bit ? '1' : '0');
#endif
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

void binary_huffman_tree_walk(bin_huffman_node_t *const root,
                              bin_huffman_char_t code,
                              bin_huffman_char_t out[256]) {
    if (root == NULL)
        return;
    if (root->type == NODE_CHAR) {
        out[root->c] = code;
    } else {
        u64_lshift1(code.bits, 4);
        bin_huffman_char_t left_code = code;
        left_code.bit_length += 1;
        bin_huffman_char_t right_code = code;
        right_code.bit_length += 1;
        right_code.bits[0] |= 0x01U;
        binary_huffman_tree_walk(root->left, left_code, out);
        binary_huffman_tree_walk(root->right, right_code, out);
    }
}

int binary_huffman_get_data_size(FILE *in, size_t *out) {
    u_int32_t magic;
    fread(&magic, sizeof(u_int32_t), 1, in);
    if (magic != bin_huffman_head_magic) {
        return -1;
    }
    bin_huffman_head_t head;
    fread(&head, sizeof(bin_huffman_head_t), 1, in);
    *out = head.data_size;
    rewind(in);
    return 0;
}

int binary_huffman_unpack(FILE *in, FILE *out) {

    u_int32_t magic;
    fread(&magic, sizeof(u_int32_t), 1, in);
    if (magic != bin_huffman_head_magic) {
        return -1;
    }
    bin_huffman_head_t head;
    fread(&head, sizeof(bin_huffman_head_t), 1, in);
    bin_huffman_head_dict_t *dict =
        malloc(sizeof(bin_huffman_head_dict_t) * head.dict_count);
    fread(dict, sizeof(bin_huffman_head_dict_t), head.dict_count, in);

    // Restore the huffman tree.
    bin_huffman_node_t *root = malloc(sizeof(bin_huffman_node_t));
    bin_huffman_char_t icode = {0, {0}};
    binary_huffman_tree_build(root, icode, dict, head.dict_count);

#ifdef DEBUG_OUTPUT
    bin_huffman_char_t debug_dict[256] = {0};
    binary_huffman_tree_walk(root, icode, debug_dict);
    for (int i = 0; i != 256; ++i) {
        if (debug_dict[i].bit_length != 0) {
            printf("restored code for: 0x%x (%c): ", i, i);
            for (int j = (int)debug_dict[i].bit_length - 1; j >= 0; --j) {
                printf("%c", debug_dict[i].bits[(unsigned int)j /
                                                (8U * sizeof(u_int64_t))] &
                                     (0x01U << ((unsigned int)j %
                                                (8U * sizeof(u_int64_t))))
                                 ? '1'
                                 : '0');
            }
            printf("\n");
        }
    }
#endif
    size_t decoded_byte_count = 0;
    unsigned char byte = 0;
    bin_huffman_node_t *position = root;
    while (decoded_byte_count != head.data_size) {
        fread(&byte, sizeof(byte), 1, in);
        for (int i = 0; i != 8; ++i, byte >>= 1) {
            // Walk a step.
            if (byte & 0x01U) {
                position = position->right;
            } else {
                position = position->left;
            }
            // error, free tree and exit.
            if (position == NULL) {
                binary_huffman_tree_clean(root);
                return -1;
            }
            // decode it.
            if (position->type == NODE_CHAR) {
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
    binary_huffman_tree_clean(root);
    return 0;
}

void binary_huffman_tree_build(bin_huffman_node_t *const root,
                               bin_huffman_char_t code,
                               bin_huffman_head_dict_t *dict, size_t n) {
    for (size_t i = 0; i != n; ++i) {
        if (code.bit_length == dict[i].code.bit_length &&
            code.bits[0] == dict[i].code.bits[0] &&
            code.bits[1] == dict[i].code.bits[1] &&
            code.bits[2] == dict[i].code.bits[2] &&
            code.bits[3] == dict[i].code.bits[3]) {
            root->p = 0;
            root->c = dict[i].byte;
            root->left = NULL;
            root->right = NULL;
            root->type = NODE_CHAR;
            return;
        }
    }

    u64_lshift1(code.bits, 4);
    bin_huffman_char_t left_code = code;
    left_code.bit_length += 1;
    bin_huffman_char_t right_code = code;
    right_code.bit_length += 1;
    right_code.bits[0] |= 0x01U;

    root->left = malloc(sizeof(bin_huffman_node_t));
    root->right = malloc(sizeof(bin_huffman_node_t));
    root->type = NODE_REDUCED;
    binary_huffman_tree_build(root->left, left_code, dict, n);
    binary_huffman_tree_build(root->right, right_code, dict, n);
}

void binary_huffman_tree_clean(bin_huffman_node_t *const root) {
    if (root != NULL) {
        binary_huffman_tree_clean(root->left);
        binary_huffman_tree_clean(root->right);
        free(root);
    }
}
