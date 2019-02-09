#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#include "termio.h"

#define prflush(...) fprflush(stdout,__VA_ARGS__)
#define fprflush(f,...) do {fprintf(f,__VA_ARGS__); fflush(f);} while(0)
#define atxy(buf,x,y) ((buf)[termsize.w*(y)+(x)])
#define atpos(buf,pos) atxy(buf,pos.x,pos.y)


typedef struct Position{
	int x,y;
} Position;

typedef struct Screencell{
	int c;
	Style style;
} Screencell;

static bool screenlive=false,keyboardinited=false,sighandlerinstalled=false;
static bool cursorisvisible=true;
static bool needresize=false;
static bool handlerefresh=false;

static void (*redrawhandler)(bool)=NULL;

static Screencell *screenbuf=NULL,*drawbuf=NULL;
static Size termsize={0,0};

static Position cursor={0,0};
static Style curstyle={9,9,false,false};


static void sighandler(int sig){
	if(!screenlive)return;
	switch(sig){
		case SIGINT:
		case SIGABRT:
			endkeyboard();
			endscreen();
			exit(130); // ^C

		case SIGWINCH:
			needresize=true;
			break;
	}
}

static Size querytermsize(void){
	struct winsize w;
	assert(ioctl(2,TIOCGWINSZ,&w)!=-1);
	Size sz={w.ws_col,w.ws_row};
	if(sz.w<=0||sz.h<=0){
		sz.w=80;
		sz.h=24;
	}
	return sz;
}

static void copyintersect(Screencell *dest,Size destsz,const Screencell *src,Size srcsz){
	for(int y=0;y<destsz.h;y++){
		for(int x=0;x<destsz.w;x++){
			if(x<srcsz.w&&y<srcsz.h){
				memcpy(dest+(destsz.w*y+x),src+(srcsz.w*y+x),sizeof(Screencell));
			} else {
				dest[destsz.w*y+x].c=' ';
				dest[destsz.w*y+x].style.fg=dest[destsz.w*y+x].style.bg=9;
				dest[destsz.w*y+x].style.bold=dest[destsz.w*y+x].style.ul=false;
			}
		}
	}
}

static void resizeterm(void){
	needresize=false;

	Size oldsize=termsize;

	termsize=querytermsize();

	Screencell *newscreen=calloc(termsize.w*termsize.h,sizeof(Screencell));
	assert(newscreen);
	Screencell *newdraw=calloc(termsize.w*termsize.h,sizeof(Screencell));
	assert(newdraw);

	copyintersect(newscreen,termsize,screenbuf,oldsize);
	copyintersect(newdraw,termsize,drawbuf,oldsize);

	free(screenbuf);
	free(drawbuf);
	screenbuf=newscreen;
	drawbuf=newdraw;
}

Size gettermsize(void){
	if(needresize)resizeterm();
	return termsize;
}

static bool have_tios_bak=false;
static struct termios tios_bak;

void initkeyboard(bool nosigkeys){
	struct termios tios;
	tcgetattr(0,&tios_bak);
	have_tios_bak=true;
	tios=tios_bak;
	tios.c_lflag&=~(
		ECHO|ECHOE // no echo of normal characters, erasing
#ifdef ECHOKE
		|ECHOKE // ...and killing
#endif
#ifdef ECHOCTL
		|ECHOCTL // don't visibly echo control characters (^V etc.)
#endif
		|ECHONL // don't even echo a newline
		|ICANON // disable canonical mode
#ifdef NOKERNINFO
		|NOKERNINFO // don't print a status line on ^T
#endif
		|IEXTEN // don't handle things like ^V specially
		);
	if(nosigkeys)tios.c_lflag&=~ISIG; // disable ^C ^\ and ^Z
	tios.c_cc[VMIN]=1; // read one char at a time
	tios.c_cc[VTIME]=0; // no timeout on reading, make it a blocking read
	tcsetattr(0,TCSAFLUSH,&tios);

	keyboardinited=true;
}

void endkeyboard(void){
	if(!keyboardinited)return;
	if(have_tios_bak)tcsetattr(0,TCSAFLUSH,&tios_bak);
	keyboardinited=false;
}

