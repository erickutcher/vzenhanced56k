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

#include "globals.h"
#include "connection.h"
#include "parsing.h"
#include "menus.h"
#include "utilities.h"
#include "message_log_utilities.h"
#include "list_operations.h"

#include "lite_advapi32.h"
#include "lite_comdlg32.h"
#include "string_tables.h"

#define DEFAULT_BUFLEN		8192

#define DEFAULT_PORT		80
#define DEFAULT_PORT_SECURE	443

#define CURRENT_VERSION		1004
#define VERSION_URL			"https://sites.google.com/site/vzenhanced/version_56k.txt"

CRITICAL_SECTION cut_cs;			// Queues additional update check threads.
CRITICAL_SECTION cuc_cs;			// Blocks the CleanupConnection().

HANDLE update_check_semaphore = NULL;

bool in_update_check_thread = false;

CONNECTION update_con = { NULL, INVALID_SOCKET, true, CONNECTION_KILL };	// Connection for update checks.

void kill_update_check_thread()
{
	if ( in_update_check_thread )
	{
		// This semaphore will be released when the thread gets killed.
		update_check_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		update_con.state = CONNECTION_KILL;			// Causes the connection thread to cease processing and release the semaphore.
		CleanupConnection( &update_con, NULL, false );

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( update_check_semaphore, 5000 );
		CloseHandle( update_check_semaphore );
		update_check_semaphore = NULL;
	}
}

