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

static inline bitset_hash *bitset_hash_new(size_t buckets) {
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

static inline void bitset_hash_free(bitset_hash *hash) {
    bitset_hash_bucket *bucket, *tmp;
    for (unsigned i = 0; i < hash->size; i++) {
        bucket = hash->buckets[i];
        if (BITSET_IS_TAGGED_POINTER(bucket)) {
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

static inline bool bitset_hash_insert(bitset_hash *hash, bitset_offset offset,
        bitset_offset key, bitset_word word) {
    bitset_hash_bucket *insert, *bucket = hash->buckets[key];
    bitset_offset off;
    if (BITSET_IS_TAGGED_POINTER(bucket)) {
        off = BITSET_UINT_FROM_POINTER(bucket);
        if (off == offset) {
            return false;
        }
        insert = (bitset_hash_bucket *) malloc(sizeof(bitset_hash_bucket));
        if (!insert) {
            bitset_oom();
        }
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
        if (BITSET_UINT_CAN_TAG(offset)) {
            hash->buckets[key] = (bitset_hash_bucket *) BITSET_UINT_IN_POINTER(offset);
            hash->words[key] = word;
            hash->count++;
        } else {
            insert = (bitset_hash_bucket *) malloc(sizeof(bitset_hash_bucket));
            if (!insert) {
                bitset_oom();
            }
            insert->offset = offset;
            insert->word = word;
            insert->next = NULL;
            hash->buckets[key] = insert;
        }
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

static inline bitset_word *bitset_hash_get(const bitset_hash *hash,
        bitset_offset offset, bitset_offset key) {
    bitset_hash_bucket *bucket = hash->buckets[key];
    bitset_offset off;
    while (bucket) {
        if (BITSET_IS_TAGGED_POINTER(bucket)) {
            off = BITSET_UINT_FROM_POINTER(bucket);
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

static inline bitset_hash *bitset_operation_iter(bitset_operation *op) {
    bitset_offset word_offset, offset_key, max = 0, b_max, length;
    bitset_operation_step *step;
    bitset_word word = 0, *hashed;
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
        b_max = bitset_max(op->steps[i]->data.b);
        max = BITSET_MAX(max, b_max);

    }
#ifdef HASH_DEBUG
    fprintf(stderr, "HASH: Bitsets count = %u, max = %u\n", count, max);
#endif
    if (count <= 8) {
        size = 16;
    } else if (count <= 8388608) {
        size = count * 2;
    } else {
        size = max / BITSET_LITERAL_LENGTH + 2;
        while (count < max / 50) {
            size /= 2;
            max /= 50;
        }
        size = size < 16 ? 16 : size > 16777216 ? 16777216 : size;
    }
    words = bitset_hash_new(size);

    unsigned start_at = 1;
    b = op->steps[0]->data.b;
    word_offset = offset_key = 0;

    //An operation such as (A OR B ..) is processed firstly by calculating (0 OR A)
    //to populate a hash containing word_offset => word. The remaining steps in
    //the operation are then applied to the words contained in the hash. If the
    //operation is (A AND B ..) then performing (O OR A) might waste cycles. Handle
    //this common case slightly differently by populating the initial hash with
    //the result of (0 OR (A AND B))

    if (op->length >= 2 && op->steps[1]->type == BITSET_AND) {

        start_at = 2;
        bitset_offset and_offset = 0;
        bitset_word and_word = 0;
        bitset *and = op->steps[1]->data.b;
        unsigned k = 0, j = 0;
        for (;;) {
            while (word_offset >= and_offset) {
                if (and_offset == word_offset) {
                    word &= and_word;
                    if (word) {
                        bitset_hash_insert(words, word_offset, offset_key, word);
                    }
                }
                if (j >= and->length) {
                    if (k >= b->length || and_offset < word_offset) {
                        goto break2;
                    }
                    break;
                }
                if (BITSET_IS_FILL_WORD(and->words[j])) {
                    and_offset += BITSET_GET_LENGTH(and->words[j]);
                    position = BITSET_GET_POSITION(and->words[j]);
                    if (!position) {
                        j++;
                        continue;
                    }
                    and_word = BITSET_CREATE_LITERAL(position - 1);
                } else {
                    and_word = and->words[j];
                }
                and_offset++;
                j++;
            }
            while (and_offset >= word_offset) {
                if (and_offset == word_offset) {
                    word &= and_word;
                    if (word) {
                        bitset_hash_insert(words, word_offset, offset_key, word);
                    }
                }
                if (k >= b->length) {
                    if (j >= and->length || word_offset < and_offset) {
                        goto break2;
                    }
                    break;
                }
                if (BITSET_IS_FILL_WORD(b->words[k])) {
                    length = BITSET_GET_LENGTH(b->words[k]);
                    word_offset += length;
                    offset_key += length;
                    if (offset_key >= size) {
                        offset_key %= size;
                    }
                    position = BITSET_GET_POSITION(b->words[k]);
                    if (!position) {
                        k++;
                        continue;
                    }
                    word = BITSET_CREATE_LITERAL(position - 1);
                } else {
                    word = b->words[k];
                }
                word_offset++;
                offset_key++;
                if (offset_key >= size) {
                    offset_key -= size;
                }
                k++;
            }
        }

    } else {

        //Populate the offset=>word hash (0 OR A)
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
            bitset_hash_insert(words, word_offset, offset_key, word);
        }

    }
break2:

    //Apply the remaining steps in the operation
    for (unsigned i = start_at; i < op->length; i++) {

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
        if (BITSET_IS_TAGGED_POINTER(bucket)) {
            offsets[j++] = BITSET_UINT_FROM_POINTER(bucket);
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
        if (BITSET_IS_TAGGED_POINTER(bucket)) {
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
        if (BITSET_IS_TAGGED_POINTER(bucket)) { tagged++; continue; }
        while (bucket) { nodes++; bucket = bucket->next; }
    }
    fprintf(stderr, "HASH: %u buckets, %u tagged, %u linked-list nodes\n",
        words->size, tagged, nodes);
#endif
    bitset_hash_free(words);
    return count;
}
