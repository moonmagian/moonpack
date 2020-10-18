TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        bin_huffman.c \
        main.c \
        quin_huffman.c \
        tern_huffman.c \
        tools.c

HEADERS += \
    bin_huffman.h \
    quin_huffman.h \
    tern_huffman.h \
    tools.h
