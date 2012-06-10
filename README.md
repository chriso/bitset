`bitset`

## What's included

- A succint, compressed bitset structure
- 64-bit offset support for very sparse bitsets
- Mutable after construction (unlike most succint structures which are append-only)
- Bitset operations such as get/set, population count, min/max (ffs/fls)
- Complex bitwise operations between bitsets, e.g. `A & (B & ~C) | (D ^ E)`
- Pack bitsets together efficiently using the included [vector
  abstraction](https://github.com/chriso/bitset/blob/master/include/bitset/vector.h#L7-25)
- Bitwise operations between one or more vectors, e.g. `V1 | (V2 & V3) => V4`
- Probabilistic algorithms

## Installing

```bash
$ ./configure
$ make
$ sudo make install
```

## Tests

Tests can be run with

```bash
$ make test
```

Stress tests / benchmarks can be run with

```bash
$ make stress
```

## License

**LGPL** - Copyright (c) 2012 Chris O'Hara <cohara87@gmail.com>

