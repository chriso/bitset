#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "bitset/bitset.h"
#include "bitset/vector.h"
#include "bitset/operation.h"

bitset_vector *bitset_vector_new() {
    bitset_vector *l = (bitset_vector *) malloc(sizeof(bitset_vector));
    if (!l) {
        bitset_oom();
    }
    l->length = l->size = l->count = l->tail_offset = 0;
    l->buffer = l->tail = NULL;
    return l;
}

void bitset_vector_free(bitset_vector *l) {
    if (l->length) {
        free(l->buffer);
    }
    free(l);
}

static inline void bitset_vector_resize(bitset_vector *l, size_t length) {
    if (length > l->size) {
        size_t next_size;
        BITSET_NEXT_POW2(next_size, length);
        if (!l->length) {
            l->buffer = (char *) malloc(sizeof(char) * next_size);
        } else {
            l->buffer = (char *) realloc(l->buffer, sizeof(char) * next_size);
        }
        if (!l->buffer) {
            bitset_oom();
        }
        l->size = next_size;
    }
    l->length = length;
}

size_t bitset_vector_length(bitset_vector *l) {
    return l->length;
}

unsigned bitset_vector_count(bitset_vector *l) {
    return l->count;
}

static inline size_t bitset_encoded_length_required_bytes(size_t length) {
    if (length < (1 << 6)) {
        return 1;
    } else if (length < (1 << 14)) {
        return 2;
    } else if (length < (1 << 22)) {
        return 3;
    } else {
        return 4;
    }
}

static inline void bitset_encoded_length_bytes(char *buffer, size_t length) {
    if (length < (1 << 6)) {
        buffer[0] = (unsigned char)length;
    } else if (length < (1 << 14)) {
        buffer[0] = 0x40 | ((unsigned char)length >> 8);
        buffer[1] = (unsigned char)length & 0xFF;
    } else if (length < (1 << 22)) {
        buffer[0] = 0x80 | ((unsigned char)length >> 16);
        buffer[1] = ((unsigned char)length >> 8) & 0xFF;
        buffer[2] = (unsigned char)length & 0xFF;
    } else {
        buffer[0] = 0xC0 | ((unsigned char)length >> 24);
        buffer[1] = ((unsigned char)length >> 16) & 0xFF;
        buffer[2] = ((unsigned char)length >> 8) & 0xFF;
        buffer[3] = (unsigned char)length & 0xFF;
    }
}

static inline size_t bitset_encoded_length_size(const char *buffer) {
    switch (buffer[0] & 0xC0) {
        case 0x00: return 1;
        case 0x40: return 2;
        case 0x80: return 3;
        default:   return 4;
    }
}

static inline size_t bitset_encoded_length(const char *buffer) {
    size_t length;
    switch (buffer[0] & 0xC0) {
        case 0x00:
            length = (unsigned char)buffer[0];
            break;
        case 0x40:
            length = (((unsigned char)buffer[0] & 0x3F) << 8);
            length += (unsigned char)buffer[1];
            break;
        case 0x80:
            length = (((unsigned char)buffer[0] & 0x3F) << 16);
            length += ((unsigned char)buffer[1] << 8);
            length += (unsigned char)buffer[2];
            break;
        default:
            length = (((unsigned char)buffer[0] & 0x3F) << 24);
            length += ((unsigned char)buffer[1] << 16);
            length += ((unsigned char)buffer[2] << 8);
            length += (unsigned char)buffer[3];
            break;
    }
    return length;
}

bitset_vector *bitset_vector_new_buffer(const char *buffer, size_t length) {
    bitset_vector *l = (bitset_vector *) malloc(sizeof(bitset_vector));
    if (!l) {
        bitset_oom();
    }
    l->buffer = (char *) malloc(length * sizeof(char));
    if (!l->buffer) {
        bitset_oom();
    }
    memcpy(l->buffer, buffer, length * sizeof(char));
    l->length = l->size = length;
    l->count = l->tail_offset = 0;
    size_t length_bytes, offset_bytes;
    char *buf = l->buffer;
    for (unsigned i = 0; i < l->length; l->count++) {
        l->tail = buf;
        l->tail_offset += bitset_encoded_length(buf);
        offset_bytes = bitset_encoded_length_size(buf);
        buf += offset_bytes;
        length = bitset_encoded_length(buf);
        length_bytes = bitset_encoded_length_size(buf);
        buf += length_bytes;
        length *= sizeof(bitset_word);
        i += offset_bytes + length_bytes + length;
        buf += length;
    }
    return l;
}

