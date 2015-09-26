// gameutil.cpp - for Space Adventure 1
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998
////////////////////////////
#include "space.h"	// Our header

// These precomputed lookup tables are used to move the ships and shots
// in the directions they're pointed.

// 40 X coordinates
static double      Dirx[40] =                           
{
    0.000000,
    0.156434,
    0.309017,
    0.453991,
    0.587785,
    0.707107,
    0.809017,
    0.891007,
    0.951057,
    0.987688,
    1.000000,
    0.987688,
    0.951057,
    0.891007,
    0.809017,
    0.707107,
    0.587785,
    0.453990,
    0.309017,
    0.156434,
    0.000000,
    -0.156435,
    -0.309017,
    -0.453991,
    -0.587785,
    -0.707107,
    -0.809017,
    -0.891007,
    -0.951057,
    -0.987688,
    -1.000000,
    -0.987688,
    -0.951056,
    -0.891006,
    -0.809017,
    -0.707107,
    -0.587785,
    -0.453990,
    -0.309017,
    -0.156434
};

// 40 Y coordinates
static double      Diry[40] =
{
    -1.000000,
    -0.987688,
    -0.951057,
    -0.891007,
    -0.809017,
    -0.707107,
    -0.587785,
    -0.453990,
    -0.309017,
    -0.156434,
    0.000000,
    0.156434,
    0.309017,
    0.453991,
    0.587785,
    0.707107,
    0.809017,
    0.891007,
    0.951057,
    0.987688,
    1.000000,
    0.987688,
    0.951057,
    0.891006,
    0.809017,
    0.707107,
    0.587785,
    0.453990,
    0.309017,
    0.156434,
    0.000000,
    -0.156435,
    -0.309017,
    -0.453991,
    -0.587785,
    -0.707107,
    -0.809017,
    -0.891007,
    -0.951057,
    -0.987688
};

// Globals

DWORD					g_dwFrameTime = 0;	
DWORD					g_dwFrames = 0;
DWORD					g_dwFrameCount = 0;

// This will release the Direct Draw Object
void DDRelease( void )
{
	//Make sure we have it before starting
    if( lpDD != NULL )
    {
        // The clipper and palette, if they exist, will destroy
        // themselves when the primary is released.
        if( lpDDSBack != NULL )
        {
            // This will fail if attempted on a complex surface
            lpDDSBack->Release();
            lpDDSBack = NULL;
        }
        if( lpDDSPrimary != NULL )
        {
            // This will release any associated backbuffers as
            // well, if it's a complex surface.
            lpDDSPrimary->Release();
            lpDDSPrimary = NULL;
        }
		// This will release our ship graphics
        if( lpDDSShips != NULL )
        {   
            lpDDSShips->Release();
            lpDDSShips = NULL;
        }
		// This will release our shor graphics
        if( lpDDSShots != NULL )
        {   
            lpDDSShots->Release();
            lpDDSShots = NULL;
        }
		// This will release our ghost graphics
        if( lpDDSGhost != NULL )
        {   
            lpDDSGhost->Release();
            lpDDSGhost = NULL;
        }

		// This will release the Direct Draw object
        lpDD->Release();
        lpDD = NULL;
    }
}

//This will cleanup our window and release our resources
void CleanUp( void )
{
	//Release Direct Draw
    DDRelease();
	//If we have a window destroy it
    if( g_hwnd )
    {
        DestroyWindow( g_hwnd );
    }
}

//This will be run at initialization
bool GameInit( void )
{
	// Create a queue for the sprites
	InitLinkedList();

	//Make the player
	g_Players[0].lpNode = CreateShip( 100, 100, 0, 0, 0, 0 );

    return TRUE;
}

//This will create a sprite
bool CreateSprite(  LPSPRITESET lpSprite,
                    LPDIRECTDRAW lpDD,
                    LPDIRECTDRAWSURFACE* lplpDDSSurface,
                    int stride,
                    int height,
                    int width,
                    LPCSTR szBitmap,
                    LPDDCOLORKEY lpddck,
                    DWORD dwFlags,
					LPDDPIXELFORMAT lpddpfFormat )
{
    HRESULT ddrval;	//Our return value

    // create a surface for the sprite and load the bitmap into it
    (*lplpDDSSurface) = DDLoadBitmap( lpDD, szBitmap, 0, 0, 
											dwFlags, lpddpfFormat );

    if( (*lplpDDSSurface) == NULL )
    {
        OutputDebugString( "Couldn't create art surface.\n" );
        return FALSE;
    }

    // use the color key to make a transparent surface
    if ( lpddck != NULL )
    {
        ddrval = (*lplpDDSSurface)->SetColorKey( DDCKEY_SRCBLT, lpddck );

        if( FAILED( ddrval ) )
        {
            OutputDebugString( "Couldn't set the color key.\n" );
            (*lplpDDSSurface)->Release();
            (*lplpDDSSurface) = NULL;
            return FALSE;
        }
    }

    // now that the surface is ready, fill in the sprite structure
    lpSprite->stride = stride;
    lpSprite->height = height;
    lpSprite->width = width;
    lpSprite->surface = (*lplpDDSSurface);

    return TRUE;
}

