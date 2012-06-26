#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#include "bitset/bitset.h"
#include "bitset/vector.h"
#include "bitset/operation.h"
#include "bitset/estimate.h"

bitset_vector *bitset_vector_new() {
    bitset_vector *l = malloc(sizeof(bitset_vector));
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

bitset_vector *bitset_vector_copy(bitset_vector *v) {
    bitset_vector *c = bitset_vector_new();
    if (v->length) {
        c->buffer = malloc(sizeof(char) * v->length);
        if (!c->buffer) {
            bitset_oom();
        }
        memcpy(c->buffer, v->buffer, v->length);
        c->length = c->size = v->length;
        c->count = v->count;
        c->tail = v->tail;
        c->tail_offset = v->tail_offset;
    }
    return c;
}

void bitset_vector_resize(bitset_vector *l, size_t length) {
    if (length > l->size) {
        size_t next_size;
        BITSET_NEXT_POW2(next_size, length);
        if (!l->length) {
            l->buffer = malloc(sizeof(char) * next_size);
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
        buffer[0] = 0x40 | (unsigned char)(length >> 8);
        buffer[1] = (unsigned char)length;
    } else if (length < (1 << 22)) {
        buffer[0] = 0x80 | (unsigned char)(length >> 16);
        buffer[1] = (unsigned char)(length >> 8);
        buffer[2] = (unsigned char)length;
    } else {
        buffer[0] = 0xC0 | (unsigned char)(length >> 24);
        buffer[1] = (unsigned char)(length >> 16);
        buffer[2] = (unsigned char)(length >> 8);
        buffer[3] = (unsigned char)length;
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
    bitset_vector *l = malloc(sizeof(bitset_vector));
    if (!l) {
        bitset_oom();
    }
    l->buffer = malloc(length * sizeof(char));
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
        i += offset_bytes;
        if (i >= l->length) break;
        buf += offset_bytes;
        length = bitset_encoded_length(buf);
        length_bytes = bitset_encoded_length_size(buf);
        i += length_bytes;
        if (i >= l->length) break;
        buf += length_bytes;
        length *= sizeof(bitset_word);
        i += length;
        if (i > l->length) break;
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
            i->bitsets = malloc(sizeof(bitset*) * next_size);
            i->offsets = malloc(sizeof(unsigned) * next_size);
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

bitset_vector_iterator *bitset_vector_iterator_new(bitset_vector *c,
        unsigned start, unsigned end) {
    bitset_vector_iterator *i = malloc(sizeof(bitset_vector_iterator));
    if (!i) {
        bitset_oom();
    }
    if (start == BITSET_VECTOR_START && end == BITSET_VECTOR_END) {
        i->bitsets = malloc(sizeof(bitset*) * c->count);
        i->offsets = malloc(sizeof(unsigned) * c->count);
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
        j += offset_bytes;
        if (j >= c->length) break;
        buffer += offset_bytes;
        length = bitset_encoded_length(buffer);
        length_bytes = bitset_encoded_length_size(buffer);
        j += length_bytes;
        if (j >= c->length) break;
        buffer += length_bytes;
        if (offset >= start && (!end || offset < end)) {
            bitset_vector_iterator_resize(i, b + 1);
            i->bitsets[b] = malloc(sizeof(bitset));
            if (!i->bitsets[b]) {
                bitset_oom();
            }
            i->bitsets[b]->words = malloc(sizeof(bitset_word) * length);
            if (!i->bitsets[b]->words) {
                bitset_oom();
            }
            memcpy(i->bitsets[b]->words, buffer, length * sizeof(bitset_word));
            i->bitsets[b]->length = i->bitsets[b]->size = length;
            i->bitsets[b]->references = 1;
            i->offsets[b] = offset;
            b++;
        }
        length *= sizeof(bitset_word);
        j += length;
        if (j > c->length) break;
        buffer += length;
    }
    return i;
}

bitset_vector_iterator *bitset_vector_iterator_new_empty() {
    bitset_vector_iterator *i = malloc(sizeof(bitset_vector_iterator));
    if (!i) {
        bitset_oom();
    }
    i->bitsets = NULL;
    i->offsets = NULL;
    i->length = i->size = 0;
    return i;
}

bitset_vector_iterator *bitset_vector_iterator_copy(bitset_vector_iterator *c) {
    bitset_vector_iterator *i = malloc(sizeof(bitset_vector_iterator));
    if (!i) {
        bitset_oom();
    }
    i->bitsets = malloc(sizeof(bitset*) * c->length);
    i->offsets = malloc(sizeof(unsigned) * c->length);
    if (!i->bitsets || !i->offsets) {
        bitset_oom();
    }
    i->length = i->size = c->length;
    for (unsigned j = 0; j < c->length; j++) {
        i->bitsets[j] = bitset_copy(c->bitsets[j]);
        i->offsets[j] = c->offsets[j];
    }
    return i;
}

void bitset_vector_iterator_concat(bitset_vector_iterator *i, bitset_vector_iterator *c, unsigned offset) {
    if (c->length) {
        unsigned previous_length = i->length;
        bitset_vector_iterator_resize(i, previous_length + c->length);
        for (unsigned j = 0; j < c->length; j++) {
            i->bitsets[j + previous_length] = c->bitsets[j];
            c->bitsets[j]->references++;
            i->offsets[j + previous_length] = c->offsets[j] + offset;
        }
    }
}

void bitset_vector_iterator_count(bitset_vector_iterator *i, unsigned *raw, unsigned *unique) {
    unsigned raw_count = 0, offset;
    bitset *b;
    BITSET_VECTOR_FOREACH(i, b, offset) {
        raw_count += bitset_count(b);
    }
    (void)offset;
    *raw = raw_count;
    if (unique) {
        bitset_linear *l = bitset_linear_new(raw_count * 10);
        BITSET_VECTOR_FOREACH(i, b, offset) {
            bitset_linear_add(l, b);
        }
        *unique = bitset_linear_count(l);
        bitset_linear_free(l);
    }
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
        bitset_free(i->bitsets[j]);
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
    ops->length = ops->max = 0;
    ops->min = UINT_MAX;
    if (i) {
        bitset_vector_operation_add(ops, i, BITSET_OR);
    }
    return ops;
}

void bitset_vector_operation_free(bitset_vector_operation *ops) {
    if (ops->length) {
        for (unsigned i = 0; i < ops->length; i++) {
            if (ops->steps[i]->is_nested) {
                if (ops->steps[i]->is_operation) {
                    bitset_vector_operation_free(ops->steps[i]->data.o);
                } else {
                    bitset_vector_iterator_free(ops->steps[i]->data.i);
                }
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
    if (!i->length) {
        return;
    }
    bitset_vector_operation_step *step = bitset_vector_operation_add_step(o);
    step->is_nested = false;
    step->is_operation = false;
    step->data.i = i;
    step->type = type;
    o->min = BITSET_MIN(o->min, i->offsets[0]);
    o->max = BITSET_MAX(o->max, i->offsets[i->length-1]);
}

void bitset_vector_operation_add_nested(bitset_vector_operation *o,
        bitset_vector_operation *op, enum bitset_operation_type type) {
    bitset_vector_operation_step *step = bitset_vector_operation_add_step(o);
    step->is_nested = true;
    step->is_operation = true;
    step->data.o = op;
    step->type = type;
    o->min = BITSET_MIN(o->min, op->min);
    o->max = BITSET_MAX(o->max, op->max);
}

void bitset_vector_operation_add_data(bitset_vector_operation *o,
        void *data, enum bitset_operation_type type) {
    bitset_vector_operation_step *step = bitset_vector_operation_add_step(o);
    step->is_nested = false;
    step->is_operation = false;
    step->data.i = NULL;
    step->type = type;
    step->userdata = data;
}

void bitset_vector_operation_resolve_data(bitset_vector_operation *o,
        bitset_vector_iterator *(*resolve_fn)(void *, void *), void *context) {
    for (unsigned j = 0; j < o->length; j++) {
        if (o->steps[j]->is_operation) {
            bitset_vector_operation_resolve_data(o->steps[j]->data.o, resolve_fn, context);
        } else if (o->steps[j]->userdata) {
            o->steps[j]->data.i = resolve_fn(o->steps[j]->userdata, context);
        }
    }
}

void bitset_vector_operation_free_data(bitset_vector_operation *o, void (*free_fn)(void *)) {
    for (unsigned j = 0; j < o->length; j++) {
        if (o->steps[j]->is_operation) {
            bitset_vector_operation_free_data(o->steps[j]->data.o, free_fn);
        } else if (o->steps[j]->userdata) {
            free_fn(o->steps[j]->userdata);
        }
    }
}

bitset_vector_iterator *bitset_vector_operation_exec(bitset_vector_operation *o) {
    if (!o->length) {
        return bitset_vector_iterator_new_empty();
    } else if (o->length == 1 && !o->steps[0]->is_operation) {
        return bitset_vector_iterator_copy(o->steps[0]->data.i);
    }

    bitset_vector_iterator *i, *step, *tmp;
    bitset *b, *b2;
    bitset_operation *op = NULL;
    unsigned offset, count = 0;

    //Recursively flatten nested operations
    for (unsigned j = 0; j < o->length; j++) {
        if (o->steps[j]->is_operation) {
            tmp = bitset_vector_operation_exec(o->steps[j]->data.o);
            bitset_vector_operation_free(o->steps[j]->data.o);
            o->steps[j]->data.i = tmp;
            o->steps[j]->is_operation = false;
        }
    }

    //Prepare the result iterator
    i = bitset_vector_iterator_new_empty();

    //Prepare hash buckets
    unsigned key, buckets = o->max - o->min + 1;
    void **and_bucket = NULL;
    void **bucket = (void **) calloc(1, sizeof(void*) * buckets);
    if (!bucket) {
        bitset_oom();
    }

    //OR the first vector
    step = o->steps[0]->data.i;
    BITSET_VECTOR_FOREACH(step, b, offset) {
        key = offset - o->min;
        if (BITSET_IS_TAGGED_POINTER(bucket[key])) {
            op = (bitset_operation *) BITSET_UNTAG_POINTER(bucket[key]);
            bitset_operation_add(op, b, o->steps[0]->type);
        } else if (bucket[key]) {
            op = bitset_operation_new((bitset *) bucket[key]);
            bucket[key] = (void *) BITSET_TAG_POINTER(op);
            bitset_operation_add(op, b, o->steps[0]->type);
        } else {
            bucket[key] = (void *) b;
            count++;
        }
    }

    enum bitset_operation_type type;

    for (unsigned j = 1; j < o->length; j++) {

        type = o->steps[j]->type;
        step = o->steps[j]->data.i;

        //Create a bitset operation per vector offset
        if (type == BITSET_AND) {
            and_bucket = (void **) calloc(1, sizeof(void*) * buckets);
            if (!and_bucket) {
                bitset_oom();
            }
            count = 0;
            BITSET_VECTOR_FOREACH(step, b, offset) {
                key = offset - o->min;
                if (BITSET_IS_TAGGED_POINTER(bucket[key])) {
                    op = (bitset_operation *) BITSET_UNTAG_POINTER(bucket[key]);
                } else if (bucket[key]) {
                    b2 = (bitset *) bucket[key];
                    op = bitset_operation_new(b2);
                }
                if (op) {
                    count++;
                    bitset_operation_add(op, b, BITSET_AND);
                    and_bucket[key] = (void *) BITSET_TAG_POINTER(op);
                    op = NULL;
                }
            }
            free(bucket);
            bucket = and_bucket;
        } else {
            BITSET_VECTOR_FOREACH(step, b, offset) {
                key = offset - o->min;
                if (BITSET_IS_TAGGED_POINTER(bucket[key])) {
                    op = (bitset_operation *) BITSET_UNTAG_POINTER(bucket[key]);
                    bitset_operation_add(op, b, type);
                } else if (bucket[key]) {
                    op = bitset_operation_new((bitset *) bucket[key]);
                    bucket[key] = (void *) BITSET_TAG_POINTER(op);
                    bitset_operation_add(op, b, type);
                } else {
                    bucket[key] = (void *) b;
                    count++;
                }
            }
        }
    }

    //Prepare the result iterator
    i->bitsets = malloc(sizeof(bitset*) * count);
    i->offsets = malloc(sizeof(unsigned) * count);
    if (!i->bitsets || !i->offsets) {
        bitset_oom();
    }
    i->length = i->size = count;

    offset = 0;
    for (unsigned j = 0; j < buckets; j++) {
        if (BITSET_IS_TAGGED_POINTER(bucket[j])) {
            op = (bitset_operation *) BITSET_UNTAG_POINTER(bucket[j]);
            b = bitset_operation_exec(op);
            if (b->length) {
                i->bitsets[offset] = b;
                i->offsets[offset++] = o->min + j;
            } else {
                bitset_free(b);
            }
            bitset_operation_free(op);
        } else if (bucket[j]) {
            i->bitsets[offset] = bitset_copy((bitset *) bucket[j]);
            i->offsets[offset++] = o->min + j;
            count++;
        }
    }
    i->length = offset;

    free(bucket);

    return i;
}

