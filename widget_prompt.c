#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "termio.h"

struct Promptwidget{
	int x,y,w;
	char *buf;
	int sz,len;
	char *title;
};


Promptwidget *prw_make(int x,int y,int w,const char *title){
	Promptwidget *prw=malloc(sizeof(Promptwidget));
	if(!prw)return NULL;
	prw->sz=128;
	prw->buf=malloc(prw->sz);
	if(!prw->buf){
		free(prw);
		return NULL;
	}
	prw->buf[0]='\0';
	prw->len=0;
	prw->x=x;
	prw->y=y;
	prw->w=w;
	prw->title=NULL;
	prw_changetitle(prw,title);

	prw_redraw(prw);
	return prw;
}

void prw_destroy(Promptwidget *prw){
	if(prw->title)free(prw->title);
	free(prw->buf);
	free(prw);
}

void prw_redraw(Promptwidget *prw){
	moveto(prw->x-1,prw->y-1);
	tputc('+');
	int len=tprintf("%s",prw->title);
	for(int i=len;i<prw->w;i++)tputc('-');
	tputc('+');

	moveto(prw->x-1,prw->y);
	tputc('|');
	const char *start=prw->len<prw->w?prw->buf:prw->buf+(prw->len+1-prw->w);
	const int startlen=prw->len-(start-prw->buf);
	tprintf("%s",start);
	for(int i=startlen;i<prw->w;i++)tputc(' ');
	tputc('|');

	moveto(prw->x-1,prw->y+1);
	tputc('+');
	for(int i=0;i<prw->w;i++)tputc('-');
	tputc('+');

	moveto(prw->x+startlen,prw->y);
}

char* prw_handlekey(Promptwidget *prw,int key){
	switch(key){
		case KEY_BACKSPACE:
		case KEY_DELETE:
			if(prw->len>0){
				prw->len--;
				prw->buf[prw->len]='\0';
			}
			prw_redraw(prw);
			break;

		case KEY_LF:
		case KEY_CR: {
			char *ret=prw->buf;
			prw->buf=malloc(prw->sz);
			assert(prw->buf);
			prw->buf[0]='\0';
			prw->len=0;
			prw_redraw(prw);
			return ret;
		}

		default:
			if (key == 21) { // ^U
				prw->buf[0]='\0';
				prw->len=0;
			} else if(key<32||key>126)bel();
			else {
				if(prw->len>=prw->sz-1){
					prw->sz*=2;
					char *newbuf=realloc(prw->buf,prw->sz);
					assert(newbuf);
					prw->buf=newbuf;
				}
				prw->buf[prw->len++]=key;
				prw->buf[prw->len]='\0';
			}
			prw_redraw(prw);
			break;
	}
	return NULL;
}

void prw_changetitle(Promptwidget *prw,const char *title){
	if(prw->title)free(prw->title);
	if(title==NULL)title="";
	prw->title=strdup(title);
	assert(prw->title);
	if((int)strlen(prw->title)>prw->w-2)prw->title[prw->w]='\0';
	prw_redraw(prw);
}