void initscreen(void){
	printf("\x1B[?1049h\x1B[2J\x1B[H"); fflush(stdout);

	termsize=querytermsize();

	screenbuf=calloc(termsize.w*termsize.h,sizeof(Screencell));
	assert(screenbuf);
	drawbuf=calloc(termsize.w*termsize.h,sizeof(Screencell));
	assert(drawbuf);

	for(int i=0;i<termsize.w*termsize.h;i++){
		screenbuf[i].c=' ';
		screenbuf[i].style.fg=screenbuf[i].style.bg=9;
		screenbuf[i].style.bold=screenbuf[i].style.ul=false;
	}
	memcpy(drawbuf,screenbuf,termsize.w*termsize.h*sizeof(Screencell));

	if(!sighandlerinstalled){
		signal(SIGWINCH,sighandler);
		signal(SIGINT,sighandler);
		signal(SIGABRT,sighandler);
	}
	sighandlerinstalled=true;
	cursorisvisible=true;
	screenlive=true;
}

void endscreen(void){
	if(!screenlive)return;

	cursorvisible(true);
	printf("\x1B[?1049l"); fflush(stdout);

	if(screenbuf)free(screenbuf);
	if(drawbuf)free(drawbuf);

	screenlive=false;
}

void installCLhandler(bool install){
	handlerefresh=install;
}

void installredrawhandler(void (*handler)(bool)){
	redrawhandler=handler;
}


void clearscreen(void){
	assert(screenlive);
	if(needresize)resizeterm();
	for(int i=0;i<termsize.w*termsize.h;i++){
		drawbuf[i].c=' ';
		drawbuf[i].style.fg=drawbuf[i].style.bg=9;
		drawbuf[i].style.bold=drawbuf[i].style.ul=false;
	}
}


void setstyle(const Style *style){
	assert((style->fg>=0&&style->fg<8)||style->fg==9);
	assert((style->bg>=0&&style->bg<8)||style->bg==9);
	curstyle=*style;
}

void setfg(int fg){
	assert((fg>=0&&fg<8)||fg==9);
	curstyle.fg=fg;
}
void setbg(int bg){
	assert((bg>=0&&bg<8)||bg==9);
	curstyle.bg=bg;
}
void setbold(bool bold){
	curstyle.bold=bold;
}
void setul(bool ul){
	curstyle.ul=ul;
}

// Modifies accstyle to match style
// Pass NULL as accstyle to unconditionally set style
static void outputstyle(Style *accstyle,const Style *style){
	char parts[6][16];
	int nparts=0;
	if(!accstyle||accstyle->bold!=style->bold){
		if(style->bold){
			strcpy(parts[nparts++],"1");
		} else {
			strcpy(parts[nparts++],"0"); // no way to reset bold
			if(accstyle)accstyle->fg=accstyle->bg=9;
			if(accstyle)accstyle->bold=accstyle->ul=false;
		}
	}
	if(!accstyle||accstyle->ul!=style->ul){
		if(style->ul)strcpy(parts[nparts++],"4");
		else strcpy(parts[nparts++],"24");
	}
	if(!accstyle||style->fg!=accstyle->fg)sprintf(parts[nparts++],"3%d",style->fg);
	if(!accstyle||style->bg!=accstyle->bg)sprintf(parts[nparts++],"4%d",style->bg);

	if(accstyle)*accstyle=*style;

	if(nparts==0)return;
	printf("\x1B[%s",parts[0]);
	for(int i=1;i<nparts;i++)printf(";%s",parts[i]);
	putchar('m');
}

static void tputcstartx(int c,int *startx){
	switch(c){
		case '\r':
			*startx=0;
			__attribute__((fallthrough));
		case '\n':
			cursor.x=*startx;
			if(cursor.y<termsize.h-1)cursor.y++;
			break;

		default:
			assert(c>=32);
			atpos(drawbuf,cursor).style=curstyle;
			atpos(drawbuf,cursor).c=c;
			cursor.x++;
			if(cursor.x==termsize.w){
				cursor.x=*startx;
				if(cursor.y<termsize.h-1){
					cursor.y++;
				}
			}
			break;
	}
}

void tputc(int c){
	assert(screenlive);
	if(needresize)resizeterm();
	int startx=0;
	tputcstartx(c,&startx);
}

