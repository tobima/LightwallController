/*
 * Copyright (C) 2012 by egnite GmbH
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 */

/*!
 * $Id$
 */

#if !defined(_WIN32) && !defined(__linux__)

#include <pro/uhttp/streamio.h>

#include <sys/version.h>
#include <sys/timer.h>
#include <sys/thread.h>

#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <memdebug.h>

#include <pro/uhttp/streamio.h>

int StreamInit(void)
{
    return 0;
}

int StreamClientAccept(HTTP_CLIENT_HANDLER handler, const char *params)
{
    int rc = -1;
    TCPSOCKET *sock;
    HTTP_STREAM *sp;
    unsigned short port = 80;

    HTTP_ASSERT(handler != NULL);
    if (params) {
        port = (unsigned short)atoi(params);
    }
    for (;;) {
        sock = NutTcpCreateSocket();
        if (sock) {
            static uint16_t mss = 1024; //1460;
            static uint16_t tcpbufsiz = 23360;

            NutTcpSetSockOpt(sock, TCP_MAXSEG, &mss, sizeof(mss));
            NutTcpSetSockOpt(sock, SO_RCVBUF, &tcpbufsiz, sizeof(tcpbufsiz));
            if (NutTcpAccept(sock, port) == 0) {
                uint32_t tmo = 1000;

                NutTcpSetSockOpt(sock, SO_RCVTIMEO, &tmo, sizeof(tmo));

                sp = calloc(1, sizeof(HTTP_STREAM));
                sp->strm_sock = sock;
                (*handler)(sp);
                free(sp);
                rc = 0;
            }
        }
        NutTcpCloseSocket(sock);
    }
    return rc;
}

int StreamReadUntilChars(HTTP_STREAM *sp, const char *delim, const char *ignore, char *buf, int siz)
{
    int rc = 0;
    int skip = 0;
    char ch;

    HTTP_ASSERT(sp != NULL);

    /* Do not read more characters than requested. */
    while (rc < siz) {
        /* Check the current stream buffer. */
        if (sp->strm_ipos == sp->strm_ilen) {
            /* No more buffered data, re-fill the buffer. */
            int got = NutTcpReceive(sp->strm_sock, sp->strm_ibuf, 1460);
            if (got <= 0) {
                /* Broken connection or timeout. */
                if (got < 0) {
                    rc = -1;
                    skip = 0;
                }
                break;
            }
            sp->strm_ilen = got;
            sp->strm_ipos = 0;
        }
        ch = sp->strm_ibuf[sp->strm_ipos];
        sp->strm_ipos++;
        if (rc == 0 && ch == ' ') {
            /* Skip leading spaces. */
            skip++;
        } else {
            rc++;
            if (delim && strchr(delim, ch)) {
                /* Delimiter found. */
                break;
            }
            if (buf && (ignore == NULL || strchr(ignore, ch) == NULL)) {
                /* Add valid character to application buffer. */
                *buf++ = ch;
            }
        }
    }
    if (buf) {
        *buf = '\0';
    }
    return rc + skip;
}

int StreamReadUntilString(HTTP_STREAM *sp, const char *delim, char *buf, int siz)
{
    int rc = 0;
    int n;
    int i;
    int delen = strlen(delim);

    HTTP_ASSERT(sp != NULL);

    /* Do not read more characters than requested. */
    while (rc < siz) {
        /* Check if the delimiter fits in the current stream buffer. */
        if (sp->strm_ipos >= sp->strm_ilen - delen) {
            int got;
            /* Not enough data to fit the delimiter, re-fill the buffer. */
            sp->strm_ilen -= sp->strm_ipos;
            memcpy(sp->strm_ibuf, sp->strm_ibuf + sp->strm_ipos, sp->strm_ilen);
            got = NutTcpReceive(sp->strm_sock, sp->strm_ibuf + sp->strm_ilen, sizeof(sp->strm_ibuf) - sp->strm_ilen);
            if (got <= 0) {
                /* Broken connection or timeout. */
                if (got < 0) {
                    rc = -1;
                }
                break;
            }
            sp->strm_ilen += got;
            sp->strm_ipos = 0;
        }
        for (i = sp->strm_ipos, n = 0; i < sp->strm_ilen && rc + n < siz; i++, n++) {
            if (*delim == sp->strm_ibuf[i]) {
                if (i + delen >= sp->strm_ilen) {
                    break;
                }
                if (memcmp(&sp->strm_ibuf[i], delim, delen) == 0) {
                    break;
                }
            }
        }
        if (n) {
            memcpy(buf, sp->strm_ibuf + sp->strm_ipos, n);
            buf += n;
            rc += n;
            sp->strm_ipos += n;
        } else {
            break;
        }
    }
    return rc;
}

int s_write(const void *buf, size_t size, size_t count, HTTP_STREAM *sp)
{
    HTTP_ASSERT(sp != NULL);
    HTTP_ASSERT(buf != NULL);

    return NutTcpDeviceWrite(sp->strm_sock, (const char *)buf, size * count);
}

int s_puts(const char *str, HTTP_STREAM *sp)
{
    HTTP_ASSERT(sp != NULL);
    HTTP_ASSERT(str != NULL);

    return NutTcpDeviceWrite(sp->strm_sock, str, strlen(str));
}

int s_vputs(HTTP_STREAM *sp, ...)
{
    int rc = -1;
    int len;
    char *cp;
    char *buf;
    va_list ap;

    HTTP_ASSERT(sp != NULL);

    va_start(ap, sp);
    for (len = 0; (cp = va_arg(ap, char *)) != NULL; len += strlen(cp));
    va_end(ap);
    buf = malloc(len + 1);
    if (buf) {
        va_start(ap, sp);
        for (*buf = '\0'; (cp = va_arg(ap, char *)) != NULL; strcat(buf, cp));
        va_end(ap);
        rc = NutTcpDeviceWrite(sp->strm_sock, buf, strlen(buf));
        free(buf);
    }
    return rc;
}

int s_printf(HTTP_STREAM *sp, const char *fmt, ...)
{
    int rc = -1;
    char *buf;
    va_list ap;

    HTTP_ASSERT(sp != NULL);
    HTTP_ASSERT(fmt != NULL);

    buf = malloc(1024);
    if (buf) {
        va_start(ap, fmt);
        rc = vsprintf(buf, fmt, ap);
        va_end(ap);
        rc = NutTcpDeviceWrite(sp->strm_sock, buf, rc);
        free(buf);
    }
    return rc;
}

int s_flush(HTTP_STREAM *sp)
{
    return NutTcpDeviceWrite(sp->strm_sock, NULL, 0);
}

const char *StreamInfo(HTTP_STREAM *sp, int item)
{
    static char *env_value;

    free(env_value);
    env_value = NULL;
    switch (item) {
    case SITEM_REMOTE_ADDR:
        //env_value = strdup(inet_ntoa(sp->strm_caddr.sin_addr));
        break;
    case SITEM_REMOTE_PORT:
        //env_value = malloc(16);
        //sprintf(env_value, "%u", sp->strm_caddr.sin_port);
        break;
    case SITEM_SERVER_NAME:
    case SITEM_SERVER_ADDR:
        //env_value = strdup(inet_ntoa(sp->strm_saddr.sin_addr));
        break;
    case SITEM_SERVER_PORT:
        //env_value = malloc(16);
        //sprintf(env_value, "%u", sp->strm_saddr.sin_port);
        break;
    }

    if (env_value == NULL) {
        env_value = strdup("");
    }
    return env_value;
}

#endif
