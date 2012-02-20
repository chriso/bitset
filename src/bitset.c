#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "bitset.h"

bitset *bitset_new() {
    bitset *b = (bitset *) BITSET_MALLOC(sizeof(bitset));
    if (!b) {
        BITSET_OOM;
    }
    b->length = b->size = 0;
    return b;
}

bitset *bitset_new_array(unsigned length, uint32_t *words) {
    bitset *b = (bitset *) BITSET_MALLOC(sizeof(bitset));
    if (!b) {
        BITSET_OOM;
    }
    b->words = (uint32_t *) BITSET_MALLOC(length * sizeof(uint32_t));
    if (!b->words) {
        BITSET_OOM;
    }
    memcpy(b->words, words, length * sizeof(uint32_t));
    b->length = b->size = length;
    return b;
}

void bitset_free(bitset *b) {
    if (b->length) {
        BITSET_FREE(b->words);
    }
    BITSET_FREE(b);
}

void bitset_resize(bitset *b, unsigned length) {
    if (length > b->size) {
        unsigned next_size;
        BITSET_NEXT_POW2(next_size, length);

        if (!b->length) {
            b->words = (uint32_t *) BITSET_MALLOC(sizeof(uint32_t) * next_size);
        } else {
            b->words = (uint32_t *) BITSET_REALLOC(b->words, sizeof(uint32_t) * next_size);
        }
        if (!b->words) {
            BITSET_OOM;
        }
        b->size = next_size;
    }

    b->length = length;
}

bitset *bitset_copy(bitset *b) {
    bitset *replica = (bitset *) BITSET_MALLOC(sizeof(bitset));
    if (!replica) {
        BITSET_OOM;
    }
    replica->words = (uint32_t *) BITSET_MALLOC(sizeof(uint32_t) * b->length);
    if (!replica->words) {
        BITSET_OOM;
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
            return offset * 31 + (31 - flsl(word));
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
            fprintf(stderr, "Fill chains are unimplemented\n");
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
    bitset_op *ops = (bitset_op *) BITSET_MALLOC(sizeof(bitset_op));
    if (!ops) {
        BITSET_OOM;
    }
    ops->initial = initial;
    ops->length = 0;
    return ops;
}

void bitset_operation_free(bitset_op *ops) {
    if (ops->length) {
        BITSET_FREE(ops->steps);
    }
    BITSET_FREE(ops);
}

void bitset_operation_add(bitset_op *ops, bitset *b, enum bitset_operation op) {
    bitset_op_step *step = (bitset_op_step *) BITSET_MALLOC(sizeof(bitset_op_step));
    if (!step) {
        BITSET_OOM;
    }
    step->b = b;
    step->operation = op;
    step->pos = 0;
    step->word_offset = step->prev_offset = 0;
    if (ops->length % 2 == 0) {
        if (!ops->length) {
            ops->steps = (bitset_op_step **) BITSET_MALLOC(sizeof(bitset_op_step *) * 2);
        } else {
            ops->steps = (bitset_op_step **) BITSET_REALLOC(ops->steps, sizeof(bitset_op_step *) * ops->length * 2);
        }
        if (!ops->steps) {
            BITSET_OOM;
        }
    }
    ops->steps[ops->length++] = step;
}

bitset *bitset_operation_exec(bitset_op *ops) {
    //TODO
    return NULL;
}

unsigned long bitset_operation_count(bitset_op *op) {
    unsigned long count = 0, len, word_offset = 0, fill_len;
    unsigned long shortest_offset, last_shortest = LONG_MAX, next_offset = 0;
    uint32_t word, word2, current_word, *words;
    bitset_op_step *step;
    bool complete = false, initial_complete = false;
    unsigned short position;

    len = op->initial->length;
    words = op->initial->words;

    for (unsigned i = 0; !complete; i++) {
        if (i < len) {
            word = words[i];
            if (BITSET_IS_FILL_WORD(word)) {
                fill_len = BITSET_GET_LENGTH(word);
                position = BITSET_GET_POSITION(word);
                word_offset += fill_len;
                if (position) {
                    word_offset++;
                    word = BITSET_CREATE_LITERAL(position - 1);
                } else {
                    word = 0;
                }
            } else {
                fill_len = 0;
                word_offset++;
            }
        } else {
            word = 0;
            complete = true;
            initial_complete = true;
            if (next_offset) {
                word_offset = next_offset;
            }
        }

        while (true) {

            current_word = word;
            last_shortest = shortest_offset;
            shortest_offset = LONG_MAX - 1;

            for (unsigned j = 0; j < op->length; j++) {
                step = op->steps[j];

                if (last_shortest && step->word_offset == last_shortest) {
                    step->pos++;
                    step->prev_offset = step->word_offset;
                } else {
                    step->word_offset = step->prev_offset;
                }

                if (step->pos < step->b->length) {
                    step->current_word = step->b->words[step->pos];
                    complete = false;

                    if (BITSET_IS_FILL_WORD(step->current_word)) {
                        step->word_offset += BITSET_GET_LENGTH(step->current_word);
                        position = BITSET_GET_POSITION(step->current_word);
                        if (position) {
                            step->word_offset++;
                            step->current_word = BITSET_CREATE_LITERAL(position - 1);
                        } else {
                            step->current_word = 0;
                        }
                    } else {
                        step->word_offset++;
                    }
                } else {
                    step->current_word = 0;
                    complete = complete && true;
                    step->word_offset = LONG_MAX;
                }

                if (step->word_offset < shortest_offset && step->word_offset != word_offset) {
                    shortest_offset = step->word_offset;
                }
            }

            next_offset = shortest_offset;

            if (shortest_offset >= word_offset) {
                current_word = word;
                shortest_offset = word_offset;
            } else {
                current_word = 0;
            }

            for (unsigned j = 0; j < op->length; j++) {
                step = op->steps[j];

                if (step->word_offset > shortest_offset) {
                    word2 = 0;
                } else {
                    word2 = step->current_word;
                }

                switch (step->operation) {
                    case BITSET_AND:
                        current_word &= word2;
                        break;
                    case BITSET_OR:
                        current_word |= word2;
                        break;
                    case BITSET_XOR:
                        current_word ^= word2;
                        break;
                    case BITSET_ANDNOT:
                        current_word &= ~word2;
                        break;
                    case BITSET_ORNOT:
                        current_word |= ~word2;
                        break;
                }
            }

            BITSET_POP_COUNT(count, current_word);

            if (next_offset > word_offset || complete) {
                break;
            }
        }

        if (initial_complete && shortest_offset > word_offset) {
            word_offset = shortest_offset;
        }
    }

    return count;
}

