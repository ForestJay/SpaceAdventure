// ddtools.cpp for Space Adventure 
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998 - 1999
////////////////////////////
#define WIN32_LEAN_AND_MEAN	// Tell the proccessor we want fast code
#include <windows.h>		// The windows header
#include <windowsx.h>		// The windows messages header
#include <ddraw.h>			// The direct draw header
#include "space.h"			// Our header

//This will initialize Direct Draw
HRESULT DDStartup( LPDIRECTDRAW* lplpDD, GUID FAR* lpGUID,
                        HWND hwnd, bool bFullScreen )
{
    HRESULT ddrval;	//The return value

	//Setup Direct Draw
    ddrval = DirectDrawCreate( lpGUID, lplpDD, NULL );
    if FAILED( ddrval ) 
    {
        return ddrval;
    }

    if ( bFullScreen )
    {
        // If fullscreen, get fullscreen exclusive mode
        ddrval = (*lplpDD)->SetCooperativeLevel( hwnd,
                            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN );
        if FAILED( ddrval )
        {
            OutputDebugString( 
				"DDStartup: Couldn't set exclusive mode.\n" );
            return ddrval;
        }

        OutputDebugString( 
			"DDStartup: Setting exclusive mode...\n" );

        if SUCCEEDED( ddrval )
        {
            // Set 640 x 480, 8 bits
            ddrval = (*lplpDD)->SetDisplayMode( 640, 480, 8 );
            return ddrval;
        }
        else
        {
            return ddrval;
        }
    }
    else
    {
        // otherwise, set normal mode for use in a window
        ddrval = (*lplpDD)->SetCooperativeLevel( hwnd,
                            DDSCL_NORMAL );
        OutputDebugString( "DDStartup: Setting windowed mode...\n" );
    }
    return ddrval;
}

//This is going to setup the primary and back buffers
HRESULT DDFullConfigure( LPDIRECTDRAW lpDD, 
                            LPDIRECTDRAWSURFACE* lplpDDSPrimary,
                            LPDIRECTDRAWSURFACE* lplpDDSBack )
{
    // Attempt flipping surface with 2 back buffers
    if FAILED ( DDCreateFlipper( lpDD, lplpDDSPrimary, lplpDDSBack, 2 ) )
    {
        // Couldn't get two, try one.
        if FAILED( DDCreateFlipper( lpDD, lplpDDSPrimary, lplpDDSBack, 1 ) )
        {
            // Couldn't even get one. Maybe flipping isn't supported
            // or we have a very small display memory
            if FAILED( DDCreateFakeFlipper( lpDD, lplpDDSPrimary,
                                                    lplpDDSBack) )
            {
                OutputDebugString( 
					"DDFullConfigure: Couldn't create fake flipper.\n" );
                return FALSE;
            }
            else
            {
                OutputDebugString( 
					"DDFullConfigure: Using fake flipper.\n" );
            }
        }
        // Load art where ever it will fit
        return( LoadGameArt( 0, NULL ) );
    }
    else
    {
        if FAILED ( LoadGameArt( DDSCAPS_OFFSCREENPLAIN | 
									DDSCAPS_VIDEOMEMORY, NULL ) )
        {
            // The art wouldn't fit with the double buffers.
            // Release them and back off to a single buffer.
            (*lplpDDSPrimary)->Release();
            if FAILED( DDCreateFlipper ( lpDD, lplpDDSPrimary, lplpDDSBack, 1 ) )
            {
                OutputDebugString( 
					"DDFullConfigure: Single back buffer failed.\n" );
                return FALSE;
            }
            // Load art where ever it will fit
			OutputDebugString( 
				"DDFullConfigure: Using two back buffers, art in display.\n" );
            return( LoadGameArt( DDSCAPS_OFFSCREENPLAIN, NULL ) );
        }
        else
        {
            OutputDebugString( 
				"DDFullConfigure: Using two back buffers, art in display.\n" );
        }
    }
    return TRUE;
}
            
//This is going to take the back buffer after it's drawn an flip it
//to the Primary surface AKA the screen
HRESULT DDCreateFlipper( LPDIRECTDRAW lpDD,
                            LPDIRECTDRAWSURFACE* lplpDDSPrimary,
                            LPDIRECTDRAWSURFACE* lplpDDSBack,
                            DWORD dwBufferCount )
{
    DDSURFACEDESC	ddsd;		//Surface description
    DDSCAPS			ddscaps;	//Clients DD hardware capability
    HRESULT			ddrval;		//return value of this function

    ddsd.dwSize = sizeof( ddsd );	// record the descriptions size
    
    // Create the primary surface with  back buffers
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
                              DDSCAPS_FLIP |
                              DDSCAPS_COMPLEX |
                              DDSCAPS_VIDEOMEMORY;
    ddsd.dwBackBufferCount = dwBufferCount;
	ddsd.dwWidth = 640;
	ddsd.dwHeight = 480;

	//Try to create the primary surface
    ddrval = lpDD->CreateSurface( &ddsd, lplpDDSPrimary, NULL );
    //If you can't create the primary surface exit
	if FAILED( ddrval ) return ddrval;
        
    // Get a pointer to the back buffer
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    ddrval = (*lplpDDSPrimary)->GetAttachedSurface( &ddscaps, lplpDDSBack );

    return ddrval;
}

