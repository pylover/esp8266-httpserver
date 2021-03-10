#ifndef HTTPD_H_
#define HTTPD_H_

#include "datamodel.h"
#include "response.h"
#include "request.h"

//#include "session.h"
//#include "router.h"
//#include "multipart.h"


httpd_err_t httpd_init();
void httpd_deinit();
void httpd_recv(struct httpd_session *s);
#endif
