#include "bitset/vector.h"

bitset_vector_t *bitset_vector_new() {
    bitset_vector_t *v = bitset_malloc(sizeof(bitset_vector_t));
    if (!v) {
        bitset_oom();
    }
    v->buffer = bitset_malloc(sizeof(char));
    if (!v->buffer) {
        bitset_oom();
    }
    v->tail_offset = 0;
    v->size = 1;
    v->length = 0;
    return v;
}

void bitset_vector_free(bitset_vector_t *v) {
    bitset_malloc_free(v->buffer);
    bitset_malloc_free(v);
}

bitset_vector_t *bitset_vector_copy(bitset_vector_t *v) {
    bitset_vector_t *c = bitset_vector_new();
    if (v->length) {
        c->buffer = bitset_malloc(sizeof(char) * v->length);
        if (!c->buffer) {
            bitset_oom();
        }
        memcpy(c->buffer, v->buffer, v->length);
        c->length = c->size = v->length;
        c->tail_offset = v->tail_offset;
    }
    return c;
}

void bitset_vector_resize(bitset_vector_t *v, size_t length) {
    size_t new_size = v->size;
    while (new_size < length) {
        new_size *= 2;
    }
    if (new_size > v->size) {
        v->buffer = bitset_realloc(v->buffer, new_size * sizeof(char));
        if (!v->buffer) {
            bitset_oom();
        }
        v->size = new_size;
    }
    v->length = length;
}

char *bitset_vector_export(bitset_vector_t *v) {
    return v->buffer;
}

size_t bitset_vector_length(bitset_vector_t *v) {
    return v->length;
}

bitset_vector_t *bitset_vector_import(const char *buffer, size_t length) {
    bitset_vector_t *v = bitset_vector_new();
    if (length) {
        bitset_vector_resize(v, length);
        memcpy(v->buffer, buffer, length);
        //Look for the tail offset
        char *buffer = v->buffer;
        bitset_t tmp;
        while (buffer < v->buffer + v->length) {
            buffer = bitset_vector_advance(buffer, &tmp, &v->tail_offset);
        }
    }
    return v;
}

static inline size_t bitset_encoded_length_required_bytes(size_t length) {
    return (length >= (1 << 15)) * 2 + 2;
}

static inline void bitset_encoded_length_bytes(char *buffer, size_t length) {
    if (length < (1 << 15)) {
        buffer[0] = (unsigned char)(length >> 8);
        buffer[1] = (unsigned char)length;
    } else {
        buffer[0] = 0x80 | (unsigned char)(length >> 24);
        buffer[1] = (unsigned char)(length >> 16);
        buffer[2] = (unsigned char)(length >> 8);
        buffer[3] = (unsigned char)length;
    }
}

static inline size_t bitset_encoded_length_size(const char *buffer) {
    return (buffer[0] & 0x80) * 2 + 2;
}

static inline size_t bitset_encoded_length(const char *buffer) {
    size_t length;
    if (buffer[0] & 0x80) {
        length = (((unsigned char)buffer[0] & 0x7F) << 24);
        length += ((unsigned char)buffer[1] << 16);
        length += ((unsigned char)buffer[2] << 8);
        length += (unsigned char)buffer[3];
    } else {
        length = (((unsigned char)buffer[0] & 0x7F) << 8);
        length += (unsigned char)buffer[1];
    }
    return length;
}

char *bitset_vector_advance(char *buffer, bitset_t *bitset, unsigned *offset) {
    *offset += bitset_encoded_length(buffer);
    buffer += bitset_encoded_length_size(buffer);
    bitset->length = bitset_encoded_length(buffer);
    buffer += bitset_encoded_length_size(buffer);
    bitset->buffer = (bitset_word *) buffer;
    return buffer + bitset->length * sizeof(bitset_word);
}

static inline char *bitset_vector_encode(bitset_vector_t *v, uintptr_t buf_offset, bitset_t *b, unsigned offset) {
    size_t length_bytes = bitset_encoded_length_required_bytes(b->length);
    size_t offset_bytes = bitset_encoded_length_required_bytes(offset);
    bitset_vector_resize(v, v->length + length_bytes + offset_bytes + b->length * sizeof(bitset_word));
    char *buffer = v->buffer + buf_offset;
    bitset_encoded_length_bytes(buffer, offset);
    buffer += offset_bytes;
    bitset_encoded_length_bytes(buffer, b->length);
    buffer += length_bytes;
    if (b->length) {
        memcpy(buffer, b->buffer, b->length * sizeof(bitset_word));
    }
    return buffer + b->length * sizeof(bitset_word);
}

