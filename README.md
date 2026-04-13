# FT9201 Linux bring-up notes

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
