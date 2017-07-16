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

#include "ssl_client.h"

#include "lite_dlls.h"
#include "lite_ntdll.h"

HMODULE g_hSecurity = NULL;
PSecurityFunctionTableA g_pSSPI;

typedef BOOL ( WINAPI *pSslEmptyCacheW )( LPSTR pszTargetName, DWORD dwFlags );
pSslEmptyCacheW	_SslEmptyCacheW;

unsigned char ssl_state = SSL_STATE_SHUTDOWN;

CredHandle g_hCreds;

void ResetCredentials()
{
	if ( SecIsValidHandle( &g_hCreds ) )
	{
		g_pSSPI->FreeCredentialsHandle( &g_hCreds );
		SecInvalidateHandle( &g_hCreds );
	}
}

int SSL_library_init( void )
{
	if ( ssl_state != SSL_STATE_SHUTDOWN )
	{
		return 1;
	}

	g_hSecurity = LoadLibraryDEMW( L"schannel.dll" );
	if ( g_hSecurity == NULL )
	{
		return 0;
	}

	_SslEmptyCacheW = ( pSslEmptyCacheW )GetProcAddress( g_hSecurity, "SslEmptyCacheW" );
	if ( _SslEmptyCacheW == NULL )
	{
		FreeLibrary( g_hSecurity );
		g_hSecurity = NULL;
		return 0;
	}

	INIT_SECURITY_INTERFACE_A pInitSecurityInterface = ( INIT_SECURITY_INTERFACE_A )GetProcAddress( g_hSecurity, SECURITY_ENTRYPOINT_ANSIA );
	if ( pInitSecurityInterface != NULL )
	{
		g_pSSPI = pInitSecurityInterface();
	}

	if ( g_pSSPI == NULL )
	{
		FreeLibrary( g_hSecurity );
		g_hSecurity = NULL;
		return 0;
	}

	ssl_state = SSL_STATE_RUNNING;

	SecInvalidateHandle( &g_hCreds );
	
	return 1;
}

int SSL_library_uninit( void )
{
	BOOL ret = 0;

	if ( ssl_state != SSL_STATE_SHUTDOWN )
	{
		ResetCredentials();

		_SslEmptyCacheW( NULL, 0 );

		ret = FreeLibrary( g_hSecurity );
	}

	ssl_state = SSL_STATE_SHUTDOWN;

	return ret;
}

SSL *SSL_new( DWORD protocol )
{
	if ( g_hSecurity == NULL )
	{
		return NULL;
	}

	SSL *ssl = ( SSL * )GlobalAlloc( GPTR, sizeof( SSL ) );
	if ( ssl == NULL )
	{
		return NULL;
	}

	//ssl->dwProtocol = protocol;

	if ( !SecIsValidHandle( &g_hCreds ) )
	{
		SCHANNEL_CRED SchannelCred;
		TimeStamp tsExpiry;
		SECURITY_STATUS scRet;

		_memzero( &SchannelCred, sizeof( SchannelCred ) );

		SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
		SchannelCred.grbitEnabledProtocols = protocol;//ssl->dwProtocol;
		SchannelCred.dwFlags = ( SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_MANUAL_CRED_VALIDATION );

		// Create an SSPI credential.
		scRet = g_pSSPI->AcquireCredentialsHandleA(
						NULL,
						UNISP_NAME_A,
						SECPKG_CRED_OUTBOUND,
						NULL,
						&SchannelCred,
						NULL,
						NULL,
						&g_hCreds,
						&tsExpiry );

		if ( scRet != SEC_E_OK )
		{
			ResetCredentials();

			GlobalFree( ssl ); 
			ssl = NULL;
		}
	}

	return ssl;
}

void SSL_free( SSL *ssl )
{
	if ( ssl == NULL )
	{
		return;
	}

	if ( ssl->pbRecDataBuf != NULL )
	{
		GlobalFree( ssl->pbRecDataBuf );
		ssl->pbRecDataBuf = NULL;
	}

	if ( ssl->pbIoBuffer != NULL )
	{
		GlobalFree( ssl->pbIoBuffer );
		ssl->pbIoBuffer = NULL;
	}

	if ( SecIsValidHandle( &ssl->hContext ) )
	{
		g_pSSPI->DeleteSecurityContext( &ssl->hContext );
		SecInvalidateHandle( &ssl->hContext );
	}

	GlobalFree( ssl );
}

