#include "httpserver.h"
#include "debug.h"

#include <osapi.h>
#include <user_interface.h>
#include <mem.h>

#include <ets_sys.h>
#include <c_types.h>
#include <os_type.h>


// TODO: Max connection: 1
static HttpRoute *routes;
static HttpServer *server;
static char *buff_header;
static char *response_buffer;
static uint32_t response_buffer_length;


#define HTTP_RESPONSE_HEADER_FORMAT \
	"HTTP/1.0 %s\r\n" \
	"Server: lwIP/1.4.0\r\n" \
	"Expires: Fri, 10 Apr 2008 14:00:00 GMT\r\n" \
	"Pragma: no-cache\r\n" \
	"Content-Length: %d\r\n" \
	"Content-Type: %s\r\n" 


static ICACHE_FLASH_ATTR
void _cleanup_request(bool disconnect) {
	Request *req = &server->request;
	if (disconnect) {
		espconn_disconnect(req->conn);
	}
	os_memset(buff_header, 0, HTTP_HEADER_BUFFER_SIZE);
	os_memset(req, 0, sizeof(Request));
	server->status = HSS_IDLE;
}


static ICACHE_FLASH_ATTR
int httpserver_send(Request *req, char *data, uint32_t length) {
	int err = espconn_send(req->conn, data, length);
	if (err == ESPCONN_MEM) {
		os_printf("TCP Send: Out of memory\r\n");
	}
	else if (err == ESPCONN_ARG) {
		os_printf("illegal argument; cannot find network transmission \
				accordingto structure espconn\r\n");
	}
	else if (err == ESPCONN_MAXNUM) {
		os_printf("buffer (or 8 packets at most) of sending data is full\r\n");
	}
	else {
		return OK;
	}
}


static ICACHE_FLASH_ATTR
int _dispatch(char *body, uint32_t body_length) {
	Request *req = &server->request;
	HttpRoute *route = NULL;
	int16_t statuscode;
	int i;
	
	while (req->handler == NULL) {
		route = &(routes[i++]);
		if (route->pattern == NULL){
			break;	
		}
		if (matchroute(route, req)) {
			req->handler = route->handler;
			os_printf("Route found: %s %s\r\n", route->verb, route->pattern);
			break;
		}
	}
	
	if (req->handler == NULL) {
		os_printf("Not found: %s\r\n", req->path);
		return httpserver_response_notfound(req);
	}
	
	bool last = (body_length - req->contentlength) == 2;
	uint32_t chunklen = last? body_length - 2: body_length;
	req->body_cursor += chunklen;
	uint32_t more = req->contentlength - req->body_cursor;
	((Handler)req->handler)(req, body, chunklen, more);
}


static ICACHE_FLASH_ATTR
int _read_header(char *data, uint16_t length) {
	// TODO: max header length check !
	char *cursor = os_strstr(data, "\r\n\r\n");
	char *headers;
	uint16_t content_type_len;
	Request *req = &server->request;

	uint16_t l = (cursor == NULL)? length: (cursor - data) + 4;
	os_memcpy(buff_header + req->buffheader_length, data, l);
	req->buffheader_length += l;

	if (cursor == NULL) {
		cursor = os_strstr(data, "\r\n");
		if (cursor == NULL) {
			// Request for more data, incomplete http header
			return 0;
		}
	}
	
	req->verb = buff_header;
	cursor = os_strchr(buff_header, ' ');
	cursor[0] = 0;

	req->path = ++cursor;
	cursor = os_strchr(cursor, ' ');
	cursor[0] = 0;
	headers = cursor + 1;

	req->contenttype = os_strstr(headers, "Content-Type:");
	if (req->contenttype != NULL) {
		cursor = os_strstr(req->contenttype, "\r\n");
		if (cursor == NULL) {
			return -1;
		}
		content_type_len = cursor - req->contenttype;
	}

	cursor = os_strstr(headers, "Content-Length:");
	if (cursor != NULL) {
		req->contentlength = atoi(cursor + 16);
		cursor = os_strstr(cursor, "\r\n");
		if (cursor == NULL) {
			return -2;
		}
	}

	// Terminating strings
	if (req->contenttype != NULL) {
		req->contenttype[content_type_len] = 0;
	}
	return l;
}


static ICACHE_FLASH_ATTR
void _client_recv(void *arg, char *data, uint16_t length) {
	uint16_t remaining;
	int readsize;
    struct espconn *conn = arg;
	Request *req = &server->request;
	req->conn = (struct espconn*) arg; 
	
	if (server->status < HSS_REQ_BODY) {
		server->status = HSS_REQ_HEADER;
		readsize = _read_header(data, length);
		if (readsize < 0) {
			os_printf("Invalid Header: %d\r\n", readsize);
			httpserver_response_badrequest(req);
			return;
		}

		if (readsize == 0) {
			// Incomplete header
			os_printf("Incomplete Header: %d\r\n", readsize);
			return;
		}

		remaining = length - readsize;
		os_printf("--> %s %s type: %s length: %d, remaining: %d-%d=%d\r\n", 
				req->verb,
				req->path,
				req->contenttype,
				req->contentlength,
				length,
				readsize,
				remaining
		);
		server->status = HSS_REQ_BODY;
	}
	else {
		remaining = length;
	}
	
	_dispatch(data + (length-remaining), remaining);
}