void bitset_vector_push(bitset_vector_t *v, bitset_t *b, unsigned offset) {
    if (v->length && v->tail_offset >= offset) {
        BITSET_FATAL("bitset vectors are append-only");
    }
    bitset_vector_encode(v, v->length, b, offset - v->tail_offset);
    v->tail_offset = offset;
}

void bitset_vector_concat(bitset_vector_t *v, bitset_vector_t *c, unsigned offset, unsigned start, unsigned end) {
    if (v->length && v->tail_offset >= offset) {
        BITSET_FATAL("bitset vectors are append-only");
    }

    char *buffer = v->buffer;
    unsigned current_offset = 0;
    bitset_t bitset;

    char *c_buffer = c->buffer, *c_start, *c_end = c->buffer + c->length;
    while (c_buffer < c_end) {
        c_buffer = bitset_vector_advance(c_buffer, &bitset, &current_offset);
        if (current_offset >= start && (!end || current_offset < end)) {
            c_start = c_buffer;

            //Copy the initial bitset from c
            buffer = bitset_vector_encode(v, v->length, &bitset, offset + current_offset - v->tail_offset);

            //Look for a slice end point
            if (end != BITSET_VECTOR_END && c_end > c_start) {
                do {
                    c_end = c_buffer;
                    if (c_end == c->buffer + c->length) {
                        break;
                    }
                    v->tail_offset = current_offset + offset;
                    c_buffer = bitset_vector_advance(c_buffer, &bitset, &current_offset);
                } while (current_offset < end);
            } else {
                v->tail_offset = c->tail_offset + offset;
            }

            //Concat the rest of the vector
            if (c_end > c_start) {
                uintptr_t buf_offset = buffer - v->buffer;
                bitset_vector_resize(v, v->length + (c_end - c_start));
                memcpy(v->buffer + buf_offset, c_start, c_end - c_start);
            }

            break;
        }
    }
}

unsigned bitset_vector_bitsets(bitset_vector_t *v) {
    unsigned count = 0, offset;
    bitset_t *tmp;
    BITSET_VECTOR_FOREACH(v, tmp, offset) {
        count++;
    }
    return count;
}

void bitset_vector_cardinality(bitset_vector_t *v, unsigned *raw, unsigned *unique) {
    unsigned offset;
    bitset_t *b;
    *raw = 0;
    BITSET_VECTOR_FOREACH(v, b, offset) {
        *raw += bitset_count(b);
    }
    if (unique) {
        if (*raw) {
            bitset_linear_t *l = bitset_linear_new(*raw * 100);
            BITSET_VECTOR_FOREACH(v, b, offset) {
                bitset_linear_add(l, b);
            }
            *unique = bitset_linear_count(l);
            bitset_linear_free(l);
        } else {
            *unique = 0;
        }
    }
}

bitset_t *bitset_vector_merge(bitset_vector_t *i) {
    unsigned offset;
    bitset_t *b;
    bitset_operation_t *o = bitset_operation_new(NULL);
    BITSET_VECTOR_FOREACH(i, b, offset) {
        bitset_operation_add(o, b, BITSET_OR);
    }
    b = bitset_operation_exec(o);
    bitset_operation_free(o);
    return b;
}

static inline void bitset_vector_start_end(bitset_vector_t *v, unsigned *start, unsigned *end) {
    if (!v->length) {
        *start = 0;
        *end = 0;
        return;
    }
    char *buffer = v->buffer;
    bitset_t bitset;
    buffer = bitset_vector_advance(buffer, &bitset, start);
    *end = v->tail_offset;
}