static SECURITY_STATUS ClientHandshakeLoop( SSL *ssl, bool fDoInitialRead )    
{
	SecBufferDesc InBuffer;
	SecBuffer InBuffers[ 2 ];
	SecBufferDesc OutBuffer;
	SecBuffer OutBuffers[ 1 ];
	DWORD dwSSPIFlags;
	DWORD dwSSPIOutFlags;
	TimeStamp tsExpiry;
	SECURITY_STATUS scRet;
	DWORD cbData;

	bool fDoRead;

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
				  ISC_REQ_REPLAY_DETECT     |
				  ISC_REQ_CONFIDENTIALITY   |
				  ISC_RET_EXTENDED_ERROR    |
				  ISC_REQ_ALLOCATE_MEMORY   |
				  ISC_REQ_STREAM;

	ssl->cbIoBuffer = 0;

	fDoRead = fDoInitialRead;

	scRet = SEC_I_CONTINUE_NEEDED;

	// Loop until the handshake is finished or an error occurs.
	while ( scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS ) 
	{
		// Read server data
		if ( ssl->cbIoBuffer == 0 || scRet == SEC_E_INCOMPLETE_MESSAGE )
		{
			if ( fDoRead )
			{
				// If buffer not large enough reallocate buffer
				if ( ssl->sbIoBuffer <= ssl->cbIoBuffer )
				{
					ssl->sbIoBuffer += 2048;
					if ( ssl->pbIoBuffer == NULL )
					{
						ssl->pbIoBuffer = ( PUCHAR )GlobalAlloc( GPTR, ssl->sbIoBuffer );
					}
					else
					{
						ssl->pbIoBuffer = ( PUCHAR )GlobalReAlloc( ssl->pbIoBuffer, ssl->sbIoBuffer, GMEM_MOVEABLE );
					}
				}

				cbData = _recv( ssl->s, ( char * )ssl->pbIoBuffer + ssl->cbIoBuffer, ssl->sbIoBuffer - ssl->cbIoBuffer, 0 );
				if ( cbData == SOCKET_ERROR || cbData == 0 )
				{
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}

				ssl->cbIoBuffer += cbData;
			}
			else
			{
				fDoRead = true;
			}
		}

		// Set up the input buffers. Buffer 0 is used to pass in data
		// received from the server. Schannel will consume some or all
		// of this. Leftover data (if any) will be placed in buffer 1 and
		// given a buffer type of SECBUFFER_EXTRA.

		InBuffers[ 0 ].pvBuffer = ssl->pbIoBuffer;
		InBuffers[ 0 ].cbBuffer = ssl->cbIoBuffer;
		InBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;

		InBuffers[ 1 ].pvBuffer = NULL;
		InBuffers[ 1 ].cbBuffer = 0;
		InBuffers[ 1 ].BufferType = SECBUFFER_EMPTY;

		InBuffer.cBuffers = 2;
		InBuffer.pBuffers = InBuffers;
		InBuffer.ulVersion = SECBUFFER_VERSION;

		// Set up the output buffers. These are initialized to NULL
		// so as to make it less likely we'll attempt to free random
		// garbage later.

		OutBuffers[ 0 ].pvBuffer = NULL;
		OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
		OutBuffers[ 0 ].cbBuffer = 0;

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		scRet = g_pSSPI->InitializeSecurityContextA(
						&g_hCreds,
						&ssl->hContext,
						NULL,
						dwSSPIFlags,
						0,
						SECURITY_NATIVE_DREP,
						&InBuffer,
						0,
						NULL,
						&OutBuffer,
						&dwSSPIOutFlags,
						&tsExpiry );

		// If success (or if the error was one of the special extended ones), send the contents of the output buffer to the server.
		if ( scRet == SEC_E_OK || scRet == SEC_I_CONTINUE_NEEDED || FAILED( scRet ) && ( dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR ) )
		{
			if ( OutBuffers[ 0 ].pvBuffer != NULL )
			{
				if ( OutBuffers[ 0 ].cbBuffer != 0 )
				{
					cbData = _send( ssl->s, ( char * )OutBuffers[ 0 ].pvBuffer, OutBuffers[ 0 ].cbBuffer, 0 );
					if ( cbData == SOCKET_ERROR || cbData == 0 )
					{
						g_pSSPI->FreeContextBuffer( OutBuffers[ 0 ].pvBuffer );
						g_pSSPI->DeleteSecurityContext( &ssl->hContext );
						SecInvalidateHandle( &ssl->hContext );
						return SEC_E_INTERNAL_ERROR;
					}
				}

				// Free output buffer.
				g_pSSPI->FreeContextBuffer( OutBuffers[ 0 ].pvBuffer );
			}
		}

		// We need to read more data from the server and try again.
		if ( scRet == SEC_E_INCOMPLETE_MESSAGE )
		{
			continue;
		}

		// Handshake completed successfully.
		if ( scRet == SEC_E_OK )
		{
			// Store remaining data for further use
			if ( InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA )
			{
				_memmove( ssl->pbIoBuffer, ssl->pbIoBuffer + ( ssl->cbIoBuffer - InBuffers[ 1 ].cbBuffer ), InBuffers[ 1 ].cbBuffer );
				ssl->cbIoBuffer = InBuffers[ 1 ].cbBuffer;
			}
			else
			{
				ssl->cbIoBuffer = 0;
			}

			break;
		}

		// Check for fatal error.
		if ( FAILED( scRet ) )
		{
			break;
		}

		// Server requested client authentication. 
		if ( scRet == SEC_I_INCOMPLETE_CREDENTIALS )
		{
			// Go around again.
			fDoRead = false;
			scRet = SEC_I_CONTINUE_NEEDED;
			continue;
		}

		// Copy any leftover data from the buffer, and go around again.
		if ( InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA )
		{
			_memmove( ssl->pbIoBuffer, ssl->pbIoBuffer + ( ssl->cbIoBuffer - InBuffers[ 1 ].cbBuffer ), InBuffers[ 1 ].cbBuffer );

			ssl->cbIoBuffer = InBuffers[ 1 ].cbBuffer;
		}
		else
		{
			ssl->cbIoBuffer = 0;
		}
	}

	// Delete the security context in the case of a fatal error.
	if ( FAILED( scRet ) )
	{
		g_pSSPI->DeleteSecurityContext( &ssl->hContext );
		SecInvalidateHandle( &ssl->hContext );
	}

	if ( ssl->cbIoBuffer == 0 )
	{
		GlobalFree( ssl->pbIoBuffer );
		ssl->pbIoBuffer = NULL;
		ssl->sbIoBuffer = 0;
	}

	return scRet;
}