// returns '?' on finding invalid or too-short utf8 character
static int scanutf8char(const char *strS,int len,int *clen){
	const unsigned char *str=(const unsigned char*)strS;
	assert(len>0);
	if((str[0]&0x80)==0){
		*clen=1;
		return str[0];
	}
	*clen=1;  // for '?'
	if((str[0]&0xe0)==0xc0){
		if(len<2)return '?';
		if((str[1]&0xc0)!=0x80)return '?';
		*clen=2;
		return ((str[0]&0x1f)<<6)|(str[1]&0x3f);
	}
	if((str[0]&0xf0)==0xe0){
		if(len<3)return '?';
		if((str[1]&0xc0)!=0x80)return '?';
		if((str[2]&0xc0)!=0x80)return '?';
		*clen=3;
		return ((str[0]&0x0f)<<12)|((str[1]&0x3f)<<6)|(str[2]&0x3f);
	}
	if((str[0]&0xf8)==0xf0){
		if(len<4)return '?';
		if((str[1]&0xc0)!=0x80)return '?';
		if((str[2]&0xc0)!=0x80)return '?';
		if((str[3]&0xc0)!=0x80)return '?';
		*clen=4;
		return ((str[0]&0x07)<<18)|((str[1]&0x3f)<<12)|((str[2]&0x3f)<<6)|(str[3]&0x3f);
	}
	return '?';
}

__attribute__((format (printf, 1,2))) int tprintf(const char *format,...){
	assert(screenlive);
	if(needresize)resizeterm();
	char *buf;
	va_list ap;
	va_start(ap,format);
	int len=vasprintf(&buf,format,ap);
	va_end(ap);

	assert(buf&&len>=0);

	int startx=cursor.x;

	int i=0;
	while(i<len){
		int clen;
		int c=scanutf8char(buf+i,len-i,&clen);
		tputcstartx(c,&startx);
		i+=clen;
	}
	free(buf);
	return len;
}

void fillrect(int x,int y,int w,int h,int c){
	assert(screenlive);
	if(needresize)resizeterm();
	if(x<0){w+=x; x=0;}
	if(y<0){h+=y; y=0;}
	if(x+w>=termsize.w){w=termsize.w-x;}
	if(y+h>=termsize.h){h=termsize.h-y;}

	for(int yy=y;yy<y+h;yy++){
		for(int xx=x;xx<x+w;xx++){
			atxy(drawbuf,xx,yy).style=curstyle;
			atxy(drawbuf,xx,yy).c=c;
		}
	}
}

// writes '?' for unrepresentable (out-of-range) values in UTF-8
static void writeutf8(FILE *stream,int c){
	if(c<0){
		fputc('?',stream);
	} else if(c<(1<<7)){
		fputc(c,stream);
	} else if(c<(1<<(5+6))){
		fputc(0xc0|(c>>6),stream);
		fputc(0x80|(c&0x3f),stream);
	} else if(c<(1<<(4+6+6))){
		fputc(0xe0|(c>>12),stream);
		fputc(0x80|((c>>6)&0x3f),stream);
		fputc(0x80|(c&0x3f),stream);
	} else if(c<(1<<(3+6+6+6))){
		fputc(0xf0|(c>>18),stream);
		fputc(0x80|((c>>12)&0x3f),stream);
		fputc(0x80|((c>>6)&0x3f),stream);
		fputc(0x80|(c&0x3f),stream);
	} else {
		fputc('?',stream);
	}
}

static void redrawfullx(bool full){
	if(needresize){
		resizeterm();
		full=true;
	}
	if(redrawhandler)redrawhandler(full);
	int x,y;
	Style st;
	bool first=true;
	if(cursorisvisible)printf("\x1B[?25l"); //civis
	for(y=0;y<termsize.h;y++){
		bool shouldmove=true;
		for(x=0;x<termsize.w;x++){
			if(!full&&memcmp(&atxy(drawbuf,x,y),&atxy(screenbuf,x,y),sizeof(Screencell))==0){
				shouldmove=true;
				continue;
			}
			if(shouldmove){
				printf("\x1B[%d;%dH",y+1,x+1);
				shouldmove=false;
			}
			if(first){
				outputstyle(NULL,&atxy(drawbuf,x,y).style);
				first=false;
				st=atxy(drawbuf,x,y).style;
			} else outputstyle(&st,&atxy(drawbuf,x,y).style);
			writeutf8(stdout,atxy(drawbuf,x,y).c);
			atxy(screenbuf,x,y).style=atxy(drawbuf,x,y).style;
			atxy(screenbuf,x,y).c=atxy(drawbuf,x,y).c;
		}
	}
	printf("\x1B[%d;%dH",cursor.y+1,cursor.x+1);
	if(cursorisvisible)printf("\x1B[?12;25h"); //cvvis
	fflush(stdout);
}

void redraw(void){
	assert(screenlive);
	redrawfullx(false);
}

void redrawfull(void){
	assert(screenlive);
	redrawfullx(true);
}


