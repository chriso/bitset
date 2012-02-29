#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "hash.h"

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

bool bitset_hash_insert(bitset_hash *hash, bitset_offset offset, bitset_word word) {
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

bool bitset_hash_replace(bitset_hash *hash, bitset_offset offset, bitset_word word) {
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

bitset_word bitset_hash_get(bitset_hash *hash, bitset_offset offset) {
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

