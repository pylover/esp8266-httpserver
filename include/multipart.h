#ifndef MULTIPART_H
#define MULTIPART_H

#include "common.h"
#include "ringbuffer.h"
#include <c_types.h>

#define MP_FIELDNAME_MAXLEN	128
#define MP_BUFFERSIZE	2048


typedef enum {
	MP_IDLE,
	MP_FIELDHEADER,
	MP_FIELDBODY
} MultipartStatus;


typedef struct {
	char name[MP_FIELDNAME_MAXLEN];
	char type[MP_FIELDNAME_MAXLEN];
	char filename[MP_FIELDNAME_MAXLEN];
} MultipartField;


typedef void (*MultipartCallback)(MultipartField*, char* body, 
		Size bodylen, bool last);


typedef struct {
	MultipartStatus status;
	MultipartCallback callback;
	MultipartField field;
	char *boundary;
	unsigned char boundarylen;
} Multipart;


typedef enum {
	MP_OK				= 0,
	MP_MORE				= -1,
	MP_NOBOUNDARY		= -2,
	MP_INVALIDBOUNDARY	= -3,
	MP_INVALIDHEADER	= -4,
	MP_DONE				= -6
} MultipartError;


int mp_init(Multipart *mp, char *contenttype, MultipartCallback callback);
int mp_feed(Multipart *mp, char *data, Size datalen);
int mp_feedbybuffer(Multipart *mp, RingBuffer *b);
void mp_close(Multipart *mp);

#endif
