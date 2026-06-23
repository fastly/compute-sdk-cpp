# Contributing to this project

Hi, thanks for contributing! If you just want to use the SDK, it's recommended
you use the prebuilt binaries in releases, as documented in the
[README](./README.md). If you want to build from source, you've come to the
right place.

### Requirements

- `rustc@1.96.0` + `cargo` (use [`rustup`](https://rustup.sh) to install/manage)
- [`wasi-sdk`](https://github.com/WebAssembly/wasi-sdk), version `33.0` [\[note\]](#notes-on-wasi-sdk)
- `cmake` and `make` (likely installed through your package manager)
- `doxygen` (optional. Only if you want to build documentation)
- `just` (optional. See `justfile` for equivalent commands to the ones documented below.)

#### Notes on `wasi-sdk`

The build script assumes you're installing `wasi-sdk` to `/opt/wasi-sdk`. You
can specify a custom path using `--set wasi-sdk /path/to/wasi-sdk-dist` in
`just`, or by supplying the relevant `wasi-sdk-p1.cmake` file with
`-DCMAKE_TOOLCHAIN_FILE` if using CMake directly.

Additionally: any time the `wasi-sdk` version is upgraded,
`./github/workflows/release.yml` needs to be updated with the new shasums.

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