void bitset_vector_push(bitset_vector *l, bitset *b, unsigned offset) {
    if (offset < l->tail_offset) {
        fprintf(stderr, "libbitset: bitset_vector_push() only supports appends\n");
        return;
    }

    size_t length = l->length;
    unsigned relative_offset = offset - l->tail_offset;
    l->count++;

    //Resize the vector to accommodate the bitset
    size_t length_bytes = bitset_encoded_length_required_bytes(b->length);
    size_t offset_bytes = bitset_encoded_length_required_bytes(relative_offset);
    bitset_vector_resize(l, length + length_bytes + offset_bytes + b->length * sizeof(bitset_word));

    //Keep a reference to the tail bitset
    char *buffer = l->tail = l->buffer + length;
    l->tail_offset = offset;

    //Encode the offset and length
    bitset_encoded_length_bytes(buffer, relative_offset);
    buffer += offset_bytes;
    bitset_encoded_length_bytes(buffer, b->length);
    buffer += length_bytes;

    //Copy the bitset
    if (b->length) {
        memcpy(buffer, b->words, b->length * sizeof(bitset_word));
    }
}

static inline void bitset_vector_iterator_resize(bitset_vector_iterator *i, size_t length) {
    if (length > i->size) {
        size_t next_size;
        BITSET_NEXT_POW2(next_size, length);
        if (!i->length) {
            i->bitsets = (bitset **) malloc(sizeof(bitset*) * next_size);
            i->offsets = (unsigned *) malloc(sizeof(unsigned) * next_size);
        } else {
            i->bitsets = (bitset **) realloc(i->bitsets, sizeof(bitset*) * next_size);
            i->offsets = (unsigned *) realloc(i->offsets, sizeof(unsigned) * next_size);
        }
        if (!i->offsets || !i->bitsets) {
            bitset_oom();
        }
        i->size = next_size;
    }
    i->length = length;
}

bitset_vector_iterator *bitset_vector_iterator_new(bitset_vector *c, unsigned start, unsigned end) {
    bitset_vector_iterator *i = (bitset_vector_iterator *) malloc(sizeof(bitset_vector_iterator));
    if (!i) {
        bitset_oom();
    }
    i->is_mutable = false;
    if (start == BITSET_VECTOR_START && end == BITSET_VECTOR_END) {
        i->bitsets = (bitset **) malloc(sizeof(bitset*) * c->count);
        i->offsets = (unsigned *) malloc(sizeof(unsigned) * c->count);
        if (!i->bitsets || !i->offsets) {
            bitset_oom();
        }
        i->length = i->size = c->count;
    } else {
        i->length = i->size = 0;
        i->bitsets = NULL;
        i->offsets = NULL;
    }
    unsigned length = 0, offset = 0;
    size_t length_bytes, offset_bytes;
    char *buffer = c->buffer;
    for (unsigned j = 0, b = 0; j < c->length; ) {
        offset += bitset_encoded_length(buffer);
        offset_bytes = bitset_encoded_length_size(buffer);
        buffer += offset_bytes;
        length = bitset_encoded_length(buffer);
        length_bytes = bitset_encoded_length_size(buffer);
        buffer += length_bytes;
        if (offset >= start && (!end || offset < end)) {
            bitset_vector_iterator_resize(i, b + 1);
            i->bitsets[b] = (bitset *) malloc(sizeof(bitset));
            if (!i->bitsets[b]) {
                bitset_oom();
            }
            i->bitsets[b]->words = (bitset_word *) buffer;
            i->bitsets[b]->length = i->bitsets[b]->size = length;
            i->offsets[b] = offset;
            b++;
        }
        length *= sizeof(bitset_word);
        j += offset_bytes + length_bytes + length;
        buffer += length;
    }
    return i;
}

void bitset_vector_iterator_concat(bitset_vector_iterator *i, bitset_vector_iterator *c, unsigned offset) {
    if (c->length) {
        unsigned previous_length = i->length;
        bitset_vector_iterator_resize(i, previous_length + c->length);
        for (unsigned j = 0; j < c->length; j++) {
            i->bitsets[j + previous_length] = c->bitsets[j];
            i->offsets[j + previous_length] = c->offsets[j] + offset;
        }
        free(c->bitsets);
        free(c->offsets);
        c->bitsets = NULL;
        c->offsets = NULL;
        c->length = c->size = 0;
    }
}

void bitset_vector_iterator_count(bitset_vector_iterator *i, unsigned *raw, unsigned *unique) {
    unsigned raw_count = 0, offset;
    bitset *b;
    bitset_operation *o = bitset_operation_new(NULL);
    BITSET_VECTOR_FOREACH(i, b, offset) {
        raw_count += bitset_count(b);
        bitset_operation_add(o, b, BITSET_OR);
    }
    (void)offset;
    *raw = raw_count;
    *unique = bitset_operation_count(o);
    bitset_operation_free(o);
}

bitset_vector *bitset_vector_iterator_compact(bitset_vector_iterator *i) {
    bitset_vector *v = bitset_vector_new();
    unsigned offset;
    bitset *b;
    BITSET_VECTOR_FOREACH(i, b, offset) {
        bitset_vector_push(v, b, offset);
    }
    (void)offset;
    return v;
}

