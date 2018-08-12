import os
from collections import namedtuple
from ctypes import *

T = cdll.LoadLibrary(os.path.dirname(os.path.realpath(__file__)) + "/libtermio.so")

class Size:
    def __init__(self, w, h):
        assert type(w) == int and type(h) == int
        self.w = w
        self.h = h

class Style:
    def __init__(self, fg, bg, bold, ul):
        assert type(fg) == int and type(bg) == int and \
               type(bold) == bool and type(ul) == bool
        self.fg = fg
        self.bg = bg
        self.bold = bold
        self.ul = ul

def initkeyboard(nosigkeys):
    T.initkeyboard(c_bool(nosigkeys))

def endkeyboard():
    T.endkeyboard()

def initscreen():
    T.initscreen()

def endscreen():
    T.endscreen()

def installCLhandler(install):
    T.installCLhandler(c_bool(install))

# TODO: void installredrawhandler(void (*handler)(bool));

def clearscreen():
    T.clearscreen()

class _TSize(Structure):
    _fields_ = [("w", c_int), ("h", c_int)]

T.gettermsize.restype = _TSize
def gettermsize():
    ts = T.gettermsize()
    return Size(ts.w, ts.h)

def setstyle(style):
    class TStyle(Structure):
        _fields_ = [("fg", c_int), ("bg", c_int), ("bold", c_bool), ("ul", c_bool)]

    assert type(style) == Style

    ts = TStyle(style.fg, style.bg, style.bold, style.ul)
    T.setstyle(byref(ts))

def setfg(fg):
    T.setfg(c_int(fg))

def setbg(bg):
    T.setbg(c_int(bg))

def setbold(bold):
    T.setbold(c_bool(bold))

def setul(ul):
    T.setul(c_bool(ul))

def tputc(c):
    T.tputc(c_int(c))

T.tprintf.restype = c_int
def tprint(s):
    if type(s) == str:
        s = bytes(s, "utf-8")
    else:
        assert type(s) == bytes

    return T.tprintf(b"%s", s)

def fillrect(x, y, w, h, c):
    T.fillrect(c_int(x), c_int(y), c_int(w), c_int(h), c_int(ord(c)))

def redraw():
    T.redraw()

def redrawfull():
    T.redrawfull()

def scrollterm(x, y, w, h, amount):
    T.scrollterm(c_int(x), c_int(y), c_int(w), c_int(h), c_int(amount))

T.getbufferchar.restype = c_int
def _getbufferchar(x, y):
    return T.getbufferchar(c_int(x), c_int(y))

def getbufferchar(x, y):
    return chr(_getbufferchar(x, y))

def moveto(x, y):
    T.moveto(c_int(x), c_int(y))

def pushcursor():
    T.pushcursor()

def popcursor():
    T.popcursor()

def bel():
    T.bel()

def cursorvisible(visible):
    T.cursorvisible(c_bool(visible))

T.tgetkey.restype = c_int
def tgetkey():
    return T.tgetkey()

T.tgetline.restype = c_char_p
def tgetline():
    charp = T.tgetline()
    if charp is None:
        return None
    else:
        return charp.value


KEY_TAB = 9
KEY_LF = 10
KEY_CR = 13
KEY_ESC = 27
KEY_BACKSPACE = 127

KEY_RIGHT = 1001
KEY_UP = 1002
KEY_LEFT = 1003
KEY_DOWN = 1004
KEY_PAGEUP = 1021
KEY_PAGEDOWN = 1022
KEY_DELETE = 1100
KEY_SHIFTTAB = 1200

KEY_CTRL = -64
KEY_SHIFT = 1000000
KEY_ALT =    120000
KEY_CTRLALT = KEY_CTRL + KEY_ALT
KEY_CTRLSHIFT = KEY_CTRL + KEY_SHIFT