//This will restore a sprite if it gets accidently destryoed
HRESULT RestoreSprite( LPSPRITESET lpSprite, LPCSTR szBitmap )
{
    HRESULT ddrval;	//Our return value

	//Restore the surface of our sprite
    ddrval = ( lpSprite->surface )->Restore();
	//If we fail exit
    if FAILED( ddrval ) return ddrval;

	//Load our picture
    if FAILED( DDReLoadBitmap( lpSprite->surface, szBitmap ) ) {
        return FALSE;
    }
    return TRUE;
}

//This will restore our surfaces
HRESULT RestoreSurfaces( void )
{
	HRESULT ddrval;	//Our return value

	// First, try restoring our sprites
	ddrval = RestoreSprite( &g_shipsprite, g_szShipBitmap );
	if FAILED( ddrval ) return ddrval;

	ddrval = RestoreSprite( &g_shotsprite, g_szShotBitmap );
	if FAILED( ddrval ) return ddrval;
	
	ddrval = RestoreSprite( &g_ghostsprite, g_szGhostBitmap );
	if FAILED( ddrval ) return ddrval;

	// Restore the other working surfaces
	if ( lpDDSPrimary ) ddrval = lpDDSPrimary->Restore();

	// If we're using a complex flipping surface with
	// an implicit back buffer, this call will fail. Oh well.
    ddrval = lpDDSBack->Restore();
	if ( FAILED( ddrval ) &&  
		( ddrval != DDERR_IMPLICITLYCREATED ) ) return ddrval;

	return DD_OK;
}

//This will try to restore our Window
void AttemptRestore( void )
{
	if FAILED( RestoreSurfaces() )
	{
		// If the restore failed, it's time to get
		// serious. Try recreating our DirectDraw
		// objects.

		g_bReInitialize = TRUE;
    
		// Destroy the window and DirectDraw objects
		CleanUp();
    
		// Create a new DirectDraw configuration
		if FAILED( DDInit() )
		{
			OutputDebugString( "Reinitialization failed.\n" );
		}
		ShowWindow( g_hwnd, SW_SHOW );
		g_bReInitialize = FALSE;
	}
}

//We'll create a color pallette
bool CreatePalette( LPDIRECTDRAW lpDD,
                    LPDIRECTDRAWPALETTE* lplpDDPalette,
                    LPDIRECTDRAWSURFACE lpDDSSurface,
                    LPCSTR szBitmap )
{
	//Get our pallette
    (*lplpDDPalette) = DDLoadPalette( lpDD, g_szShipBitmap );

	//If we fail exit
    if( (*lplpDDPalette) == NULL )
    {
        OutputDebugString("Couldn't create palette.\n");
        return FALSE;
    }

	//Try to attach our pallette to the surface
    if FAILED( lpDDSSurface->SetPalette( *lplpDDPalette ) )
    {
        OutputDebugString( "Couldn't attach palette.\n" );
        (*lplpDDPalette)->Release();
        (*lplpDDPalette) = NULL;
        return FALSE;
    }

    // Decrement the palette's ref count so it will
    // go away when the surface does.
    (*lplpDDPalette)->Release();

    return TRUE;
}