void scrollterm(int x,int y,int w,int h,int amount){
	assert(screenlive);
	if(needresize)resizeterm();
	if(amount==0)return;
	if(x>=termsize.w||y>=termsize.h)return;
	if(x<0){
		w+=x;
		x=0;
	}
	if(y<0){
		h+=y;
		y=0;
	}
	if(w<=0||h<=0)return;
	int starty,endy;
	if(amount>0){
		starty=y;
		endy=y+h-amount;
	} else {
		starty=y+amount;
		endy=y+h;
	}
	for(int yy=starty;yy<endy;yy++){
		memcpy(drawbuf+w*yy+x,drawbuf+w*(yy+amount)+x,termsize.w*sizeof(Screencell));
	}
	if(amount>0){
		fillrect(x,endy,w,amount,' ');
	} else {
		fillrect(x,y,w,amount,' ');
	}
}


int getbufferchar(int x,int y){
	assert(screenlive);
	if(needresize)resizeterm();
	if(x>=termsize.w)x=termsize.w-1;
	if(x<0)x=0;
	if(y>=termsize.h)y=termsize.h-1;
	if(y<0)y=0;
	return atxy(drawbuf,x,y).c;
}


void moveto(int x,int y){
	assert(screenlive);
	if(needresize)resizeterm();
	if(x>=termsize.w)x=termsize.w-1;
	if(x<0)x=0;
	if(y>=termsize.h)y=termsize.h-1;
	if(y<0)y=0;
	cursor.x=x;
	cursor.y=y;
	//printf("\x1B[%d;%dH",y+1,x+1);
}

typedef struct Llitem{
	Position pos;
	struct Llitem *next;
} Llitem;
static Llitem *cursorstack=NULL;

void pushcursor(void){
	assert(screenlive);
	Llitem *item=malloc(sizeof(Llitem));
	assert(item);
	item->pos=cursor;
	item->next=cursorstack;
	cursorstack=item;
}

void popcursor(void){
	assert(screenlive);
	assert(cursorstack);
	Llitem *item=cursorstack;
	cursorstack=item->next;
	cursor=item->pos;
	free(item);
}


void bel(void){
	putchar('\007');
	fflush(stdout);
}

void cursorvisible(bool visible){
	if(visible!=cursorisvisible){
		if(visible)printf("\x1B[?12;25h"); // cvvis
		else printf("\x1B[?25l"); // civis
	}
	cursorisvisible=visible;
}


static int readretry(int fd,void *buf,size_t nbyte){
	int ret;
	do {
		ret=read(fd,buf,nbyte);
	} while(ret==-1&&errno==EINTR);
	return ret;
}

//- returns pointer to static buffer that is shared over calls
//- returns NULL on error; if EOF is encountered, returns string till then
//- number of characters read is stored in *nchars if no error occurred
//- [from,to] is inclusive on both sides
//- terminating character is included in string
//- maximum number of characters can be passed in maxchars (or -1 if no limit)
//- returned string is NOT null-terminated
static const char* readuntilrange(int fd,unsigned char from,unsigned char to,int maxchars,int *nchars){
	static char smallbuffer[16],*bigbuffer=NULL;
	static int bigsize=-1;
	if(maxchars==0)return NULL;
	*nchars=0;
	while(*nchars<(int)sizeof(smallbuffer)){
		int ret=readretry(fd,smallbuffer+*nchars,1);
		if(ret==-1)return NULL;
		if(ret==0)return smallbuffer;
		(*nchars)++;
		if((smallbuffer[*nchars-1]>=from&&smallbuffer[*nchars-1]<=to)||
		   (maxchars!=-1&&*nchars==maxchars)){
			return smallbuffer;
		}
	}
	if(bigbuffer==NULL){
		bigsize=2*sizeof(smallbuffer);
		bigbuffer=malloc(bigsize);
		assert(bigbuffer);
	}
	memcpy(bigbuffer,smallbuffer,sizeof(smallbuffer));
	while(true){
		if(*nchars==bigsize){
			bigsize*=2;
			bigbuffer=realloc(bigbuffer,bigsize);
			assert(bigbuffer);
		}
		int ret=readretry(fd,bigbuffer+*nchars,1);
		if(ret==-1)return NULL;
		if(ret==0)return bigbuffer;
		(*nchars)++;
		if((bigbuffer[*nchars-1]>=from&&bigbuffer[*nchars-1]<=to)||
		   (maxchars!=-1&&*nchars==maxchars)){
			return bigbuffer;
		}
	}
}

