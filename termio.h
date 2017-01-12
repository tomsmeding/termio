#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- CORE LIBRARY

typedef struct Size{
	int w,h;
} Size;

typedef struct Style{
	int fg,bg;
	bool bold,ul;
} Style;


void initkeyboard(bool nosigkeys); // if nosigkeys is true, ^C and others will be read by tgetkey
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

// Values [0,255] are that character; -1 is EOF, -2 is error. Other values
// are one of the KEY_* constants.
int tgetkey(void);

char* tgetline(void); // Returns newly allocated string; null if escape was pressed


#define KEY_TAB 9
#define KEY_LF 10
#define KEY_CR 13
#define KEY_ESC 27
#define KEY_BACKSPACE 127

#define KEY_RIGHT 1001
#define KEY_UP 1002
#define KEY_LEFT 1003
#define KEY_DOWN 1004
#define KEY_PAGEUP 1021
#define KEY_PAGEDOWN 1022
#define KEY_DELETE 1100
#define KEY_SHIFTTAB 1200

// Add (+) the relevant character: ascii '@' (64) till '_' (95)
// E.g. ^C is KEY_CTRL + 'C'
// Note that alt-[ is interpreted as the start of an escape sequence, and not normally readable
#define KEY_CTRL (-64)
#define KEY_ALT 120000
#define KEY_CTRLALT (KEY_CTRL+KEY_ALT)



// --- WIDGET LIBRARY

struct Logwidget;
typedef struct Logwidget Logwidget;

Logwidget* lgw_make(int x,int y,int w,int h,const char *title); //title may be "" or NULL for no title; is copied
void lgw_destroy(Logwidget *lgw);
void lgw_redraw(Logwidget *lgw); //called automatically; should only be needed if something else overwrote the widget
void lgw_add(Logwidget *lgw,const char *line);
void lgw_addf(Logwidget *lgw,const char *format,...) __attribute__((format (printf, 2,3)));
void lgw_clear(Logwidget *lgw);
void lgw_changetitle(Logwidget *lgw,const char *title);


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

#ifdef __cplusplus
}
#endif
