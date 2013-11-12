#ifndef _CFG_HTTP_H_
#define _CFG_HTTP_H_

/*!
 * \file cfg/http.h
 * \brief HTTP configuration.
 */

#define HTTPD_EXCLUDE_DATE

#ifndef HTTP_MEDIATYPE_CSS
#define HTTP_MEDIATYPE_CSS 
#endif

#ifndef HTTP_MEDIATYPE_GIF
#define HTTP_MEDIATYPE_GIF 
#endif

#ifndef HTTP_MEDIATYPE_JS
#define HTTP_MEDIATYPE_JS 
#endif

#ifndef HTTP_MEDIATYPE_JPG
#define HTTP_MEDIATYPE_JPG 
#endif

#ifndef HTTP_MEDIATYPE_SHTML
//#define HTTP_MEDIATYPE_SHTML 
#endif

#ifndef HTTP_MEDIATYPE_SVG
#define HTTP_MEDIATYPE_SVG 
#endif

#ifndef HTTP_MAJOR_VERSION
#define HTTP_MAJOR_VERSION 1
#endif

#ifndef HTTP_MINOR_VERSION
#define HTTP_MINOR_VERSION 1
#endif

#ifndef HTTP_DEFAULT_ROOT
#define HTTP_DEFAULT_ROOT "UROM:"
#endif

#ifndef HTTP_MAX_REQUEST_SIZE
#define HTTP_MAX_REQUEST_SIZE 256
#endif

#ifndef HTTP_FILE_CHUNK_SIZE
#define HTTP_FILE_CHUNK_SIZE 512
#endif

#ifndef HTTP_KEEP_ALIVE_REQ
#define HTTP_KEEP_ALIVE_REQ 0
#endif

#endif
