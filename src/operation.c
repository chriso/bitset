#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "operation.h"

bitset_op *bitset_operation_new(bitset *initial) {
    bitset_op *ops = (bitset_op *) malloc(sizeof(bitset_op));
    if (!ops) {
        bitset_oom();
    }
    ops->length = 0;
    bitset_operation_add(ops, initial, BITSET_OR);
    ops->words = NULL;
    return ops;
}

void bitset_operation_free(bitset_op *ops) {
    if (ops->length) {
        for (unsigned i = 0; i < ops->length; i++) {
            free(ops->steps[i]);
        }
        free(ops->steps);
    }
    if (ops->words) {
        bitset_hash_free(ops->words);
    }
    free(ops);
}

void bitset_operation_add(bitset_op *ops, bitset *b, enum bitset_operation op) {
    if (!b->length) {
        if (op == BITSET_AND && ops->length) {
            for (unsigned i = 0; i < ops->length; i++) {
                free(ops->steps[i]);
            }
            free(ops->steps);
            ops->length = 0;
        }
        return;
    }
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

static inline bitset_hash *bitset_operation_iter(bitset_op *op) {
    bitset_offset word_offset;
    bitset_op_step *step;
    bitset_word word, current;
    unsigned char position;
    unsigned size;
    bitset_hash *and_words = NULL;

    if (!op->length) {
        return bitset_hash_new(1);
    }

    //Guess the number of required hash buckets since we don't do any adaptive resizing
    size = op->length * op->steps[1]->b->length / 100;
    size = size < 32 ? 32 : size > 1048576 ? 1048576 : size;
    op->words = bitset_hash_new(size);

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
                    current = bitset_hash_get(op->words, word_offset);
                    if (current) {
                        bitset_hash_replace(op->words, word_offset, current | word);
                    } else {
                        bitset_hash_insert(op->words, word_offset, word);
                    }
                    break;
                case BITSET_AND:
                    current = bitset_hash_get(op->words, word_offset);
                    if (current) {
                        word &= current;
                        if (word) {
                            if (and_words == NULL) {
                                and_words = bitset_hash_new(op->words->size);
                            }
                            bitset_hash_insert(and_words, word_offset, word);
                        }
                    }
                    break;
                case BITSET_ANDNOT:
                    current = bitset_hash_get(op->words, word_offset);
                    if (current) {
                        bitset_hash_replace(op->words, word_offset, current & ~word);
                    }
                    break;
                case BITSET_XOR:
                    current = bitset_hash_get(op->words, word_offset);
                    if (current) {
                        bitset_hash_replace(op->words, word_offset, current ^ word);
                    } else {
                        bitset_hash_insert(op->words, word_offset, word);
                    }
                    break;
            }
        }

        if (step->operation == BITSET_AND) {
            bitset_hash_free(op->words);
            op->words = and_words;
            and_words = NULL;
        }
    }

    return op->words;
}

bitset *bitset_new_array(unsigned length, bitset_word *words) {
    bitset *b = (bitset *) malloc(sizeof(bitset));
    if (!b) {
        bitset_oom();
    }
    b->words = (bitset_word *) malloc(length * sizeof(bitset_word));
    if (!b->words) {
        bitset_oom();
    }
    memcpy(b->words, words, length * sizeof(bitset_word));
    b->length = b->size = length;
    return b;
}

static int bitset_new_bits_sort(const void *a, const void *b) {
    bitset_offset al = *(bitset_offset *)a, bl = *(bitset_offset *)b;
    return al > bl ? 1 : -1;
}