int tgetkey(void){
	static unsigned char bufchar='\0';
	if(bufchar!='\0'){
		int ret=bufchar;
		bufchar='\0';
		return ret;
	}
	unsigned char c;
	int ret=readretry(0,&c,1);
	if(ret==0)return -1; //EOF
	if(ret==-1)return -2; //error
	if(c==12&&handlerefresh){
		redrawfull();
		return tgetkey();
	}
	if(c!=27)return c;

	//escape key was pressed, now listen for escape sequence
	fd_set inset;
	FD_ZERO(&inset);
	FD_SET(0,&inset);
	struct timeval tv;
	tv.tv_sec=0;
	tv.tv_usec=100000; //100ms escape timeout
	ret=select(1,&inset,NULL,NULL,&tv);

	if(ret==0)return 27; //just escape key
	// if(ret==-1)do_something(); //in case of select error, we just continue reading

	ret=readretry(0,&c,1);
	if(ret==0)return -1; //EOF
	if(ret==-1)return -2; //error
	int addalt=0;
	if(c!='['){
		if(c==27){
			ret=readretry(0,&c,1);
			if(ret==0)return -1; //EOF
			if(ret==-1)return -2; //error
			if(c!='['){
				bufchar=c;
				return KEY_ALT+KEY_ESC;
			}
			addalt=KEY_ALT;
		} else {
			return KEY_ALT+c;
		}
	}

	const int maxsequencelen=64;
	int sequencelen;
	const char *sequence=readuntilrange(0,0x40,0x7e,maxsequencelen,&sequencelen);
	if(sequence==NULL)return -2; //read error
	if(sequencelen==maxsequencelen||sequencelen==0)return tgetkey(); //corrupted escape sequence?

	int arguments[maxsequencelen/2],narguments=0;
	for(int i=0;i<sequencelen;i++){
		if(!isdigit(sequence[i]))continue; //digits are out of the 0x40-0x7e range
		int arg=sequence[i]-'0';
		for(i++;i<sequencelen;i++){
			if(!isdigit(sequence[i]))break;
			arg=10*arg+sequence[i]-'0';
		}
		i--;
		arguments[narguments++]=arg;
	}

	char commandchar=sequence[sequencelen-1];

	switch(commandchar){
		case 'A': case 'B': case 'C': case 'D': {
			int k;
			switch(commandchar){
				case 'A': k=KEY_UP; break;
				case 'B': k=KEY_DOWN; break;
				case 'C': k=KEY_RIGHT; break;
				case 'D': k=KEY_LEFT; break;
			}
			if(narguments==2&&arguments[0]==1){
				switch(arguments[1]){
					case 2: return KEY_SHIFT+k;
					case 3: return KEY_ALT+k;
					case 5: return KEY_CTRL+k;
					case 6: return KEY_CTRLSHIFT+k;
					case 7: return KEY_CTRLALT+k;
				}
			}
			return addalt+k;
		}
		case 'Z': return addalt+KEY_SHIFTTAB;
		case '~': {
			if(narguments==0)return addalt+KEY_DELETE;
			int k=addalt;
			switch(arguments[0]){
				case 3: k+=KEY_DELETE; break;
				case 5: k+=KEY_PAGEUP; break;
				case 6: k+=KEY_PAGEDOWN; break;
			}
			if(narguments>=2)switch(arguments[1]){
				case 2: k+=KEY_SHIFT; break;
				case 5: k+=KEY_CTRL; break;
			}
			return k;
		}
	}

	//unrecognised sequence!
	bel();
	return tgetkey();
}

// TODO: Better editing capabilities
char* tgetline(void){
	int bufsz=64,buflen=0;
	char *buf=malloc(bufsz);
	assert(buf);
	buf[0]='\0';

	while(true){
		int key=tgetkey();
		switch(key){
		case KEY_ESC:
			free(buf);
			buf=NULL;
			goto cleanup_return;

		case KEY_BACKSPACE:
		case KEY_DELETE:
			fflush(stdout);
			if(buflen>0){
				printf("\x1B[D \x1B[D");
				fflush(stdout);
				buf[buflen]='\0';
				buflen--;
			} else bel();
			break;

		case KEY_LF:
		case KEY_CR:
			goto cleanup_return;

		default:
			if(key>=32&&key<127){
				if(buflen==bufsz-1){
					bufsz*=2;
					char *newbuf=realloc(buf,bufsz);
					assert(newbuf);
					buf=newbuf;
				}
				buf[buflen++]=(char)key;
				buf[buflen]='\0';
				putchar((char)key);
				fflush(stdout);
			} else bel();
			break;
		}
	}

cleanup_return:
	printf("\x1B[%dD",buflen);
	for(int i=0;i<buflen;i++)putchar(' ');
	printf("\x1B[%dD",buflen);
	fflush(stdout);
	return buf;
}
