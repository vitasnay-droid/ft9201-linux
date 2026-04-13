# Linux validation

## Environment
- Ubuntu: 26.04 development branch / resolute
- Userspace approach: libusb
- fprintd stopped during test

## Result
- Device opened successfully
- Status polling worked
- Finger detection changed from 00 43 00 00 to 01 43 00 00
- Image frame 64x80 captured successfully
- frame.raw and frame.pgm were generated locally

## Important
- Public repo does not contain biometric output files
