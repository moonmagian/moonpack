TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        src/bin_huffman.c \
        src/main.c \
        src/quin_huffman.c \
        src/tern_huffman.c \
        src/tools.c

HEADERS += \
    src/bin_huffman.h \
    src/quin_huffman.h \
    src/tern_huffman.h \
    src/tools.h
