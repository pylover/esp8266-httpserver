#include "request.h"
#include "session.h"


static ICACHE_FLASH_ATTR 
httpd_err_t httpd_request_header_parse(struct httpd_request *r, char *c) {
    struct httpd_header *h;
    httpd_err_t retval = HTTPD_OK;
    char *e;
    int i = 0;
    while (true) {
        /* Headers's CR */
        e = os_strstr(c, CR);
        if (e == NULL) {
            return HTTPD_ERR_BADHEADER;
        }
        if (e == c) {
            /* Last header */
            break;
        }
        
        /* Overflow */
        if (i >= HTTPD_REQ_HEADERS_MAX) {
            return HTTPD_ERR_MAXHEADER;
        }
        
        /* Null terminate the line */
        e[0] = 0;
       
        /* Headers reallocate */
        r->headers = (struct httpd_header *) os_realloc(r->headers, 
            sizeof(struct httpd_header) * (i + 1));

        /* Header name */
        h = &r->headers[i];
        h->name = c;
        c = os_strstr(c, ": ");
        if (c == NULL) {
            return HTTPD_ERR_BADHEADER;
        }
        c[0] = 0;
        
        /* Header value */
        h->value = c + 2;
        
        /* Prepare for the next iteration */
        c = e + 2;
        
        if (strcasecmp(h->name, "content-type") == 0) {
            /* content length */
            r->contenttype = h->value;
        }
        else if (strcasecmp(h->name, "content-length") == 0) {
            /* content length */
            r->contentlen = atoi(h->value);
        }
        else if ((strcasecmp(h->name, "connection") == 0) &&
            (strcasecmp(h->value, "keep-alive") == 0)) {
            /* connectino keep-alive / close */
            r->keepalive = true;
        }
        else if ((strcasecmp(h->name, "expect") == 0) &&
            (strcasecmp(h->value, "100-continue") == 0)) {
            /* Expect: continue */
            retval = HTTPD_ERR_HTTPCONTINUE;        
        }
        else {
            r->headerscount = ++i;
        }
    }
    return retval;
}


ICACHE_FLASH_ATTR 
httpd_err_t httpd_request_parse(struct httpd_session *s) {
    size16_t len; 
    httpd_err_t err;
    struct httpd_request *r = &s->request;
    char *c = s->request.header_buff;  // Cursor
    char *e;  // Line end
    
    /* Ignore primitive CRs */
    while (true) {
        /* Search for \r\n or just \n */
        err = HTTPD_RECV_UNTIL(s, c, 2, "\n", 1, &len);
        if (err == RB_ERR_NOTFOUND) {
            break;
        }
    }
    
    /* Read entire header into buffer. */
    err = HTTPD_RECV_UNTIL(s, c, HTTPD_REQ_HEADERSIZE, CR CR, 4, &len);
    if (err == RB_ERR_NOTFOUND) {
        return HTTPD_MORE;
    }

    /* Parsing start line.
     * (https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html)
     */

    /* Startline's CR */
    e = os_strstr(c, CR);
    if (e == NULL) {
        return HTTPD_ERR_BADSTARTLINE;
    }
    e[0] = 0;
   
    /* Verb */
    r->verb = c;
    c = os_strchr(c, ' ');
    if (c == NULL) {
        return HTTPD_ERR_BADSTARTLINE;
    }
    c[0] = 0;

    /* Path */
    r->path = ++c;
    c = os_strchr(c, ' ');
    if (c != NULL) {
        c[0] = 0;
        c++;
    } 

    /* Query string */
    c = os_strchr(r->path, '?');
    if (c != NULL) {
        c[0] = 0;
        r->query = ++c;
    }
    c = e + 2;
    
    /* Parse headers */
    err = httpd_request_header_parse(&s->request, c);
    if (err) {
        return err;
    }

    INFO("%s %s type: %s length: %d", r->verb, r->path, 
            r->contenttype, r->contentlen);

    return HTTPD_OK;
}


