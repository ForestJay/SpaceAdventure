// Space Adventure1.cpp - for Space Adventure
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998 - 1999
////////////////////////////
#include <windowsx.h>	//Windows API header
#include <dsound.h>		//The DirectSound header
#include "Sound.h"		//Our sound header
#include "Static.h"		//Our header for satic buffers
#include "space.h"		//Our space header
#include "dptools.h"	//Our Direct Play header

////////////////////////////////////////////////////////////////////
// Our Sound Variables
////////////////////////////////////////////////////////////////////
extern LPDIRECTSOUND        lpds;	//Pointer to DirectSound
extern LPDIRECTSOUNDBUFFER  lpdsb;	//Pointer to DirectSound buffer
GUID                        *pguid;	//The user interface

LPDIRECTDRAW            lpDD;			//Our Direct Draw pointer
LPDIRECTDRAWSURFACE     lpDDSPrimary;	//Our pointer to the primary buffer
LPDIRECTDRAWSURFACE     lpDDSBack;		//Our pointer to the back buffer
LPDIRECTDRAWSURFACE		lpDDSOverlay;	//Our pointer to the screen overlay
LPDIRECTDRAWPALETTE		lpDDPalette;	//Our pointer to the pallette
LPDIRECTDRAWCLIPPER		lpDDClipper;	//Our pointer to the clipper

LPDIRECTDRAWSURFACE		lpDDSShips;	//A pointer to the ships
LPDIRECTDRAWSURFACE		lpDDSShots;	//A pointer to the shots
LPDIRECTDRAWSURFACE		lpDDSGhost;	//A pointer to the ghosts

HANDLE                  g_hInstance;	//Our Instance
HWND                	g_hwnd;			//Our window
PLAYERINFO 				g_Players[4];	//Allow for four players
bool                    g_bActive;       //If the screen is active
bool					g_bFullScreen = FALSE;		//If we are in full screen mode
bool					g_bAllowWindowed = TRUE;	//If Windowed mode is allowed
bool                    g_bReInitialize = FALSE;	//If it should be reset
DWORD					g_dwRenderSetup;	//The setup info
BYTE					g_byInput;			//The input
BYTE					g_byLastInput;		//The last input
RECT					g_rcWindow;			//The rectangle of the window

SPRITESET				g_shipsprite;	//This will hold the ships
SPRITESET				g_shotsprite;	//This will hold the shots
SPRITESET				g_ghostsprite;	//This will hold the client's ghostships

char*	g_szShipBitmap = "SHIPS";	
char*	g_szShotBitmap = "SHOTS";
char*	g_szGhostBitmap = "GHOST";

