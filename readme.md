# moonpack

A simple huffman compressing and decompressing program supporting
binary, ternary and quinary huffman encoding.

## Build

moonpack uses qmake to build.

```shell
mkdir build && cd build && qmake .. && make
```

## Usage

Use `moonpack -h` to see help below.

```shell
moonpack - A simple huffman based compressing program.
Usage: moonpack [-xbtq] -i infile -o outfile
	-i infile: input file.
	-o outfile: output file.
	-x: Decompress instead of compressing.
	-b: Use binary huffman encoding (default).
	-t: Use ternary huffman encoding.
	-q: Use quinary huffman encoding.
	-h: Display this help message.
```

For example, if we want to compress infile, decompress it and test the correctness.

```shell
moonpack -i infile -o outfile && ./moonpack -x -i outfile -o infile.restored && md5sum infile infile.restore
```