int SSL_connect( SSL *ssl, const char *host )
{
	SecBufferDesc OutBuffer;
	SecBuffer OutBuffers[ 1 ];
	DWORD dwSSPIFlags;
	DWORD dwSSPIOutFlags;
	TimeStamp tsExpiry;
	SECURITY_STATUS scRet;
	DWORD cbData;

	if ( ssl == NULL )
	{
		return 0;
	}

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT	|
				  ISC_REQ_REPLAY_DETECT		|
				  ISC_REQ_CONFIDENTIALITY	|
				  ISC_RET_EXTENDED_ERROR	|
				  ISC_REQ_ALLOCATE_MEMORY	|
				  ISC_REQ_STREAM;

	// Initiate a ClientHello message and generate a token.

	OutBuffers[ 0 ].pvBuffer = NULL;
	OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
	OutBuffers[ 0 ].cbBuffer = 0;

	OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	scRet = g_pSSPI->InitializeSecurityContextA(
					&g_hCreds,
					NULL,
					( SEC_CHAR * )host,
					dwSSPIFlags,
					0,
					SECURITY_NATIVE_DREP,
					NULL,
					0,
					&ssl->hContext,
					&OutBuffer,
					&dwSSPIOutFlags,
					&tsExpiry );

	if ( scRet != SEC_I_CONTINUE_NEEDED )
	{
		return 0;
	}

	// Send response to server if there is one.
	if ( OutBuffers[ 0 ].pvBuffer != NULL )
	{
		if ( OutBuffers[ 0 ].cbBuffer != 0 )
		{
			cbData = _send( ssl->s, (char * )OutBuffers[ 0 ].pvBuffer, OutBuffers[ 0 ].cbBuffer, 0 );
			if ( cbData == SOCKET_ERROR || cbData == 0 )
			{
				g_pSSPI->FreeContextBuffer( OutBuffers[ 0 ].pvBuffer );
				g_pSSPI->DeleteSecurityContext( &ssl->hContext );
				SecInvalidateHandle( &ssl->hContext );
				return 0;
			}
		}

		// Free output buffer.
		g_pSSPI->FreeContextBuffer( OutBuffers[ 0 ].pvBuffer );
	}

	return ClientHandshakeLoop( ssl, true ) == SEC_E_OK;
}

