# TODO

## Near term
- clean up the current PoC code
- replace getenv("PWD") usage with a safer path print
- print and document the 32-byte header contents
- print and document the 4-byte marker contents
- add better error handling and timeouts
- test repeated captures in one session

## Mid term
- turn the PoC into a reusable capture/debug tool
- document the full capture state machine
- investigate enrollment flow
- investigate image quality / preprocessing needs
- test on more machines with the same FT9201 reader
- confirm whether other VID:PID variants exist

## Long term
- prototype a libfprint driver
- integrate capture into libfprint image path
- support enrollment / verify flow
- prepare upstream submission
