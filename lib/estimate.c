#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "bitset/estimate.h"

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
}

void bitset_linear_add(bitset_linear *e, bitset *b) {
    bitset_offset offset = 0;
    bitset_word word, mask, tmp;
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
            word &= ~e->words[offset];
            tmp = word;
            BITSET_POP_COUNT(e->count, tmp);
            e->words[offset] |= word;
        }
        offset++;
        if (offset >= e->size) {
            offset -= e->size;
        }
    }
}

unsigned bitset_linear_count(bitset_linear *e) {
    return e->count;
}

void bitset_linear_free(bitset_linear *e) {
    free(e->words);
    free(e);
}

bitset_countn *bitset_countn_new(unsigned n, size_t size) {
    bitset_countn *e = (bitset_countn *) malloc(sizeof(bitset_countn));
    if (!e) {
        bitset_oom();
    }
    e->n = n;
    size = (size_t)(size / BITSET_LITERAL_LENGTH) + 1;
    e->size = size;
    e->words = (bitset_word **) malloc(sizeof(bitset_word *) * (e->n + 1));
    if (!e->words) {
        bitset_oom();
    }
    //Create N+1 uncompressed bitsets
    for (unsigned i = 0; i <= n; i++) {
        e->words[i] = (bitset_word *) calloc(1, e->size * sizeof(bitset_word));
        if (!e->words[i]) {
            bitset_oom();
        }
    }
    return e;
}

void bitset_countn_add(bitset_countn *e, bitset *b) {
    bitset_offset offset = 0;
    bitset_word word, tmp;
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
            word = BITSET_CREATE_LITERAL(position - 1);
        }
        for (unsigned n = 0; n <= e->n; n++) {
            tmp = word & e->words[n][offset];
            e->words[n][offset] |= word;
            word = tmp;
        }
        offset++;
        if (offset >= e->size) {
            offset -= e->size;
        }
    }
}

unsigned bitset_countn_count(bitset_countn *e) {
    unsigned count = 0, nth = e->n - 1, last = e->n;
    bitset_word word;
    //Find bits that occur in the Nth bitset, but not the N+1th bitset
    for (unsigned offset = 0; offset < e->size; offset++) {
        word = e->words[nth][offset] & ~e->words[last][offset];
        if (word) {
            BITSET_POP_COUNT(count, word);
        }
    }
    return count;
}

unsigned *bitset_countn_count_all(bitset_countn *e) {
    unsigned *counts = (unsigned *) calloc(1, sizeof(unsigned) * e->n);
    if (!counts) {
        bitset_oom();
    }
    bitset_word word;
    for (unsigned offset = 0; offset < e->size; offset++) {
        for (unsigned n = 1; n <= e->n; n++) {
            word = e->words[n-1][offset] & ~e->words[n][offset];
            if (word) {
                BITSET_POP_COUNT(counts[n-1], word);
            }
        }
    }
    return counts;
}

unsigned *bitset_countn_count_mask(bitset_countn *e, bitset *mask) {
    bitset_word *mask_words = (bitset_word *) calloc(1, e->size * sizeof(bitset_word));
    if (!mask_words) {
        bitset_oom();
    }
    bitset_offset offset = 0;
    bitset_word word, mask_word;
    unsigned position;

    for (unsigned i = 0; i < mask->length; i++) {
        mask_word = mask->words[i];
        if (BITSET_IS_FILL_WORD(mask_word)) {
            offset += BITSET_GET_LENGTH(mask_word);
            if (offset >= e->size) {
                offset %= e->size;
            }
            position = BITSET_GET_POSITION(mask_word);
            if (!position) {
                continue;
            }
            mask_word = BITSET_CREATE_LITERAL(position - 1);
        }
        for (unsigned n = 1; n <= e->n; n++) {
            mask_words[offset] |= mask_word;
        }
        offset++;
        if (offset >= e->size) {
            offset -= e->size;
        }
    }

    unsigned *counts = (unsigned *) calloc(1, sizeof(unsigned) * e->n);
    if (!counts) {
        bitset_oom();
    }
    for (unsigned offset = 0; offset < e->size; offset++) {
        for (unsigned n = 1; n <= e->n; n++) {
            word = e->words[n-1][offset] & ~e->words[n][offset];
            word &= mask_words[offset];
            if (word) {
                BITSET_POP_COUNT(counts[n-1], word);
            }
        }
    }
    return counts;
}

void bitset_countn_free(bitset_countn *e) {
    for (unsigned i = 0; i <= e->n; i++) {
        free(e->words[i]);
    }
    free(e->words);
    free(e);
}

