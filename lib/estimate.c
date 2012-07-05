#include "bitset/estimate.h"

bitset_linear_t *bitset_linear_new(size_t size) {
    bitset_linear_t *counter = malloc(sizeof(bitset_linear_t));
    if (!counter) {
        bitset_oom();
    }
    counter->count = 0;
    size = (size_t)(size / BITSET_LITERAL_LENGTH) + 1;
    size_t pow2;
    BITSET_NEXT_POW2(pow2, size);
    counter->size = pow2;
    counter->words = calloc(1, counter->size * sizeof(bitset_word));
    if (!counter->words) {
        bitset_oom();
    }
    return counter;
}

void bitset_linear_add(bitset_linear_t *counter, bitset_t *bitset) {
    bitset_offset offset = 0;
    bitset_word word, mask;
    unsigned position;
    unsigned offset_mask = counter->size - 1;
    for (size_t i = 0; i < bitset->length; i++) {
        word = bitset->buffer[i];
        if (BITSET_IS_FILL_WORD(word)) {
            offset += BITSET_GET_LENGTH(word);
            position = BITSET_GET_POSITION(word);
            if (!position) {
                continue;
            }
            mask = BITSET_CREATE_LITERAL(position - 1);
            if ((counter->words[offset & offset_mask] & mask) == 0) {
                counter->count++;
                counter->words[offset & offset_mask] |= mask;
            }
        } else {
            counter->words[offset & offset_mask] |= word;
            word &= ~counter->words[offset & offset_mask];
            BITSET_POP_COUNT(counter->count, word);
        }
        offset++;
    }
}

unsigned bitset_linear_count(bitset_linear_t *counter) {
    return counter->count;
}

void bitset_linear_free(bitset_linear_t *counter) {
    free(counter->words);
    free(counter);
}

bitset_countn_t *bitset_countn_new(unsigned n, size_t size) {
    bitset_countn_t *counter = malloc(sizeof(bitset_countn_t));
    if (!counter) {
        bitset_oom();
    }
    counter->n = n;
    size = (size_t)(size / BITSET_LITERAL_LENGTH) + 1;
    size_t pow2;
    BITSET_NEXT_POW2(pow2, size);
    counter->size = pow2;
    counter->words = malloc(sizeof(bitset_word *) * (counter->n + 1));
    if (!counter->words) {
        bitset_oom();
    }
    //Create N+1 uncompressed bitsets
    for (size_t i = 0; i <= n; i++) {
        counter->words[i] = calloc(1, counter->size * sizeof(bitset_word));
        if (!counter->words[i]) {
            bitset_oom();
        }
    }
    return counter;
}

void bitset_countn_add(bitset_countn_t *counter, bitset_t *b) {
    bitset_offset offset = 0;
    bitset_word word, tmp;
    unsigned position;
    unsigned offset_mask = counter->size - 1;
    for (size_t i = 0; i < b->length; i++) {
        word = b->buffer[i];
        if (BITSET_IS_FILL_WORD(word)) {
            offset += BITSET_GET_LENGTH(word);
            position = BITSET_GET_POSITION(word);
            if (!position) {
                continue;
            }
            word = BITSET_CREATE_LITERAL(position - 1);
        }
        for (unsigned n = 0; n <= counter->n; n++) {
            tmp = word & counter->words[n][offset & offset_mask];
            counter->words[n][offset & offset_mask] |= word;
            word = tmp;
        }
        offset++;
    }
}

unsigned bitset_countn_count(bitset_countn_t *counter) {
    unsigned count = 0, nth = counter->n - 1, last = counter->n;
    bitset_word word;
    //Find bits that occur in the Nth bitset, but not the N+1th bitset
    for (size_t offset = 0; offset < counter->size; offset++) {
        word = counter->words[nth][offset] & ~counter->words[last][offset];
        if (word) {
            BITSET_POP_COUNT(count, word);
        }
    }
    return count;
}

unsigned *bitset_countn_count_all(bitset_countn_t *counter) {
    unsigned *counts = calloc(1, sizeof(unsigned) * counter->n);
    if (!counts) {
        bitset_oom();
    }
    bitset_word word;
    for (size_t offset = 0; offset < counter->size; offset++) {
        for (unsigned n = 1; n <= counter->n; n++) {
            word = counter->words[n-1][offset] & ~counter->words[n][offset];
            if (word) {
                BITSET_POP_COUNT(counts[n-1], word);
            }
        }
    }
    return counts;
}

unsigned *bitset_countn_count_mask(bitset_countn_t *counter, bitset_t *mask) {
    bitset_word *mask_words = calloc(1, counter->size * sizeof(bitset_word));
    if (!mask_words) {
        bitset_oom();
    }
    bitset_offset offset = 0;
    bitset_word word, mask_word;
    unsigned position;
    for (size_t i = 0; i < mask->length; i++) {
        mask_word = mask->buffer[i];
        if (BITSET_IS_FILL_WORD(mask_word)) {
            offset += BITSET_GET_LENGTH(mask_word);
            if (offset >= counter->size) {
                offset %= counter->size;
            }
            position = BITSET_GET_POSITION(mask_word);
            if (!position) {
                continue;
            }
            mask_word = BITSET_CREATE_LITERAL(position - 1);
        }
        for (unsigned n = 1; n <= counter->n; n++) {
            mask_words[offset] |= mask_word;
        }
        offset++;
        if (offset >= counter->size) {
            offset -= counter->size;
        }
    }
    unsigned *counts = calloc(1, sizeof(unsigned) * counter->n);
    if (!counts) {
        bitset_oom();
    }
    for (size_t offset = 0; offset < counter->size; offset++) {
        for (unsigned n = 1; n <= counter->n; n++) {
            word = counter->words[n-1][offset] & ~counter->words[n][offset];
            word &= mask_words[offset];
            BITSET_POP_COUNT(counts[n-1], word);
        }
    }
    free(mask_words);
    return counts;
}

void bitset_countn_free(bitset_countn_t *counter) {
    for (size_t i = 0; i <= counter->n; i++) {
        free(counter->words[i]);
    }
    free(counter->words);
    free(counter);
}