static ICACHE_FLASH_ATTR
void _client_recon(void *arg, int8_t err) {
    struct espconn *conn = arg;
	os_printf("HTTPServer "IP_FORMAT" err %d reconnecting...\r\n",  
			unpack_tcp(conn->proto.tcp),
			err
		);
}


static ICACHE_FLASH_ATTR
void _client_disconnected(void *arg) {
    struct espconn *conn = arg;
	os_printf("Client "IP_FORMAT" has been disconnected.\r\n",  
			unpack_tcp(conn->proto.tcp)
		);
}


static ICACHE_FLASH_ATTR
void _client_connected(void *arg)
{
    struct espconn *conn = arg;
    espconn_regist_recvcb(conn, _client_recv);
    espconn_regist_disconcb(conn, _client_disconnected);
}


ICACHE_FLASH_ATTR
int httpserver_response_start(Request *req, char *status, char *contenttype, 
		uint32_t contentlength, char **headers, uint8_t headers_count) {
	int i;
	response_buffer_length = os_sprintf(response_buffer, 
			HTTP_RESPONSE_HEADER_FORMAT, status, contentlength, 
			contenttype);

	for (i = 0; i < headers_count; i++) {
		response_buffer_length += os_sprintf(
				response_buffer + response_buffer_length, "%s\r\n", 
				headers[i]);
	}
	response_buffer_length += os_sprintf(
			response_buffer + response_buffer_length, "\r\n");

	return OK;
}


ICACHE_FLASH_ATTR
int httpserver_response_finalize(Request *req, char *body, uint32_t body_length) {
	if (body_length > 0) {
		os_memcpy(response_buffer + response_buffer_length, body, 
				body_length);
		response_buffer_length += body_length;
		response_buffer_length += os_sprintf(
				response_buffer + response_buffer_length, "\r\n");
	}
	response_buffer_length += os_sprintf(
			response_buffer + response_buffer_length, "\r\n");

	httpserver_send(req, response_buffer, response_buffer_length);
	_cleanup_request(false);
}


ICACHE_FLASH_ATTR
int httpserver_response(Request *req, char *status, char *contenttype, 
		char *content, uint32_t contentlength, char **headers, 
		uint8_t headers_count) {
	httpserver_response_start(req, status, contenttype, contentlength, headers, 
			headers_count);
	httpserver_response_finalize(req, content, contentlength);
}


ICACHE_FLASH_ATTR
void httpserver_parse_querystring(const char *form, 
		QueryStringCallback callback) {
	char *field = (char*)&form[0];
	char *value;
	char *tmp;

	while (true) {
		value = os_strstr(field, "=") + 1;
		(value-1)[0] = 0;
		tmp  = os_strstr(value, "&");
		if (tmp != NULL) {
			tmp[0] = 0;
		}
		callback(field, value);
		if (tmp == NULL) {
			return;
		}
		field = tmp + 1;
	}
}


ICACHE_FLASH_ATTR 
int httpserver_init(uint16_t port, HttpRoute routes_[]) {
	routes = routes_;
	buff_header = (char*)os_zalloc(HTTP_HEADER_BUFFER_SIZE);
	response_buffer = (char*)os_zalloc(HTTP_RESPONSE_BUFFER_SIZE);
	server = os_zalloc(sizeof(HttpServer));

	server->status = HSS_IDLE;
    server->connection.type = ESPCONN_TCP;
    server->connection.state = ESPCONN_NONE;
    server->connection.proto.tcp = &server->esptcp;
    server->connection.proto.tcp->local_port = port;
	os_printf("HTTP Server is listening on: "IP_FORMAT"\r\n",  
			unpack_tcp(server->connection.proto.tcp)
	);

	espconn_regist_connectcb(&server->connection, _client_connected);
    espconn_regist_reconcb(&server->connection, _client_recon);
	espconn_tcp_set_max_con_allow(&server->connection, 1);
	espconn_regist_time(&server->connection, HTTPSERVER_TIMEOUT, 1);
	espconn_set_opt(&server->connection, ESPCONN_NODELAY);
	espconn_accept(&server->connection);
	return OK;
}


ICACHE_FLASH_ATTR
void httpserver_stop() {
	espconn_disconnect(&server->connection);
	espconn_delete(&server->connection);
	if (buff_header != NULL) {
		os_free(buff_header);
	}

	if (response_buffer != NULL) {
		os_free(response_buffer);
	}

	if (server != NULL) {
		os_free(server);
		server = NULL;
	}
}