bitset *bitset_vector_iterator_merge(bitset_vector_iterator *i) {
    unsigned offset;
    bitset *b;
    bitset_operation *o = bitset_operation_new(NULL);
    BITSET_VECTOR_FOREACH(i, b, offset) {
        bitset_operation_add(o, b, BITSET_OR);
    }
    (void)offset;
    b = bitset_operation_exec(o);
    bitset_operation_free(o);
    return b;
}

void bitset_vector_iterator_free(bitset_vector_iterator *i) {
    for (unsigned j = 0; j < i->length; j++) {
        if (i->is_mutable) {
            bitset_free(i->bitsets[j]);
        } else {
            free(i->bitsets[j]);
        }
    }
    if (i->bitsets) free(i->bitsets);
    if (i->offsets) free(i->offsets);
    free(i);
}

bitset_vector_operation *bitset_vector_operation_new(bitset_vector_iterator *i) {
    bitset_vector_operation *ops = (bitset_vector_operation *)
        malloc(sizeof(bitset_vector_operation));
    if (!ops) {
        bitset_oom();
    }
    ops->length = 0;
    if (i) {
        bitset_vector_operation_add(ops, i, BITSET_OR);
    }
    return ops;
}

void bitset_vector_operation_free(bitset_vector_operation *ops) {
    if (ops->length) {
        for (unsigned i = 0; i < ops->length; i++) {
            if (ops->steps[i]->is_nested) {
                bitset_vector_operation_free(ops->steps[i]->data.o);
            }
            free(ops->steps[i]);
        }
        free(ops->steps);
    }
    free(ops);
}

static inline bitset_vector_operation_step *
        bitset_vector_operation_add_step(bitset_vector_operation *ops) {
    bitset_vector_operation_step *step = (bitset_vector_operation_step *)
        malloc(sizeof(bitset_vector_operation_step));
    if (!step) {
        bitset_oom();
    }
    if (ops->length % 2 == 0) {
        if (!ops->length) {
            ops->steps = (bitset_vector_operation_step **)
                malloc(sizeof(bitset_vector_operation_step *) * 2);
        } else {
            ops->steps = (bitset_vector_operation_step **) realloc(ops->steps,
                sizeof(bitset_vector_operation_step *) * ops->length * 2);
        }
        if (!ops->steps) {
            bitset_oom();
        }
    }
    ops->steps[ops->length++] = step;
    return step;
}

void bitset_vector_operation_add(bitset_vector_operation *o,
        bitset_vector_iterator *i, enum bitset_operation_type type) {
    bitset_vector_operation_step *step = bitset_vector_operation_add_step(o);
    step->is_nested = false;
    step->data.i = i;
    step->type = type;
}

void bitset_vector_operation_add_nested(bitset_vector_operation *o,
        bitset_vector_operation *op, enum bitset_operation_type type) {
    bitset_vector_operation_step *step = bitset_vector_operation_add_step(o);
    step->is_nested = true;
    step->data.o = op;
    step->type = type;
}

static inline bitset_vector_hash *bitset_vector_hash_new(size_t buckets) {
    bitset_vector_hash *h = (bitset_vector_hash *) malloc(sizeof(bitset_vector_hash));
    if (!h) {
        bitset_oom();
    }
    h->buckets = (bitset_vector_hash_node **) calloc(1,
        sizeof(bitset_vector_hash) * buckets);
    if (!h->buckets) {
        bitset_oom();
    }
    h->size = buckets;
    h->count = 0;
    return h;
}

static inline void bitset_vector_hash_free(bitset_vector_hash *h) {
    if (h->count) {
        bitset_vector_hash_node *bucket, *tmp;
        for (unsigned i = 0; i < h->size; i++) {
            bucket = h->buckets[i];
            while (bucket) {
                tmp = bucket->next;
                free(bucket);
                bucket = tmp;
            }
        }
    }
    free(h->buckets);
    free(h);
}

static inline void bitset_vector_hash_resize(bitset_vector_hash *, unsigned);

static inline void bitset_vector_hash_put(bitset_vector_hash *h, unsigned offset,
        bitset_operation *o) {
    unsigned key = offset % h->size;
    bitset_vector_hash_node *bucket = h->buckets[key];
    bitset_vector_hash_node *next = (bitset_vector_hash_node *)
        malloc(sizeof(bitset_vector_hash_node));
    if (!next) {
        bitset_oom();
    }
    next->offset = offset;
    next->o = o;
    next->next = NULL;
    if (bucket) {
        next->next = bucket;
    }
    h->buckets[key] = next;
    if (h->count++ > (h->size / 2)) {
        bitset_vector_hash_resize(h, h->size * 2);
    }
}

