**A compressed bitset** - `GPLv3`

### About

The bitset is
[compressed](https://github.com/chriso/bitset/blob/master/include/bitset/bitset.h#L6-24)
and supports the same operations as its uncompressed counterpart without
requiring a decompression step
([succinct](http://en.wikipedia.org/wiki/Succinct_data_structure)).
Unlike most succinct data structures which are append-only and then immutable, the bitset includes support for setting random bit offsets. 64-bit offsets are also supported for very sparse bitsets.

### What's included

- Bitset operations such as get/set, population count, min/max (ffs/fls)
- Complex bitwise operations between one or more bitsets, e.g. `A & (B & ~C) | (D ^ E)`
- Pack multiple bitsets together using the included [vector
  abstraction](https://github.com/chriso/bitset/blob/master/include/bitset/vector.h#L7-25)
- Bitwise operations between one or more vectors, e.g. `V1 | (V2 & V3) => V4`
- Probabilistic algorithms for estimating cardinality, top-k, etc.
- [C++ class
  wrappers](https://github.com/chriso/bitset/blob/master/include/bitset/bitset.hpp)

### License

Copyright (C) 2012 Chris O'Hara <cohara87@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
