# FT9201 Linux bring-up notes

## Status

**Working now**
- Device opens on Linux via `libusb`
- Finger presence polling works
- A real **64x80 grayscale image frame** can be captured
- Protocol sequence for capture is partially documented

**Confirmed device**
- Vendor ID: `2808`
- Product ID: `9338`
- Device: **FocalTech FT9201 fingerprint reader**

**Current scope**
- Userspace PoC only
- Not integrated into `libfprint`
- Not integrated into PAM / login
- No kernel driver yet

**Privacy**
- This repository does **not** include real fingerprint images
- This repository does **not** include raw biometric USB captures

## Quick start

    sudo systemctl stop fprintd
    gcc -O2 -Wall -o ft9201_poc ft9201_poc.c -lusb-1.0
    sudo ./ft9201_poc

Expected behavior:
- idle status: `00 43 00 00`
- finger detected: `01 43 00 00`

Output files:
- `frame.raw`
- `frame.pgm`


## Device
- Vendor ID: 2808
- Product ID: 9338
- Name: FocalTech FT9201 fingerprint reader

## Current status
- Linux userspace PoC works through libusb
- A real 64x80 grayscale frame was successfully captured on Linux
- Finger detect status observed:
  - idle: 00 43 00 00
  - finger present: 01 43 00 00

## What is included
- ft9201_poc.c — minimal proof-of-concept userspace reader
- docs/protocol.md — reverse-engineered protocol notes
- docs/linux-session.md — exact Linux test session
- docs/windows-capture.md — Windows USB capture notes

## What is NOT included
- No raw fingerprint images
- No public USB captures containing biometric image payload

## Goal
Turn the PoC into something useful for upstream libfprint support.
