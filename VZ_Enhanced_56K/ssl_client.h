/*
	VZ Enhanced 56K is a caller ID notifier that can block phone calls.
	Copyright (C) 2013-2018 Eric Kutcher

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SCHANNEL_SSL_H
#define _SCHANNEL_SSL_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define SECURITY_WIN32
#include <security.h>
#include <schannel.h>

#include "lite_ws2_32.h"

#define SSL_STATE_SHUTDOWN		0
#define SSL_STATE_RUNNING		1

#define SP_PROT_TLS1_1_SERVER		0x00000100
#define SP_PROT_TLS1_1_CLIENT		0x00000200
#define SP_PROT_TLS1_1				( SP_PROT_TLS1_1_SERVER | SP_PROT_TLS1_1_CLIENT )
#define SP_PROT_TLS1_2_SERVER		0x00000400
#define SP_PROT_TLS1_2_CLIENT		0x00000800
#define SP_PROT_TLS1_2				( SP_PROT_TLS1_2_SERVER | SP_PROT_TLS1_2_CLIENT )

struct SSL
{
	CtxtHandle hContext;

	BYTE *pbRecDataBuf;
	BYTE *pbIoBuffer;

	SOCKET s;

	//DWORD dwProtocol;

	DWORD cbRecDataBuf;
	DWORD sbRecDataBuf;

	DWORD cbIoBuffer;
	DWORD sbIoBuffer;
};

int SSL_library_init( void );
int SSL_library_uninit( void );

SSL *SSL_new( DWORD protocol );
void SSL_free( SSL *ssl );

int SSL_connect( SSL *ssl, const char *host );
int SSL_shutdown( SSL *ssl );

int SSL_read( SSL *ssl, char *buf, int num );
int SSL_write( SSL *ssl, const char *buf, int num );

extern unsigned char ssl_state;

#endif
