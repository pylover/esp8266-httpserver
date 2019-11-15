#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ringbuffer.h"
#include "multipart.h"


char *sample = 
	"-----------------------------9051914041544843365972754266\r\n"
	"Content-Disposition: form-data; name=\"text\"\r\n"
	"\r\n"
	"text default\r\n"
	"-----------------------------9051914041544843365972754266\r\n"
	"Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r\n"
	"Content-Type: text/plain\r\n"
	"\r\n"
	"Content of a.txt.\r\n"
	"\r\n"
	"-----------------------------9051914041544843365972754266\r\n"
	"Content-Disposition: form-data; name=\"file2\"; filename=\"a.html\"\r\n"
	"Content-Type: text/html\r\n"
	"\r\n"
	"<!DOCTYPE html><title>Content of a.html.</title>\r\n"
	"\r\n"
	"-----------------------------9051914041544843365972754266--\r\n";


#define CONTENTTYPE	"Content-Type: multipart/form-data; boundary=" \
	"---------------------------9051914041544843365972754266\r\n"

#define CONTENTLEN	554
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


void test_multipart_chunked() {
	int err;
	Multipart mp;
	rb_reset(&rb);
	
	if((err = mp_init(&mp, CONTENTTYPE, cb)) != MP_OK) {
		printf("Cannot init: %d\r\n", err);
		goto failed;
	}
	printf("Boundary: %d:%s\r\n", mp.boundarylen, mp.boundary);
	
	if ((err = _feed(&mp, sample, 0, 50)) != MP_MORE) {
		goto failed;
	}

	if ((err = _feed(&mp, sample, 50, 70)) != MP_MORE) {
		goto failed;
	}

	if ((err = _feed(&mp, sample, 120, strlen(sample) - 120)) != MP_DONE) {
		goto failed;
	}
	
	mp_close(&mp);

	return;

failed:
	printf("Failed: %d.\r\n", err);
	mp_close(&mp);
}


void test_multipart_whole() {
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
	test_multipart_chunked();
	test_multipart_whole();
}