//This will load our sprites
bool LoadGameArt( DWORD dwFlags, LPDDPIXELFORMAT lpddpfFormat ) 
{
    DDCOLORKEY  ddck;	//This will hold our Direct Draw Color key
    DDPIXELFORMAT ddpf;	//This will hold our direct draw pixel format

	//Record the format sixe
    ddpf.dwSize = sizeof( DDPIXELFORMAT );

	//Attach the format to the Primary buffer
    lpDDSPrimary->GetPixelFormat( &ddpf );

	//Make sure we have a compatible format
    if ( ddpf.dwFlags & DDPF_PALETTEINDEXED8 )
    {
        OutputDebugString( 
			"LoadGameArt: Attaching palette...\n" );
        
        if FAILED( CreatePalette( lpDD, &lpDDPalette, lpDDSPrimary,
                            g_szShipBitmap ) )
        {
            OutputDebugString( 
				"LoadGameArt: Couldn't configure palette.\n" );
            return FALSE;
        }
    }
    else
    {
        OutputDebugString( 
			"LoadGameArt: Non-palettized primary...\n" );
    }

    // set a transparent color key using black
    ddck.dwColorSpaceLowValue = 0x00;
    ddck.dwColorSpaceHighValue = 0x00;

	//Create our ships sprite
    if FAILED( CreateSprite( &g_shipsprite, lpDD, &lpDDSShips, 10,
                    32, 32, g_szShipBitmap, &ddck, dwFlags, lpddpfFormat ) ) 
    {   
        OutputDebugString( 
			"LoadGameArt: Couldn't load ship sprite.\n" );
        return FALSE;
    }

	//Create our shots sprite
    if FAILED( CreateSprite( &g_shotsprite, lpDD, &lpDDSShots, 4,
                    3, 3, g_szShotBitmap, &ddck, dwFlags, lpddpfFormat ) ) 
    {   
        OutputDebugString( 
			"LoadGameArt: Couldn't load shot sprite.\n" );
        return FALSE;
    }

	//Create our ghost ships sprite
	if FAILED( CreateSprite( &g_ghostsprite, lpDD, &lpDDSGhost, 10,
                    32, 32, g_szGhostBitmap, &ddck, dwFlags, lpddpfFormat ) ) 
    {   
        OutputDebugString( 
			"LoadGameArt: Couldn't load ghost sprite.\n" );
        return FALSE;
    }

    OutputDebugString( "LoadGameArt: Art loaded.\n" );
    return TRUE;
}

//This will update the screen
HRESULT UpdateFrame( bool bFull )
{
    char str[255];
    DWORD time, time2;	//This will hold time values
    HRESULT ddrval;		//This will hold our return value

    // Update everyone's position
	UpdateStates();

	//Get the current time
	time = timeGetTime();

	//Check if the player has been hit
	CheckForHits( g_Players[0].lpNode );

	//Check if we can update the screen
	if ( !bFull ) return TRUE;

	// If it's ok to update the screen, do that too

    if FAILED( DDFillSurface( lpDDSBack, 0 ) )
    {
        OutputDebugString( "UpdateFrame: Couldn't fill back buffer.\n" );
        return ddrval;
    }

	//Draw our shots and ghosts
    if FAILED( DrawSprites( lpDDSBack,  TRUE ) )
    {
        OutputDebugString( "UpdateFrame: Couldn't draw sprites.\n" );
        return ddrval;
    }

	// Calculate and output the frame rate
	//This would be omitted in a payware version
    g_dwFrameCount++;	//We updated the frame so add to the frame number
    time2 = time - g_dwFrameTime;	//Get the time since last reset	
    //After 1 second reset the tie
	if( time2 > 1000)
    {
		//Evaluate frames per second
        g_dwFrames = ( g_dwFrameCount * 1000 ) / time2;
        g_dwFrameTime = timeGetTime();
        g_dwFrameCount = 0;
    }
	//Print to the screen
    sprintf(str, "%d", g_dwFrames);
	//Send the text through Direct Draw
    DDTextOut( lpDDSBack,str, RGB(0,0,0), RGB(255,255,0), 320, 20 );

	//Try to flip our surface
    if FAILED( FlipSurfaces( g_dwRenderSetup ) )
    {
        OutputDebugString( "UpdateFrame: Couldn't flip.\n" );
        return ddrval;
    }
    return TRUE;
}

//This will create our ship in memory
LPNODE CreateShip( double x, double y, double dx, 
						double dy, int offset, int frame )
{
    LPNODE ship;	//Our ship ode

    ship = (LPNODE) malloc( sizeof(NODE) );	//Allocate memory
    
	//See if he ship was made
    if ( ship == NULL )
        return ship;

    ship->frame = frame;
    ship->offset = offset;
    ship->timeupdate = timeGetTime();
    ship->dwtype = SPRITE_SHIP;
    ship->posx = x;
    ship->posy = y;
    ship->velx = dx;
    ship->vely = dy;
	ship->timedisabled = 0;
	ship->state = UpdateShip;

    ship->spriteset = &g_shipsprite;

	//Place this piece onto the queue
    AddNode ( ship );

    return ship;
}

