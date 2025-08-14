# Contributing to this project

Hi, thanks for contributing! If you just want to use the SDK, it's recommended
you use the prebuilt binaries in releases, as documented in the
[README](./README.md). If you want to build from source, you've come to the
right place.

### Requirements

- `rustc@1.86.0` + `cargo` (use [`rustup`](https://rustup.sh) to install/manage)
- [`wasi-sdk`](https://github.com/WebAssembly/wasi-sdk), version `25.0` [\[note\]](#notes-on-wasi-sdk)
- `cmake` and `make` (likely installed through your package manager)
- `doxygen` (optional. Only if you want to build documentation)
- `just` (optional. See `justfile` for equivalent commands to the ones documented below.)

#### Notes on `wasi-sdk`

`wasi-sdk-25.0` is only compatible with `rustc` versions up to `1.86.0`. If you
use `1.87.0` or later, you'll need to use a higher `wasi-sdk` version. If
building this project manually, you must make sure that the version of
`wasi-sdk` version you're using has an equal or greater LLVM version than the
one used by your `rustc`.

The build script assumes you're installing `wasi-sdk` to `/opt/wasi-sdk`. You
can specify a custom path using `--set wasi-sdk /path/to/wasi-sdk-dist` in
`just`, or by supplying the relevant `wasi-sdk-p1.cmake` file with `-DCMAKE_TOOLCHAIN_FILE` if using CMake directly.

### Example(s)

You can run the examples directly using `just`, if you have all the above set up.

To run the "hello world" example (see `./examples/echo.cpp`), you can use:

```sh
just example
```

To run any other examples, you can use:

```sh
just example example-name
```

Where `example-name` will refer to a corresponding `example-name.cpp` in the
`examples/` directory. Every example will have appropriate links and
explanations to what it's doing.

### Building Static library

```sh
just
```

#### Building in Debug mode

```sh
just --set type Debug
```

#### Using a different `wasi-sdk`

```sh
just --set wasi-sdk /path/to/your/wasi-sdk-XX.Y
```

#### Building the docs

```sh
just docs
```
