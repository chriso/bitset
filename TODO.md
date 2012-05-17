# TODO

- Add bounds checking to bitset_vector_iterator_new()
- Lazy initialisation of operations in vector_operation_exec()

## Low priority

- Encode lengths > 2^26 using a fill with P=0 followed by another. Length is L1 << 26 & L2
- Return NULL / error code instead of dying on oom
- Implement probabilistic cardinality/topk algorithms for streams of bitsets
- Investigate behavior on illumos/smartos (read: its broken)
- Create macros for echo output
- Make newstyle compile output optional
