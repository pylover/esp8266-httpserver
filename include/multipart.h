#ifndef MULTIPART_H
#define MULTIPART_H

#include "datamodel.h"


#define httpd_multipart_read(m, d, l) rb_read(&(m)->rb, (d), (l))

#endif
