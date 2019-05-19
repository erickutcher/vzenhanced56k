/*
	VZ Enhanced 56K is a caller ID notifier that can block phone calls.
	Copyright (C) 2013-2019 Eric Kutcher

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

#ifndef _CONNECTION_H
#define _CONNECTION_H

#include "ssl_client.h"

#define CONNECTION_KILL		0
#define CONNECTION_ACTIVE	1
#define CONNECTION_CANCEL	2
#define CONNECTION_CREATE	4

THREAD_RETURN CheckForUpdates( void *pArguments );

struct UPDATE_CHECK_INFO
{
	char *notes;
	char *download_url;
	unsigned long version;
	bool got_update;
};

struct CONNECTION
{
	SSL *ssl_socket;
	SOCKET socket;
	bool secure;
	unsigned char state;	// 0 = kill, 1 = active
};

void CleanupConnection( CONNECTION *con, char *host = NULL, bool write_to_log = true );

extern CONNECTION update_con;		// Connection for update checks.

#endif
