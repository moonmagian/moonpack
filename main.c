#include <stdio.h>
#include <stdlib.h>
#include "bin_huffman.h"
#include "tern_huffman.h"
#include "quin_huffman.h"
int main() {
    FILE* in = fopen("infile", "rb");
    FILE* f = fopen("test.mpk", "wb");
    FILE* out = fopen("outfile", "wb");
    quinary_huffman_pack(in, f);
    f = freopen("test.mpk", "rb", f);
    quinary_huffman_unpack(f, out);
    fclose(in);
    fclose(f);
    fclose(out);
    system("md5sum infile outfile");
    return 0;
}
