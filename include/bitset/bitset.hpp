#ifndef BITSET_BITSET_HPP_
#define BITSET_BITSET_HPP_

#include <cstddef>

#include "bitset/bitset.h"
#include "bitset/vector.h"
#include "bitset/operation.h"
#include "bitset/probabilistic.h"

/**
 * C++ wrapper classes for bitset structures.
 */

namespace bit {

class Bitset;
class BitsetOperation;
class Vector;
class VectorIterator;
class VectorOperation;

const enum bitset_operation_type AND = BITSET_AND;
const enum bitset_operation_type OR = BITSET_OR;
const enum bitset_operation_type XOR = BITSET_XOR;
const enum bitset_operation_type ANDNOT = BITSET_ANDNOT;
const unsigned START = BITSET_VECTOR_START;
const unsigned END = BITSET_VECTOR_END;

class Bitset {
  public:
    Bitset() { b = bitset_new(); }
    Bitset(bitset *bs) { b = bs; }
    Bitset(const char *buffer, size_t len) {
        b = bitset_new_buffer(buffer, len);
    }
    Bitset(bitset_offset *bits, size_t len) {
        b = bitset_new_bits(bits, len);
    }
    ~Bitset() { bitset_free(b); }
    bool get(bitset_offset offset) const {
        return bitset_get(b, offset);
    }
    bool set(bitset_offset offset) {
        return bitset_set(b, offset);
    }
    bool unset(bitset_offset offset) {
        return bitset_unset(b, offset);
    }
    unsigned count() const {
        return bitset_count(b);
    }
    unsigned length() const {
        return bitset_length(b);
    }
    bitset_offset min() const {
        return bitset_min(b);
    }
    bitset_offset max() const {
        return bitset_max(b);
    }
    void clear() {
        bitset_clear(b);
    }
    BitsetOperation *getOperation();
    char *getBuffer() const {
        return (char *) b->words;
    }
    bitset *getBitset() const {
        return b;
    }
  protected:
    bitset *b;
};

class BitsetOperation {
  public:
    BitsetOperation() {
        o = bitset_operation_new(NULL);
    }
    BitsetOperation(const Bitset& b) {
        o = bitset_operation_new(b.getBitset());
    }
    ~BitsetOperation() {
        if (o) bitset_operation_free(o);
    }
    BitsetOperation& add(const Bitset& b, enum bitset_operation_type op) {
        bitset_operation_add(o, b.getBitset(), op);
        return *this;
    }
    BitsetOperation& add(BitsetOperation& nested, enum bitset_operation_type op) {
        bitset_operation_add_nested(o, nested.getBitsetOperation(), op);
        //Note: parent operation takes control of the pointer
        nested.removeBitsetOperation();
        return *this;
    }
    Bitset *exec() {
        return new Bitset(bitset_operation_exec(o));
    }
    unsigned count() {
        return bitset_operation_count(o);
    }
    bitset_operation *getBitsetOperation() const {
        return o;
    }
    void removeBitsetOperation() { o = NULL; }
  protected:
    bitset_operation *o;
};

class Vector {
  public:
    Vector() { v = bitset_vector_new(); }
    Vector(bitset_vector *ve) { v = ve; };
    Vector(const char *buffer, unsigned length) {
        v = bitset_vector_new_buffer(buffer, length);
    }
    ~Vector() { bitset_vector_free(v); }
    Vector& push(const Bitset &b, unsigned offset) {
        bitset_vector_push(v, b.getBitset(), offset);
        return *this;
    }
    unsigned length() const {
        return bitset_vector_length(v);
    }
    unsigned count() const {
        return bitset_vector_count(v);
    }
    VectorIterator *getIterator(unsigned start, unsigned end);
    char *getBuffer() const {
        return v->buffer;
    }
    bitset_vector *getVector() const {
        return v;
    }
  protected:
    bitset_vector *v;
};

class VectorIterator {
  public:
    VectorIterator(bitset_vector_iterator *it) { i = it; }
    VectorIterator(const Vector& list, unsigned start=START, unsigned end=END) {
        i = bitset_vector_iterator_new(list.getVector(), start, end);
    }
    ~VectorIterator() {
        bitset_vector_iterator_free(i);
    }
    VectorIterator& concat(const VectorIterator& next, unsigned offset) {
        bitset_vector_iterator_concat(i, next.getVectorIterator(), offset);
        return *this;
    }
    void count(unsigned *raw, unsigned *unique) const {
        bitset_vector_iterator_count(i, raw, unique);
    }
    Bitset *merge() const {
        return new Bitset(bitset_vector_iterator_merge(i));
    }
    Vector *compact() const {
        return new Vector(bitset_vector_iterator_compact(i));
    }
    VectorOperation *getOperation();
    bitset_vector_iterator *getVectorIterator() const {
        return i;
    }
  protected:
    bitset_vector_iterator *i;
};

class VectorOperation {
  public:
    VectorOperation() {
        o = bitset_vector_operation_new(NULL);
    }
    VectorOperation(const VectorIterator& i) {
        o = bitset_vector_operation_new(i.getVectorIterator());
    }
    ~VectorOperation() {
        if (o) bitset_vector_operation_free(o);
    }
    VectorOperation& add(const VectorIterator& v, enum bitset_operation_type op) {
        bitset_vector_operation_add(o, v.getVectorIterator(), op);
        return *this;
    }
    VectorOperation& add(VectorOperation& nested, enum bitset_operation_type op) {
        bitset_vector_operation_add_nested(o, nested.getVectorOperation(), op);
        nested.removeVectorOperation();
        return *this;
    }
    VectorIterator *exec() {
        return new VectorIterator(bitset_vector_operation_exec(o));
    }
    bitset_vector_operation *getVectorOperation() const {
        return o;
    }
    void removeVectorOperation() {
        o = NULL;
    }
  protected:
    bitset_vector_operation *o;
};

BitsetOperation *Bitset::getOperation() {
    return new BitsetOperation(*this);
}

VectorIterator *Vector::getIterator(unsigned start=START, unsigned end=END) {
    return new VectorIterator(*this, start, end);
}

VectorOperation *VectorIterator::getOperation() {
    return new VectorOperation(*this);
}

} //namespace compressedbitset

#endif

