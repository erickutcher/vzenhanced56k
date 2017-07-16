/*
	VZ Enhanced 56K is a caller ID notifier that can block phone calls.
	Copyright (C) 2013-2017 Eric Kutcher

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

#ifndef _RANGE_H
#define _RANGE_H

struct RANGE
{
	RANGE *next;
	RANGE *child;
	char value;
};

RANGE *RangeCreateNode( char value );
void RangeAdd( RANGE **root, const char *value, int length );
bool RangeRemove( RANGE **head, const char *value, RANGE *parent = NULL );
bool RangeSearch( RANGE **head, const char *value, char found_value[ 32 ] );
bool RangeCompare( const char *range, const char *value );
void RangeDelete( RANGE **head );

#endif
