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

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "range.h"

bool RangeCompare( const wchar_t *range, const wchar_t *value )
{
	for ( ; *range == L'*' || *range == *value; range++, value++ )
	{
		if ( *range == L'\0' )
		{
			return true;
		}
	}

	return false;
}

RANGE *RangeCreateNode( wchar_t value )
{
	RANGE *range = ( RANGE * )GlobalAlloc( GPTR, sizeof( RANGE ) );
	if ( range != NULL )
	{
		range->next = NULL;
		range->child = NULL;
		range->value = value;
	}

	return range;
}


void RangeAdd( RANGE **root, const wchar_t *value, int length )
{
	RANGE *head = NULL;
	RANGE *tmp = NULL;

	// Create the root node if none exists.
	if ( *root == NULL )
	{
		// Move to the next value also.
		*root = RangeCreateNode( *value );
	}

	head = *root;
	tmp = head;

	for ( int i = 0; i < length; ++i )
	{
		// Find the value in the siblings list.
		while ( tmp != NULL )
		{
			if ( tmp->value == *( value + i ) )
			{
				break;
			}

			tmp = tmp->next;
		}

		// If it's not in the list, add it.
		if ( tmp == NULL )
		{
			tmp = RangeCreateNode( *( value + i ) );

			if ( i == 0 )
			{
				if ( head->value == L'*' )
				{
					tmp->next = head->next;
					head->next = tmp;
				}
				else
				{
					tmp->next = head;
					head = tmp;
				}

				*root = head;
			}
			else
			{
				if ( head->child != NULL && head->child->value == L'*' )
				{
					tmp->next = head->child->next;
					head->child->next = tmp;
				}
				else
				{
					tmp->next = head->child;
					head->child = tmp;
				}
			}
		}

		head = tmp;
		tmp = head->child;
	}
}

bool RangeSearch( RANGE **head, const wchar_t *value, wchar_t found_value[ 32 ] )
{
	if ( *head == NULL )
	{
		return false;
	}

	bool found = false;

	RANGE *tmp = *head;

	// Start searching for wildcard numbers.
	while ( tmp != NULL )
	{
		if ( tmp->value == L'*' )
		{
			found = true;

			*found_value = L'*';

			if ( tmp->child != NULL )
			{
				found = RangeSearch( &( tmp->child ), value + 1, found_value + 1 );
			}

			break;
		}

		tmp = tmp->next;
	}

	// Skip searching for numbers if a wildcard matches.
	if ( found )
	{
		return true;
	}

	found = false;

	tmp = *head;

	// Otherwise, search for numbers.
	while ( tmp != NULL )
	{
		if ( tmp->value == *value )
		{
			found = true;

			*found_value = *value;

			if ( tmp->child != NULL )
			{
				found = RangeSearch( &( tmp->child ), value + 1, found_value + 1 );
			}

			break;
		}

		tmp = tmp->next;
	}

	return found;
}

void RangeDelete( RANGE **head )
{
	if ( *head == NULL )
	{
		return;
	}

	RANGE *tmp = *head;

	// Otherwise, search for numbers.
	if ( tmp != NULL )
	{
		// Recurse all child nodes.
		RangeDelete( &( tmp->child ) );
	}

	// Free the child node and move to the next siblings.
	RANGE *del = *head;
	tmp = ( *head )->next;

	GlobalFree( del );

	// Recurse any children of the sibling.
	RangeDelete( &tmp );
}

bool RangeRemove( RANGE **head, const wchar_t *value, RANGE *parent )
{
	if ( *head == NULL )
	{
		return false;
	}

	RANGE *tmp = *head;
	RANGE *last = tmp;		// Used to keep track of the previous sibling.

	bool found = false;

	// Recurse through the value we're looking for.
	while ( tmp != NULL )
	{
		if ( tmp->value == *value )
		{
			found = true;

			if ( tmp->child != NULL )
			{
				found = RangeRemove( &( tmp->child ), value + 1, tmp );
			}

			break;
		}

		last = tmp;
		tmp = tmp->next;
	}

	// If at the end of our recursion we find an exact match, then begin removing the necessary values.
	if ( found )
	{
		// Only remove the value if it has no children.
		if ( tmp->child == NULL )
		{
			// If the value is the first child of its parent, then set the parent's new child to the value's sibling.
			if ( parent != NULL && tmp == parent->child )
			{
				parent->child = tmp->next;
			}
			else // Otherwise, set the last sibling's next sibling to the value's next sibling.
			{
				last->next = tmp->next;

				// If we're removing the head, set the new head to the value's sibling.
				if ( *head == tmp )
				{
					*head = tmp->next;
				}
			}

			GlobalFree( tmp );
		}
	}

	return found;
}
