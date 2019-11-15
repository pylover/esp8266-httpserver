#include <osapi.h>
#include <mem.h>

#include "multipart.h"


static ICACHE_FLASH_ATTR
int _parse_header(Multipart *mp, char *data, Size datalen) {
	char *line = data;
	char *lineend;
	char *temp;
	char *end;
	
	os_memset(&mp->field, 0, sizeof(MultipartField));

	if ((lineend = os_strstr(line, "\r\n")) == NULL) {
		return MP_MORE;
	}
	lineend[0] = '\0';
	
	if ((os_strncmp(line, "--", 2) != 0) || 
		(os_strncmp(line + 2, mp->boundary, mp->boundarylen) != 0)) {
		return MP_INVALIDBOUNDARY;
	}

	// DONE state, the last boundary
	if (os_strncmp(line + mp->boundarylen + 2, "--", 2) == 0) {
		return MP_DONE;
	}

	// Content-Disposition
	line = lineend + 2;
	if ((lineend = os_strstr(line, "\r\n")) == NULL) {
		return MP_MORE;
	}
	lineend[0] = '\0';

	if ((line = os_strstr(line, "Content-Disposition: ")) == NULL) {
		return MP_INVALIDHEADER;
	}
	line += 30;

	// Field's name
	if ((temp = os_strstr(line, "name=\"")) == NULL) {
		return MP_INVALIDHEADER;
	}
	temp += 6;
	if ((end = os_strstr(temp, "\"")) == NULL ) {
		return MP_INVALIDHEADER;
	}
	os_strncpy(mp->field.name, temp, end - temp);
	
	// Field's filename
	if ((temp = os_strstr(line, "filename=\"")) != NULL) {
		temp += 10;
		if ((end = os_strstr(temp, "\"")) == NULL ) {
			return MP_INVALIDHEADER;
		}
		os_strncpy(mp->field.filename, temp, end - temp);
	}
	
	if (os_strncmp(lineend + 2, "\r\n", 2) != 0) {
		// Content-Type
		line = lineend + 2;
		if ((lineend = os_strstr(line, "\r\n")) == NULL) {
			return MP_MORE;
		}
		lineend[0] = '\0';
	
		if ((temp = os_strstr(line, "Content-Type: ")) == NULL) {
			return MP_INVALIDHEADER;
		}
		temp += 14;
		os_strncpy(mp->field.type, temp, lineend - temp);
	}
	return (lineend + 4) - data;
}


static ICACHE_FLASH_ATTR
int _feed(Multipart *mp, char *data, Size datalen, Size *used) {
	int err;
	bool last = false;
	int bodylen = datalen;
	char *body = data;
	char *nextfield = data;
	*used = 0;
	
	do {
		if (last) {
			bodylen = datalen - (nextfield - data);
			last = false;
			body = nextfield;
		}
		switch (mp->status) {
			case MP_FIELDHEADER:
				if ((err = _parse_header(mp, nextfield, datalen)) < MP_OK) {
					return err;
				}
				mp->status = MP_FIELDBODY;
				body += err;
				*used += err;
				bodylen -= err;
			
			case MP_FIELDBODY:
				if ((nextfield = memmem(body, bodylen, "\r\n--", 4)) != NULL) {
					if (os_strncmp(nextfield + 4, mp->boundary, 
								mp->boundarylen) == 0) {
						bodylen = nextfield - body;
						last = true;
						mp->status = MP_FIELDHEADER;
						nextfield += 2;
					}
					//else {
					//	int remaining = datalen - (nextfield - data);
					//	bodylen -= remaining;
					//	nextfield = NULL;
					//}
				}

				if (bodylen < 1) {
					return MP_MORE;
				}

				*used += bodylen + (last? 2: 0);
				mp->callback(&mp->field, body, bodylen, last);
				break;

			case MP_IDLE:
				nextfield = NULL;
		}
	} while (nextfield != NULL);
	return MP_MORE;
}


ICACHE_FLASH_ATTR
int mp_feedbybuffer(Multipart *mp, RingBuffer *b) {
	int err;
	Size used = 0;
	Size bufflen = rb_used(b);
	char temp[bufflen + 1];
	rb_drypop(b, temp, bufflen);
	temp[bufflen] = '\0';
	err = _feed(mp, temp, bufflen, &used);
	rb_skip(b, used);
	return err;
}



ICACHE_FLASH_ATTR
int mp_init(Multipart *mp, char *contenttype, MultipartCallback callback) {
	char *e;
	char *b = os_strstr(contenttype, "boundary");
	if (b == NULL) {
		return MP_NOBOUNDARY;
	}

	b += 9;

	e = os_strstr(b, "\r\n");
	if (e == NULL) {
		mp->boundarylen = os_strlen(b);
	}
	else {
		mp->boundarylen = e - b;
	}
	
	mp->boundary = (char*)os_malloc(mp->boundarylen + 1);
	strncpy(mp->boundary, b, mp->boundarylen);
	mp->boundary[mp->boundarylen] = '\0';
	mp->status = MP_FIELDHEADER;
	mp->callback = callback;
	return MP_OK;
}

 
ICACHE_FLASH_ATTR
void mp_close(Multipart *mp) {
	os_free(mp->boundary);
	mp->status = MP_IDLE;
}


