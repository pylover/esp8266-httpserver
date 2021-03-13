#ifndef HTTPD_H
#define HTTPD_H

#include "common.h"
#include "response.h"
#include "request.h"
#include "multipart.h"


httpd_err_t httpd_init();
void httpd_deinit();
#endif