int SSL_shutdown( SSL *ssl )
{
	DWORD dwType;

	SecBufferDesc OutBuffer;
	SecBuffer OutBuffers[ 1 ];
	DWORD dwSSPIFlags;
	DWORD dwSSPIOutFlags;
	TimeStamp tsExpiry;
	DWORD Status;

	if ( ssl == NULL )
	{
		return SOCKET_ERROR;
	}

	dwType = SCHANNEL_SHUTDOWN;

	OutBuffers[ 0 ].pvBuffer = &dwType;
	OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
	OutBuffers[ 0 ].cbBuffer = sizeof( dwType );

	OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	Status = g_pSSPI->ApplyControlToken( &ssl->hContext, &OutBuffer );
	if ( FAILED( Status ) )
	{
		return Status;
	}

	// Build an SSL close notify message.

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT	|
				  ISC_REQ_REPLAY_DETECT		|
				  ISC_REQ_CONFIDENTIALITY	|
				  ISC_RET_EXTENDED_ERROR	|
				  ISC_REQ_ALLOCATE_MEMORY	|
				  ISC_REQ_STREAM;

	OutBuffers[ 0 ].pvBuffer = NULL;
	OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
	OutBuffers[ 0 ].cbBuffer = 0;

	OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	Status = g_pSSPI->InitializeSecurityContextA(
					&g_hCreds,
					&ssl->hContext,
					NULL,
					dwSSPIFlags,
					0,
					SECURITY_NATIVE_DREP,
					NULL,
					0,
					&ssl->hContext,
					&OutBuffer,
					&dwSSPIOutFlags,
					&tsExpiry );

	if ( FAILED( Status ) )
	{
		return Status;
	}

	// Send the close notify message to the server.
	if ( OutBuffers[ 0 ].pvBuffer != NULL )
	{
		if ( OutBuffers[ 0 ].cbBuffer != 0 )
		{
			_send( ssl->s, ( char * )OutBuffers[ 0 ].pvBuffer, OutBuffers[ 0 ].cbBuffer, 0 );
		}
		g_pSSPI->FreeContextBuffer( OutBuffers[ 0 ].pvBuffer );
	}

	// Free the security context.
	g_pSSPI->DeleteSecurityContext( &ssl->hContext );
	SecInvalidateHandle( &ssl->hContext );

	return Status;
}