SOCKET Client_Connection( const char *server, unsigned short port, int timeout )
{
	SOCKET client_socket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;

	char cport[ 6 ];

	if ( ws2_32_state == WS2_32_STATE_SHUTDOWN )
	{
		#ifndef WS2_32_USE_STATIC_LIB
			if ( !InitializeWS2_32() ){ return INVALID_SOCKET; }
		#else
			StartWS2_32();
		#endif
	}

	_memzero( &hints, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	__snprintf( cport, 6, "%hu", port );

	// Resolve the server address and port
	if ( _getaddrinfo( server, cport, &hints, &result ) != 0 )
	{
		if ( result != NULL )
		{
			_freeaddrinfo( result );
		}

		return INVALID_SOCKET;
	}

	// Attempt to connect to an address until one succeeds
	for ( ptr = result; ptr != NULL; ptr = ptr->ai_next )
	{
		// Create a SOCKET for connecting to server
		client_socket = _socket( ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol );
		if ( client_socket == INVALID_SOCKET )
		{
			_closesocket( client_socket );
			client_socket = INVALID_SOCKET;

			break;
		}

		// Set a timeout for the connection.
		if ( timeout != 0 )
		{
			timeout *= 1000;
			if ( _setsockopt( client_socket, SOL_SOCKET, SO_RCVTIMEO, ( char * )&timeout, sizeof( int ) ) == SOCKET_ERROR )
			{
				_closesocket( client_socket );
				client_socket = INVALID_SOCKET;

				break;
			}
		}

		// Connect to server.
		if ( _connect( client_socket, ptr->ai_addr, ( int )ptr->ai_addrlen ) == SOCKET_ERROR )
		{
			_closesocket( client_socket );
			client_socket = INVALID_SOCKET;

			continue;
		}

		break;
	}

	if ( result != NULL )
	{
		_freeaddrinfo( result );
	}

	return client_socket;
}

SSL *Client_Connection_Secure( const char *server, unsigned short port, int timeout )
{
	// Create a normal socket connection.
	SOCKET client_socket = Client_Connection( server, port, timeout );

	if ( client_socket == INVALID_SOCKET )
	{
		return NULL;
	}

	// Wrap the socket as an SSL connection.

	if ( ssl_state == SSL_STATE_SHUTDOWN )
	{
		if ( SSL_library_init() == 0 )
		{
			_shutdown( client_socket, SD_BOTH );
			_closesocket( client_socket );

			return NULL;
		}
	}

	DWORD protocol = 0;
	switch ( cfg_connection_ssl_version )
	{
		case 4:	protocol |= SP_PROT_TLS1_2_CLIENT;
		case 3:	protocol |= SP_PROT_TLS1_1_CLIENT;
		case 2:	protocol |= SP_PROT_TLS1_CLIENT;
		case 1:	protocol |= SP_PROT_SSL3_CLIENT;
		case 0:	{ if ( cfg_connection_ssl_version < 4 ) { protocol |= SP_PROT_SSL2_CLIENT; } }
	}

	SSL *ssl = SSL_new( protocol );
	if ( ssl == NULL )
	{
		_shutdown( client_socket, SD_BOTH );
		_closesocket( client_socket );

		return NULL;
	}

	ssl->s = client_socket;

	if ( SSL_connect( ssl, server ) != 1 )
	{
		SSL_shutdown( ssl );
		SSL_free( ssl );

		_shutdown( client_socket, SD_BOTH );
		_closesocket( client_socket );

		return NULL;
	}

	return ssl;
}

void CleanupConnection( CONNECTION *con, char *host, bool write_to_log )
{
	char message_log[ 320 ];

	EnterCriticalSection( &cuc_cs );

	if ( con->ssl_socket != NULL )
	{
		SOCKET s = con->ssl_socket->s;
		SSL_shutdown( con->ssl_socket );
		SSL_free( con->ssl_socket );
		con->ssl_socket = NULL;
		_shutdown( s, SD_BOTH );
		_closesocket( s );

		if ( write_to_log && cfg_enable_message_log )
		{
			if ( host != NULL )
			{
				__snprintf( message_log, 320, "Closed connection to: %.253s:%d", host, ( con->secure ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
				_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
			}
			else
			{
				_SendNotifyMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_W | ML_NOTICE, ( LPARAM )ST_Closed_connection );
			}
		}
	}

	if ( con->socket != INVALID_SOCKET )
	{
		_shutdown( con->socket, SD_BOTH );
		_closesocket( con->socket );
		con->socket = INVALID_SOCKET;

		if ( write_to_log && cfg_enable_message_log )
		{
			if ( host != NULL )
			{
				__snprintf( message_log, 320, "Closed connection: to %.253s:%d", host, ( con->secure ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
				_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
			}
			else
			{
				_SendNotifyMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_W | ML_NOTICE, ( LPARAM )ST_Closed_connection );
			}
		}
	}

	LeaveCriticalSection( &cuc_cs );
}

int SendHTTPRequest( CONNECTION *con, char *request_buffer, int request_buffer_length )
{
	if ( con == NULL || ( con->secure && con->ssl_socket == NULL ) || ( !con->secure && con->socket == INVALID_SOCKET ) )
	{
		return SOCKET_ERROR;
	}

	if ( request_buffer == NULL || request_buffer_length <= 0 )
	{
		return 0;
	}

	if ( con->secure )
	{
		return SSL_write( con->ssl_socket, request_buffer, request_buffer_length );
	}
	else
	{
		return _send( con->socket, request_buffer, request_buffer_length, 0 );
	}
}

int GetHTTPResponse( CONNECTION *con, char **response_buffer, unsigned int &response_buffer_length, unsigned short &http_status, unsigned int &content_length, unsigned int &last_buffer_size )
{
	response_buffer_length = 0;
	http_status = 0;
	content_length = 0;

	if ( con == NULL || ( con->secure && con->ssl_socket == NULL ) || ( !con->secure && con->socket == INVALID_SOCKET ) )
	{
		if ( *response_buffer != NULL )
		{
			GlobalFree( *response_buffer );
			*response_buffer = NULL;
			last_buffer_size = 0;
		}

		return SOCKET_ERROR;
	}

	unsigned int buffer_size = 0;

	// Set the buffer size.
	if ( last_buffer_size > 0 )
	{
		buffer_size = last_buffer_size;	// Set it to the last size if we've supplied the last size.
	}
	else	// Assume the buffer is NULL if no last size was supplied.
	{
		// Free it if it's not.
		if ( *response_buffer != NULL )
		{
			GlobalFree( *response_buffer );
			*response_buffer = NULL;
		}

		buffer_size = DEFAULT_BUFLEN * 2;
		last_buffer_size = buffer_size;
	}

	// If the response buffer is not NULL, then we are reusing it. If not, then create it.
	if ( *response_buffer == NULL )
	{
		*response_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( buffer_size + 1 ) );	// Add one for the NULL character.
	}

	int read = 0;	// Must be signed.
	unsigned int header_offset = 0;
	bool chunked_encoding_set = false;
	bool content_length_set = false;

	while ( true )
	{
		if ( con->secure )
		{
			read = SSL_read( con->ssl_socket, *response_buffer + response_buffer_length, DEFAULT_BUFLEN );
		}
		else
		{
			read = _recv( con->socket, *response_buffer + response_buffer_length, DEFAULT_BUFLEN, 0 );
		}

		if ( read == SOCKET_ERROR )
		{
			GlobalFree( *response_buffer );
			*response_buffer = NULL;
			last_buffer_size = 0;
			response_buffer_length = 0;
			http_status = 0;
			content_length = 0;

			return SOCKET_ERROR;
		}
		else if ( read <= 0 )
		{
			break;
		}

		*( *response_buffer + response_buffer_length + read ) = 0;	// Sanity.

		if ( header_offset == 0 )
		{
			// Find the end of the header.
			char *end_of_header = fields_tolower( *response_buffer );
			if ( end_of_header != NULL )
			{
				end_of_header += 4;

				header_offset = ( end_of_header - *response_buffer );

				char *find_status = _StrChrA( *response_buffer, ' ' );
				if ( find_status != NULL )
				{
					++find_status;
					char *end_status = _StrChrA( find_status, ' ' );
					if ( end_status != NULL )
					{
						// Status response.
						char cstatus[ 4 ];
						_memzero( cstatus, 4 );
						_memcpy_s( cstatus, 4, find_status, ( ( end_status - find_status ) > 3 ? 3 : ( end_status - find_status ) ) );
						cstatus[ 3 ] = 0; // Sanity
						http_status = ( unsigned short )_strtoul( cstatus, NULL, 10 );
					}
				}

				if ( http_status == 0 )
				{
					GlobalFree( *response_buffer );
					*response_buffer = NULL;
					last_buffer_size = 0;
					response_buffer_length = 0;
					http_status = 0;
					content_length = 0;

					return -1;
				}

				char *transfer_encoding_header = _StrStrA( *response_buffer, "transfer-encoding:" );
				if ( transfer_encoding_header != NULL )
				{
					transfer_encoding_header += 18;
					char *end_of_chunk_header = _StrStrA( transfer_encoding_header, "\r\n" );
					if ( end_of_chunk_header != NULL )
					{
						// Skip whitespace.
						while ( transfer_encoding_header < end_of_chunk_header )
						{
							if ( transfer_encoding_header[ 0 ] != ' ' && transfer_encoding_header[ 0 ] != '\t' && transfer_encoding_header[ 0 ] != '\f' )
							{
								break;
							}

							++transfer_encoding_header;
						}

						// Skip whitespace that could appear before the "\r\n", but after the field value.
						while ( ( end_of_chunk_header - 1 ) >= transfer_encoding_header )
						{
							if ( *( end_of_chunk_header - 1 ) != ' ' && *( end_of_chunk_header - 1 ) != '\t' && *( end_of_chunk_header - 1 ) != '\f' )
							{
								break;
							}

							--end_of_chunk_header;
						}

						if ( ( end_of_chunk_header - transfer_encoding_header ) == 7 && _StrCmpNIA( transfer_encoding_header, "chunked", 7 ) == 0 )
						{
							chunked_encoding_set = true;
						}
					}
				}

				char *content_length_header = _StrStrA( *response_buffer, "content-length:" );
				if ( content_length_header != NULL )
				{
					content_length_header += 15;
					char *end_of_length_header = _StrStrA( content_length_header, "\r\n" );
					if ( end_of_length_header != NULL )
					{
						// Skip whitespace.
						while ( content_length_header < end_of_length_header )
						{
							if ( content_length_header[ 0 ] != ' ' && content_length_header[ 0 ] != '\t' && content_length_header[ 0 ] != '\f' )
							{
								break;
							}

							++content_length_header;
						}

						// Skip whitespace that could appear before the "\r\n", but after the field value.
						while ( ( end_of_length_header - 1 ) >= content_length_header )
						{
							if ( *( end_of_length_header - 1 ) != ' ' && *( end_of_length_header - 1 ) != '\t' && *( end_of_length_header - 1 ) != '\f' )
							{
								break;
							}

							--end_of_length_header;
						}

						char clength[ 11 ];
						_memzero( clength, 11 );
						_memcpy_s( clength, 11, content_length_header, ( ( end_of_length_header - content_length_header ) > 10 ? 10 : ( end_of_length_header - content_length_header ) ) );
						clength[ 10 ] = 0;	// Sanity
						content_length = _strtoul( clength, NULL, 10 );

						content_length_set = true;
					}
				}
			}
		}

		response_buffer_length += read;

		if ( chunked_encoding_set )
		{
			// If we've receive the end of a chunked transfer, then exit the loop.
			if ( _StrCmpNA( *response_buffer + response_buffer_length - 4, "\r\n\r\n", 4 ) == 0 )
			{
				break;
			}
		}
			
		if ( content_length_set )
		{
			// If there is content, see how much we've received and compare it to the content length.
			if ( content_length + header_offset == response_buffer_length )
			{
				break;
			}
		}

		// Resize the response_buffer if we need more room.
		if ( response_buffer_length + DEFAULT_BUFLEN >= buffer_size )
		{
			buffer_size += DEFAULT_BUFLEN;
			last_buffer_size = buffer_size;

			char *realloc_buffer = ( char * )GlobalReAlloc( *response_buffer, sizeof( char ) * buffer_size, GMEM_MOVEABLE );
			if ( realloc_buffer == NULL )
			{
				GlobalFree( *response_buffer );
				*response_buffer = NULL;
				last_buffer_size = 0;
				response_buffer_length = 0;
				http_status = 0;
				content_length = 0;
				
				return -1;
			}

			*response_buffer = realloc_buffer;
		}
	}

	*( *response_buffer + response_buffer_length ) = 0;	// Sanity

	// Ensure that the content length was accurately represented.
	if ( content_length + header_offset != response_buffer_length )
	{
		content_length = response_buffer_length - header_offset;
	}



	// Now we need to decode the buffer in case it was a chunked transfer.



	if ( chunked_encoding_set )
	{
		// Beginning of the first chunk. (After the header)
		char *find_chunk = *response_buffer + header_offset;

		unsigned int chunk_offset = header_offset;

		// Find the end of the first chunk.
		char *end_chunk = _StrStrA( find_chunk, "\r\n" );
		if ( end_chunk == NULL )
		{
			return -1;	// If it doesn't exist, then we exit.
		}

		unsigned int data_length = 0;
		char chunk[ 32 ];

		do
		{
			// Get the chunk value (including any extensions).
			_memzero( chunk, 32 );
			_memcpy_s( chunk, 32, find_chunk, ( ( end_chunk - find_chunk ) > 32 ? 32 : ( end_chunk - find_chunk ) ) );
			chunk[ 31 ] = 0; // Sanity

			end_chunk += 2;
			
			// Find any chunked extension and ignore it.
			char *find_extension = _StrChrA( chunk, ';' );
			if ( find_extension != NULL )
			{
				*find_extension = NULL;
			}

			// Convert the hexidecimal chunk value into an integer. Stupid...
			data_length = _strtoul( chunk, NULL, 16 );
			
			// If the chunk value is less than or 0, or it's too large, then we're done.
			if ( data_length == 0 || ( data_length > ( response_buffer_length - chunk_offset ) ) )
			{
				break;
			}

			// Copy over our reply_buffer.
			_memcpy_s( *response_buffer + chunk_offset, response_buffer_length - chunk_offset, end_chunk, data_length );
			chunk_offset += data_length;				// data_length doesn't include any NULL characters.
			*( *response_buffer + chunk_offset ) = 0;	// Sanity

			find_chunk = _StrStrA( end_chunk + data_length, "\r\n" ); // This should be the token after the data.
			if ( find_chunk == NULL )
			{
				break;
			}
			find_chunk += 2;

			end_chunk = _StrStrA( find_chunk, "\r\n" );
			if ( end_chunk == NULL )
			{
				break;
			}
		} 
		while ( data_length > 0 );

		response_buffer_length = chunk_offset;
		content_length = response_buffer_length - header_offset;
	}

	return 0;
}

bool Try_Connect( CONNECTION *con, char *host, int timeout = 0, bool retry = true )
{
	int total_retries = 0;

	if ( cfg_connection_reconnect && retry )
	{
		total_retries = cfg_connection_retries;
	}

	char message_log[ 320 ];
	if ( cfg_enable_message_log )
	{
		__snprintf( message_log, 320, "Connecting to: %.253s:%d", host, ( con->secure ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
		_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
	}

	// We consider retry == 0 to be our normal connection.
	for ( int retry = 0; retry <= total_retries; ++retry )
	{
		// Stop processing and exit the thread.
		if ( con->state == CONNECTION_KILL )
		{
			break;
		}

		// Connect to server.
		if ( con->secure )
		{
			con->ssl_socket = Client_Connection_Secure( host, DEFAULT_PORT_SECURE, timeout );
		}
		else
		{
			con->socket = Client_Connection( host, DEFAULT_PORT, timeout );
		}

		// If the connection failed, try again.
		if ( ( con->secure && con->ssl_socket == NULL ) || ( !con->secure && con->socket == INVALID_SOCKET ) )
		{
			// If the connection failed our first attempt, and then the subsequent number of retries, then we exit.
			if ( retry == total_retries )
			{
				break;
			}

			if ( cfg_enable_message_log )
			{
				__snprintf( message_log, 320, "Retrying connection to: %.253s:%d", host, ( con->secure ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
				_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_WARNING, ( LPARAM )message_log );
			}

			// Retry from the beginning.
			continue;
		}

		if ( cfg_enable_message_log )
		{
			__snprintf( message_log, 320, "Connected to: %.253s:%d", host, ( con->secure ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
			_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
		}

		// We've made a successful connection.
		return true;
	}

	if ( cfg_enable_message_log )
	{
		__snprintf( message_log, 320, "Failed to connect to %.253s:%d", host, ( con->secure ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
		_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_ERROR, ( LPARAM )message_log );
	}

	// If we couldn't establish a connection, or we've abuptly shutdown, then free any ssl_socket resources.
	CleanupConnection( con, host );

	return false;
}

int Try_Send_Receive( CONNECTION *con, char *host, char *resource /*for logging only*/,
					  char *request_buffer, unsigned int request_buffer_length,
					  char **response_buffer, unsigned int &response_buffer_length, unsigned short &http_status, unsigned int &content_length, unsigned int &last_buffer_size,
					  int timeout = 0, bool retry = true )
{
	int response_code = 0;
	int total_retries = 0;
	http_status = 0;

	if ( cfg_connection_reconnect && retry )
	{
		total_retries = cfg_connection_retries;
	}

	// We consider retry == 0 to be our normal connection.
	for ( int retry = 0; retry <= total_retries; ++retry )
	{
		// Stop processing and exit the thread.
		if ( con->state == CONNECTION_KILL )
		{
			break;
		}

		response_code = 0;

		char message_log[ 2430 ];
		if ( cfg_enable_message_log )
		{
			__snprintf( message_log, 2430, "Sending %lu byte request for: http%s://%.253s:%d%.2048s", request_buffer_length, ( con->secure ? "s" : "" ), host, ( con->secure ? DEFAULT_PORT_SECURE : DEFAULT_PORT ), resource );
			_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
		}

		if ( SendHTTPRequest( con, request_buffer, request_buffer_length ) > 0 )
		{
			response_code = GetHTTPResponse( con, response_buffer, response_buffer_length, http_status, content_length, last_buffer_size );
		}

		if ( cfg_enable_message_log )
		{
			__snprintf( message_log, 2430, "Received %lu byte response with status code %lu from: http%s://%.253s:%d%.2048s", response_buffer_length, http_status, ( con->secure ? "s" : "" ), host, ( con->secure ? DEFAULT_PORT_SECURE : DEFAULT_PORT ), resource );
			_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
		}

		// HTTP Status codes: 200 OK, 302 Found (redirect)
		if ( http_status == 200 || http_status == 302 )
		{
			return 0;
		}
		else if ( http_status == 403 )	// Bad cookies.
		{
			return 0;
		}
		else if ( response_code <= 0 )	// No response was received: 0, or recv failed: -1
		{
			// Stop processing and exit the thread.
			if ( con->state == CONNECTION_KILL )
			{
				break;
			}

			if ( retry == total_retries )
			{
				break;
			}

			if ( cfg_enable_message_log )
			{
				__snprintf( message_log, 2430, "Retrying %lu byte request for: http%s://%.253s:%d%.2048s", request_buffer_length, ( con->secure ? "s" : "" ), host, ( con->secure ? DEFAULT_PORT_SECURE : DEFAULT_PORT ), resource );
				_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_WARNING, ( LPARAM )message_log );
			}

			// Close the connection since no more data will be sent or received.
			CleanupConnection( con, host );

			// Establish the connection again.
			if ( !Try_Connect( con, host, timeout ) )
			{
				break;
			}
		}
		else	// Some other HTTP Status code.
		{
			break;
		}
	}

	// If we couldn't establish a connection, or we've abuptly shutdown, then free any ssl_socket resources.
	CleanupConnection( con, host );

	return -1;
}

void GetVersionInfo( char *version_url, unsigned long &version, char **download_url, char **notes )
{
	char *resource = NULL;
	char *host = NULL;

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	int send_buffer_length = 0;

	char *version_send_buffer = NULL;

	if ( !ParseURL( version_url, &host, &resource ) )
	{
		goto CLEANUP;
	}

	version_send_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( DEFAULT_BUFLEN * 2 ) );

	// First determine the server type (http or https)
	if ( _memcmp( version_url, "https://", 8 ) == 0 )
	{
		update_con.secure = true;
	}
	else
	{
		update_con.secure = false;
	}

	// Request the file and go through any redirects that exist.
	for ( unsigned char redirects = 0; redirects < 5; ++redirects )
	{
		send_buffer_length = __snprintf( version_send_buffer, DEFAULT_BUFLEN * 2,
		"GET %s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Connection: keep-alive\r\n\r\n", resource, host );

		if ( !Try_Connect( &update_con, host, cfg_connection_timeout ) )
		{
			goto CLEANUP;
		}

		if ( Try_Send_Receive( &update_con, host, resource, version_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
		{
			goto CLEANUP;
		}

		CleanupConnection( &update_con, host );

		GlobalFree( resource );
		resource = NULL;

		GlobalFree( host );
		host = NULL;
		
		if ( http_status == 302 )	// Redirect
		{
			if ( !ParseRedirect( response, &host, &resource ) )
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if ( http_status == 200 )
	{
		char *file = _StrStrA( response, "\r\n\r\n" );
		if ( file != NULL )
		{
			char *line_start = file + 4;

			// Find the end of the version number line.
			char *line_end = _StrStrA( line_start, "\r\n" );
			if ( line_end != NULL )
			{
				*line_end = 0;

				version = _strtoul( line_start, NULL, 10 );

				line_start = line_end + 2;

				// Find the end of the download url line.
				line_end = _StrStrA( line_start, "\r\n" );
				if ( line_end != NULL )
				{
					*line_end = 0;

					unsigned int download_url_length = ( line_end - line_start );

					*download_url = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( download_url_length + 1 ) );
					_memcpy_s( *download_url, download_url_length + 1, line_start, download_url_length );

					*( *download_url + download_url_length ) = 0;	// Sanity.

					line_start = line_end + 2;

					// Find the notes block.
					if ( content_length >= ( unsigned int )( line_start - ( file + 4 ) ) )
					{
						unsigned int notes_length = ( content_length - ( line_start - ( file + 4 ) ) );

						*notes = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( notes_length + 1 ) );
						_memcpy_s( *notes, notes_length + 1, line_start, notes_length );

						*( *notes + notes_length ) = 0;	// Sanity.
					}
				}
				else if ( content_length >= ( unsigned int )( line_start - ( file + 4 ) ) )	// There's no notes section. Just get the download url.
				{
					unsigned int download_url_length = ( content_length - ( line_start - ( file + 4 ) ) );

					*download_url = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( download_url_length + 1 ) );
					_memcpy_s( *download_url, download_url_length + 1, line_start, download_url_length );

					*( *download_url + download_url_length ) = 0;	// Sanity.
				}
			}
		}
	}

CLEANUP:

	CleanupConnection( &update_con, host );

	GlobalFree( response );
	GlobalFree( resource );
	GlobalFree( host );
	GlobalFree( version_send_buffer );
}

bool DownloadUpdate( char *download_url )
{
	char *host = NULL;
	char *resource = NULL;

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	int send_buffer_length = 0;

	char *download_buffer = NULL;

	wchar_t *file_path = NULL;

	bool update_status = false;

	if ( !ParseURL( download_url, &host, &resource ) )
	{
		goto CLEANUP;
	}

	download_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( DEFAULT_BUFLEN * 32 ) );

	// First determine the server type (http or https)
	if ( _memcmp( download_url, "https://", 8 ) == 0 )
	{
		update_con.secure = true;
	}
	else
	{
		update_con.secure = false;
	}

	// Get the file name from the url.
	char *file_name = resource;
	int offset = lstrlenA( file_name );

	while ( offset != 0 && file_name[ --offset ] != '/' );

	if ( file_name[ offset ] == '/' )
	{
		++offset;
	}
	
	file_name += offset;

	int val_length = MultiByteToWideChar( CP_UTF8, 0, file_name, -1, NULL, 0 );	// Include the NULL terminator.
	wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
	MultiByteToWideChar( CP_UTF8, 0, file_name, -1, val, val_length );

	file_path = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );
	_wmemcpy_s( file_path, MAX_PATH, val, val_length );

	GlobalFree( val );

	// Request the file and go through any redirects that exist.
	for ( unsigned char redirects = 0; redirects < 5; ++redirects )
	{
		send_buffer_length = __snprintf( download_buffer, DEFAULT_BUFLEN * 32,
		"GET %s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Connection: keep-alive\r\n\r\n", resource, host );

		if ( !Try_Connect( &update_con, host, cfg_connection_timeout ) )
		{
			goto CLEANUP;
		}

		if ( Try_Send_Receive( &update_con, host, resource, download_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
		{
			goto CLEANUP;
		}

		CleanupConnection( &update_con, host );

		GlobalFree( resource );
		resource = NULL;

		GlobalFree( host );
		host = NULL;
		
		if ( http_status == 302 )	// Redirect
		{
			if ( !ParseRedirect( response, &host, &resource ) )
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if ( http_status == 200 )
	{
		char *file = _StrStrA( response, "\r\n\r\n" );
		if ( file != NULL )
		{
			file += 4;

			update_status = true;

			OPENFILENAME ofn;
			_memzero( &ofn, sizeof( OPENFILENAME ) );
			ofn.lStructSize = sizeof( OPENFILENAME );
			ofn.hwndOwner = g_hWnd_main;
			ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
			ofn.lpstrTitle = L"Save As";
			ofn.lpstrFile = file_path;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_READONLY;

			if ( _GetSaveFileNameW( &ofn ) )
			{
				// Save file.
				DWORD write = 0;
				HANDLE hFile = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
				if ( hFile != INVALID_HANDLE_VALUE )
				{
					WriteFile( hFile, file, content_length, &write, NULL );

					CloseHandle( hFile );
				}
				else
				{
					if ( update_con.state != CONNECTION_KILL && update_con.state != CONNECTION_CANCEL ){ MESSAGE_LOG_OUTPUT_MB( ML_ERROR, ST_File_could_not_be_saved ) }
				}
			}
		}
	}

CLEANUP:

	CleanupConnection( &update_con, host );

	GlobalFree( response );
	GlobalFree( resource );
	GlobalFree( host );
	GlobalFree( download_buffer );
	GlobalFree( file_path );

	return update_status;
}

THREAD_RETURN CheckForUpdates( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cut_cs );

	in_update_check_thread = true;

	UPDATE_CHECK_INFO *update_info = ( UPDATE_CHECK_INFO * )pArguments;

	if ( update_info == NULL )
	{
		goto CLEANUP;
	}

	update_con.state = CONNECTION_ACTIVE;

	// Check the version2.txt file.
	if ( !update_info->got_update )
	{
		//MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Checking_for_updates )
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Checking_for_updates___ )

		GetVersionInfo( VERSION_URL, update_info->version, &( update_info->download_url ), &( update_info->notes ) );

		update_info->got_update = true;

		if ( update_info->version == CURRENT_VERSION )
		{
			// Up to date.
			_SendMessageW( g_hWnd_update, WM_PROPAGATE, 0, 0 );

			//MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_The_program_is_up_to_date )
			MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_VZ_Enhanced_56K_is_up_to_date_ )
		}
		else if ( update_info->version > CURRENT_VERSION )
		{
			// update_info is freed in in the update window, or passed back to this function to download and then freed.
			_SendMessageW( g_hWnd_update, WM_PROPAGATE, 1, ( LPARAM )update_info );

			update_info = NULL;	// We don't want to free this below since it was passed to the update window.

			//MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_A_new_version_of_the_program_is_available )
			MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_A_new_version_is_available_ )
		}
		else
		{
			if ( update_con.state != CONNECTION_KILL && update_con.state != CONNECTION_CANCEL ){ _SendNotifyMessageW( g_hWnd_update, WM_ALERT, 0, NULL ); }

			//MESSAGE_LOG_OUTPUT( ML_WARNING, ST_The_update_check_could_not_be_completed )
			MESSAGE_LOG_OUTPUT( ML_WARNING, ST_The_update_check_has_failed_ )
		}
	}
	else	// If we have a download url, then get the file.
	{
		//MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Downloading_update )
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Downloading_the_update___ )

		// Disable download button.
		_SendMessageW( g_hWnd_update, WM_PROPAGATE, 2, 0 );	// Downloading.

		if ( !DownloadUpdate( update_info->download_url ) )
		{
			// Will also enable and hide download button.
			if ( update_con.state != CONNECTION_KILL && update_con.state != CONNECTION_CANCEL ){ _SendNotifyMessageW( g_hWnd_update, WM_ALERT, 1, NULL ); }

			//MESSAGE_LOG_OUTPUT( ML_WARNING, ST_The_download_could_not_be_completed )
			MESSAGE_LOG_OUTPUT( ML_WARNING, ST_The_download_has_failed_ )
		}
		else
		{
			// Enable and hide download button.
			_SendMessageW( g_hWnd_update, WM_PROPAGATE, 3, 0 );	// Downloaded successfully.

			//MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Downloaded_update )
			MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_The_download_has_completed_ )
		}
	}

CLEANUP:

	CleanupConnection( &update_con );
	update_con.state = CONNECTION_KILL;

	if ( update_info != NULL )
	{
		GlobalFree( update_info->notes );
		GlobalFree( update_info->download_url );
		GlobalFree( update_info );
	}

	// Release the semaphore if we're killing the thread.
	if ( update_check_semaphore != NULL )
	{
		ReleaseSemaphore( update_check_semaphore, 1, NULL );
	}

	in_update_check_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cut_cs );

	_ExitThread( 0 );
	return 0;
}
