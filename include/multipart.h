#ifndef MULTIPART_H
#define MULTIPART_H

#include "session.h"
#include <c_types.h>


typedef httpd_err_t (*httpd_multipart_handler_t)(struct httpd_multipart *m);


struct httpd_multipart {
    struct httpd_session *s;
    void *handlerbackup;
    void *handler;
};


#endif
