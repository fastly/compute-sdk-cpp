# Developer's Guide

This document includes notes for devs of the SDK itself. This is largely for
things involving releases, architecture, etc. See
[CONTRIBUTING.md](./CONTRIBUTING.md) for docs meant for contributors to get up
and running enough to send in pull requests.

## Releasing

We use [this workflow](./.github/workflows/release.yml) to mostly automate
releases. New releases are a three-step process:

1. Go to [the list of closed
   PRs](https://github.com/fastly/compute-sdk-cpp/pulls?q=is%3Apr%20is%3Aclosed)
   and make sure they all have appropriate labels—the release system will use
   this to generate changelogs. To see what labels map to what, refer to the
   [Release Action config](./.github/release.yml).
1. Locally, tag a new release, following semver conventions, prefixed with a `v`
   (so, `v1.2.3`). Push the tag.

From here, just wait for the [release
action](https://github.com/fastly/compute-sdk-cpp/actions/workflows/release.yml)
to complete, and you'll have a live release!

## Updating `wasi-sdk` shasums

Whenever we update the version of `wasi-sdk` we release with, we need to update
[`wasi-sdk-shasums.txt`](./wasi-sdk-shasums.txt) to have the right shasums and
filenames, as well as updating our workflow to use the new version. To do so:

1. Go to [the release workflow](./.github/workflows/release.yml) and update the
   `WASI_SDK_VERSION` env var to the new version.
1. Go to the `Assets` section in the relevant release under
   https://github.com/WebAssembly/wasi-sdk/releases.
1. Update `wasi-sdk-shasums.txt` with the filenames and shasums from this
   section, removing the `sha256:` prefix from each shasum.

Everything should checksum during release now. You can check this by downloading
all the relevant tarballs and running `sha256sum -c wasi-sdk-shasums.txt`
