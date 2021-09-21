# geobuf

C++ port of <https://github.com/mapbox/geobuf>

As command line utils.

## Dev

pull all code:

```
make reset_submodules
```

compile & test:

```
make build
make test_all
```

## TODOs

-   c++ double -> string rounding print: `-1.1` instead of `-1.0999999999999999`
-   custom properties