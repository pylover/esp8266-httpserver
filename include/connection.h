#ifndef CONNECTION_H
#define CONNECTION_H

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>


struct httpd_conn{
    struct espconn *conn;
    uint8_t id;

    char *req_buff;
    uint16_t req_buff_len;

    char *resp_headerbuff;
    uint16_t resp_headerbuff_len;

    char *resp_buff;
    uint16_t resp_buff_len;
    
    uint32_t body_cursor;
};


#endif
