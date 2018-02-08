#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "termio.h"

struct Menuwidget{
	int x,y,choice;
	const Menudata *data;
};

Menuwidget* menu_make(int basex,int basey,const Menudata *data){
	Menuwidget *mw=malloc(sizeof(Menuwidget));
	if(!mw)return NULL;
	mw->x=basex;
	mw->y=basey;
	mw->choice=0;
	mw->data=data;
	menu_redraw(mw);
	return mw;
}

void menu_destroy(Menuwidget *mw){
	assert(mw);
	free(mw);
}

void menu_redraw(Menuwidget *mw){
	assert(mw);
	for(int i=0;i<mw->data->nitems;i++){
		moveto(mw->x,mw->y+i);
		tprintf("> %s",mw->data->items[i].text);
		if(mw->data->items[i].hotkey!='\0'){
			tputc(' ');
			tputc('(');
			setbold(true);
			tputc(mw->data->items[i].hotkey);
			setbold(false);
			tputc(')');
		}
	}
	moveto(mw->x,mw->y+mw->choice);
}

Menukey menu_handlekey(Menuwidget *mw,int key){
	switch(key){
		case 'k': case KEY_UP:
			if(mw->choice>0){
				mw->choice--;
				menu_redraw(mw);
			} else bel();
			return MENUKEY_HANDLED;

		case 'j': case KEY_DOWN:
			if(mw->choice<mw->data->nitems-1){
				mw->choice++;
				menu_redraw(mw);
			} else bel();
			return MENUKEY_HANDLED;

		case '\n':
			if(mw->data->items[mw->choice].func){
				mw->data->items[mw->choice].func(mw->choice);
				return MENUKEY_CALLED;
			} else return MENUKEY_QUIT;

		default: {
			int i;
			for(i=0;i<mw->data->nitems;i++){
				if(mw->data->items[i].hotkey==key)break;
			}
			if(i==mw->data->nitems)return MENUKEY_IGNORED;
			else {
				mw->choice=i;
				if(mw->data->items[i].func==NULL)return MENUKEY_QUIT;
				mw->data->items[i].func(i);
				return MENUKEY_CALLED;
			}
		}
	}
}
