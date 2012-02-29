#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "bitset.h"
#include "operation.h"

bitset *bitset_new() {
    bitset *b = (bitset *) malloc(sizeof(bitset));
    if (!b) {
        bitset_oom();
    }
    b->length = b->size = 0;
    return b;
}

void bitset_free(bitset *b) {
    if (b->length) {
        free(b->words);
    }
    free(b);
}

void bitset_resize(bitset *b, unsigned length) {
    if (length > b->size) {
        unsigned next_size;
        BITSET_NEXT_POW2(next_size, length);
        if (!b->length) {
            b->words = (bitset_word *) malloc(sizeof(bitset_word) * next_size);
        } else {
            b->words = (bitset_word *) realloc(b->words, sizeof(bitset_word) * next_size);
        }
        if (!b->words) {
            bitset_oom();
        }
        b->size = next_size;
    }
    b->length = length;
}

bitset *bitset_copy(bitset *b) {
    bitset *replica = (bitset *) malloc(sizeof(bitset));
    if (!replica) {
        bitset_oom();
    }
    replica->words = (bitset_word *) malloc(sizeof(bitset_word) * b->length);
    if (!replica->words) {
        bitset_oom();
    }
    memcpy(replica->words, b->words, b->length * sizeof(bitset_word));
    replica->length = b->length;
    return replica;
}

bool bitset_get(bitset *b, bitset_offset bit) {
    if (!b->length) {
        return false;
    }

    bitset_word word, *words = b->words;
    bitset_offset length, word_offset = bit / BITSET_LITERAL_LENGTH;

    bit %= BITSET_LITERAL_LENGTH;
    unsigned char position;

    for (unsigned i = 0; i < b->length; i++) {
        word = words[i];
        if (BITSET_IS_FILL_WORD(word)) {
            length = BITSET_GET_LENGTH(word);
            position = BITSET_GET_POSITION(word);
            if (word_offset < length) {
                return BITSET_GET_COLOUR(word);
            } else if (position) {
                if (word_offset == length) {
                    if (position - 1 == bit) {
                        return !BITSET_GET_COLOUR(word);
                    } else if (BITSET_GET_COLOUR(word)) {
                        return true;
                    }
                }
                word_offset--;
            }
            word_offset -= length;
        } else if (!word_offset--) {
            return word & BITSET_CREATE_LITERAL(bit);
        }
    }
    return false;
}

bitset_offset bitset_count(bitset *b) {
    bitset_offset count = 0;
    bitset_word word, *words = b->words;
    for (unsigned i = 0; i < b->length; i++) {
        word = words[i];
        if (BITSET_IS_FILL_WORD(word)) {
            if (BITSET_GET_POSITION(word)) {
                if (BITSET_GET_COLOUR(word)) {
                    count += BITSET_GET_LENGTH(word) * BITSET_LITERAL_LENGTH;
                    count += 30;
                } else {
                    count += 1;
                }
            } else if (BITSET_GET_COLOUR(word)) {
                count += BITSET_GET_LENGTH(word) * BITSET_LITERAL_LENGTH;
            }
        } else {
            BITSET_POP_COUNT(count, word);
        }
    }
    return count;
}

unsigned char bitset_word_fls(bitset_word word) {
    static char table[64] = {
        32, 31, 0, 16, 0, 30, 3, 0, 15, 0, 0, 0, 29, 10, 2, 0,
        0, 0, 12, 14, 21, 0, 19, 0, 0, 28, 0, 25, 0, 9, 1, 0,
        17, 0, 4, 0, 0, 0, 11, 0, 13, 22, 20, 0, 26, 0, 0, 18,
        5, 0, 0, 23, 0, 27, 0, 6, 0, 24, 7, 0, 8, 0, 0, 0
    };
    word = word | (word >> 1);
    word = word | (word >> 2);
    word = word | (word >> 4);
    word = word | (word >> 8);
    word = word | (word >> 16);
    word = (word << 3) - word;
    word = (word << 8) - word;
    word = (word << 8) - word;
    word = (word << 8) - word;
    return table[word >> 26] - 1;
}

bitset_offset bitset_fls(bitset *b) {
    bitset_offset offset = 0;
    unsigned char position;
    bitset_word word;
    for (unsigned i = 0; i < b->length; i++) {
        word = b->words[i];
        if (BITSET_IS_FILL_WORD(word)) {
            offset += BITSET_GET_LENGTH(word);
            if (BITSET_GET_COLOUR(word)) {
                return offset * BITSET_LITERAL_LENGTH;
            }
            position = BITSET_GET_POSITION(word);
            if (position) {
                return offset * BITSET_LITERAL_LENGTH + position - 1;
            }
        } else {
            return offset * BITSET_LITERAL_LENGTH + bitset_word_fls(word);
        }
    }
    return 0;
}

