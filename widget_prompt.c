#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "termio.h"

struct Promptwidget{
	int x,y,w;
	char *buf;
	int sz,len;
};


Promptwidget *prw_make(int x,int y,int w){
	Promptwidget *prw=malloc(sizeof(Promptwidget));
	if(!prw)return NULL;
	prw->sz=128;
	prw->buf=malloc(prw->sz);
	if(!prw->buf){
		free(prw);
		return NULL;
	}
	prw->len=0;
	prw->x=x;
	prw->y=y;
	prw->w=w;

	prw_redraw(prw);
	return prw;
}

void prw_destroy(Promptwidget *prw){
	free(prw->buf);
	free(prw);
}

void prw_redraw(Promptwidget *prw){
	moveto(prw->x-1,prw->y-1);
	tputc('+');
	for(int i=0;i<prw->w;i++)tputc('-');
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
			FILE *f=fopen("/dev/ttys002","w"); fprintf(f,"bs: len=%d,buf=<%s>\n",prw->len,prw->buf); fclose(f);
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
			{FILE *f=fopen("/dev/ttys002","w"); fprintf(f,"key=%d\n",key); fclose(f);}
			if(key<32||key>126)bel();
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
