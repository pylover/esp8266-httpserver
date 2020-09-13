#include <osapi.h>
#include <mem.h>
#include <c_types.h>

#include "querystring.h"


unsigned char h2int(char c) {
    if (c >= '0' && c <='9'){
      return (unsigned char)c - '0';
    }
    if (c >= 'a' && c <='f'){
      return (unsigned char)c - 'a' + 10;
    }
    if (c >= 'A' && c <='F'){
      return (unsigned char)c - 'A' + 10;
    }
    return 0;
}


void querystring_decode(char *s) {
    int si, ti = 0;
    int slen = strlen(s);
    char c;
    char *temp = os_zalloc(slen);
    char d;
    
    for (si = 0; si < slen; si++) {
        c = s[si];
        if (c != '%') {
            d = c;
        }
        else {
            /* Decode */
            d = h2int(s[si+1]) << 4;
            d |= h2int(s[si+2]);
            si += 2;
        }
        
        temp[ti++] = d;
    }
    temp[ti++] = 0;
    os_strncpy(s, temp, ti);
    os_free(temp);
}


ICACHE_FLASH_ATTR
void querystring_parse(char *form, 
		QueryStringCallback callback) {
	char *field = form;
	char *value;
	char *tmp;

	while (true) {
		value = os_strstr(field, "=");
        value[0] = 0;
        value++;

		tmp  = os_strstr(value, "&");
		if (tmp != NULL) {
			tmp[0] = 0;
		}
        querystring_decode(value);
		callback(field, value);
		if (tmp == NULL) {
			return;
		}
		field = tmp + 1;
	}
}


