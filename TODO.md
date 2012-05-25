# TODO

- Encode lengths > 2^26 using a fill with P=0 followed by another. Length is L1 << 26 | L2
- Implement hyperloglog for cardinality estimation
- Implement top-k algorithm
- Return NULL / error code instead of dying on oom
- Investigate behavior on illumos/smartos (read: its broken)
- Create macros for echo output
- Make newstyle compile output optional

