# Cabbagelang
This is the official repository of Cabbagelang Programming Language.

# Install
> Windows

`make windows`

> Unix-like system

`make unix`

### Attention!

- If you are on 64-bit system, you have to change `BIT` in `Makefile` to 64.
- function `request` is not compatible on Unix-like system.(It causes core dumping) If you want to send socket requests, if can use `system` and CURL.

## Dependences

- **GNU Readline**
