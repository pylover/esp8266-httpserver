#ifndef QUERYSTRING_H
#define QUERYSTRING_H

typedef void (*QueryStringCallback)(const char*, const char*);

void querystring_parse(
        char *form, 
        QueryStringCallback callback
    );


#endif
