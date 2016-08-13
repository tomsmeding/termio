#pragma once

struct Circbuf;
typedef struct Circbuf Circbuf;


Circbuf* cb_make(int size,void (*deleteitem)(void*));
void cb_destroy(Circbuf *cb);
void cb_append(Circbuf *cb,void *item);
void cb_clear(Circbuf *cb);
void* cb_get(Circbuf *cb,int index);
int cb_length(Circbuf *cb);
