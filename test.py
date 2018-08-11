#!/usr/bin/env python3

import atexit
import termio

termio.initscreen()
atexit.register(termio.endscreen)
termio.initkeyboard(False)
atexit.register(termio.endkeyboard)

termio.installCLhandler(True)

size = termio.gettermsize()
termio.moveto(2, 2)
termio.tprint("size: {} x {}".format(size.w, size.h))
termio.redraw()

key = termio.tgetkey()
termio.moveto(2, 3)
termio.setstyle(termio.Style(1, 4, True, True))
termio.tprint("key: {}".format(key))
termio.redraw()

termio.tgetkey()