//This will setup our shot in memory
LPNODE CreateShot( double x, double y, double dx, double dy, int offset )
{
    LPNODE shot;	//Our shot node

    shot = (LPNODE) malloc( sizeof(NODE) );	//Allocate the memory
    //Check to see if the shot was made
	if ( shot == NULL )
        return shot;

    shot->frame = 0;
    shot->offset = offset;
    shot->dwtype = SPRITE_SHOT;
    shot->posx = x;
    shot->posy = y;
    shot->velx = dx;
    shot->vely = dy;
    shot->timeborn = timeGetTime();
    shot->timeupdate = shot->timeborn;
    shot->state = UpdateShot;

    shot->spriteset = &g_shotsprite;

	//Place this shot onto the queue
    AddNode ( shot );

    return shot;
}

//This will update our shot values
void UpdateShot( LPNODE shot )
{
    DWORD dwTime;		//The time
    double FrameRatio;	//The Frame ratio

    dwTime = timeGetTime();	//Record the time
	FrameRatio = float( dwTime - shot->timeupdate ) / float( FRAME_RATE );

    // shots have a lifetime of SHOTLIFE ms
    if ( ( dwTime - shot->timeborn ) > SHOTLIFE )
    {
        RemoveNode( shot );
        return;
    }

    // calculate a new position based on the time
    // elapsed since the last update

    shot->posx += ( shot->velx * FrameRatio );
    shot->posy += ( shot->vely * FrameRatio );

// Uncomment this section of code if you'd like shots
// to dissappear when they hit the edge of the screen.
// Move the comments to the next section, of course.
/*
    // shots dissapear if they go off the screen/window
    if (  (shot->posx > SHOT_X ) ||
            ( shot->posx < 0 ) ||
            ( shot->posy > SHOT_Y ) ||
            ( shot->posy < 0 ) )
    {
        RemoveNode( shot );
        return;
    }
*/
	// shots reflect off edges of playing field

	if ( shot->posx > SHOT_X )
    {
        shot->posx = SHOT_X;
        shot->velx = -shot->velx;
    }

    if ( shot->posx < 0 )
    {
        shot->posx = 0;
        shot->velx = -shot->velx;
    }

    if ( shot->posy > SHOT_Y )
    {
        shot->posy = SHOT_Y;
        shot->vely = -shot->vely;
    }

    if ( shot->posy < 0 )
    {
        shot->posy = 0;
        shot->vely = -shot->vely;
    }
}

//Update our ship
void UpdateShip( LPNODE ship )
{
    DWORD	dwTime, dwDelta;
	double	x, y, dx, dy;
    double	FrameRatio;

	//Get the time
    dwTime = timeGetTime();
	dwDelta = dwTime - ship->timeupdate;

	//Check the frame ratio
    FrameRatio = float ( dwDelta ) / 
                            float ( FRAME_RATE );

	//Set when to update the ship
    ship->timeupdate = dwTime;

	//See if the ship needs to be updated
	if ( ( dwTime - ship->timeinput ) > INPUT_RATE )
	{
		ship->timeinput = dwTime;

		//turn left?
		if( g_byInput & KEY_LEFT )
		{
			ship->frame -= 1;
			if( ship->frame < 0)
				ship->frame = 39;
		}
		//turn right?
		if( g_byInput & KEY_RIGHT )
		{
			ship->frame += 1;
			if( ship->frame > 39 )
				ship->frame = 0;
		}

		//go faster
		if( g_byInput & KEY_THRUST )
		{
			ship->velx += Dirx[ship->frame] / 100;
			ship->vely += Diry[ship->frame] / 100;
		}
	}

	//adjust the position
    ship->posx += ship->velx * FrameRatio;
    ship->posy += ship->vely * FrameRatio;

	//Set the X position
    if ( ship->posx > SHIP_X )
    {
        ship->posx = SHIP_X;
        ship->velx = -ship->velx;
    }

    if ( ship->posx < 0 )
    {
        ship->posx = 0;
        ship->velx = -ship->velx;
    }

	//Set the Y position
    if ( ship->posy > SHIP_Y )
    {
        ship->posy = SHIP_Y;
        ship->vely = -ship->vely;
    }

    if ( ship->posy < 0 )
    {
        ship->posy = 0;
        ship->vely = -ship->vely;
    }

	//Fire torpedo #1!!
    if( g_byInput & KEY_FIRE )
    {
		// You can't fire if you've been disabled, Doh!
		if ( !( ship->timedisabled ) )
		{
			// Ships can only fire every SHOTFREQ ms.
			if( timeGetTime() - ship->timeborn > SHOTFREQ )
			{
				// Make the shot sort of look like it's coming
				// from the nose of the ship, but not so close we
				// shoot our own foot.
				x = ship->posx + ( Dirx[ship->frame] * 23.0 ) + 16.0;
				y = ship->posy + ( Diry[ship->frame] * 23.0 ) + 16.0;
				// Bias the shot's velocity depending on the ship's
				dx = ( Dirx[ship->frame] * 7.0 ) + ship->velx;
				dy = ( Diry[ship->frame] * 7.0 ) + ship->vely;
				// Add the shot to the linked list and send a message
				CreateShot( x, y, dx, dy, 0 );
				// Store the time the last shot was fired. 
				ship->timeborn = timeGetTime();	
			}
		}
    }

	// See if it's time to re-enable the ship
	if ( ( dwTime - ( ship->timedisabled ) ) > DISABLEDTIME )
	{
		ship->offset = 0;
		ship->spriteset = &g_shipsprite;
		ship->timedisabled = 0;
		ship->status = STATUS_OK;
	}

    return;
}

