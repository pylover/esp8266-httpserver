#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ringbuffer.h"
#include "multipart.h"


char *sample = 
	"-----------------------------9051914041544843365972754266\r\n"
	"Content-Disposition: form-data; name=\"a\"\r\n"
	"\r\n"
	"b\r\n"
	"-----------------------------9051914041544843365972754266--\r\n";


#define CONTENTTYPE	"Content-Type: multipart/form-data; boundary=" \
	"---------------------------9051914041544843365972754266\r\n"

#define CONTENTLEN	165
#define BUFFSIZE	2048

static char buff[BUFFSIZE];
static RingBuffer rb = {BUFFSIZE, 0, 0, buff};


void cb(MultipartField *f, char *body, Size bodylen, bool last) {
	char b[bodylen + 1];
	memset(b, 0, bodylen + 1);
	strncpy(b, body, bodylen);
	printf("CB: Field: %s, last: %d, type: %s, filename: %s, len: %d\r\n%s\r\n",
			f->name, last, f->type, f->filename, bodylen, b);
}


int _feed(Multipart *mp, char *data, int offset, Size datalen) {
	char temp[2048];
	memset(temp, 0, 2048);
	strncpy(temp, data + offset, datalen);
	printf("#**********Start Feeding:\r\n%s\r\n#**********End\r\n", temp);
	rb_safepush(&rb, temp, datalen);
	return mp_feedbybuffer(mp, &rb);
	//return mp_feed(mp, temp, datalen);
}


void test_multipart_short_whole() {
	int err;
	Multipart mp;
	rb_reset(&rb);
	
	mp_init(&mp, CONTENTTYPE, cb);
	if ((err = _feed(&mp, sample, 0, strlen(sample))) != MP_DONE) {
		goto failed;
	}
	mp_close(&mp);
	return;

failed:
	printf("Failed: %d.\r\n", err);
	mp_close(&mp);
}




int main() {
	test_multipart_short_whole();
}

