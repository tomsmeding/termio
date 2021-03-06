#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "termio.h"
#include "circbuf.h"

// #define DBG(...) do {FILE *f__=fopen("/dev/ttys002","w"); fprintf(f__,__VA_ARGS__); fclose(f__);} while(0)
#define DBG(...)

struct Logwidget{
	Circbuf *cb;
	int nrows;
	int x,y,w,h;
	char *title;
	bool timestamps;
};


static void lgw_drawborder(Logwidget *lgw){
	moveto(lgw->x,lgw->y);
	tputc('+');
	tputc('-');
	int len=tprintf("%s",lgw->title);
	for(int i=len+2;i<lgw->w-1;i++)tputc('-');
	tputc('+');
	for(int i=1;i<lgw->h-1;i++){
		moveto(lgw->x,lgw->y+i);
		tputc('|');
		moveto(lgw->x+lgw->w-1,lgw->y+i);
		tputc('|');
	}
	moveto(lgw->x,lgw->y+lgw->h-1);
	tputc('+');
	for(int i=1;i<lgw->w-1;i++)tputc('-');
	tputc('+');
}

Logwidget* lgw_make(int x,int y,int w,int h,const char *title,bool timestamps){
	assert(x>=0&&y>=0);
	assert(w>=3&&h>=3);
	Logwidget *lgw=malloc(sizeof(Logwidget));
	if(!lgw)return NULL;
	lgw->cb=cb_make(h-2,free);
	if(!lgw->cb){
		free(lgw);
		return NULL;
	}
	lgw->nrows=h-2;
	lgw->x=x;
	lgw->y=y;
	lgw->w=w;
	lgw->h=h;

	lgw->title=NULL;
	lgw_changetitle(lgw,title);

	lgw->timestamps=timestamps;

	return lgw;
}

void lgw_destroy(Logwidget *lgw){
	cb_destroy(lgw->cb);
	if(lgw->title)free(lgw->title);
	free(lgw);
}

void lgw_redraw(Logwidget *lgw){
	pushcursor();
	lgw_drawborder(lgw);

	int len=cb_length(lgw->cb);
	DBG("len=%d nrows=%d\n",len,lgw->nrows);
	int i;
	for(i=0;i<len;i++){
		moveto(lgw->x+1,lgw->y+1+i);
		char *line=cb_get(lgw->cb,i);
		int linelen=tprintf("%s",line);
		DBG("line=<%s>  linelen=%d\n",line,linelen);
		for(int j=linelen;j<lgw->w-2;j++)tputc(' ');
	}
	for(;i<lgw->nrows;i++){
		moveto(lgw->x+1,lgw->y+1+i);
		for(int j=0;j<lgw->w-2;j++)tputc(' ');
	}
	popcursor();
}

static const char* formatTime(time_t tt) {
	static char buf[6];
	struct tm *timeinfo=localtime(&tt);
	strftime(buf,6,"%H:%M",timeinfo);
	return buf;
}

void lgw_add(Logwidget *lgw,const char *line){
	char *allocLine=NULL;
	if(lgw->timestamps){
		asprintf(&allocLine,"%s %s",formatTime(time(NULL)),line);
		assert(allocLine!=NULL);
		line=allocLine;
	}

	int len=strlen(line);
	while(len>0){
		int copylen=lgw->w-2;
		if(copylen>len)copylen=len;
		char *copy=calloc(copylen+1,1);
		assert(copy);
		for(int i=0,j=0;i<len&&j<copylen;i++){
			if(line[i]>=32&&line[i]<127)copy[j++]=line[i];
		}
		cb_append(lgw->cb,copy);
		len-=copylen;
		line+=copylen;
	}

	if(allocLine!=NULL){
		free(allocLine);
	}

	lgw_redraw(lgw);
}

__attribute__((format (printf, 2,3))) void lgw_addf(Logwidget *lgw,const char *format,...){
	char *str;
	va_list ap;
	va_start(ap,format);
	vasprintf(&str,format,ap);
	va_end(ap);
	assert(str);
	lgw_add(lgw,str);
	free(str);
}

void lgw_clear(Logwidget *lgw){
	cb_clear(lgw->cb);
	lgw_redraw(lgw);
}

void lgw_changetitle(Logwidget *lgw,const char *title){
	if(lgw->title)free(lgw->title);
	if(title==NULL)title="";
	lgw->title=strdup(title);
	assert(lgw->title);
	if((int)strlen(lgw->title)>lgw->w-2)lgw->title[lgw->w]='\0';
	lgw_redraw(lgw);
}
