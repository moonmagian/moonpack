#include <stdio.h>
#include <stdlib.h>
#include "bin_huffman.h"
int main() {
    FILE* in = fopen("infile", "rb");
    FILE* f = fopen("test.mpk", "wb");
    FILE* out = fopen("outfile", "wb");
    binary_huffman_pack(in, f);
    f = freopen("test.mpk", "rb", f);
    binary_huffman_unpack(f, out);
    fclose(in);
    fclose(f);
    fclose(out);
    system("md5sum infile outfile");
    return 0;
}
