// Sound.cpp - for Space Adventure
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998 - 1999
////////////////////////////

#include "sound.h"		//Our sound header
#include "wave.h"		//Our wave header
#include <stdio.h>		//the standard I/O header
#include "resource.h"	//Our resource header

LPDIRECTSOUND               lpds;			//pointer to direct sound
LPDIRECTSOUNDBUFFER         lpdsb;			//pointer to SirectDound buffer
LPDIRECTSOUNDBUFFER         lpdsbPrimary;	//pointer to the primary sound buffer
WAVEFORMATEX                *pwfx;			//The wave file format
HMMIO                       hmmio;			//I/O stuff
MMCKINFO                    mmckinfo, mmckinfoParent;
DWORD                       dwMidBuffer;	//The middle of the buffer
   
/* --------------------------------------------------------

   FillBufferWithSilence()
   Write silence to entire buffer

   -------------------------------------------------------- */

bool FillBufferWithSilence( LPDIRECTSOUNDBUFFER lpDsb )
{
    WAVEFORMATEX    wfx;			//The wave format
    DWORD           dwSizeWritten;	//The size of what has been written

    PBYTE   pb1;
    DWORD   cb1;

	//get the wave files format
    if ( FAILED( lpDsb->GetFormat( &wfx, sizeof( WAVEFORMATEX ), &dwSizeWritten ) ) )
        return FALSE;

	//Lock the buffer
    if ( SUCCEEDED( lpDsb->Lock( 0, 0, 
                         ( LPVOID * )&pb1, &cb1, 
                         NULL, NULL, DSBLOCK_ENTIREBUFFER ) ) )
    {
        FillMemory( pb1, cb1, ( wfx.wBitsPerSample == 8 ) ? 128 : 0 );

        lpDsb->Unlock( pb1, cb1, NULL, 0 );
        return TRUE;
    }

    return FALSE;
}  // FillBufferWithSilence


/* --------------------------------------------------------

   SetupStreamBuffer()
   Create a streaming buffer in same format as wave data

   -------------------------------------------------------- */

bool SetupStreamBuffer( LPSTR lpzFileName )
{
    DSBUFFERDESC    dsbdesc;	//The buffer description
    HRESULT         hr;			//the return result

    // Close any open file and free the buffer.
    WaveCloseReadFile( &hmmio, &pwfx );
    if ( lpdsb != NULL )
    {
        lpdsb->Release();	//release the buffer
        lpdsb = NULL;		//empty the buffer
    }

    // Open the file, get wave format, and descend to the data chunk.
    if ( WaveOpenFile( lpzFileName, &hmmio, &pwfx, &mmckinfoParent ) != 0 )
        return FALSE;
    if ( WaveStartDataRead( &hmmio, &mmckinfo, &mmckinfoParent ) != 0 )
        return FALSE;

    // Create secondary buffer able to hold 2 seconds of data.
    memset( &dsbdesc, 0, sizeof( DSBUFFERDESC ) ); 
    dsbdesc.dwSize = sizeof( DSBUFFERDESC ); 
    dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 
                    | DSBCAPS_GLOBALFOCUS
                    | DSBCAPS_CTRLPAN; 
    dsbdesc.dwBufferBytes = pwfx->nAvgBytesPerSec * 2;  
    dsbdesc.lpwfxFormat = pwfx; 
 
    if ( FAILED( hr = lpds->CreateSoundBuffer( &dsbdesc, &lpdsb, NULL ) ) )
    {
        WaveCloseReadFile( &hmmio, &pwfx );
        return FALSE; 
    }

    FillBufferWithSilence( lpdsb );
    hr = lpdsb->Play( 0, 0, DSBPLAY_LOOPING );

    dwMidBuffer = dsbdesc.dwBufferBytes / 2;

    return TRUE;
} // SetupStreamBuffer


/* --------------------------------------------------------
   
   PanStreaming()
   Pan the steaming buffer in response to keypress 

   -------------------------------------------------------- */

void PanStreaming( int PanShift )
{
    LONG lCurrent;	//current position

    if ( lpdsb != NULL )
    {
        lpdsb->GetPan( &lCurrent );
        lCurrent += PanShift;
        if ( lCurrent > DSBPAN_RIGHT ) lCurrent = DSBPAN_RIGHT;
        if ( lCurrent < DSBPAN_LEFT ) lCurrent = DSBPAN_LEFT;
        lpdsb->SetPan( lCurrent );
    }
}


/* --------------------------------------------------------

   PlayBuffer()
   Stream data to the buffer every time the current play position
   has covered half the distance.

   -------------------------------------------------------- */