//This will create an overlay surface for flipping
HRESULT DDCreateOverFlipper( LPDIRECTDRAW lpDD,
								LPDIRECTDRAWSURFACE* lplpDDSOverlay,
								LPDIRECTDRAWSURFACE* lplpDDSBack,
								DWORD dwBufferCount )
{
    DDSURFACEDESC	ddsd;		//Surface description
    DDSCAPS			ddscaps;	//Clients DD hardware capability
    HRESULT			ddrval;		//return value of this function

    ddsd.dwSize = sizeof( ddsd );	// record the descriptions size

	// We're only going to try one format in Space Adventure 1. The Overlay
	// sample tries two, and the SDK's Mosquito tries YUV formats
	// as well.

	// Set up the surface format, 16 bit RGB 565
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    ddsd.ddpfPixelFormat.dwFourCC = 0;
    ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
    ddsd.ddpfPixelFormat.dwRBitMask = 0x7C00;	//0x7C00 is a hexadecimal memory address
    ddsd.ddpfPixelFormat.dwGBitMask = 0x03e0;
    ddsd.ddpfPixelFormat.dwBBitMask = 0x001F;
    ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;

    // Create the overlay surface with  back buffers
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT | 
					DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OVERLAY |
                              DDSCAPS_FLIP |
                              DDSCAPS_COMPLEX |
                              DDSCAPS_VIDEOMEMORY;
    ddsd.dwBackBufferCount = dwBufferCount;
	ddsd.dwWidth = 640;
	ddsd.dwHeight = 480;

	//Try to create the overlay surface
	ddrval = lpDD->CreateSurface( &ddsd, lplpDDSOverlay, NULL );
    //if you fail exit
	if FAILED( ddrval ) return ddrval;
        
    // Get a pointer to the back buffer
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    ddrval = (*lplpDDSOverlay)->GetAttachedSurface( &ddscaps, lplpDDSBack );

    return ddrval;
}

//This will create an overlay
HRESULT DDCreateOverlay( LPDIRECTDRAW lpDD,
                            LPDIRECTDRAWSURFACE* lplpDDSPrimary,
							LPDIRECTDRAWSURFACE* lplpDDSOverlay,
                            LPDIRECTDRAWSURFACE* lplpDDSBack )
{
    DDSURFACEDESC	ddsd;	//Surface description
    HRESULT			ddrval; //The return result of this function

	// We will need a primary surface to display the overlay on

	//get the surface descriptions size
    ddsd.dwSize = sizeof( ddsd );
    // Create the primary surface
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	//Try to create the surface
    ddrval = lpDD->CreateSurface( &ddsd, lplpDDSPrimary, NULL );
    //if you fail return the result and send a message to the debugger.
	if FAILED( ddrval ) 
    {
        OutputDebugString( "DDCreateOverlay: Couldn't create primary.\n" );
        return ddrval;
    }

	// Create the flipping overlay surface

	ddrval = DDCreateOverFlipper( lpDD, lplpDDSOverlay, 
									lplpDDSBack, 1 );
	if FAILED( ddrval )
	{
		OutputDebugString( "DDCreateOverlay: Couldn't create flipper.\n" );
		return ddrval;
	}
	return ddrval;
}

//This will be used if a hardware flipper is unavailable
HRESULT DDCreateFakeFlipper( LPDIRECTDRAW lpDD,
                                LPDIRECTDRAWSURFACE* lplpDDSPrimary,
                                LPDIRECTDRAWSURFACE* lplpDDSBack )
{
    DDSURFACEDESC	ddsd;	// Surface Description
    HRESULT			ddrval;	// The return value

	//Store the size of the surface description
    ddsd.dwSize = sizeof( ddsd );
    // Create the primary surface
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	//Try to create the primary surface
    ddrval = lpDD->CreateSurface( &ddsd, lplpDDSPrimary, NULL );
    //if you fail output to the debugger and exit
	if FAILED( ddrval ) 
    {
        OutputDebugString( 
			"DDCreateFakeFlipper: Couldn't create primary.\n" );
        return ddrval;
    }

    // Create an offscreen surface to serve as the backbuffer
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;    
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
	//Try to create back buffer surface
    ddrval = lpDD->CreateSurface( &ddsd, lplpDDSBack, NULL );
    // If the surface isn't created output to the debugger and exit
	if FAILED( ddrval ) 
    {
        OutputDebugString( 
			"DDCreateFakeFlipper: Couldn't create backbuffer.\n" );
        return ddrval;
    }

	// If we got this far we can use the fake flipper.  Output
	// to the debugger the type of flipper then exit.
    OutputDebugString( "DDCreateFakeFlipper: Using fake flipper.\n" );
    return ddrval;
}