int SSL_read( SSL *ssl, char *buf, int num )
{
	SECURITY_STATUS scRet;
	DWORD cbData;
	int i;

	SecBufferDesc Message;
	SecBuffer Buffers[ 4 ];
	SecBuffer *pDataBuffer;
	SecBuffer *pExtraBuffer;

	_memzero( Buffers, sizeof( SecBuffer ) * 4 );

	if ( ssl == NULL )
	{
		return SOCKET_ERROR;
	}

	if ( num == 0 )
	{
		return 0;
	}

	if ( ssl->cbRecDataBuf != 0 )
	{
		DWORD bytes = min( ( DWORD )num, ssl->cbRecDataBuf );
		_memcpy_s( buf, num, ssl->pbRecDataBuf, bytes );

		DWORD rbytes = ssl->cbRecDataBuf - bytes;
		_memmove( ssl->pbRecDataBuf, ( ( char * )ssl->pbRecDataBuf ) + bytes, rbytes );
		ssl->cbRecDataBuf = rbytes;

		return bytes;
	}

	scRet = SEC_E_OK;

	while ( true )
	{
		if ( ssl->cbIoBuffer == 0 || scRet == SEC_E_INCOMPLETE_MESSAGE )
		{
			if ( ssl->sbIoBuffer <= ssl->cbIoBuffer )
			{
				ssl->sbIoBuffer += 2048;

				if ( ssl->pbIoBuffer == NULL )
				{
					ssl->pbIoBuffer = ( PUCHAR )GlobalAlloc( GPTR, ssl->sbIoBuffer );
				}
				else
				{
					ssl->pbIoBuffer = ( PUCHAR )GlobalReAlloc( ssl->pbIoBuffer, ssl->sbIoBuffer, GMEM_MOVEABLE );
				}
			}

			cbData = _recv( ssl->s, ( char * )ssl->pbIoBuffer + ssl->cbIoBuffer, ssl->sbIoBuffer - ssl->cbIoBuffer, 0 );
			if ( cbData == SOCKET_ERROR )
			{
				return SOCKET_ERROR;
			}
			else if ( cbData == 0 )
			{
				// Server disconnected.
				if ( ssl->cbIoBuffer )
				{
					scRet = SEC_E_INTERNAL_ERROR;
					return SOCKET_ERROR;
				}
				else
				{
					return 0;
				}
			}
			else
			{
				ssl->cbIoBuffer += cbData;
			}
		}

		// Attempt to decrypt the received data.
		Buffers[ 0 ].pvBuffer = ssl->pbIoBuffer;
		Buffers[ 0 ].cbBuffer = ssl->cbIoBuffer;
		Buffers[ 0 ].BufferType = SECBUFFER_DATA;

		Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
		Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
		Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

		Message.ulVersion = SECBUFFER_VERSION;
		Message.cBuffers = 4;
		Message.pBuffers = Buffers;

		if ( g_pSSPI->DecryptMessage != NULL )
		{
			scRet = g_pSSPI->DecryptMessage( &ssl->hContext, &Message, 0, NULL );
		}
		else
		{
			scRet = ( ( DECRYPT_MESSAGE_FN )g_pSSPI->Reserved4 )( &ssl->hContext, &Message, 0, NULL );
		}

		if ( scRet == SEC_E_INCOMPLETE_MESSAGE )
		{
			// The input buffer contains only a fragment of an
			// encrypted record. Loop around and read some more
			// data.
			continue;
		}

		// Server signaled end of session
		if ( scRet == SEC_I_CONTEXT_EXPIRED )
		{
			SSL_shutdown( ssl );
			return 0;
		}

		if ( scRet != SEC_E_OK && scRet != SEC_I_RENEGOTIATE )
		{
			return SOCKET_ERROR;
		}

		// Locate data and (optional) extra buffers.
		pDataBuffer = NULL;
		pExtraBuffer = NULL;
		for ( i = 1; i < 4; i++ )
		{
			if ( pDataBuffer == NULL && Buffers[ i ].BufferType == SECBUFFER_DATA )
			{
				pDataBuffer = &Buffers[ i ];
			}

			if ( pExtraBuffer == NULL && Buffers[ i ].BufferType == SECBUFFER_EXTRA )
			{
				pExtraBuffer = &Buffers[ i ];
			}
		}

		// Return decrypted data.
		DWORD bytes = 0;
		if ( pDataBuffer != NULL )
		{
			bytes = min( ( DWORD )num, pDataBuffer->cbBuffer );
			_memcpy_s( buf, num, pDataBuffer->pvBuffer, bytes );

			DWORD rbytes = pDataBuffer->cbBuffer - bytes;
			if ( rbytes > 0 )
			{
				if ( ssl->sbRecDataBuf < rbytes ) 
				{
					ssl->sbRecDataBuf = rbytes;

					if ( ssl->pbRecDataBuf == NULL )
					{
						ssl->pbRecDataBuf = ( PUCHAR )GlobalAlloc( GPTR, ssl->sbRecDataBuf );
					}
					else
					{
						ssl->pbRecDataBuf = ( PUCHAR )GlobalReAlloc( ssl->pbRecDataBuf, ssl->sbRecDataBuf, GMEM_MOVEABLE );
					}
				}

				_memcpy_s( ssl->pbRecDataBuf, ssl->sbRecDataBuf, ( char * )pDataBuffer->pvBuffer + bytes, rbytes );
				ssl->cbRecDataBuf = rbytes;
			}
		}

		// Move any "extra" data to the input buffer.
		if ( pExtraBuffer != NULL )
		{
			_memmove( ssl->pbIoBuffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer );
			ssl->cbIoBuffer = pExtraBuffer->cbBuffer;
		}
		else
		{
			ssl->cbIoBuffer = 0;
		}

		if ( pDataBuffer != NULL && bytes != 0 )
		{
			return bytes;
		}

		if ( scRet == SEC_I_RENEGOTIATE )
		{
			// The server wants to perform another handshake sequence.
			scRet = ClientHandshakeLoop( ssl, false );
			if ( scRet != SEC_E_OK )
			{
				return SOCKET_ERROR;
			}
		}
	}
}

