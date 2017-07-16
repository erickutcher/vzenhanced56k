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

#ifndef _STACKTREE_H
#define _STACKTREE_H

struct StackTree
{
	StackTree *parent;
	StackTree *sibling;
	StackTree *child;
	StackTree *closing_element;
	char *element_name;
	int element_name_length;
	int element_length;
};

StackTree *ST_CreateNode( StackTree data );
void ST_PushNode( StackTree **head, StackTree *node );
StackTree *ST_PopNode( StackTree **head );

#endif
