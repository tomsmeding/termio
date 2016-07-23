#pragma once

#include <stdbool.h>

// --- CORE LIBRARY

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
void fillrect(int x,int y,int w,int h,char c);
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



// --- WIDGET LIBRARY

struct Logwidget;
typedef struct Logwidget Logwidget;

Logwidget* lgw_make(int x,int y,int w,int h,const char *title); //title may be "" or NULL for no title; is copied
void lgw_destroy(Logwidget *lgw);
void lgw_redraw(Logwidget *lgw); //called automatically; should only be needed if something else overwrote the widget
void lgw_add(Logwidget *lgw,const char *line);
void lgw_addf(Logwidget *lgw,const char *format,...) __attribute__((format (printf, 2,3)));
void lgw_clear(Logwidget *lgw);


struct Promptwidget;
typedef struct Promptwidget Promptwidget;

Promptwidget *prw_make(int x,int y,int w);
void prw_destroy(Promptwidget *prw);
void prw_redraw(Promptwidget *prw); //should only be needed if overwritten
char* prw_handlekey(Promptwidget *prw,int key); //newly allocated input string if enter, NULL otherwise


typedef struct Menuitem{
	char *text;
	int hotkey;
	void (*func)(void); //will be called by menu_handlekey()
} Menuitem;

typedef struct Menudata{
	int nitems;
	Menuitem *items;
} Menudata;

struct Menuwidget;
typedef struct Menuwidget Menuwidget;

typedef enum Menukey{
	MENUKEY_HANDLED,
	MENUKEY_IGNORED,
	MENUKEY_QUIT,
	MENUKEY_CALLED
} Menukey;

Menuwidget* menu_make(int basex,int basey,const Menudata *data); //keeps a reference to the data!
void menu_destroy(Menuwidget *mw);
void menu_redraw(Menuwidget *mw); //should only be needed if overwritten
Menukey menu_handlekey(Menuwidget *mw,int key);