static inline void bitset_vector_hash_resize(bitset_vector_hash *h, unsigned buckets) {
    bitset_vector_hash_node *bucket, *tmp, **old = h->buckets;
    size_t old_len = h->size;
    h->buckets = (bitset_vector_hash_node **) calloc(1,
        sizeof(bitset_vector_hash) * buckets);
    if (!h->buckets) {
        bitset_oom();
    }
    h->count = 0;
    for (unsigned i = 0; i < old_len; i++) {
        bucket = old[i];
        while (bucket) {
            bitset_vector_hash_put(h, bucket->offset, bucket->o);
            tmp = bucket->next;
            free(bucket);
            bucket = tmp;
        }
    }
    free(h->buckets);
}

static inline bitset_operation *bitset_vector_hash_get(bitset_vector_hash *h,
        unsigned offset) {
    unsigned key = offset % h->size;
    bitset_vector_hash_node *bucket = h->buckets[key];
    while (bucket) {
        if (bucket->offset == offset) {
            return bucket->o;
        }
        bucket = bucket->next;
    }
    return NULL;
}

static int bitset_vector_offset_sort(const void *a, const void *b) {
    unsigned a_offset = *(unsigned *)a;
    unsigned b_offset = *(unsigned *)b;
    return a_offset < b_offset ? -1 : a_offset > b_offset;
}

bitset_vector_iterator *bitset_vector_operation_exec(bitset_vector_operation *o) {
    bitset_vector_iterator *i, *step, *tmp;
    bitset *b;
    bitset_operation *op;
    unsigned offset;

    //Prepare the result iterator
    i = (bitset_vector_iterator *) malloc(sizeof(bitset_vector_iterator));
    if (!i) {
        bitset_oom();
    }
    i->bitsets = NULL;
    i->offsets = NULL;
    i->length = i->size = 0;
    i->is_mutable = true;

    if (!o->length) return i;

    bitset_vector_hash *and_hash, *h = bitset_vector_hash_new(32);

    for (unsigned j = 0; j < o->length; j++) {

        //Recursively flatten nested operations
        if (o->steps[j]->is_nested) {
            tmp = bitset_vector_operation_exec(o->steps[j]->data.o);
            bitset_vector_operation_free(o->steps[j]->data.o);
            o->steps[j]->data.i = tmp;
        }

        step = o->steps[j]->data.i;

        //Create a bitset operation per vector offset
        if (o->steps[j]->type == BITSET_AND) {
            and_hash = bitset_vector_hash_new(32);
            BITSET_VECTOR_FOREACH(step, b, offset) {
                op = bitset_vector_hash_get(h, offset);
                if (op) {
                    bitset_operation_add(op, b, BITSET_AND);
                    bitset_vector_hash_put(and_hash, offset, op);
                }
            }
            bitset_vector_hash_free(h);
            h = and_hash;
        } else {
            BITSET_VECTOR_FOREACH(step, b, offset) {
                op = bitset_vector_hash_get(h, offset);
                if (!op) {
                    op = bitset_operation_new(NULL);
                    bitset_operation_add(op, b, o->steps[j]->type);
                    bitset_vector_hash_put(h, offset, op);
                } else {
                    bitset_operation_add(op, b, o->steps[j]->type);
                }
            }
        }
    }

    //Prepare the result iterator
    i->bitsets = (bitset **) malloc(sizeof(bitset*) * h->count);
    i->offsets = (unsigned *) malloc(sizeof(unsigned) * h->count);
    if (!i->bitsets || !i->offsets) {
        bitset_oom();
    }
    i->length = i->size = h->count;

    //Vectors are append-only so sort the offsets
    bitset_vector_hash_node *bucket;
    unsigned *offsets = (unsigned *) malloc(sizeof(unsigned) * h->count);
    for (unsigned j = 0, k = 0; j < h->size; j++) {
        bucket = h->buckets[j];
        while (bucket) {
            offsets[k++] = bucket->offset;
            bucket = bucket->next;
        }
    }
    qsort(offsets, h->count, sizeof(unsigned), bitset_vector_offset_sort);

    //Execute the operations and store the results
    for (unsigned j = 0; j < h->count; j++) {
        offset = offsets[j];
        op = bitset_vector_hash_get(h, offset);
        i->bitsets[j] = bitset_operation_exec(op);
        i->offsets[j] = offset;
        bitset_operation_free(op);
    }

    free(offsets);
    bitset_vector_hash_free(h);

    //Free temporaries created by nested operations
    for (unsigned j = 0; j < o->length; j++) {
        if (o->steps[j]->is_nested) {
            step = o->steps[j]->data.i;
            bitset_vector_iterator_free(o->steps[j]->data.i);
            o->steps[j]->is_nested = false;
        }
    }

    return i;
}

