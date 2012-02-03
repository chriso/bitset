#ifndef BITSET_H_
#define BITSET_H_

/**
 * The bitset types and operations.
 */

typedef struct bitset_ {
    uint32_t *words;
    unsigned length;
} bitset;

enum bitset_operation {
    BITSET_AND,
    BITSET_OR,
    BITSET_XOR,
    BITSET_ANDNOT
};

typedef struct bitset_op_ {
    bitset **bitsets;
    enum bitset_operation *ops;
    unsigned length;
} bitset_op;

/**
 * PLWAH compression macros for 32-bit words.
 */

#define BITSET_IS_FILL_WORD(word) ((word) & 0x80000000)
#define BITSET_IS_LITERAL_WORD(word) (((word) & 0x80000000) == 0)

#define BITSET_GET_COLOUR(word) ((word) & 0x40000000)
#define BITSET_SET_COLOUR(word) ((word) | 0x40000000)
#define BITSET_UNSET_COLOUR(word) ((word) & 0xBF000000)

#define BITSET_GET_LENGTH(word) ((word) & 0x01FFFFFF)
#define BITSET_SET_LENGTH(word, len) ((word) | (len))

#define BITSET_GET_POSITION(word) (((word) & 0x3E000000) >> 25)
#define BITSET_SET_POSITION(word, pos) ((word) | ((pos) << 25))
#define BITSET_UNSET_POSITION(word) ((word) & 0xC1FFFFFF)

#define BITSET_CREATE_FILL(len, pos) ((0x80000000 | (((pos % 31) + 1) << 25)) | (len))
#define BITSET_CREATE_LITERAL(bit) (0x40000000 >> (bit))

#define BITSET_MAX(a, b) ((a) > (b) ? (a) : (b));

#define BITSET_GET_LITERAL_MASK(bit) (0x80000000 >> ((bit % 31) + 1))

#define BITSET_MAX_LENGTH (0x01FFFFFF)

/**
 * Allow a custom malloc library.
 */

#ifndef BITSET_MALLOC
#define BITSET_MALLOC malloc
#endif

#ifndef BITSET_REALLOC
#define BITSET_REALLOC realloc
#endif

#ifndef BITSET_FREE
#define BITSET_FREE free
#endif

#ifndef BITSET_OOM
#define BITSET_OOM \
    fprintf(stderr, "Out of memory\n"); \
    exit(1)
#endif

/**
 * Function prototypes.
 */

bitset *bitset_new();
void bitset_free(bitset *);
void bitset_resize(bitset *, unsigned);
bitset *bitset_new_array(unsigned, uint32_t *);
bitset *bitset_copy(bitset *);
bool bitset_get(bitset *, unsigned long);
unsigned long bitset_count(bitset *);
bool bitset_set(bitset *, unsigned long, bool);
unsigned long bitset_fls(bitset *);
bitset_op *bitset_operation_new();
void bitset_operation_free(bitset_op *);
void bitset_operation_add(bitset_op *, bitset *, enum bitset_operation);
bitset *bitset_operation_exec(bitset_op *);
long bitset_operation_count(bitset_op *);

#endif