bitset_vector_operation_t *bitset_vector_operation_new(bitset_vector_t *i) {
    bitset_vector_operation_t *ops = bitset_malloc(sizeof(bitset_vector_operation_t));
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

void bitset_vector_operation_free(bitset_vector_operation_t *ops) {
    if (ops->length) {
        for (unsigned i = 0; i < ops->length; i++) {
            if (ops->steps[i]->is_nested) {
                if (ops->steps[i]->is_operation) {
                    bitset_vector_operation_free(ops->steps[i]->data.o);
                } else {
                    bitset_vector_free(ops->steps[i]->data.i);
                }
            }
            bitset_malloc_free(ops->steps[i]);
        }
        bitset_malloc_free(ops->steps);
    }
    bitset_malloc_free(ops);
}

void bitset_vector_operation_free_operands(bitset_vector_operation_t *ops) {
    if (ops->length) {
        for (unsigned i = 0; i < ops->length; i++) {
            if (ops->steps[i]->is_nested) {
                if (ops->steps[i]->is_operation) {
                    bitset_vector_operation_free_operands(ops->steps[i]->data.o);
                }
            } else {
                bitset_vector_free(ops->steps[i]->data.i);
            }
        }
    }
}

static inline bitset_vector_operation_step_t *
        bitset_vector_operation_add_step(bitset_vector_operation_t *ops) {
    bitset_vector_operation_step_t *step = bitset_malloc(sizeof(bitset_vector_operation_step_t));
    if (!step) {
        bitset_oom();
    }
    if (ops->length % 2 == 0) {
        if (!ops->length) {
            ops->steps = bitset_malloc(sizeof(bitset_vector_operation_step_t *) * 2);
        } else {
            ops->steps = bitset_realloc(ops->steps, sizeof(bitset_vector_operation_step_t *) * ops->length * 2);
        }
        if (!ops->steps) {
            bitset_oom();
        }
    }
    ops->steps[ops->length++] = step;
    return step;
}

void bitset_vector_operation_add(bitset_vector_operation_t *o,
        bitset_vector_t *i, enum bitset_operation_type type) {
    if (!i->length) {
        return;
    }
    bitset_vector_operation_step_t *step = bitset_vector_operation_add_step(o);
    step->is_nested = false;
    step->is_operation = false;
    step->data.i = i;
    step->type = type;
    unsigned start = 0, end = 0;
    bitset_vector_start_end(i, &start, &end);
    o->min = BITSET_MIN(o->min, start);
    o->max = BITSET_MAX(o->max, end);
}

void bitset_vector_operation_add_nested(bitset_vector_operation_t *o,
        bitset_vector_operation_t *op, enum bitset_operation_type type) {
    bitset_vector_operation_step_t *step = bitset_vector_operation_add_step(o);
    step->is_nested = true;
    step->is_operation = true;
    step->data.o = op;
    step->type = type;
    o->min = BITSET_MIN(o->min, op->min);
    o->max = BITSET_MAX(o->max, op->max);
}

void bitset_vector_operation_add_data(bitset_vector_operation_t *o,
        void *data, enum bitset_operation_type type) {
    bitset_vector_operation_step_t *step = bitset_vector_operation_add_step(o);
    step->is_nested = false;
    step->is_operation = false;
    step->data.i = NULL;
    step->type = type;
    step->userdata = data;
}

void bitset_vector_operation_resolve_data(bitset_vector_operation_t *o,
        bitset_vector_t *(*resolve_fn)(void *, void *), void *context) {
    if (o->length) {
        unsigned start = 0, end = 0;
        for (unsigned j = 0; j < o->length; j++) {
            if (o->steps[j]->is_operation) {
                bitset_vector_operation_resolve_data(o->steps[j]->data.o, resolve_fn, context);
            } else if (o->steps[j]->userdata) {
                bitset_vector_t *i = resolve_fn(o->steps[j]->userdata, context);
                o->steps[j]->data.i = i;
                if (i && i->length) {
                    bitset_vector_start_end(i, &start, &end);
                    o->min = BITSET_MIN(o->min, start);
                    o->max = BITSET_MAX(o->max, end);
                }
            }
        }
    }
}

void bitset_vector_operation_free_data(bitset_vector_operation_t *o, void (*free_fn)(void *)) {
    if (o->length) {
        for (unsigned j = 0; j < o->length; j++) {
            if (o->steps[j]->is_operation) {
                bitset_vector_operation_free_data(o->steps[j]->data.o, free_fn);
            } else if (o->steps[j]->userdata) {
                free_fn(o->steps[j]->userdata);
            }
        }
    }
}

static bitset_t *bitset_vector_encoded_bitset(char *buffer) {
    bitset_t *bitset = bitset_new();
    bitset->length = bitset_encoded_length(buffer);
    buffer += bitset_encoded_length_size(buffer);
    size_t size;
    BITSET_NEXT_POW2(size, bitset->length);
    bitset->buffer = bitset_malloc(sizeof(bitset_word) * size);
    if (!bitset->buffer) {
        bitset_oom();
    }
    memcpy(bitset->buffer, buffer, sizeof(bitset_word) * bitset->length);
    return bitset;
}

bitset_vector_t *bitset_vector_operation_exec(bitset_vector_operation_t *o) {
    if (!o->length) {
        return bitset_vector_new();
    } else if (o->length == 1 && !o->steps[0]->is_operation) {
        return bitset_vector_copy(o->steps[0]->data.i);
    }

    bitset_vector_t *v, *step, *tmp;
    bitset_t *b, *b2, bitset;
    bitset_operation_t *op;
    unsigned offset;

    //Recursively flatten nested operations
    for (unsigned j = 0; j < o->length; j++) {
        if (o->steps[j]->is_operation) {
            tmp = bitset_vector_operation_exec(o->steps[j]->data.o);
            bitset_vector_operation_free(o->steps[j]->data.o);
            o->steps[j]->data.i = tmp;
            o->steps[j]->is_operation = false;
        }
    }

    //Prepare the result vector
    v = bitset_vector_new();

    //Prepare hash buckets
    unsigned key, buckets = o->max - o->min + 1;
    void **and_bucket = NULL;
    void **bucket = bitset_calloc(1, sizeof(void*) * buckets);
    if (!bucket) {
        bitset_oom();
    }

    //OR the first vector
    char *buffer, *next;
    step = o->steps[0]->data.i;
    if (step) {
        buffer = step->buffer;
        offset = 0;
        while (buffer < step->buffer + step->length) {
            next = bitset_vector_advance(buffer, &bitset, &offset);
            bucket[offset - o->min] = buffer + bitset_encoded_length_size(buffer);
            buffer = next;
        }
    }

    for (unsigned j = 1; j < o->length; j++) {

        enum bitset_operation_type type = o->steps[j]->type;
        step = o->steps[j]->data.i;

        //Create a bitset operation per vector offset
        if (type == BITSET_AND) {

            and_bucket = bitset_calloc(1, sizeof(void*) * buckets);
            if (!and_bucket) {
                bitset_oom();
            }
            if (step) {
                buffer = step->buffer;
                offset = 0;
                while (buffer < step->buffer + step->length) {
                    next = bitset_vector_advance(buffer, &bitset, &offset);
                    key = offset - o->min;
                    if (bucket[key]) {
                        if (BITSET_IS_TAGGED_POINTER(bucket[key])) {
                            op = (bitset_operation_t *) BITSET_UNTAG_POINTER(bucket[key]);
                        } else {
                            b2 = bitset_vector_encoded_bitset(bucket[key]);
                            op = bitset_operation_new(b2);
                        }
                        bitset_operation_add(op, bitset_copy(&bitset), BITSET_AND);
                        and_bucket[key] = (void *) BITSET_TAG_POINTER(op);
                    }
                    buffer = next;
                }
            }
            for (size_t i = 0; i < buckets; i++) {
                if (and_bucket[i] && BITSET_IS_TAGGED_POINTER(bucket[i])) {
                    op = (bitset_operation_t *) BITSET_UNTAG_POINTER(bucket[key]);
                    for (size_t x = 0; x < op->length; x++) {
                        bitset_free(op->steps[x]->data.bitset);
                    }
                    bitset_operation_free(op);
                }
            }
            bitset_malloc_free(bucket);
            bucket = and_bucket;

        } else if (step) {

            buffer = step->buffer;
            offset = 0;
            while (buffer < step->buffer + step->length) {
                next = bitset_vector_advance(buffer, &bitset, &offset);
                key = offset - o->min;
                if (BITSET_IS_TAGGED_POINTER(bucket[key])) {
                    op = (bitset_operation_t *) BITSET_UNTAG_POINTER(bucket[key]);
                    bitset_operation_add(op, bitset_copy(&bitset), type);
                } else if (bucket[key]) {
                    b2 = bitset_vector_encoded_bitset(bucket[key]);
                    op = bitset_operation_new(b2);
                    bucket[key] = (void *) BITSET_TAG_POINTER(op);
                    bitset_operation_add(op, bitset_copy(&bitset), type);
                } else {
                    bucket[key] = buffer + bitset_encoded_length_size(buffer);
                }
                buffer = next;
            }
        }
    }

    //Prepare the result vector
    offset = 0;
    buffer = v->buffer;
    size_t bitset_length, copy_length, offset_bytes;
    for (unsigned j = 0; j < buckets; j++) {
        if (BITSET_IS_TAGGED_POINTER(bucket[j])) {
            op = (bitset_operation_t *) BITSET_UNTAG_POINTER(bucket[j]);
            b = bitset_operation_exec(op);
            if (b->length) {
                buffer = bitset_vector_encode(v, buffer - v->buffer, b, o->min + j - offset);
                offset = o->min + j;
            }
            bitset_free(b);
            for (size_t x = 0; x < op->length; x++) {
                bitset_free(op->steps[x]->data.bitset);
            }
            bitset_operation_free(op);
        } else if (bucket[j]) {
            offset_bytes = bitset_encoded_length_required_bytes(o->min + j - offset);
            bitset_length = bitset_encoded_length(bucket[j]);
            copy_length = bitset_encoded_length_required_bytes(bitset_length) + bitset_length * sizeof(bitset_word);
            uintptr_t buf_offset = buffer - v->buffer;
            bitset_vector_resize(v, v->length + offset_bytes + copy_length);
            buffer = v->buffer + buf_offset;
            bitset_encoded_length_bytes(buffer, o->min + j - offset);
            buffer += offset_bytes;
            memcpy(buffer, bucket[j], copy_length);
            buffer += copy_length;
            offset = o->min + j;
        }
    }

    bitset_malloc_free(bucket);

    return v;
}

