#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "bitset.h"

bitset *bitset_new() {
    bitset *b = (bitset *) malloc(sizeof(bitset));
    if (!b) {
        bitset_oom();
    }
    b->length = b->size = 0;
    return b;
}

bitset *bitset_new_array(unsigned length, uint32_t *words) {
    bitset *b = (bitset *) malloc(sizeof(bitset));
    if (!b) {
        bitset_oom();
    }
    b->words = (uint32_t *) malloc(length * sizeof(uint32_t));
    if (!b->words) {
        bitset_oom();
    }
    memcpy(b->words, words, length * sizeof(uint32_t));
    b->length = b->size = length;
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
            b->words = (uint32_t *) malloc(sizeof(uint32_t) * next_size);
        } else {
            b->words = (uint32_t *) realloc(b->words, sizeof(uint32_t) * next_size);
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
    replica->words = (uint32_t *) malloc(sizeof(uint32_t) * b->length);
    if (!replica->words) {
        bitset_oom();
    }
    memcpy(replica->words, b->words, b->length * sizeof(uint32_t));
    replica->length = b->length;
    return replica;
}

bool bitset_get(bitset *b, unsigned long bit) {
    if (!b->length) {
        return false;
    }

    uint32_t word, *words = b->words;
    long word_offset = bit / 31;
    bit %= 31;
    unsigned short position;

    for (unsigned i = 0; i < b->length; i++) {
        word = words[i];
        if (BITSET_IS_FILL_WORD(word)) {
            word_offset -= BITSET_GET_LENGTH(word);
            position = BITSET_GET_POSITION(word);
            if (word_offset < 0) {
                return BITSET_GET_COLOUR(word);
            } else if (position) {
                if (!word_offset) {
                    if (position - 1 == bit) {
                        return !BITSET_GET_COLOUR(word);
                    } else if (BITSET_GET_COLOUR(word)) {
                        return true;
                    }
                }
                word_offset--;
            }
        } else if (!word_offset--) {
            return word & (0x80000000 >> (bit + 1));
        }
    }
    return false;
}

unsigned long bitset_count(bitset *b) {
    unsigned long count = 0;
    uint32_t word, *words = b->words;
    for (unsigned i = 0; i < b->length; i++) {
        word = words[i];
        if (BITSET_IS_FILL_WORD(word)) {
            if (BITSET_GET_POSITION(word)) {
                if (BITSET_GET_COLOUR(word)) {
                    count += BITSET_GET_LENGTH(word) * 31;
                    count += 30;
                } else {
                    count += 1;
                }
            } else if (BITSET_GET_COLOUR(word)) {
                count += BITSET_GET_LENGTH(word) * 31;
            }
        } else {
            BITSET_POP_COUNT(count, word);
        }
    }
    return count;
}

unsigned long bitset_fls(bitset *b) {
    unsigned long offset = 0;
    unsigned short position;
    uint32_t word;
    for (unsigned i = 0; i < b->length; i++) {
        word = b->words[i];
        if (BITSET_IS_FILL_WORD(word)) {
            offset += BITSET_GET_LENGTH(word);
            if (BITSET_GET_COLOUR(word)) {
                return offset * 31;
            }
            position = BITSET_GET_POSITION(word);
            if (position) {
                return offset * 31 + position - 1;
            }
        } else {
            return offset * 31 + (31 - fls(word));
        }
    }
    return 0;
}

