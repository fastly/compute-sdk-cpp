# Fastly Compute C++ SDK

This SDK is lets you use Fastly Compute platform directly with C++, using a
C++-native API with all the usual facilities of modern C++. In case you got to
this page from elsewhere, the full documentation is available online at [this
link](https://cuddly-adventure-lrw9z3m.pages.github.io).

### Examples

There's a [list of examples](./examples) you can refer to for a variety of
tasks.

The examples are fully runnable right away if used as the contents of `main.cpp`
for the `quickstart` project.

### Requirements

To use the SDK from the prebuilt static library, all you need is:

- [`wasi-sdk`](https://github.com/WebAssembly/wasi-sdk), version `25.0` (see note below)
- [The Fastly CLI](https://www.fastly.com/documentation/reference/tools/cli)
  (optional, but recommended)

> [!note]
> As documented in its README, `wasi-sdk` is _typically_ installed to `/opt/wasi-sdk`
> to reduce duplication, but you're free to install it anywhere, as long as you
> configure your build system appropriately.

#### Quickstart

The fastest way to get started with the SDK is to fetch one of the `quickstart`
tarballs from the [latest GitHub Release's assets
list](https://github.com/fastly/compute-sdk-cpp/releases/latest). This tarball
includes a full prebuilt version of the library, its headers, a copy of the full
reference docs, and a set of quickstart files with a preconfigured, CMake-based
project.

The tarballs that include `-wasi-sdk-` in their name additionally include a full
copy of an appropriate `wasi-sdk` version such that your only system requirement
after downloading will be `cmake`.

#### Just the SDK

If you only need the latest version of the library, fetch
`fastly-cpp-vX.Y.Z.tar.gz` from the [latest GitHub Release's
assets list](https://github.com/fastly/compute-sdk-cpp/releases/latest) and
place it somewhere accessible to your preferred build system.

The `libfastly.a` file is a fully-linked, static library/archive built against
`wasi-sdk@25.0`. It is fairly large, as it includes every possible call, so it
is recommended that you use IPO/LTO to reduce your final `main.wasm` file size
to only what's actually used. If you're using CMake, see
[CheckIPOSupported](https://cmake.org/cmake/help/latest/module/CheckIPOSupported.html)
and [CMP0069](https://cmake.org/cmake/help/latest/policy/CMP0069.html), or
simply reference the `quickstart` project for a working example configuration.

All header files are included in the `fastly/` folder. You can choose to import
individual headers only for the things you need, or to import the entire Fastly
SDK using `#include <fastly/sdk.h>`.

#### Building from source

Building the SDK from sources involves a few more requirements and steps. Please
refer to [CONTRIBUTING.md](./CONTRIBUTING.md) for more details.
