# Talking Bass

Talking Bass is a MIDI-controlled LV2 bass instrument. The bundle contains
three plugin variants: **Talking Bass**, **Talking Bass Growl**, and
**Talking Bass Whisper**. It has no custom graphical interface; use the host's
plugin controls and automation.

## Download

GitHub Actions builds a ready-to-install `talkingbass.lv2` bundle for Linux,
universal macOS (Intel and Apple Silicon), and 64-bit Windows on pushes,
pull requests, and manual runs. Open the successful workflow run and download
the artifact for your operating system. Keep the complete `.lv2` folder
together; the binary and both Turtle metadata files are all required.

Install that folder in a location scanned by your DAW's LV2 host, then restart
or rescan the host. On Windows, the DAW must support LV2 instruments and its
LV2 search path must include the bundle's parent directory.

## Releases

Push a version tag such as `v1.0.0` to build all platforms and publish a GitHub
Release with one installable `.lv2` ZIP per platform:

```sh
git tag v1.0.0
git push origin v1.0.0
```

## Build from source

Requirements: a C99 compiler, GNU Make, `pkg-config`/`pkgconf`, and the LV2
development headers.

```sh
make bundle
```

The platform-native plugin and metadata bundle will be placed at
`build/talkingbass.lv2/`. On macOS, the CI build uses:

```sh
make bundle MACOS_ARCHS='-arch x86_64 -arch arm64'
```

The Windows CI build uses the MSYS2 UCRT64 MinGW-w64 toolchain. The source is
portable C, but the plugin binary must be built for the target OS and
architecture; Linux `.so` binaries cannot be loaded on Windows or macOS.

## License

MIT. See [LICENSE](LICENSE).
