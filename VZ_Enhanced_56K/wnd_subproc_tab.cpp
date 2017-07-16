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

#include "globals.h"
#include "string_tables.h"
#include "lite_gdi32.h"
#include "lite_uxtheme.h"

WNDPROC TabProc = NULL;

HTHEME hTheme = NULL;
bool tab_is_dragging = false;
int cur_over = 0;		// The tab item we're currently hovering over.
bool drag_pos = false;	// true = right, false = left
int tab_x_pos = 0;		// The position when we first click on a tab.
int tab_y_pos = 0;		// The position when we first click on a tab.
int cur_tab_x_pos = 0;	// The current position of the mouse over our tabs.
bool tab_moved = false;	// If the current and first positions don't match, then the tab moved.

RECT client_rc;
bool size_changed = true;
HBITMAP hbm_tab = NULL;

wchar_t *GetTabTextByHandle( HWND hWnd )
{
	if ( hWnd == g_hWnd_call_log )
	{
		return ST_Call_Log;
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		return ST_Contact_List;
	}
	else if ( hWnd == g_hWnd_ignore_tab )
	{
		return ST_Ignore_Lists;
	}
	else
	{
		return L"";
	}
}

// Subclassed tab control.
// Allows tabs to be reordered by dragging them.
LRESULT CALLBACK TabSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_PROPAGATE:
		{
			if ( wParam == 1 )
			{
				bool use_theme = true;
				#ifndef UXTHEME_USE_STATIC_LIB
					if ( uxtheme_state == UXTHEME_STATE_SHUTDOWN )
					{
						use_theme = InitializeUXTheme();
					}
				#endif

				if ( use_theme )
				{
					// Open our tab theme. Closed in WM_DESTROY of the tab subclass.
					hTheme = _OpenThemeData( hWnd, L"Tab" );
				}
			}
		}
		break;

		case WM_SIZE:
		{
			size_changed = true;
			_GetClientRect( hWnd, &client_rc );
		}
		break;

		case WM_CAPTURECHANGED:
		{
			tab_is_dragging = ( ( HWND )lParam == hWnd ) ? true : false;

			if ( !tab_is_dragging )
			{
				cur_over = -1;
				tab_moved = false;
				drag_pos = false;
			}

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_LBUTTONDOWN:
		{
			_SetCapture( hWnd );

			cur_over = -1;
			tab_is_dragging = true;
			tab_moved = false;
			drag_pos = false;

			tab_x_pos = GET_X_LPARAM( lParam );
			tab_y_pos = GET_Y_LPARAM( lParam );
		}
		break;

		case WM_LBUTTONUP:
		{
			if ( tab_is_dragging )
			{
				int index = _SendMessageW( hWnd, TCM_GETCURSEL, 0, 0 );		// Get the selected tab

				// Only drag if it's over an item and not the currently selected item.
				if ( cur_over >= 0 && cur_over != index )
				{
					int new_index = 0;

					// The point we dragged to is to the right of the hittest tab item.
					if ( drag_pos )
					{
						new_index = cur_over + 1;
					}
					else	// The point we dragged to is to the left of the hittest tab item.
					{
						new_index = cur_over;
					}

					// Make sure the item we want to drag isn't going to replace itself.
					if ( new_index != index )
					{
						// Get the old tab.
						TCITEM tci;
						_memzero( &tci, sizeof( TCITEM ) );
						tci.mask = TCIF_PARAM;
						_SendMessageW( hWnd, TCM_GETITEM, index, ( LPARAM )&tci );	// Get the tab's information

						// Set the new tab.
						tci.mask = TCIF_PARAM | TCIF_TEXT;
						tci.pszText = GetTabTextByHandle( ( HWND )( tci.lParam ) );

						// Delete the old tab.
						_SendMessageW( hWnd, TCM_DELETEITEM, index, 0 );

						// The item was before the new index.
						if ( index < new_index )
						{
							new_index -= 1;

							// Update the tab order.
							// The tab was moved to the right.
							for ( unsigned char i = 0; i < NUM_TABS; ++i )
							{
								if ( *tab_order[ i ] != -1 )
								{
									// Decrease the order values that are greater than the old index and less than or equal to the new index.
									// We're essentially shifting that range to the left.
									if ( ( *tab_order[ i ] <= ( char )new_index && *tab_order[ i ] > index ) )
									{
										( *( tab_order[ i ] ) )--;
									}
									else if ( *tab_order[ i ] == index )
									{
										*tab_order[ i ] = new_index;	// Update the old index to the new one.
									}
								}
							}
						}
						else
						{
							// Update the tab order.
							// The tab was moved to the left.
							for ( unsigned char i = 0; i < NUM_TABS; ++i )
							{
								if ( *tab_order[ i ] != -1 )
								{
									// Increase the order values that are less than the old index and greater than or equal to the new index.
									// We're essentially shifting that range to the right.
									if ( ( *tab_order[ i ] >= ( char )new_index && *tab_order[ i ] < index ) )
									{
										( *( tab_order[ i ] ) )++;
									}
									else if ( *tab_order[ i ] == index )
									{
										*tab_order[ i ] = new_index;	// Update the old index to the new one.
									}
								}
							}
						}

						// Insert the new item.
						_SendMessageW( hWnd, TCM_INSERTITEM, new_index, ( LPARAM )&tci );
						_SendMessageW( hWnd, TCM_SETCURFOCUS, new_index, 0 );	// Give the new tab focus.
					}

					// We're no longer over a hittest item.
					//cur_over = -1;
				}
			}

			_ReleaseCapture();

			cur_over = -1;
			tab_moved = false;
			tab_is_dragging = false;
			drag_pos = false;
		}
		break;

		case WM_MOUSEMOVE:
		{
			// Find the current tab item that the mouse is over.
			TCHITTESTINFO tcht;
			tcht.pt.x = GET_X_LPARAM( lParam );
			tcht.pt.y = ( tab_is_dragging ? tab_y_pos : GET_Y_LPARAM( lParam ) );
			tcht.flags = TCHT_ONITEM;

			cur_over = _SendMessageW( hWnd, TCM_HITTEST, 0, ( LPARAM )&tcht );

			if ( tab_is_dragging )
			{
				cur_tab_x_pos = GET_X_LPARAM( lParam );

				if ( cur_over == -1 )
				{
					if ( cur_tab_x_pos < tab_x_pos )
					{
						cur_over = 0;
					}
					else if ( cur_tab_x_pos > tab_x_pos )
					{
						cur_over = _SendMessageW( hWnd, TCM_GETITEMCOUNT, 0, 0 ) - 1;
					}
				}

				// See if our hittest is different from our WM_LBUTTONDOWN position.
				if ( cur_tab_x_pos != tab_x_pos )
				{
					tab_moved = true;
				}

				int index = _SendMessageW( hWnd, TCM_GETCURSEL, 0, 0 );		// Get the selected tab

				// We're over an item while dragging and it's not the currently selected item.
				if ( cur_over >= 0 && cur_over != index )
				{
					RECT rc_drag;
					// Get the rectangle of the item we dragged over.
					_SendMessageW( hWnd, TCM_GETITEMRECT, cur_over, ( LPARAM )&rc_drag );

					// The point we dragged to is to the right of the hittest tab item.
					if ( GET_X_LPARAM( lParam ) > ( rc_drag.right - ( ( rc_drag.right - rc_drag.left ) / 2 ) ) )
					{
						drag_pos = true;
					}
					else	// The point we dragged to is to the left of the hittest tab item.
					{
						drag_pos = false;
					}
				}

				_InvalidateRect( hWnd, NULL, TRUE );
			}
		}
		break;

		case WM_MOUSELEAVE:
		{
			// If the mouse is no longer over the control, then no tab item is being hovered over.
			cur_over = -1;
			tab_moved = false;
			tab_is_dragging = false;
			drag_pos = false;

			//_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_ERASEBKGND:
		{
			// We'll handle the background painting in WM_PAINT.
			return TRUE;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;

			// Get a handle to the device context.
			HDC hDC = _BeginPaint( hWnd, &ps );

			// Create and save a bitmap in memory to paint to. (Double buffer)
			HDC hdcMem = _CreateCompatibleDC( hDC );

			if ( hbm_tab == NULL || size_changed )
			{
				if ( hbm_tab != NULL )
				{
					_DeleteObject( hbm_tab );
				}

				hbm_tab = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
				size_changed = false;
			}

			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm_tab );
			_DeleteObject( ohbm );
			
			// Fill the background.
			HBRUSH hBrush = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_MENU ) );
			_FillRect( hdcMem, &client_rc, hBrush );
			_DeleteObject( hBrush );

			// Get the tab item's height. (bottom)
			RECT rc_tab;
			_SendMessageW( hWnd, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			// Draw our tab border.
			RECT rc_pane;
			rc_pane.left = 0;
			rc_pane.top = rc_tab.bottom;
			rc_pane.bottom = client_rc.bottom;
			rc_pane.right = client_rc.right;
			if ( hTheme != NULL )
			{
				_DrawThemeBackground( hTheme, hdcMem, TABP_PANE, 0, &rc_pane, 0 );
			}

			// Set our font.
			HFONT hf = ( HFONT )_SelectObject( hdcMem, hFont );
			// Delete our old font.
			_DeleteObject( hf );

			TCITEM tci;
			_memzero( &tci, sizeof( TCITEM ) );
			tci.mask = TCIF_PARAM;

			int index = _SendMessageW( hWnd, TCM_GETCURSEL, 0, 0 );	// Get the selected tab

			// Get the bounding rect for each tab item.
			int tab_count = _SendMessageW( hWnd, TCM_GETITEMCOUNT, 0, 0 );
			for ( int i = 0; i < tab_count; i++ )
			{
				// Exclude the selected tab. We draw it last so it can clip the non-selected tabs.
				if ( i != index )
				{
					_SendMessageW( hWnd, TCM_GETITEMRECT, i, ( LPARAM )&rc_tab );

					if ( rc_tab.left >= 0 )
					{
						// If the mouse is over the current selection, then set it to normal.
						if ( i == cur_over )
						{
							// This is handling two scenarios.
							// 1. The last visible tab is partially hidden (it's right is greater than the window right). That means the Up/Down control is visible.
							// 2. The last completely visible tab and the partially hidden tab are within the dimensions of the Up/Down control.
							// We want to repaint the area for both tabs to prevent tearing.
							if ( rc_tab.right + ( 2 * _GetSystemMetrics( SM_CXHTHUMB ) ) >= client_rc.right )
							{
								_InvalidateRect( hWnd, &rc_tab, TRUE );
							}

							if ( hTheme != NULL )
							{
								if ( tab_is_dragging )
								{
									_DrawThemeBackground( hTheme, hdcMem, TABP_TABITEM, TIS_FOCUSED, &rc_tab, 0 );
								}
								else
								{
									_DrawThemeBackground( hTheme, hdcMem, TABP_TABITEM, TIS_HOT, &rc_tab, 0 );
								}
							}
						}
						else
						{
							if ( hTheme != NULL )
							{
								_DrawThemeBackground( hTheme, hdcMem, TABP_TABITEM, TIS_NORMAL, &rc_tab, 0 );
							}
						}

						// Offset the text position.
						++rc_tab.top;

						_SetBkMode( hdcMem, TRANSPARENT );
						_SetTextColor( hdcMem, RGB( 0x00, 0x00, 0x00 ) );

						_SendMessageW( hWnd, TCM_GETITEM, i, ( LPARAM )&tci );	// Get the tab's information.
						_DrawTextW( hdcMem, GetTabTextByHandle( ( HWND )( tci.lParam ) ), -1, &rc_tab, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );
					}
				}
			}

			// Draw the dragging anchor on top of the TIS_FOCUSED item.
			if ( tab_is_dragging && cur_over != -1 )
			{
				_SendMessageW( hWnd, TCM_GETITEMRECT, cur_over, ( LPARAM )&rc_tab );

				RECT rectangle;
				rectangle.left = 0;
				rectangle.right = 0;
				rectangle.top = rc_tab.top;
				rectangle.bottom = rc_tab.bottom;
				
				HBRUSH hBrush = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HOTLIGHT ) );
				if ( drag_pos )	// Draw right anchor
				{
					rectangle.right = rc_tab.right + 1;
					rectangle.left = rc_tab.right - 1;
					_FillRect( hdcMem, &rectangle, hBrush );
				}
				else	// Draw left anchor
				{
					rectangle.right = rc_tab.left + 1;
					rectangle.left = rc_tab.left - 1;
					_FillRect( hdcMem, &rectangle, hBrush ); 
				}

				_DeleteObject( hBrush );
			}

			// Draw the selected tab on top of the others.
			_SendMessageW( hWnd, TCM_GETITEMRECT, index, ( LPARAM )&rc_tab );

			// Only show the tab if it's completely visible.
			if ( rc_tab.left > 0 )
			{
				// Enlarge the selected tab's area.
				RECT rc_selected;
				rc_selected.left = rc_tab.left - 2;
				rc_selected.top = rc_tab.top - 2;
				rc_selected.bottom = rc_tab.bottom + 2;
				rc_selected.right = rc_tab.right + 2;
	
				// Exclude the 1 pixel white bottom.
				RECT rc_clip;
				rc_clip.left = rc_selected.left;
				rc_clip.right = rc_selected.right;
				rc_clip.top  = rc_selected.top;
				rc_clip.bottom = rc_selected.bottom - 1;

				if ( hTheme != NULL )
				{
					_DrawThemeBackground( hTheme, hdcMem, TABP_TABITEM, TIS_SELECTED, &rc_selected, &rc_clip );
				}

				// Position of our text (offset up).
				RECT rc_text;
				rc_text.left = rc_tab.left;
				rc_text.right = rc_tab.right;
				rc_text.top  = rc_tab.top - 1;
				rc_text.bottom = rc_tab.bottom - 2;

				_SetBkMode( hdcMem, TRANSPARENT );
				_SetTextColor( hdcMem, RGB( 0x00, 0x00, 0x00 ) );

				_SendMessageW( hWnd, TCM_GETITEM, index, ( LPARAM )&tci );	// Get the tab's information.
				wchar_t *tab_text = GetTabTextByHandle( ( HWND )( tci.lParam ) );
				_DrawTextW( hdcMem, tab_text, -1, &rc_text, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );

				// Draw the transparent selected tab on top of everything else.
				if ( tab_is_dragging && tab_moved )
				{
					// Tab position.
					RECT rc_drag_tab;
					rc_drag_tab.top = rc_selected.top;
					rc_drag_tab.bottom = rc_selected.bottom;
					rc_drag_tab.left = rc_selected.left + ( cur_tab_x_pos - tab_x_pos );
					rc_drag_tab.right = rc_drag_tab.left + ( rc_selected.right - rc_selected.left );

					// Bitmap dimensions
					rc_clip.left = 0;
					rc_clip.right = rc_selected.right - rc_selected.left;	// Width
					rc_clip.top = 0;
					rc_clip.bottom = rc_selected.bottom - rc_selected.top;	// Height.

					HDC hdcMem2 = _CreateCompatibleDC( hDC );
					HBITMAP hbm2 = _CreateCompatibleBitmap( hDC, rc_clip.right, rc_clip.bottom );
					HBITMAP ohbm2 = ( HBITMAP )_SelectObject( hdcMem2, hbm2 );
					_DeleteObject( ohbm2 );
					_DeleteObject( hbm2 );

					// Set our font.
					hf = ( HFONT )_SelectObject( hdcMem2, hFont );
					// Delete our old font.
					_DeleteObject( hf );

					if ( hTheme != NULL )
					{
						_DrawThemeBackground( hTheme, hdcMem2, TABP_TABITEM, TIS_DISABLED, &rc_clip, 0 );
					}

					// Position of our text (offset up).
					rc_text.left = 0;
					rc_text.right = rc_selected.right - rc_selected.left;
					rc_text.top = 1;
					rc_text.bottom = rc_tab.bottom - rc_tab.top;

					_SetBkMode( hdcMem2, TRANSPARENT );
					_SetTextColor( hdcMem2, RGB( 0xFF, 0xFF, 0xFF ) );

					_DrawTextW( hdcMem2, tab_text, -1, &rc_text, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );

					BLENDFUNCTION blend;
					blend.BlendOp = AC_SRC_OVER;
					blend.BlendFlags = 0;
					blend.SourceConstantAlpha = 192;
					blend.AlphaFormat = AC_SRC_OVER;
					_GdiAlphaBlend( hdcMem, rc_drag_tab.left, rc_drag_tab.top, rc_clip.right, rc_clip.bottom - 1, hdcMem2, 0, 0, rc_clip.right, rc_clip.bottom - 1, blend );	// Exclude the 1 pixel white bottom.

					// Delete our back buffer.
					_DeleteDC( hdcMem2 );
				}
			}

			// Draw our back buffer.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top, hdcMem, 0, 0, SRCCOPY );

			// Delete our back buffer.
			_DeleteDC( hdcMem );

			// Release the device context.
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			if ( hbm_tab != NULL )
			{
				_DeleteObject( hbm_tab );
			}

			// Close the theme.
			if ( hTheme != NULL )
			{
				_CloseThemeData( hTheme );
			}

			#ifndef UXTHEME_USE_STATIC_LIB
				UnInitializeUXTheme();
			#endif

			return 0;
		}
		break;
	}

	// Everything that we don't handle gets passed back to the parent to process.
	return _CallWindowProcW( TabProc, hWnd, msg, wParam, lParam );
}
