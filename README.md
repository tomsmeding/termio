# termio

This is a light-weight terminal UI library to make your life a bit easier when making a simple terminal user interface. It is a bit like the well-known and sometimes criticised `ncurses`. Termio attempts to be simpler.

Central to termio is the "draw buffer": all the I/O functions provided by the library draw on the draw buffer. You can write the buffer to the actual terminal screen using `redraw()`. Nothing else will call this function for you, so if your screen doesn't seem to be updating as you expect, try calling `redraw()`. When writing the draw buffer to the screen, the buffer is actively compared to what (termio thinks) was on the screen before, and only the parts that have actually changed are redrawn. This is because application logic is usually *way* faster than terminal I/O, so a little sacrifice in processing time is, in most use cases, worth the trouble.

Termio also provides some ready-to-use widgets. Their `..._make()` functions return an opaque struct that is to be destroyed using the corresponding `..._destroy()` function. The header files should be reasonably clear as to their interface; if not, look at the source.

Termio is actively tested on MacOS, but should also work on Linux. It will probably never work on Windows.

Thoughts, bug reports and contributions are welcome. This is not a high-profile project for me, so don't expect your favourite feature to be implemented within a day. ;)


## Building / Installing

`make`; `make install`. This will install in `~/prefix/{lib,include}`. To change that, see the first few lines of the `Makefile`.


## Unicode

The library has some basic understanding of UTF-8; internally, it stores characters as 4-byte integers, and I/O is assumed to be in UTF-8. If you stay within the ASCII range, you don't need to care about this; if you don't, note that the terminal is assumed to support UTF-8 in the first place and no invalid unicode checking is done.