bool PlayBuffer( void )
{
    HRESULT         hr;				//The return result
    DWORD           dwPlay;			//Play cursor
    DWORD           dwStartOfs;		//Offset of lock start
    static DWORD    dwLastPlayPos;	//the last play position
    VOID            *lpvData;		//address of lock start
    DWORD           dwBytesLocked;	//number of bytes locked
    UINT            cbBytesRead;	//number of bytes read

	//I there isn't a buffer leave
    if ( lpdsb == NULL ) return FALSE;

	//Get the current play position
    if ( FAILED( lpdsb->GetCurrentPosition( &dwPlay, NULL ) ) ) 
		return FALSE;

    // If the play cursor has just reached the first or second half of the
    // buffer, it's time to stream data to the other half.
    if ( ( ( dwPlay >= dwMidBuffer ) && ( dwLastPlayPos < dwMidBuffer ) )
        || ( dwPlay < dwLastPlayPos ) )
    {
		//Get the lock start offset by comparing the play position to the middle
        dwStartOfs = ( dwPlay >= dwMidBuffer ) ? 0 : dwMidBuffer;	

		//Lock and setup a return value
        hr = lpdsb->Lock( dwStartOfs, // Offset of lock start.
                    dwMidBuffer,      // Number of bytes to lock.
                    &lpvData,         // Address of lock start.
                    &dwBytesLocked,   // Number of bytes locked.
                    NULL,             // Address of wraparound lock.
                    NULL,             // Number of wraparound bytes.
                    0 );              // Flags.
  
		//Read the wave file
        WaveReadFile( hmmio,             // File handle.
                      dwBytesLocked,     // Number of bytes to read.
                      ( BYTE * )lpvData, // Destination.
                      &mmckinfo,         // File chunk info.
                      &cbBytesRead );    // Number of bytes read.

        if ( cbBytesRead < dwBytesLocked )  // Reached end of file.
        {
            if ( WaveStartDataRead( &hmmio, &mmckinfo, &mmckinfoParent ) 
				 != 0 )
            {
                OutputDebugString( "Can't reset file.\n" );
            }
            else
            {
                WaveReadFile( hmmio,          
                              dwBytesLocked - cbBytesRead,
                              ( BYTE * )lpvData + cbBytesRead, 
                              &mmckinfo,      
                              &cbBytesRead );    
            }
        }

		//Unlock the buffer
        lpdsb->Unlock( lpvData, dwBytesLocked, NULL, 0 );
    }

	//Reset the last play position for later
    dwLastPlayPos = dwPlay;
    return TRUE;
} // PlayBuffer


/* --------------------------------------------------------

   InitDSound()
   Initialize DirectSound

   -------------------------------------------------------- */

bool InitDSound( HWND hwnd, HINSTANCE hinst, GUID *pguid )
{
    HRESULT             hr;		//The return result
    DSCAPS              dscaps;	//The local capabilities
 
    // Create DirectSound.
    if ( FAILED( hr = DirectSoundCreate( pguid, &lpds, NULL ) ) )
        return FALSE;

    // Set cooperative level.
    if ( FAILED( hr = lpds->SetCooperativeLevel( hwnd, DSSCL_PRIORITY ) ) )
        return FALSE;

    // Get capabilities; we don't actually do anything with these.
    dscaps.dwSize = sizeof( DSCAPS );
    hr = lpds->GetCaps( &dscaps );

    return TRUE;
}  // InitDSound()


/* --------------------------------------------------------

   Cleanup()
   Cleans up DirectSound objects and closes any open wave file

   -------------------------------------------------------- */

void Cleanup( void )
{
    WaveCloseReadFile( &hmmio, &pwfx );
    if ( lpds ) lpds->Release();  // This releases buffers as well.
}


/* --------------------------------------------------------

    EnumCallback()

    Enumerates available devices and populates listbox;
    called for each device by DirectInputEnumerate().

    The GUID for the device is stored as the item data for
    each string added to the list.

   -------------------------------------------------------- */

bool CALLBACK EnumCallback( LPGUID lpGuid,            
                            LPCSTR lpstrDescription,  
                            LPCSTR lpstrModule,       
                            LPVOID hwnd )          
{
    LONG    i;
    LPGUID  lpTemp = NULL;

    // lpGuid is NULL when "Primary Sound Driver" is enumerated.    
	if ( lpGuid != NULL )
    {
        if ( ( lpTemp = ( LPGUID )LocalAlloc( LPTR, sizeof( GUID ) ) ) == NULL )
            return TRUE;

        memcpy( lpTemp, lpGuid, sizeof( GUID ) );
    }

    // Add the friendly name of the device to the list box.
    i = SendDlgItemMessage( ( HWND )hwnd, IDC_LIST1, 
                            LB_ADDSTRING, 0, 
                            ( LPARAM )lpstrDescription );

    // Save the GUID as the item data.
    SendDlgItemMessage( ( HWND )hwnd, IDC_LIST1, 
                        LB_SETITEMDATA, i, ( LPARAM )lpTemp );

    return TRUE;
} // EnumCallback

