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