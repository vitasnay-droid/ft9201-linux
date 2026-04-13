# FT9201 protocol notes

## Device
- VID:PID = 2808:9338
- Interface = 0
- Bulk IN endpoint = 0x83

## Image
- Width = 64
- Height = 80
- Frame size = 5120 bytes

## Observed status polling
Control IN:
- bmRequestType = 0xC0
- bRequest = 0x43
- wValue = 0x0000
- wIndex = 0x0000
- length = 4

Observed replies:
- 00 43 00 00 = idle
- 01 43 00 00 = finger detected

## Observed successful capture sequence
1. Poll status until 01 43 00 00
2. ctrl_out(0x34, 0x00ff, 0x0000)
3. ctrl_out(0x34, 0x0003, 0x0000)
4. ctrl_out(0x6f, 0x0020, 0x9180)
5. bulk IN 32 bytes
6. ctrl_out(0x34, 0x00ff, 0x0000)
7. ctrl_out(0x34, 0x0003, 0x0000)
8. ctrl_out(0x6f, 0x0000, 0x00ff)
9. ctrl_out(0x34, 0x00ff, 0x0000)
10. ctrl_out(0x34, 0x0003, 0x0000)
11. ctrl_out(0x6f, 0x0004, 0x9180)
12. bulk IN 4 bytes
13. ctrl_out(0x34, 0x00ff, 0x0000)
14. ctrl_out(0x34, 0x0003, 0x0000)
15. ctrl_out(0x6f, 0x1400, 0x9080)
16. bulk IN 5120 bytes image

## Notes
- Current PoC is userspace/libusb based
- Public repository must not include real fingerprint images
