# Neat VNC (macOS Screen Sharing Fork)

> If this fork helped you, consider supporting development:
> **[Buy me a coffee on Ko-fi](https://ko-fi.com/alistairreay)**

## Fork Overview
This is a fork of [neatvnc](https://github.com/any1/neatvnc) that adds
compatibility with **macOS Screen Sharing.app**. The upstream library does not
support the legacy VNC protocol features that macOS Screen Sharing requires.

### Changes from upstream
 * **VNC Authentication (RFB security type 2)** -- The DES challenge-response
   auth that macOS Screen Sharing expects. Opt-in; does not replace VeNCrypt or
   RSA-AES.
 * **RFB 3.3 support** -- macOS Screen Sharing negotiates RFB version 3.3.
   Upstream only accepts 3.8. Both versions are now handled.
 * **CPIXEL calculation fix** -- The RFB spec defines CPIXEL as 3 bytes when
   bpp=32, true-colour, all max values are 255, and shifts are in {0, 8, 16}.
   The previous code incorrectly used the `depth` field, which caused ZRLE
   frame corruption with clients (like macOS) that send depth=32. This fix is
   spec-correct and benefits all clients, not just macOS.

These changes are designed to coexist with the existing auth and protocol
handling. Both legacy (type 2) and modern (VeNCrypt/RSA-AES) security types
are offered simultaneously -- the client picks whichever it supports.

See the companion [wayvnc fork](https://github.com/alistairreay/wayvnc) for
configuration and usage instructions.

### Applying as patches
If you prefer to patch upstream neatvnc rather than use this fork:
```
cd /path/to/upstream/neatvnc
git am /path/to/this-repo/patches/*.patch
```
Pre-generated patch files are available in the `patches/` directory.

---

## Introduction
This is a liberally licensed VNC server library that's intended to be fast and
neat.

## Goals
 * Speed.
 * Clean interface.
 * Interoperability with the Freedesktop.org ecosystem.

## Building

### Runtime Dependencies
 * aml - https://github.com/any1/aml/
 * ffmpeg (optional)
 * gbm (optional)
 * gnutls (optional)
 * libdrm (optional)
 * libturbojpeg (optional)
 * nettle (optional)
 * hogweed (optional)
 * gmp (optional)
 * pixman
 * zlib

### Build Dependencies
 * libdrm
 * meson
 * pkg-config

To build just run:
```
meson build
ninja -C build
```