int SSL_write( SSL *ssl, const char *buf, int num )
{
	SecPkgContext_StreamSizes Sizes;
	SECURITY_STATUS scRet;
	DWORD cbData;

	SecBufferDesc Message;
	SecBuffer Buffers[ 4 ];

	PUCHAR pbDataBuffer;

	DWORD cbMessage;

	DWORD sent = 0;

	_memzero( Buffers, sizeof( SecBuffer ) * 4 );

	if ( ssl == NULL )
	{
		return SOCKET_ERROR;
	}

	scRet = g_pSSPI->QueryContextAttributesA( &ssl->hContext, SECPKG_ATTR_STREAM_SIZES, &Sizes );
	if ( scRet != SEC_E_OK )
	{
		return scRet;
	}

	pbDataBuffer = ( PUCHAR )GlobalAlloc( GPTR, Sizes.cbHeader + Sizes.cbMaximumMessage + Sizes.cbTrailer );

	while ( sent < ( DWORD )num )
	{
		cbMessage = min( Sizes.cbMaximumMessage, ( DWORD )num - sent );
		_memcpy_s( pbDataBuffer + Sizes.cbHeader, Sizes.cbMaximumMessage, buf + sent, cbMessage );

		Buffers[ 0 ].pvBuffer = pbDataBuffer;
		Buffers[ 0 ].cbBuffer = Sizes.cbHeader;
		Buffers[ 0 ].BufferType = SECBUFFER_STREAM_HEADER;

		Buffers[ 1 ].pvBuffer = pbDataBuffer + Sizes.cbHeader;
		Buffers[ 1 ].cbBuffer = cbMessage;
		Buffers[ 1 ].BufferType = SECBUFFER_DATA;

		Buffers[ 2 ].pvBuffer = pbDataBuffer + Sizes.cbHeader + cbMessage;
		Buffers[ 2 ].cbBuffer = Sizes.cbTrailer;
		Buffers[ 2 ].BufferType = SECBUFFER_STREAM_TRAILER;

		Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

		Message.ulVersion = SECBUFFER_VERSION;
		Message.cBuffers = 4;
		Message.pBuffers = Buffers;

		if ( g_pSSPI->EncryptMessage != NULL )
		{
			scRet = g_pSSPI->EncryptMessage( &ssl->hContext, 0, &Message, 0 );
		}
		else
		{
			scRet = ( ( ENCRYPT_MESSAGE_FN)g_pSSPI->Reserved3 )( &ssl->hContext, 0, &Message, 0 );
		}

		if ( FAILED( scRet ) )
		{
			break;
		}

		// Calculate encrypted packet size 
		cbData = Buffers[ 0 ].cbBuffer + Buffers[ 1 ].cbBuffer + Buffers[ 2 ].cbBuffer;

		// Send the encrypted data to the server.
		cbData = _send( ssl->s, ( char * )pbDataBuffer, cbData, 0 );
		if ( cbData == SOCKET_ERROR || cbData == 0 )
		{
			g_pSSPI->DeleteSecurityContext( &ssl->hContext );
			SecInvalidateHandle( &ssl->hContext );
			scRet = SEC_E_INTERNAL_ERROR;
			break;
		}

		sent += cbMessage;
	}

	GlobalFree( pbDataBuffer );

	return scRet == SEC_E_OK ? num : SOCKET_ERROR;
}
