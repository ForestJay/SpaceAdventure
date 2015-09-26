// Static.cpp - for Space Adventure 
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998 - 1999
////////////////////////////

#include "wave.h"		//Our wave header
#include <dsound.h>		//The direct sound header
#include "static.h"		//Our static header
#include <windowsx.h>	//The windows API header

extern LPDIRECTSOUND lpds;			//Pointer to Direct Sound
LPDIRECTSOUNDBUFFER  lpdsbStatic;	//Pointer to a static buffer



/* --------------------------------------------------------

   LoadStatic()
   Loads a sound from file, creating a static buffer for it
   if necessary.

   -------------------------------------------------------- */

bool LoadStatic( LPDIRECTSOUND lpds,	//Pointer to Direct Sound 
				LPSTR lpzFileName )		//File name of the sound
{
    WAVEFORMATEX    *pwfx;            // Wave format info.
    HMMIO           hmmio;            // File handle.
    MMCKINFO        mmckinfo;         // Chunk info.
    MMCKINFO        mmckinfoParent;   // Parent chunk info.

	//Open the file
    if ( WaveOpenFile( lpzFileName, &hmmio, &pwfx, &mmckinfoParent ) != 0 )
        return FALSE;
 
	//Read the file
    if ( WaveStartDataRead( &hmmio, &mmckinfo, &mmckinfoParent ) != 0 )
        return FALSE;

    // Create buffer if it doesn't yet exist.
 
    DSBUFFERDESC         dsbdesc;	//buffer description

    // If the buffer already exists, we're just reloading the 
    // wave file after a call to Restore().

	//get buffer info
    memset( &dsbdesc, 0, sizeof( DSBUFFERDESC ) ); 
    dsbdesc.dwSize = sizeof( DSBUFFERDESC ); 
    dsbdesc.dwFlags = DSBCAPS_STATIC; 
    dsbdesc.dwBufferBytes = mmckinfo.cksize;  
    dsbdesc.lpwfxFormat = pwfx; 

	//create the buffer
    if ( FAILED( lpds->CreateSoundBuffer( 
            &dsbdesc, &lpdsbStatic, NULL ) ) )
    {
		//we failed so close the file and leave
        WaveCloseReadFile( &hmmio, &pwfx );
        return FALSE; 
    }

    LPVOID lpvAudio1;
    DWORD  dwBytes1;

	//Lock the buffer
    if ( FAILED( lpdsbStatic->Lock(
            0,              // Offset of lock start.
            0,              // Size of lock; ignored in this case.
            &lpvAudio1,     // Address of lock start.
            &dwBytes1,      // Number of bytes locked.
            NULL,           // Address of wraparound start.
            NULL,           // Number of wraparound bytes.
            DSBLOCK_ENTIREBUFFER ) ) )  // Flag.
    {
        // Error handling.
        WaveCloseReadFile( &hmmio, &pwfx );
        return FALSE;
    }
 
    UINT cbBytesRead;	//Number of bytes read
 
    if ( WaveReadFile( hmmio,     // File handle.
            dwBytes1,             // Number of bytes to read.
            ( BYTE * )lpvAudio1,  // Destination.
            &mmckinfo,            // File chunk info.
            &cbBytesRead ) )      // Actual number of bytes read.
    {
        // Handle failure on nonzero return.
        WaveCloseReadFile( &hmmio, &pwfx );
        return FALSE;
    }

	//unlock the buffer
    lpdsbStatic->Unlock( lpvAudio1, dwBytes1, NULL, 0 );

	//close the file
    WaveCloseReadFile( &hmmio, &pwfx );

    return TRUE;
}  // LoadStatic


/* --------------------------------------------------------

   PlayStatic()
   Starts the static buffer from the beginning

   -------------------------------------------------------- */


void PlayStatic( int wavenumber )	//This number identifies the sound
{
    HRESULT hr;	//The return result

	//If there is no buffer we can't play anything so leave
    if ( lpdsbStatic == NULL ) return;

	//If the buffer is already playing we MUST set the play position
    lpdsbStatic->SetCurrentPosition( 0 );
    hr = lpdsbStatic->Play( 0, 0, 0 );    // OK if redundant.
 
	//Did we lose the buffer to another, more devious app?
	if ( hr == DSERR_BUFFERLOST )
    {
		//restore the buffer
        if ( SUCCEEDED( lpdsbStatic->Restore() ) )
        {
			//Play the correct sound
			if(wavenumber == 1)
			{
				if ( LoadStatic( lpds, SHOTWAVE ) )
					lpdsbStatic->Play( 0, 0, 0 );
			}
			else if(wavenumber == 2)
			{
				if ( LoadStatic( lpds, HITWAVE ) )
					lpdsbStatic->Play( 0, 0, 0 );
			}
			else
			{
				if ( LoadStatic( lpds, WELCOMEWAVE ) )
					lpdsbStatic->Play( 0, 0, 0 );
			}
        }
    }
}