bool bitset_set(bitset *b, bitset_offset bit, bool value) {
    bitset_offset word_offset = bit / BITSET_LITERAL_LENGTH;
    bit %= BITSET_LITERAL_LENGTH;

    if (b->length) {
        bitset_word word;
        bitset_offset fill_length;
        unsigned char position;

        for (unsigned i = 0; i < b->length; i++) {
            word = b->words[i];
            if (BITSET_IS_FILL_WORD(word)) {
                position = BITSET_GET_POSITION(word);
                fill_length = BITSET_GET_LENGTH(word);

                if (word_offset == fill_length - 1) {
                    if (position) {
                        bitset_resize(b, b->length + 1);
                        if (i < b->length - 1) {
                            memmove(b->words+i+2, b->words+i+1, sizeof(bitset_word) * (b->length - i - 2));
                        }
                        b->words[i+1] = BITSET_CREATE_LITERAL(position - 1);
                        if (word_offset) {
                            b->words[i] = BITSET_CREATE_FILL(fill_length - 1, bit);
                        } else {
                            b->words[i] = BITSET_CREATE_LITERAL(bit);
                        }
                    } else {
                        if (fill_length - 1 > 0) {
                            b->words[i] = BITSET_CREATE_FILL(fill_length - 1, bit);
                        } else {
                            b->words[i] = BITSET_CREATE_LITERAL(bit);
                        }
                    }
                    return false;
                } else if (word_offset < fill_length) {
                    bitset_resize(b, b->length + 1);
                    if (i < b->length - 1) {
                        memmove(b->words+i+2, b->words+i+1, sizeof(bitset_word) * (b->length - i - 2));
                    }
                    if (!word_offset) {
                        b->words[i] = BITSET_CREATE_LITERAL(bit);
                    } else {
                        b->words[i] = BITSET_CREATE_FILL(word_offset, bit);
                    }
                    b->words[i+1] = BITSET_CREATE_FILL(fill_length - word_offset - 1, position - 1);
                    return false;
                }

                word_offset -= fill_length;

                if (position) {
                    if (!word_offset) {
                        if (position - 1 == bit) {
                            if (!value || BITSET_GET_COLOUR(word)) {
                                b->words[i] = BITSET_UNSET_POSITION(word);
                            }
                            return !BITSET_GET_COLOUR(word);
                        } else {
                            bitset_resize(b, b->length + 1);
                            if (i < b->length - 1) {
                                memmove(b->words+i+2, b->words+i+1, sizeof(bitset_word) * (b->length - i - 2));
                            }
                            b->words[i] = BITSET_UNSET_POSITION(word);
                            bitset_word literal = 0;
                            literal |= BITSET_CREATE_LITERAL(position - 1);
                            literal |= BITSET_CREATE_LITERAL(bit);
                            b->words[i+1] = literal;
                            return false;
                        }
                    }
                    word_offset--;
                } else if (!word_offset && i == b->length - 1) {
                    b->words[i] = BITSET_SET_POSITION(word, bit + 1);
                    return false;
                }
            } else if (!word_offset--) {
                bitset_word mask = BITSET_CREATE_LITERAL(bit);
                bool previous = word & mask;
                if (value) {
                    b->words[i] |= mask;
                } else {
                    b->words[i] &= ~mask;
                }
                return previous;
            }
        }

    }

    if (value) {
        if (word_offset > BITSET_MAX_LENGTH) {
            bitset_offset fills = word_offset / BITSET_MAX_LENGTH;
            word_offset %= BITSET_MAX_LENGTH;
            bitset_resize(b, b->length + fills);
            bitset_word fill = BITSET_CREATE_EMPTY_FILL(BITSET_MAX_LENGTH);
            for (bitset_offset i = 0; i < fills; i++) {
                b->words[b->length - fills + i] = fill;
            }
        }
        bitset_resize(b, b->length + 1);
        if (word_offset) {
            b->words[b->length - 1] = BITSET_CREATE_FILL(word_offset, bit);
        } else {
            b->words[b->length - 1] = BITSET_CREATE_LITERAL(bit);
        }
    }

    return false;
}

