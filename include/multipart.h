#ifndef MULTIPART_H
#define MULTIPART_H

#include "session.h"


struct httpd_multipart {
    struct httpd_session *session;
    void *handlerbackup;
    void *handler;
};


typedef httpd_err_t (*httpd_multipart_handler_t)(struct httpd_multipart *m);

#endif
