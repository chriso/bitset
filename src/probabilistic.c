#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "bitset/probabilistic.h"

bitset_linear *bitset_linear_new(size_t size) {
    bitset_linear *e = (bitset_linear *) malloc(sizeof(bitset_linear));
    if (!e) {
        bitset_oom();
    }
    e->count = 0;
    size = (size_t)(size / BITSET_LITERAL_LENGTH) + 1;
    e->size = size;
    e->words = (bitset_word *) calloc(1, e->size * sizeof(bitset_word));
    if (!e->words) {
        bitset_oom();
    }
    return e;
};

void bitset_linear_add(bitset_linear *e, bitset *b) {
    bitset_offset offset = 0;
    bitset_word word = 0, mask;
    unsigned position;
    for (unsigned i = 0; i < b->length; i++) {
        word = b->words[i];
        if (BITSET_IS_FILL_WORD(word)) {
            offset += BITSET_GET_LENGTH(word);
            if (offset >= e->size) {
                offset %= e->size;
            }
            position = BITSET_GET_POSITION(word);
            if (!position) {
                continue;
            }
            mask = BITSET_CREATE_LITERAL(position - 1);
            if ((e->words[offset] & mask) == 0) {
                e->count++;
                e->words[offset] |= mask;
            }
        } else {
            for (unsigned i = BITSET_LITERAL_LENGTH - 1; i; i--) {
                mask = 1 << i;
                if (word & mask && (e->words[offset] & mask) == 0) {
                    e->count++;
                    e->words[offset] |= mask;
                }
            }
        }
        offset++;
        if (offset >= e->size) {
            offset -= e->size;
        }
    }
};

unsigned bitset_linear_count(bitset_linear *e) {
    return e->count;
};

void bitset_linear_free(bitset_linear *e) {
    free(e->words);
    free(e);
};

