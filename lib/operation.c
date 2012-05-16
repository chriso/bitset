#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "bitset/operation.h"

bitset_operation *bitset_operation_new(bitset *initial) {
    bitset_operation *ops = (bitset_operation *) malloc(sizeof(bitset_operation));
    if (!ops) {
        bitset_oom();
    }
    ops->length = 0;
    if (initial) {
        bitset_operation_add(ops, initial, BITSET_OR);
    }
    return ops;
}

void bitset_operation_free(bitset_operation *ops) {
    if (ops->length) {
        for (unsigned i = 0; i < ops->length; i++) {
            if (ops->steps[i]->is_nested) {
                if (ops->steps[i]->is_operation) {
                    bitset_operation_free(ops->steps[i]->data.op);
                } else {
                    bitset_free(ops->steps[i]->data.b);
                }
            }
            free(ops->steps[i]);
        }
        free(ops->steps);
    }
    free(ops);
}

static inline bitset_operation_step *bitset_operation_add_step(bitset_operation *ops) {
    bitset_operation_step *step = (bitset_operation_step *) malloc(sizeof(bitset_operation_step));
    if (!step) {
        bitset_oom();
    }
    if (ops->length % 2 == 0) {
        if (!ops->length) {
            ops->steps = (bitset_operation_step **) malloc(sizeof(bitset_operation_step *) * 2);
        } else {
            ops->steps = (bitset_operation_step **) realloc(ops->steps,
                sizeof(bitset_operation_step *) * ops->length * 2);
        }
        if (!ops->steps) {
            bitset_oom();
        }
    }
    ops->steps[ops->length++] = step;
    return step;
}

void bitset_operation_add(bitset_operation *ops, bitset *b, enum bitset_operation_type type) {
    if (!b->length) {
        if (type == BITSET_AND && ops->length) {
            for (unsigned i = 0; i < ops->length; i++) {
                free(ops->steps[i]);
            }
            free(ops->steps);
            ops->length = 0;
        }
        return;
    }
    bitset_operation_step *step = bitset_operation_add_step(ops);
    step->is_nested = false;
    step->is_operation = false;
    step->data.b = b;
    step->type = type;
}

void bitset_operation_add_nested(bitset_operation *ops, bitset_operation *o,
        enum bitset_operation_type type) {
    bitset_operation_step *step = bitset_operation_add_step(ops);
    step->is_nested = true;
    step->is_operation = true;
    step->data.op = o;
    step->type = type;
}

