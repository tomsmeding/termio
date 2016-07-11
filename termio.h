#pragma once

#include <stdbool.h>

typedef struct Size{
	int w,h;
} Size;

typedef struct Style{
	int fg,bg;
	bool bold,ul;
} Style;


void initkeyboard(void);
void endkeyboard(void);
void initscreen(void);
void endscreen(void);

void installCLhandler(bool install); // on ^L
void installredrawhandler(void (*handler)(bool)); //arg: whether full redraw

void clearscreen(void);

Size gettermsize(void);
void setstyle(const Style *style);
void setfg(int fg);
void setbg(int bg);
void setbold(bool bold);
void setul(bool ul);
void tputc(char c);
int tprintf(const char *format,...) __attribute__((format (printf, 1, 2)));
void redraw(void);
void redrawfull(void);

char getbufferchar(int x,int y);

void moveto(int x,int y);
void pushcursor(void);
void popcursor(void);

void bel(void);

int tgetkey(void); // If in [0,254], actually that character; else one of the KEY_ constants
char* tgetline(void); // Returns newly allocated string; null if escape was pressed

#define KEY_BACKSPACE 8
#define KEY_LF 10
#define KEY_CR 13
#define KEY_ESC 27
#define KEY_DELETE 127

#define KEY_RIGHT 1001
#define KEY_UP 1002
#define KEY_LEFT 1003
#define KEY_DOWN 1004
