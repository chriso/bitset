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
    length = 0;
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
        free(i->bitsets[j]);
    }
    if (i->bitsets) free(i->bitsets);
    if (i->offsets) free(i->offsets);
    free(i);
}

