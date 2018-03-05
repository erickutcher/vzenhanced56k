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
#include "parsing.h"
#include "utilities.h"
#include "stack_tree.h"

struct FORM_ELEMENTS
{
	char *name;
	char *value;
	int name_length;
	int value_length;
};

char *strnchr( const char *s, int c, int n )
{
	if ( s == NULL )
	{
		return NULL;
	}

	while ( *s != NULL && n-- > 0 )
	{
		if ( *s == c )
		{
			return ( ( char * )s );
		}
		++s;
	}
	
	return NULL;
}

char *fields_tolower( char *decoded_buffer )
{
	if ( decoded_buffer == NULL )
	{
		return NULL;
	}

	// First find the end of the header.
	char *end_of_header = _StrStrA( decoded_buffer, "\r\n\r\n" );
	if ( end_of_header == NULL )
	{
		return end_of_header;
	}

	char *str_pos_end = decoded_buffer;
	char *str_pos_start = decoded_buffer;

	while ( str_pos_start < end_of_header )
	{
		// The first line won't have a field.
		str_pos_start = _StrStrA( str_pos_end, "\r\n" );
		if ( str_pos_start == NULL || str_pos_start >= end_of_header )
		{
			break;
		}
		str_pos_start += 2;

		str_pos_end = _StrChrA( str_pos_start, ':' );
		if ( str_pos_end == NULL || str_pos_end >= end_of_header )
		{
			break;
		}

		/*for ( int i = 0; i < str_pos_end - str_pos_start; ++i )
		{
			str_pos_start[ i ] = ( char )_CharLowerA( ( LPSTR )str_pos_start[ i ] );
		}*/

		_CharLowerBuffA( str_pos_start, str_pos_end - str_pos_start );
	}

	return end_of_header;
}

bool ParseURL( char *url, char **host, char **resource )
{
	if ( url == NULL )
	{
		return false;
	}

	// Find the start of the host.
	char *str_pos_start = _StrStrA( url, "//" );
	if ( str_pos_start == NULL )
	{
		return false;
	}
	str_pos_start += 2;

	// Find the end of the host.
	char *str_pos_end = _StrChrA( str_pos_start, '/' );
	if ( str_pos_end == NULL )
	{
		return false;
	}

	int host_length = str_pos_end - str_pos_start;

	// Save the host.
	*host = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( host_length + 1 ) );
	_memcpy_s( *host, host_length + 1, str_pos_start, host_length );
	*( *host + host_length ) = 0;	// Sanity

	// Save the resource.
	*resource = GlobalStrDupA( str_pos_end );

	return true;
}

// Modifies decoded_buffer
bool ParseRedirect( char *decoded_buffer, char **host, char **resource )
{
	char *end_of_header = fields_tolower( decoded_buffer );	// Normalize the header fields.
	if ( end_of_header == NULL )
	{
		return false;
	}
	*( end_of_header + 2 ) = 0;	// Make our search string shorter.

	char *str_pos_start = _StrStrA( decoded_buffer, "location:" );
	if ( str_pos_start == NULL )
	{
		*( end_of_header + 2 ) = '\r';	// Restore the end of header.
		return false;
	}
	str_pos_start = _StrStrA( str_pos_start + 9, "//" );					// Find the beginning of the URL's domain name.
	if ( str_pos_start == NULL )
	{
		*( end_of_header + 2 ) = '\r';	// Restore the end of header.
		return false;
	}
	str_pos_start += 2;

	char *str_pos_end = _StrChrA( str_pos_start, '/' );
	if ( str_pos_end == NULL )
	{
		*( end_of_header + 2 ) = '\r';	// Restore the end of header.
		return false;
	}
	int host_length = str_pos_end - str_pos_start;

	*host = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( host_length + 1 ) );
	_memcpy_s( *host, host_length + 1, str_pos_start, host_length );
	*( *host + host_length ) = 0;	// Sanity

	str_pos_start += host_length;

	str_pos_end = _StrStrA( str_pos_start, "\r\n" );					// Find the end of the URL.
	if ( str_pos_end == NULL )
	{
		*( end_of_header + 2 ) = '\r';	// Restore the end of header.
		return false;
	}

	// Skip whitespace that could appear before the "\r\n", but after the field value.
	while ( ( str_pos_end - 1 ) >= str_pos_start )
	{
		if ( *( str_pos_end - 1 ) != ' ' && *( str_pos_end - 1 ) != '\t' )
		{
			break;
		}

		--str_pos_end;
	}

	int resource_length = str_pos_end - str_pos_start;

	*resource = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( resource_length + 1 ) );
	_memcpy_s( *resource, resource_length + 1, str_pos_start, resource_length );
	*( *resource + resource_length ) = 0;	// Sanity

	*( end_of_header + 2 ) = '\r';	// Restore the end of header.

	return true;
}