bool bitset_set(bitset *b, unsigned long bit, bool value) {
    long word_offset = bit / 31;
    bit %= 31;

    if (b->length) {
        uint32_t word;
        unsigned long fill_length;
        unsigned short position;

        for (unsigned i = 0; i < b->length; i++) {
            word = b->words[i];
            if (BITSET_IS_FILL_WORD(word)) {
                position = BITSET_GET_POSITION(word);
                fill_length = BITSET_GET_LENGTH(word);

                if (word_offset == fill_length - 1) {
                    if (position) {
                        bitset_resize(b, b->length + 1);
                        if (i < b->length - 1) {
                            memmove(b->words+i+2, b->words+i+1, sizeof(uint32_t) * (b->length - i - 2));
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
                        memmove(b->words+i+2, b->words+i+1, sizeof(uint32_t) * (b->length - i - 2));
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
                                memmove(b->words+i+2, b->words+i+1, sizeof(uint32_t) * (b->length - i - 2));
                            }
                            b->words[i] = BITSET_UNSET_POSITION(word);
                            uint32_t literal = 0;
                            literal |= BITSET_GET_LITERAL_MASK(position - 1);
                            literal |= BITSET_GET_LITERAL_MASK(bit);
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
                uint32_t mask = BITSET_GET_LITERAL_MASK(bit);
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
            //TODO: Handle fills > 2^25
            fprintf(stderr, "bitset error: fills of > 2^25 are unimplemented\n");
            exit(1);
        } else {
            bitset_resize(b, b->length + 1);
            if (word_offset) {
                b->words[b->length - 1] = BITSET_CREATE_FILL(word_offset, bit);
            } else {
                b->words[b->length - 1] = BITSET_CREATE_LITERAL(bit);
            }
        }
    }

    return false;
}

bitset_op *bitset_operation_new(bitset *initial) {
    bitset_op *ops = (bitset_op *) malloc(sizeof(bitset_op));
    if (!ops) {
        bitset_oom();
    }
    ops->length = 0;
    bitset_operation_add(ops, initial, BITSET_OR);
    return ops;
}

void bitset_operation_free(bitset_op *ops) {
    if (ops->length) {
        free(ops->steps);
    }
    free(ops);
}

void bitset_operation_add(bitset_op *ops, bitset *b, enum bitset_operation op) {
    bitset_op_step *step = (bitset_op_step *) malloc(sizeof(bitset_op_step));
    if (!step) {
        bitset_oom();
    }
    step->b = b;
    step->operation = op;
    if (ops->length % 2 == 0) {
        if (!ops->length) {
            ops->steps = (bitset_op_step **) malloc(sizeof(bitset_op_step *) * 2);
        } else {
            ops->steps = (bitset_op_step **) realloc(ops->steps, sizeof(bitset_op_step *) * ops->length * 2);
        }
        if (!ops->steps) {
            bitset_oom();
        }
    }
    ops->steps[ops->length++] = step;
}

bitset *bitset_operation_exec(bitset_op *op) {
    unsigned pos = 0;
    unsigned long word_offset = 0;
    bitset *result = bitset_new();
    bitset_op_hash *current, *tmp, *words = bitset_operation_iter(op);

    HASH_SORT(words, bitset_operation_sort);

    HASH_ITER(hh, words, current, tmp) {
        if (current->offset - word_offset == 1) {
            bitset_resize(result, result->length + 1);
            result->words[pos++] = current->word;
        } else {
            //TODO: Handle fills > 2^25
            if (BITSET_IS_POW2(current->word)) {
                bitset_resize(result, result->length + 1);
                result->words[pos++] = BITSET_CREATE_FILL(current->offset - word_offset - 1, 31 - fls(current->word));
                current->offset++;
            } else {
                bitset_resize(result, result->length + 2);
                result->words[pos++] = BITSET_CREATE_EMPTY_FILL(current->offset - word_offset - 1);
                result->words[pos++] = current->word;
            }
        }
        word_offset = current->offset;
        free(current);
    }
    return result;
}

unsigned long bitset_operation_count(bitset_op *op) {
    unsigned long count = 0;
    bitset_op_hash *current, *tmp, *words = bitset_operation_iter(op);
    HASH_ITER(hh, words, current, tmp) {
        BITSET_POP_COUNT(count, current->word);
        free(current);
    }
    return count;
}

bitset_op_hash *bitset_operation_iter(bitset_op *op) {
    unsigned long word_offset;
    uint32_t fill = 0;
    bitset_op_step *step;
    uint32_t word;
    unsigned short position;
    bitset_op_hash *words = NULL, *current, *tmp, *and_words = NULL;

    for (unsigned i = 0; i < op->length; i++) {

        step = op->steps[i];
        word_offset = 0;

        for (unsigned j = 0; j < step->b->length; j++) {
            word = step->b->words[j];

            if (BITSET_IS_FILL_WORD(word)) {
                word_offset += BITSET_GET_LENGTH(word);
                position = BITSET_GET_POSITION(word);
                if (!position) {
                    continue;
                }
                word = BITSET_CREATE_LITERAL(position - 1);
            }

            word_offset++;

            switch (step->operation) {
                case BITSET_OR:
                    HASH_FIND(hh, words, &word_offset, sizeof(unsigned long), current);
                    if (!current) {
                        current = (bitset_op_hash *) malloc(sizeof(bitset_op_hash));
                        if (!current) {
                            bitset_oom();
                        }
                        current->offset = word_offset;
                        current->word = fill;
                        HASH_ADD(hh, words, offset, sizeof(unsigned long), current);
                    }
                    current->word |= word;
                    break;
                case BITSET_AND:
                    HASH_FIND(hh, words, &word_offset, sizeof(unsigned long), current);
                    if (current) {
                        current->word &= word;
                        word &= current->word;
                    }
                    if (word) {
                        tmp = (bitset_op_hash *) malloc(sizeof(bitset_op_hash));
                        if (!tmp) {
                            bitset_oom();
                        }
                        tmp->offset = word_offset;
                        tmp->word = word;
                        HASH_ADD(hh, and_words, offset, sizeof(unsigned long), tmp);
                    }
                    break;
                case BITSET_XOR:
                case BITSET_ANDNOT:
                case BITSET_ORNOT:
                    //TODO: Implement these ops
                    fprintf(stderr, "bitset error: this operationn is unimplemented\n");
                    exit(1);
            }
        }

        if (step->operation == BITSET_AND) {
            HASH_ITER(hh, words, current, tmp) {
                free(current);
            }
            words = and_words;
            and_words = NULL;
        }
    }

    return words;
}

int bitset_operation_sort(bitset_op_hash *a, bitset_op_hash *b) {
    return a->offset > b->offset ? 1 : -1;
}