static inline bitset_hash *bitset_operation_iter(bitset_operation *op) {
    bitset_offset word_offset, offset_key, max = 0, length;
    bitset_operation_step *step;
    bitset_word word, *hashed;
    unsigned position, count = 0;
    size_t size;
    bitset_hash *words, *and_words = NULL;
    bitset *tmp, *b;

    for (unsigned i = 0; i < op->length; i++) {

        //Recursively flatten nested operations
        if (op->steps[i]->is_operation) {
            tmp = bitset_operation_exec(op->steps[i]->data.op);
            bitset_operation_free(op->steps[i]->data.op);
            op->steps[i]->data.b = tmp;
            op->steps[i]->is_operation = false;
        }

        //Work out the number of hash buckets required. Note that setting the right
        //number of buckets to avoid collisions is the biggest available win here
        count += op->steps[i]->data.b->length;
        max = BITSET_MAX(max, bitset_max(op->steps[i]->data.b));

    }
#ifdef HASH_DEBUG
    fprintf(stderr, "HASH: Bitsets count = %u, max = %u\n", count, max);
#endif
    size = max / BITSET_LITERAL_LENGTH + 2;
    while (count < max / 50) {
        size /= 2;
        max /= 50;
    }
    size = size < 16 ? 16 : size > 16777216 ? 16777216 : size;
    words = bitset_hash_new(size);

    for (unsigned i = 0; i < op->length; i++) {

        step = op->steps[i];
        b = step->data.b;
        word_offset = offset_key = 0;

        if (step->type == BITSET_AND) {

            and_words = bitset_hash_new(words->size);

            for (unsigned j = 0; j < b->length; j++) {
                word = b->words[j];
                if (BITSET_IS_FILL_WORD(word)) {
                    length = BITSET_GET_LENGTH(word);
                    word_offset += length;
                    offset_key += length;
                    if (offset_key >= size) {
                        offset_key %= size;
                    }
                    position = BITSET_GET_POSITION(word);
                    if (!position) {
                        continue;
                    }
                    word = BITSET_CREATE_LITERAL(position - 1);
                }
                word_offset++;
                offset_key++;
                if (offset_key >= size) {
                    offset_key -= size;
                }
                hashed = bitset_hash_get(words, word_offset, offset_key);
                if (hashed && *hashed) {
                    word &= *hashed;
                    if (word) {
                        bitset_hash_insert(and_words, word_offset, offset_key, word);
                    }
                }
            }

            bitset_hash_free(words);
            words = and_words;
            and_words = NULL;

        } else {

            for (unsigned j = 0; j < b->length; j++) {
                word = b->words[j];

                if (BITSET_IS_FILL_WORD(word)) {
                    length = BITSET_GET_LENGTH(word);
                    word_offset += length;
                    offset_key += length;
                    if (offset_key >= size) {
                        offset_key %= size;
                    }
                    position = BITSET_GET_POSITION(word);
                    if (!position) {
                        continue;
                    }
                    word = BITSET_CREATE_LITERAL(position - 1);
                }

                word_offset++;
                offset_key++;
                if (offset_key >= size) {
                    offset_key -= size;
                }

                hashed = bitset_hash_get(words, word_offset, offset_key);

                if (hashed) {
                    switch (step->type) {
                        case BITSET_OR:     *hashed |= word;  break;
                        case BITSET_ANDNOT: *hashed &= ~word; break;
                        case BITSET_XOR:    *hashed ^= word;  break;
                        default: break;
                    }
                } else if (step->type != BITSET_ANDNOT) {
                    bitset_hash_insert(words, word_offset, offset_key, word);
                }
            }
        }
    }

    return words;
}

static int bitset_operation_offset_sort(const void *a, const void *b) {
    bitset_offset a_offset = *(bitset_offset *)a;
    bitset_offset b_offset = *(bitset_offset *)b;
    return a_offset < b_offset ? -1 : a_offset > b_offset;
}

static inline unsigned char bitset_fls(bitset_word word) {
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

bitset *bitset_operation_exec(bitset_operation *op) {
    if (!op->length) {
        return bitset_new();
    } else if (op->length == 1 && !op->steps[0]->is_operation) {
        return bitset_copy(op->steps[0]->data.b);
    }

    bitset_hash *words = bitset_operation_iter(op);
    bitset_hash_bucket *bucket;
    bitset *result = bitset_new();
    unsigned pos = 0;
    bitset_offset word_offset = 0, fills, offset;
    bitset_word word, *hashed, fill = BITSET_CREATE_EMPTY_FILL(BITSET_MAX_LENGTH);

    if (!words->count) {
        bitset_hash_free(words);
        return result;
    }

    bitset_offset *offsets = (bitset_offset *) malloc(sizeof(bitset_offset) * words->count);
    if (!offsets) {
        bitset_oom();
    }
    for (unsigned i = 0, j = 0; i < words->size; i++) {
        bucket = words->buckets[i];
        if ((uintptr_t)bucket & 1) {
            offsets[j++] = (uintptr_t)bucket >> 1;
            continue;
        }
        while (bucket) {
            offsets[j++] = bucket->offset;
            bucket = bucket->next;
        }
    }
    qsort(offsets, words->count, sizeof(bitset_offset), bitset_operation_offset_sort);

    for (unsigned i = 0; i < words->count; i++) {
        offset = offsets[i];
        hashed = bitset_hash_get(words, offset, offset % words->size);
        word = *hashed;
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
                    bitset_fls(word));
            } else {
                bitset_resize(result, result->length + 2);
                result->words[pos++] = BITSET_CREATE_EMPTY_FILL(offset - word_offset - 1);
                result->words[pos++] = word;
            }
        }
        word_offset = offset;
    }

    free(offsets);
    bitset_hash_free(words);

    return result;
}

