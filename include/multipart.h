#ifndef MULTIPART_H
#define MULTIPART_H

#include "common.h"


httpd_err_t httpd_form_multipart_parse(struct httpd_session *s, 
        httpd_multipart_handler_t handler);
#endif