//Flip surfaces to the screen.
HRESULT FlipSurfaces( DWORD dwMode )
{
    HRESULT ddrval;	//The return value

	// Check what mode we're in
	switch ( dwMode )
	{
		//If we can overlay then do it
		case MODE_OVERLAY:
			ddrval = lpDDSOverlay->Flip( NULL, DDFLIP_WAIT );
			break;

		//If they want full screen mode flip
		case MODE_FULL:
			// If we are fullscreen, call Flip as usual
			ddrval = lpDDSPrimary->Flip( NULL, DDFLIP_WAIT );
			break;

		case MODE_WINDOWED:
			// If we are in a window, use Blt instead of Flip.  We must use Blt
			// because BltFast does not perform clipping.
			ddrval = lpDDSPrimary->Blt( &g_rcWindow,
					lpDDSBack,
					NULL,
					DDBLT_WAIT,
					NULL );
			break;
	}

    return ddrval;
}

//recreate the desktop underneath
HWND CreateDesktopWindow(   HANDLE hInstance,
                            WNDPROC WindowProc,
                            DWORD dwWidth,  // client area width
                            DWORD dwHeight  // client area height
                        )
{
    WNDCLASS    wc;		//The window class
    RECT        rc;		//A rectangle
    HWND        hwnd;	//The window

     // Set up and register window class
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = (HINSTANCE)hInstance;	//??
    wc.hIcon = LoadIcon( (HINSTANCE)hInstance, IDI_APPLICATION );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NAME;
    wc.lpszClassName = NAME;
    RegisterClass( &wc );
    
    // Create a window
    hwnd = CreateWindowEx(
        0,
        NAME,
        TITLE,
        WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        SW_SHOW,
        0,
        0,
        NULL,
        NULL,
        (HINSTANCE)hInstance,
        NULL );

	//Do we have a window?  if not exit
    if( !hwnd )
    {
        return NULL;
    }

    // Set the desired size for the *client* area of the window.
    SetRect( &rc, 0, 0, dwWidth, dwHeight );

    // Adjust that to a size that includes the border, etc.
    AdjustWindowRectEx( &rc,
        GetWindowStyle( hwnd ),     // style of our main window
        GetMenu( hwnd ) != NULL,    // does the window have a menu?
        GetWindowExStyle( hwnd ));  // extended style of the main window

    // Adjust the window to the new size
    MoveWindow( hwnd, 
                    CW_USEDEFAULT, 
                    CW_USEDEFAULT, 
                    rc.right-rc.left, 
                    rc.bottom-rc.top, 
                    FALSE );

    return hwnd;
}

//Create a full screen window
HWND CreateFullScreenWindow(HANDLE hInstance,
                            WNDPROC WindowProc
                        )
{
    WNDCLASS    wc;		//The windowclass
    HWND        hwnd;	//The window

     // Set up and register window class
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = (HINSTANCE)hInstance;
    wc.hIcon = LoadIcon( (HINSTANCE)hInstance, IDI_APPLICATION );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NAME;
    wc.lpszClassName = NAME;
    RegisterClass( &wc );
    
    // Create a fullscreen window
    hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        NAME,
        TITLE,
        WS_POPUP,
        0, 0,
        GetSystemMetrics( SM_CXSCREEN ),
        GetSystemMetrics( SM_CYSCREEN ),
        NULL,
        NULL,
        (HINSTANCE)hInstance,
        NULL );

	//If we don't have a window exit
    if( !hwnd )
    {
        return NULL;
    }

    return hwnd;
}