//Our windows procedure
long FAR PASCAL WindowProc( HWND hWnd, UINT message, 
                            WPARAM wParam, LPARAM lParam )
{
    switch( message )
    {
    case WM_ACTIVATEAPP:
		// If we wanted to pause the application when it
        // became inactive, we could do so here. In this case,
		// we decided to make it active at all times. When it is
		// minimized to the task bar, it just updates position.
		OutputDebugString( "Active App!\n ");
        break;

    case WM_CREATE:
        break;

	case WM_SIZE:
		// Our window size is fixed, so this could
        // only be a minimize or maximize
        if ( wParam == SIZE_MINIMIZED ) {
            // We've been minimized, no need to
            // redraw the screen.
            InvalidateRect( hWnd, NULL, TRUE );
            g_bActive = FALSE;
        }
        else
        {
            g_bActive = TRUE;
        }
        return 0;
        break;

    case WM_MOVE:
		// get the client rectangle
        if ( g_bFullScreen )
        {
            SetRect( &g_rcWindow, 0, 0, GetSystemMetrics( SM_CXSCREEN ), 
                        GetSystemMetrics( SM_CYSCREEN ) );
        }
        else
        {
            GetClientRect( hWnd, &g_rcWindow );
            ClientToScreen( hWnd, (LPPOINT)&g_rcWindow );
            ClientToScreen( hWnd, (LPPOINT)&g_rcWindow+1 );
        }
		break;

	case WM_PALETTECHANGED:
		// First check to see if we caused this message
		if ( (HWND)wParam != hWnd ) {
			// We didn't cause it, so we have lost the palette.
			OutputDebugString( "Palette lost.\n" );

			// Realize our palette
			lpDDSPrimary->SetPalette( lpDDPalette );
			
			// convert the sprite images to the new palette
			RestoreSprite( &g_shipsprite, g_szShipBitmap );
			RestoreSprite( &g_shotsprite, g_szShotBitmap );
			RestoreSprite( &g_ghostsprite, g_szGhostBitmap );
		}
		break;

	case WM_QUERYNEWPALETTE:
		// Ignore this message if we're transitioning -- we may
		// not yet have created the the surface and palette.
		if ( !lpDDSPrimary || !lpDDPalette )
		{
			OutputDebugString( "Ignoring palette message.\n" );
			return TRUE;
		}
		// We have control of the palette.
		OutputDebugString( "We have the palette.\n" );
		lpDDSPrimary->SetPalette( lpDDPalette );

		// convert the sprite images to the new palette
		RestoreSprite( &g_shipsprite, g_szShipBitmap );
		RestoreSprite( &g_shotsprite, g_szShotBitmap );
		RestoreSprite( &g_ghostsprite, g_szGhostBitmap );
		break;

    case WM_SETCURSOR:
        if ( g_bFullScreen ) SetCursor( NULL );
        return FALSE;
 
	// For the sake of simplicity, we're not using DirectInput but are
	// reading the keyboard through the message queue. You will see the
	// detrimental effect this has on the frame rate. DirectInput will
	// be used later.
    case WM_KEYDOWN:
        switch( wParam )
        {
            case VK_LEFT:
            case VK_NUMPAD4:
                g_byInput |= KEY_LEFT;
                break;
            case VK_RIGHT:
            case VK_NUMPAD6:
                g_byInput |= KEY_RIGHT;
                break;
		    case VK_UP:
		    case VK_NUMPAD5:
			    g_byInput |= KEY_THRUST;
			    break;
            case VK_ESCAPE:
                PostMessage( hWnd, WM_CLOSE, 0, 0 );
                return 0;
		    case VK_SPACE:
                g_byInput |= KEY_FIRE;
                break;
        }
        break;
 
    case WM_KEYUP:
        switch( wParam )
        {
            case VK_LEFT:
            case VK_NUMPAD4:
                g_byInput &= ~KEY_LEFT;
                break;
            case VK_RIGHT:
            case VK_NUMPAD6:
                g_byInput &= ~KEY_RIGHT;
                break;
            case VK_UP:
            case VK_NUMPAD5:
		        g_byInput &= ~KEY_THRUST;
		        break;
	        case VK_SPACE:
                g_byInput &= ~KEY_FIRE;
                break;
        }
        break;

    case WM_PAINT:
        // We are redrawing every frame so we don't need
        // to process this message. If we were using a
        // pause screen, we could paint it here.
        break;

    case WM_SYSKEYUP:
        switch( wParam )
        {
            // handle ALT+ENTER ( fullscreen/windowed switch )
            case VK_RETURN:
                OutputDebugString( "Alt enter...\n" );

				// If windowed configs aren't allowed, get out
				if ( !g_bAllowWindowed ) break;

                g_bReInitialize = TRUE;
                g_bFullScreen = !g_bFullScreen;
                
                // Destroy the window and DirectDraw objects
                CleanUp();
                
                // Create a new DirectDraw configuration
                if FAILED( DDInit() )
                {
                    OutputDebugString( "Reinitialization failed.\n" );
                }

                ShowWindow( g_hwnd, SW_SHOW );
                g_bReInitialize = FALSE;
                break;
        }
        break;

	case WM_DESTROY:
        if ( !g_bReInitialize )
        {
			ShutDownDPSession();	
            PostQuitMessage( 0 );
        }
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

//Initialize Direct Draw
bool DDInit( void )
{
	DWORD	dwOverlayStretch;
	RECT	rc;

    // Use globals to intialize DD setup

	if ( GetSystemMetrics( SM_CXSCREEN ) == 640 )
	{
		// The desktop is already in 640x480. We can't use
		// windowed mode because there's not enough room for
		// our 640x480 playing field and the window.
		g_bAllowWindowed = FALSE;
		g_bFullScreen = TRUE;
	}

	// If we're going to try to use overlays, we need to know
	// the stretching factor first, to see if the current desktop
	// mode can accomodate it.

	dwOverlayStretch = DDCheckOverlay( NULL );
	if ( dwOverlayStretch )
	{	
		// The required client area. We'll stretch in the Y
		// direction as well, to maintain square sprites
		SetRect( &rc, 0, 0, ( ( 640 * dwOverlayStretch ) / 1000 ),
						( ( 480 * dwOverlayStretch ) / 1000 ) );

		// Adjust the client area for window border, etc.
		AdjustWindowRectEx( &rc,
			WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
			FALSE, 0 );
	}

	if ( g_bFullScreen )
	{
		g_hwnd = CreateFullScreenWindow( g_hInstance, WindowProc );
	}
	else
	{
		g_hwnd = CreateDesktopWindow( g_hInstance, WindowProc,
										640, 480 );
	}

	if ( g_hwnd == NULL ) return FALSE;

	if FAILED( DDStartup( &lpDD, NULL, g_hwnd, g_bFullScreen ) )
	{
        OutputDebugString( "DDStartup failed.\n" );
        return FALSE;
    }

/*		
	if FAILED( DDCreateOverlay( lpDD, &lpDDSPrimary,
							&lpDDSOverlay, &lpDDSBack ) )
	{
		OutputDebugString( "Overlay attempt failed.\n" );
	}

*/
    if ( g_bFullScreen )
    {
	    if FAILED( DDFullConfigure( lpDD, &lpDDSPrimary, &lpDDSBack ) )
	    {
            return FALSE;
        }
    }
    else
    {
        if FAILED( DDWinConfigure( lpDD, &lpDDSPrimary, &lpDDSBack,
                                        &lpDDClipper, g_hwnd ) )
        {
            return FALSE;
        }
    }
    return TRUE;
}

//setup our game
static bool doInit( HANDLE hInstance, int nCmdShow )
{
	int rc;

    g_hInstance = hInstance;

	// Attempt to initialize DirectPlay
	rc = StartDPSession();

	switch ( rc )
	{
		case -1:
			// Something failed when attempting DirectPlay init
			MessageBox( NULL, "DirectPlay Init FAILED", "ERROR", MB_OK );
			// Fall through
		case -2:
			// The user pressed the cancel button.
			OutputDebugString( "The user cancelled DPlay.\n" );
			return FALSE;
	}

	// Attempt to initialize DirectDraw
    if FAILED( DDInit() ) 
	{
        MessageBox( NULL, "DirectDraw Init FAILED", "ERROR", MB_OK );
    	CleanUp();
		return FALSE;
	}

 	if( !GameInit() ) return FALSE;

	ShowWindow( g_hwnd, SW_SHOW );
          
    return TRUE;
}

//Our main function
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPSTR lpCmdLine, int nCmdShow)
{
    MSG         msg;	//A message

    // We'll use CoCreateInstance in this application

    CoInitialize( NULL );

    if( !doInit( hInstance, nCmdShow ) )
    {
       	return FALSE;
    }

	g_dwFrameTime = timeGetTime();

	//Initialize DirectSound
	InitDSound( g_hwnd, hInstance, pguid);

	//Load our welcome message
	LoadStatic( lpds, WELCOMEWAVE);
	// Play our welcome message
	PlayStatic(WELCOMEWAVE);

	//Infinite look for message loop
	while( 1 )
	{
    	if( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
    	{
            if( !GetMessage( &msg, NULL, 0, 0 ) )
        	{
            	return msg.wParam;
        	}
        	TranslateMessage(&msg);
        	DispatchMessage(&msg);
    	}
		else
		{	
			// We're active all the time, even partially active when
			// the window is minimized. This is wasteful of CPU but
			// makes it easier to demonstrate clipping and handle
			// multiple players.
			if FAILED( UpdateFrame( g_bActive ) )
			{
				// Inside UpdateFrame, we clear the backbuffer, draw
				// our sprites, then perform a flip. Any one of these
				// operations could encounter lost surfaces. You can
				// test your application's lost surface recovery using
				// the DDTest SDK sample.
				AttemptRestore();
			}
		}
	}
	CoUninitialize();
	CloseLinkedList();
}

