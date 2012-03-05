#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "operation.h"

bitset_op *bitset_operation_new(const bitset *initial) {
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

void bitset_operation_add(bitset_op *ops, const bitset *b, enum bitset_operation op) {
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
    step->b = (bitset *) b;
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

static int bitset_operation_offset_sort(const void *a, const void *b) {
    bitset_offset a_offset = *(bitset_offset *)a;
    bitset_offset b_offset = *(bitset_offset *)b;
    return a_offset < b_offset ? -1 : a_offset > b_offset;
}

static inline unsigned char bitset_word_fls(bitset_word word) {
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

bitset_hash *bitset_hash_new(unsigned buckets) {
    unsigned size;
    bitset_hash *hash = (bitset_hash *) malloc(sizeof(bitset_hash));
    if (!hash) {
        bitset_oom();
    }
    BITSET_NEXT_POW2(size, buckets);
    hash->size = size;
    hash->count = 0;
    hash->buckets = (bitset_hash_bucket **) calloc(1, sizeof(bitset_hash_bucket *) * size);
    if (!hash->buckets) {
        bitset_oom();
    }
    return hash;
}

void bitset_hash_free(bitset_hash *hash) {
    bitset_hash_bucket *bucket, *tmp;
    for (unsigned i = 0; i < hash->size; i++) {
        bucket = hash->buckets[i];
        while (bucket) {
            tmp = bucket;
            bucket = bucket->next;
            free(tmp);
        }
    }
    free(hash->buckets);
    free(hash);
}

inline bool bitset_hash_insert(bitset_hash *hash, bitset_offset offset, bitset_word word) {
    unsigned key = offset % hash->size;
    bitset_hash_bucket *insert, *bucket = hash->buckets[key];
    if (!bucket) {
        insert = (bitset_hash_bucket *) malloc(sizeof(bitset_hash_bucket));
        if (!insert) {
            bitset_oom();
        }
        insert->offset = offset;
        insert->word = word;
        insert->next = NULL;
        hash->count++;
        hash->buckets[key] = insert;
        return true;
    }
    for (;;) {
        if (bucket->offset == offset) {
            return false;
        }
        if (bucket->next == NULL) {
            break;
        }
        bucket = bucket->next;
    }
    insert = (bitset_hash_bucket *) malloc(sizeof(bitset_hash_bucket));
    if (!insert) {
        bitset_oom();
    }
    insert->offset = offset;
    insert->word = word;
    insert->next = NULL;
    bucket->next = insert;
    hash->count++;
    return true;
}

inline bool bitset_hash_replace(bitset_hash *hash, bitset_offset offset, bitset_word word) {
    unsigned key = offset % hash->size;
    bitset_hash_bucket *bucket = hash->buckets[key];
    if (!bucket) {
        return false;
    }
    for (;;) {
        if (bucket->offset == offset) {
            bucket->word = word;
            return true;
        }
        if (bucket->next == NULL) {
            break;
        }
        bucket = bucket->next;
    }
    return false;
}

inline bitset_word bitset_hash_get(const bitset_hash *hash, bitset_offset offset) {
    unsigned key = offset % hash->size;
    bitset_hash_bucket *bucket = hash->buckets[key];
    while (bucket) {
        if (bucket->offset == offset) {
            return bucket->word;
        }
        bucket = bucket->next;
    }
    return 0;
}