//This will set-up our Window
HRESULT DDWinConfigure( LPDIRECTDRAW lpDD,
							LPDIRECTDRAWSURFACE* lplpDDSPrimary,
							LPDIRECTDRAWSURFACE* lplpDDSBack,
							LPDIRECTDRAWCLIPPER* lplpDDClipper,
							HWND hWnd )
{
    HRESULT	ddrval;	//The return value

	//Try to create a fake flipper
	ddrval = DDCreateFakeFlipper( lpDD, lplpDDSPrimary, lplpDDSBack );
	//If the flipper doesn't work exit
	if FAILED( ddrval ) return ddrval;

	// Create a clipper and attach it to the primary surface
	ddrval = lpDD->CreateClipper( 0, lplpDDClipper, NULL );
	//if the clipper isn't made exit
	if FAILED( ddrval ) return ddrval;

	//attach the clipper to the window
	ddrval = (*lplpDDClipper)->SetHWnd( 0, hWnd );
	// If it won't attach exit
	if FAILED( ddrval ) return ddrval;

	(*lplpDDSPrimary)->SetClipper( *lplpDDClipper );
	// So clipper will go away "automatically" when the primary
	// is released.
	(*lplpDDClipper)->Release();

	// Load art where ever it will fit in the memory
	return( LoadGameArt( DDSCAPS_OFFSCREENPLAIN, NULL ) );

}

// This will fill or draw our surface . .  directly in fact!!
HRESULT DDFillSurface( LPDIRECTDRAWSURFACE lpDDSurface, DWORD color )
{
    DDBLTFX     ddbltfx;	// Direct Draw blit
    HRESULT     ddrval;		//The return value of the function

	//set the size and color
    ddbltfx.dwSize = sizeof( ddbltfx );
    ddbltfx.dwFillColor = color;

	//Blit the surface and put the return value in ddrval
    ddrval = lpDDSurface->Blt(
                    NULL,                 
                    NULL,          
                    NULL, 
                    DDBLT_COLORFILL | DDBLT_WAIT,
                    &ddbltfx );

    return ddrval;
}

// This will output text
HRESULT DDTextOut( LPDIRECTDRAWSURFACE lpDDSSurface, 
                                char* lpString, 
                                COLORREF BackColor,
                                COLORREF TextColor,
                                int posx,
                                int posy)
{  
    HRESULT ddrval;	//The return value
    HDC hdc;		//The device context
     
	//Try to get a device context
    ddrval = lpDDSSurface->GetDC( &hdc );
    if SUCCEEDED( ddrval )
    {
		//Color the background black
        SetBkColor( hdc, BackColor );
		//Have text the same color as the passed value
        SetTextColor( hdc, TextColor);
		//Send the text string to posx,posy
        TextOut( hdc, posx, posy, lpString, lstrlen(lpString) );
        //Release the device context
		lpDDSSurface->ReleaseDC( hdc );
    }
    return ddrval;
}

//Check to see if overlays are available
DWORD DDCheckOverlay( GUID *lpGUID )
{
    LPDIRECTDRAW	lpDD;		//Direct Draw Pointer
	DDCAPS			ddCaps;		//Client hardware capabilities
	DWORD			dwResult;	//The return value

	// A non-zero result indicates overlays are available,
	// and the required stretch factor is returned.

	dwResult = 0;	//Initialize to unavailable

	//Try to create and if we fail exit
	//Note that the goto keeps us from having too much code
	if FAILED( DirectDrawCreate( lpGUID, &lpDD, NULL ) ) return dwResult;

    // Check the hardware caps for "uncomplicated"
    // overlay support

    ddCaps.dwSize = sizeof( DDCAPS );
    lpDD->GetCaps( &ddCaps, NULL );

	//If their is complicated overlay support skip to CleanUp
    if (!( ddCaps.dwCaps & DDCAPS_OVERLAY )) goto CleanUp; 

    // Some type of support is provided, check further
    // We won't tolerate any alignment requirements

    if ( ( ddCaps.dwCaps & DDCAPS_ALIGNBOUNDARYDEST ) ||
         ( ddCaps.dwCaps & DDCAPS_ALIGNBOUNDARYSRC ) ||
         ( ddCaps.dwCaps & DDCAPS_ALIGNSIZEDEST ) ||
         ( ddCaps.dwCaps & DDCAPS_ALIGNSIZESRC ) ) 
    {
		goto CleanUp;
    }

    // Are any overlays available for use?
    if ( ddCaps.dwMaxVisibleOverlays == 
				ddCaps.dwCurrVisibleOverlays )
	{
		goto CleanUp;
	}

    // Finally check for stretching requirements

	if ( ddCaps.dwMinOverlayStretch )
	{
		dwResult = ddCaps.dwMinOverlayStretch;
	}
	else
	{
		dwResult = 1000;
	}

CleanUp:

	//Release the Direct Draw object
	lpDD->Release();
    return dwResult;
}
    