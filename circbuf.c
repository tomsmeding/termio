#include <stdlib.h>
#include <assert.h>

#include "circbuf.h"

typedef struct Circbuf{
	char **data;
	int sz,len,base;
	void (*deleteitem)(void*);
} Circbuf;


Circbuf* cb_make(int size,void (*deleteitem)(void*)){
	Circbuf *cb=malloc(sizeof(Circbuf));
	if(!cb)return NULL;
	cb->data=calloc(size,sizeof(char*));
	if(!cb->data){
		free(cb);
		return NULL;
	}
	cb->sz=size;
	cb->len=0;
	cb->base=0;
	cb->deleteitem=deleteitem;
	return cb;
}

void cb_destroy(Circbuf *cb){
	cb_clear(cb);
	free(cb);
}

void cb_append(Circbuf *cb,char *item){
	int index=(cb->base+cb->len)%cb->sz;
	if(cb->data[index])cb->deleteitem(cb->data[index]);
	cb->data[index]=item;
	if(cb->len==cb->sz)cb->base=(cb->base+1)%cb->sz;
	else cb->len++;
}

void cb_clear(Circbuf *cb){
	for(int i=0;i<cb->len;i++){
		cb->deleteitem(cb->data[(cb->base+i)%cb->sz]);
		cb->data[(cb->base+i)%cb->sz]=NULL;
	}
	cb->len=0;
}

char* cb_get(Circbuf *cb,int index){
	assert(-cb->len<=index&&index<cb->len);
	if(index<0)index+=cb->len;
	return cb->data[(cb->base+index)%cb->sz];
}

int cb_length(Circbuf *cb){
	return cb->len;
}
