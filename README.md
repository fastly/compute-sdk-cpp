# compute-sdk-cpp

Creates a static library for C++ usage.

### Example(s)

To run the "hello world" example (see `./examples/echo.cpp`), you can use:

```sh
just example
```

To run any other examples, you can use:

```sh
just example example-name
```

Where `example-name` will refer to a corresponding `example-name.cpp` in the `examples/` directory. Every example will have appropriate links and explanations to what it's doing.

### Requirements

* `rustc` + `cargo` (use [`rustup`](https://rustup.sh) to install/manage) \*
* `wasi-sdk` (must use LLVM 19 or higher) \*\* \*\*\*
* `cmake`
* `make`
* `just` (optional. See `justfile` for equivalent commands to the ones documented below.)

\* Must be `1.86.0` or higher.

\*\* _`wasi-sdk-25.0` is only compatible with `rustc` versions up to `1.86.0`. If you use `1.87.0` or later, you'll need to use a higher `wasi-sdk` version. If building this project manually, you must make sure that the version of `wasi-sdk` version you're using has an equal or greater LLVM version than the one used by your `rustc`.

\*\*\* The build script assumes you're installing `wasi-sdk` to `/opt/wasi-sdk`. You can specify a custom path using `--set wasi-sdk /path/to/wasi-sdk-dist` in `just`, or `-DWASI_SDK=/path/to/wasi-sdk-dist` if using `cmake` along.

### Building Static library

```sh
just
```

#### Using a different `wasi-sdk`

```sh
just --set wasi-sdk /path/to/your/wasi-sdk-XX.Y
```