bitset *bitset_new_bits(unsigned length, bitset_offset *bits) {
    bitset *b = bitset_new();
    if (!length) {
        return b;
    }
    unsigned pos = 0, rem, next_rem, i;
    bitset_offset word_offset = 0, div, next_div, fills;
    bitset_word fill = BITSET_CREATE_EMPTY_FILL(BITSET_MAX_LENGTH);
    qsort(bits, length, sizeof(bitset_offset), bitset_new_bits_sort);
    div = bits[0] / BITSET_LITERAL_LENGTH;
    rem = bits[0] % BITSET_LITERAL_LENGTH;
    bitset_resize(b, 1);
    b->words[0] = 0;

    for (i = 1; i < length; i++) {
        next_div = bits[i] / BITSET_LITERAL_LENGTH;
        next_rem = bits[i] % BITSET_LITERAL_LENGTH;

        if (div == word_offset) {
            b->words[pos] |= BITSET_CREATE_LITERAL(rem);
            if (div != next_div) {
                bitset_resize(b, b->length + 1);
                b->words[++pos] = 0;
                word_offset = div + 1;
            }
        } else {
            bitset_resize(b, b->length + 1);
            if (div == next_div) {
                b->words[pos++] = BITSET_CREATE_EMPTY_FILL(div - word_offset);
                b->words[pos] = BITSET_CREATE_LITERAL(rem);
                word_offset = div;
            } else {
                b->words[pos++] = BITSET_CREATE_FILL(div - word_offset, rem);
                b->words[pos] = 0;
                word_offset = div + 1;
            }
        }

        if (next_div - word_offset > BITSET_MAX_LENGTH) {
            fills = (next_div - word_offset) / BITSET_MAX_LENGTH;
            bitset_resize(b, b->length + fills);
            for (bitset_offset j = 0; j < fills; j++) {
                b->words[pos++] = fill;
            }
            word_offset += fills * BITSET_MAX_LENGTH;
        }

        div = next_div;
        rem = next_rem;
    }

    if (length == 1 || div == bits[i-2] / BITSET_LITERAL_LENGTH) {
        b->words[pos] |= BITSET_CREATE_LITERAL(rem);
    } else {
        b->words[pos] = BITSET_CREATE_FILL(div - word_offset, rem);
    }

    return b;
}

static int bitset_operation_offset_sort(const void *a, const void *b) {
    bitset_offset a_offset = *(bitset_offset *)a;
    bitset_offset b_offset = *(bitset_offset *)b;
    return a_offset < b_offset ? -1 : a_offset > b_offset;
}

bitset *bitset_operation_exec(bitset_op *op) {
    bitset_hash *words = bitset_operation_iter(op);
    bitset_hash_bucket *bucket;
    bitset *result = bitset_new();
    unsigned pos = 0;
    bitset_offset word_offset = 0, fills, offset;
    bitset_word word, fill = BITSET_CREATE_EMPTY_FILL(BITSET_MAX_LENGTH);

    if (!words->count) {
        return result;
    }

    bitset_offset *offsets = (bitset_offset *) malloc(sizeof(bitset_offset) * words->count);
    for (unsigned i = 0, j = 0; i < words->size; i++) {
        bucket = words->buckets[i];
        while (bucket) {
            offsets[j++] = bucket->offset;
            bucket = bucket->next;
        }
    }
    qsort(offsets, words->count, sizeof(bitset_offset), bitset_operation_offset_sort);

    for (unsigned i = 0; i < words->count; i++) {
        offset = offsets[i];
        word = bitset_hash_get(op->words, offset);
        if (!word) continue;

        if (offset - word_offset == 1) {
            bitset_resize(result, result->length + 1);
            result->words[pos++] = word;
        } else {
            if (offset - word_offset > BITSET_MAX_LENGTH) {
                fills = (offset - word_offset) / BITSET_MAX_LENGTH;
                bitset_resize(result, result->length + fills);
                for (bitset_offset i = 0; i < fills; i++) {
                    result->words[pos++] = fill;
                }
                word_offset += fills * BITSET_MAX_LENGTH;
            }
            if (BITSET_IS_POW2(word)) {
                bitset_resize(result, result->length + 1);
                result->words[pos++] = BITSET_CREATE_FILL(offset - word_offset - 1,
                    bitset_word_fls(word));
            } else {
                bitset_resize(result, result->length + 2);
                result->words[pos++] = BITSET_CREATE_EMPTY_FILL(offset - word_offset - 1);
                result->words[pos++] = word;
            }
        }
        word_offset = offset;
    }

    free(offsets);

    return result;
}

bitset_offset bitset_operation_count(bitset_op *op) {
    bitset_offset count = 0;
    bitset_hash *words = bitset_operation_iter(op);
    bitset_hash_bucket *bucket;
    for (unsigned i = 0; i < words->size; i++) {
        bucket = words->buckets[i];
        while (bucket) {
            BITSET_POP_COUNT(count, bucket->word);
            bucket = bucket->next;
        }
    }
    return count;
}

