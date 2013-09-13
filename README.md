![Bitset](bitset.png)

The bitset structure uses [word-aligned run-length encoding](include/bitset/bitset.h#L12-29) to compress sets of unsigned integers. 64-bit offsets are supported for very sparse sets. Unlike most succinct data structures which are immutable and append-only, the included bitset structure is mutable after construction.

The library includes a vector abstraction (vector of bitsets) which can be used to represent another dimension
such as time. Bitsets are packed together [contiguously](include/bitset/vector.h#L7-23) to improve cache locality.

See the [headers](include/bitset) for usage details.

## Installation

```bash
$ ./configure
$ make
$ sudo make install
```

## Tests

Tests and benchmarks can be run with

```bash
$ make check
```

There's also a stress test available:

```bash
$ cd test
$ make stress && ./stress
```

## Credits

The symbol in the logo is from the [helveticons](http://helveticons.ch) library

## License

**LGPL** - Copyright (c) 2013 Chris O'Hara <cohara87@gmail.com>

