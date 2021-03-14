#include "querystring.h"
#include "request.h"


static ICACHE_FLASH_ATTR
uint8_t h2int(char c) {
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


ICACHE_FLASH_ATTR
void httpd_querystring_decode(char *s) {
    int slen, si, ti = 0;
    char c, d;
    char *temp;
    if (s == NULL) {
        return;
    }
    
    slen = strlen(s);
    if (slen <= 0) {
        return;
    }

    temp = os_zalloc(slen);
    
    
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
httpd_err_t httpd_querystring_parse(struct httpd_session *s, 
        httpd_querystring_cb cb) {
    httpd_err_t err;
    char *field = s->request.query;
    char *value;
    char *tmp;
    
    while (field) {
        value = os_strstr(field, "=");
        value[0] = 0;
        value++;

        tmp  = os_strstr(value, "&");
        if (tmp != NULL) {
            tmp[0] = 0;
        }
        httpd_querystring_decode(value);
        err = cb(s, field, value);
        if (err) {
            return err;
        }
        if (tmp == NULL) {
            return;
        }
        field = tmp + 1;
    }
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR
httpd_err_t httpd_form_urlencoded_parse(struct httpd_session *s, 
        httpd_querystring_cb cb) {
    httpd_err_t err;
    size16_t l;
    char name[HTTPD_QS_NAME_MAX + 1];
    char value[HTTPD_QS_VALUE_MAX + 1];

    while (true) {
        /* eq */
        err = HTTPD_RECV_UNTIL_CHR(s, name, HTTPD_QS_NAME_MAX, '=', &l);
        if (err) {
            /* No querystring found */
            break;
        }
        name[l-1] = 0;
        /* amp */
        err = HTTPD_RECV_UNTIL_CHR(s, value, HTTPD_QS_VALUE_MAX, '&', &l);
        if (err == RB_OK) {
            value[l-1] = 0;
        }
        else {
            value[l] = 0;
        }
        /* decode */
        httpd_querystring_decode(value);

        /* call handler */
        err = cb(s, name, value);
        if (err) {
            return err;
        }
    }
    return HTTPD_OK;
}

