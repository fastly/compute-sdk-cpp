# Fastly C++ Compute SDK Quickstart

This is a quickstart project that you're able to copy as-in in order to get
started quickly. It includes a `fastly.toml` with a basic example backend, a
`CMakeLists.txt` with some baseline configuration, and a `main.cpp` file with a
scaffolded request handler.

### Requirements

This project should be ready to use as soon as you extract the tarball from the
GitHub release, as long as you have the following prerequisites installed:

- `cmake` and `make`
- `wasi-sdk`, version `25.0` \*

\* Already included if you used one of the `...-wasi-sdk-...` quickstart
tarballs. Otherwise, assumed to be located at `/opt/wasi-sdk`. If you place it
elsewhere, you'll need to pass `-DWASI_SDK=/path/to/wasi-sdk-dist` when you
configure with `cmake`.

### Getting Started

Assuming you're in the `quickstart` directory, you should be able to simply use
the [Fastly CLI](https://www.fastly.com/documentation/reference/cli/) to run
your Compute app locally:

```shell
fastly compute serve --watch
```

> [!note]
> This relies on `scripts.build` in the quickstart's `fastly.toml`. You can use
> that as a baseline for your own command running setup.

And you can curl the app directly from there:

```shell
curl -d "hello, world!" http://127.0.0.1:7676/
```

> [!tip]
> You can make `main.cpp` significantly smaller by using `wasi-sdk`'s `strip`
> binary, though that will remove useful debug symbols:
>
> ```shell
> # You can add this to your fastly.toml build script
> /opt/wasi-sdk/bin/strip bin/main.wasm
> ```

✨Happy hacking!✨

### Documentation

Online documentation can be accessed
[here](https://cpp-compute-sdk.fastly.dev/).

The quickstart tarball also includes the HTML reference docs in
`docs/index.html`, which you can open locally. If you've configured your editor
properly, you may also be able to hover over Fastly SDK symbols to see their
documentation in-place.
