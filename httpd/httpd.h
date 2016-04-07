/*
-----------------------------------------------------------------------------------------
Author:         Mark F (Cicero-MF) mark@cdelec.co.za    
Known Issues:   none
Version:        04.04.2016
Description:    Esp8266 http server - core routines modified for ENC28J60 conns

  Based off Jeroen Domburg's HTTPD ESP8266 version, modified to handle
  the addition of wired Ethernet connections from an ENC28J60
  

 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */
#ifndef HTTPD_H
#define HTTPD_H

#define HTTPDVER "0.3"

#define HTTPD_CGI_MORE 0
#define HTTPD_CGI_DONE 1
#define HTTPD_CGI_NOTFOUND 2
#define HTTPD_CGI_AUTHENTICATED 3

#define HTTPD_METHOD_GET 1
#define HTTPD_METHOD_POST 2

typedef struct HttpdPriv HttpdPriv;
typedef struct HttpdConnData HttpdConnData;
typedef struct HttpdPostData HttpdPostData;

typedef int (* cgiSendCallback)(HttpdConnData *connData);

//A struct describing a http connection. This gets passed to cgi functions.
struct HttpdConnData {
	struct espconn *conn;
	char requestType;
	char *url;
	char *getArgs;
	const void *cgiArg;
	void *cgiData;
	void *cgiPrivData; // Used for streaming handlers storing state between requests
	HttpdPriv *priv;
	cgiSendCallback cgi;
	HttpdPostData *post;
  int i;
};

//A struct describing the POST data sent inside the http connection.  This is used by the CGI functions
struct HttpdPostData {
	int len; // POST Content-Length
	int buffSize; // The maximum length of the post buffer
	int buffLen; // The amount of bytes in the current post buffer
	int received; // The total amount of bytes received so far
	char *buff; // Actual POST data buffer
	char *multipartBoundary;
};

//A struct describing an url. This is the main struct that's used to send different URL requests to
//different routines.
typedef struct {
	const char *url;
	cgiSendCallback cgiCb;
	const void *cgiArg;
} HttpdBuiltInUrl;

int ICACHE_FLASH_ATTR cgiRedirect(HttpdConnData *connData);
void ICACHE_FLASH_ATTR httpdRedirect(HttpdConnData *conn, char *newUrl);
int httpdUrlDecode(char *val, int valLen, char *ret, int retLen);
int ICACHE_FLASH_ATTR httpdFindArg(char *line, char *arg, char *buff, int buffLen);
void ICACHE_FLASH_ATTR httpdInit(HttpdBuiltInUrl *fixedUrls, int port);
const char *httpdGetMimetype(char *url);
void ICACHE_FLASH_ATTR httpdStartResponse(HttpdConnData *conn, int code);
void ICACHE_FLASH_ATTR httpdHeader(HttpdConnData *conn, const char *field, const char *val);
void ICACHE_FLASH_ATTR httpdEndHeaders(HttpdConnData *conn);
int ICACHE_FLASH_ATTR httpdGetHeader(HttpdConnData *conn, char *header, char *ret, int retLen);
int ICACHE_FLASH_ATTR httpdSend(HttpdConnData *conn, const char *data, int len);

/* Additions for the wired connection via ENC28J60 */
void ICACHE_FLASH_ATTR httpdWiredSentCb(int i);
int ICACHE_FLASH_ATTR httpdWiredConnect(void);
void ICACHE_FLASH_ATTR httpdWiredRec(char *data, unsigned short len, int i);

#endif