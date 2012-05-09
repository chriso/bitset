# TODO
 - Encode lengths > 2^26 using a fill with P=0 followed by another. Length is L1 << 26 & L2
 - Use builtin ffs(), fls(), POPCNT if available
 - Implement probabilistic HyperLogLog and Top-k algorithms for streams of bitsets
 - Ensure const-correctness
 - Consider merging bitset_hash_* and bitset_vector_hash_* using macros/void*
 - Consider renaming the "bitset" type to prevent keyword conflicts
 - Investigate behavior on illumos/smartos (read: its broken)
 - Create macros for echo output
 - Make newstyle compile output optional