bitset_offset bitset_operation_count(bitset_operation *op) {
    bitset_offset count = 0;
    bitset_hash *words = bitset_operation_iter(op);
    bitset_hash_bucket *bucket;
    bitset_word word;
    for (unsigned i = 0; i < words->size; i++) {
        bucket = words->buckets[i];
        if ((uintptr_t)bucket & 1) {
            word = words->words[i];
            BITSET_POP_COUNT(count, word);
            continue;
        }
        while (bucket) {
            BITSET_POP_COUNT(count, bucket->word);
            bucket = bucket->next;
        }
    }
#ifdef HASH_DEBUG
    unsigned tagged = 0, nodes = 0;
    for (unsigned i = 0; i < words->size; i++) {
        bucket = words->buckets[i];
        if ((uintptr_t)bucket & 1) { tagged++; continue; }
        while (bucket) { nodes++; bucket = bucket->next; }
    }
    fprintf(stderr, "HASH: %u buckets, %u tagged, %u linked-list nodes\n",
        words->size, tagged, nodes);
#endif
    bitset_hash_free(words);
    return count;
}

bitset_hash *bitset_hash_new(size_t buckets) {
    bitset_hash *hash = (bitset_hash *) malloc(sizeof(bitset_hash));
    if (!hash) {
        bitset_oom();
    }
#ifdef HASH_DEBUG
    fprintf(stderr, "HASH: %u buckets\n", buckets);
#endif
    hash->size = buckets;
    hash->count = 0;
    hash->buckets = (bitset_hash_bucket **) calloc(1, sizeof(bitset_hash_bucket *) * buckets);
    hash->words = (bitset_word *) malloc(sizeof(bitset_word) * buckets);
    if (!hash->buckets || !hash->words) {
        bitset_oom();
    }
    return hash;
}

void bitset_hash_free(bitset_hash *hash) {
    bitset_hash_bucket *bucket, *tmp;
    for (unsigned i = 0; i < hash->size; i++) {
        bucket = hash->buckets[i];
        if ((uintptr_t)bucket & 1) {
            continue;
        }
        while (bucket) {
            tmp = bucket;
            bucket = bucket->next;
            free(tmp);
        }
    }
    free(hash->buckets);
    free(hash->words);
    free(hash);
}

inline bool bitset_hash_insert(bitset_hash *hash, bitset_offset offset,
        bitset_offset key, bitset_word word) {
    bitset_hash_bucket *insert, *bucket = hash->buckets[key];
    bitset_offset off;
    if ((uintptr_t)bucket & 1) {
        off = (uintptr_t)bucket >> 1;
        if (off == offset) {
            return false;
        }
        insert = (bitset_hash_bucket *) malloc(sizeof(bitset_hash_bucket));
        if (!insert) {
            bitset_oom();
        }
        //Pointer tagging is out if malloc returns an address with the LSB set
        assert(!((uintptr_t)insert & 1));
        insert->offset = off;
        insert->word = (uintptr_t)hash->words[key];
        bucket = hash->buckets[key] = insert;
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
    } else if (!bucket) {
        hash->buckets[key] = (bitset_hash_bucket *)(uintptr_t)((offset << 1) | 1);
        hash->words[key] = word;
        hash->count++;
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
    assert(!((uintptr_t)insert & 1));
    insert->offset = offset;
    insert->word = word;
    insert->next = NULL;
    bucket->next = insert;
    hash->count++;
    return true;
}

inline bitset_word *bitset_hash_get(const bitset_hash *hash,
        bitset_offset offset, bitset_offset key) {
    bitset_hash_bucket *bucket = hash->buckets[key];
    bitset_offset off;
    while (bucket) {
        if ((uintptr_t)bucket & 1) {
            off = ((uintptr_t)bucket) >> 1;
            if (off != offset) {
                return NULL;
            }
            return &hash->words[key];
        }
        if (bucket->offset == offset) {
            return &bucket->word;
        }
        bucket = bucket->next;
    }
    return NULL;
}

