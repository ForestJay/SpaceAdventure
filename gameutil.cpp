// gameutil.cpp - for Space Adventure 
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998 - 1999
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
DWORD					g_dwSyncTime = 0;
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
	InitLinkedList();

    if ( !CreateGamePlayer() )
	{
		return TRUE;
	}

	if ( g_bHost )
	{
		// We are being initialized by the host, so just
		// create a new ship. Otherwise, do nothing.
		FillPlayerSlot( FindPlayerSlot(), g_dpPlayerID );
	}

    return TRUE;
}

// Find a player slot
BYTE FindPlayerSlot( void )
{
	BYTE	i;

	// loop through slots
	for ( i = 0; i < 4; i++ )
	{
		if ( g_Players[i].dwStatus == 0 )
		{
			return i;
		}
	}
	return 0xFF;
}

// Fill a player slot
void FillPlayerSlot( BYTE i, DPID dpID )
{
	int		offset;
	double	x, y;

	switch( i )
	{
		case 0:
			x = 100;
			y = 100;
		break;
		case 1:
			x = 540;
			y = 380;
		break;
		case 2:
			x = 540;
			y = 100;
		break;
		case 3:
			x = 100;
			y = 380;
		break;
	}
	offset = i * 40;
	// Create the ship
	g_Players[i].lpNode = CreateShip( x, y, 0, 0, offset, 0, TRUE );
	g_Players[i].dwStatus = 1;
	g_Players[i].dpID = dpID;
}

// Empty a player slot
BYTE EmptyPlayerSlot( DPID dpID )
{
	BYTE	i;

	// Find the player slot belonging to the dpID
	// of a player that left the game and empty it.
	for ( i = 0; i < 4; i++ )
	{
		if ( g_Players[i].dpID == dpID )
		{
			OutputDebugString( "Removing player.\n" );
			g_Players[i].dwStatus = 0;
			g_Players[i].dpID = 0;
			RemoveNode( g_Players[i].lpNode );
			g_Players[i].lpNode = NULL;
		}
	}
	return 0xFF;
}

//This will create a sprite
BOOL CreateSprite(  LPSPRITESET lpSprite,
                    LPDIRECTDRAW lpDD,
                    LPDIRECTDRAWSURFACE* lplpDDSSurface,
                    int stride,
                    int height,
                    int width,
                    LPCSTR szBitmap,
                    LPDDCOLORKEY lpddck,
                    DWORD dwFlags )
{
    HRESULT ddrval;

    // create a surface for the sprite and load the bitmap into it
    (*lplpDDSSurface) = DDLoadBitmap( lpDD, szBitmap, 0, 0, dwFlags );

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
                    32, 32, g_szShipBitmap, &ddck, dwFlags ) ) 
    {   
        OutputDebugString( 
			"LoadGameArt: Couldn't load ship sprite.\n" );
        return FALSE;
    }

	//Create our shots sprite
    if FAILED( CreateSprite( &g_shotsprite, lpDD, &lpDDSShots, 4,
                    3, 3, g_szShotBitmap, &ddck, dwFlags ) ) 
    {   
        OutputDebugString( 
			"LoadGameArt: Couldn't load shot sprite.\n" );
        return FALSE;
    }

	//Create our ghost ships sprite
	if FAILED( CreateSprite( &g_ghostsprite, lpDD, &lpDDSGhost, 10,
                    32, 32, g_szGhostBitmap, &ddck, dwFlags ) ) 
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
    DWORD time, time2;
    HRESULT ddrval;

    // Update everyone's position
	UpdateStates();

	time = timeGetTime();
	time2 = time - g_dwSyncTime;

	// If the local player was hit, OR the sync
	// interval has passed, send a sync message

	if ( CheckForHits( g_Players[g_byPlayerSlot].lpNode ) ||
									( time2 > SYNC_INTERVAL ) )
	{
//		OutputDebugString("Sending Sync.\n" );
		g_dwSyncTime = time;
		SendSyncMessage();
	}

	if ( !bFull ) return TRUE;

	// If it's ok to update the screen, do that too

    if FAILED( DDFillSurface( lpDDSBack, 0 ) )
    {
        OutputDebugString( "UpdateFrame: Couldn't fill back buffer.\n" );
        return ddrval;
    }

    if FAILED( DrawSprites( lpDDSBack,  TRUE ) )
    {
        OutputDebugString( "UpdateFrame: Couldn't draw sprites.\n" );
        return ddrval;
    }

	// Calculate and output the frame rate

    g_dwFrameCount++;
    time2 = time - g_dwFrameTime;
    if( time2 > 1000)
    {
        g_dwFrames = ( g_dwFrameCount * 1000 ) / time2;
        g_dwFrameTime = timeGetTime();
        g_dwFrameCount = 0;
    }
    sprintf(str, "%d", g_dwFrames);
    DDTextOut( lpDDSBack,str, RGB(0,0,0), RGB(255,255,0), 320, 20 );

    if FAILED( FlipSurfaces() )
    {
        OutputDebugString( "UpdateFrame: Couldn't flip.\n" );
        return ddrval;
    }
    return TRUE;
}

//This will create our ship in memory
LPNODE CreateShip( double x, double y, double dx, 
						double dy, int offset, int frame, bool blocal )
{
    LPNODE ship;	//Our ship node

    ship = (LPNODE) malloc( sizeof(NODE) );	//Allocate memory
    
	//See if he ship was made
    if ( ship == NULL )
        return ship;

	//Setup ship values
    ship->frame = frame;
    ship->offset = offset;
	ship->last_known_good_timeupdatey =
	ship->last_known_good_timeupdatex = 0;
	ship->sample_timeupdate =
    ship->timeupdate = timeGetTime();
    ship->dwtype = SPRITE_SHIP;
	ship->last_known_goodx =
    ship->posx = x;
	ship->last_known_goody =
    ship->posy = y;
    ship->velx = dx;
    ship->vely = dy;
	ship->timedisabled = 0;
	ship->byinput = 0;

	// Check who's ship it is and update accordingly
	if ( blocal )
	{
		ship->state = UpdateShip;
	}
	else
	{
		ship->state = UpdateRemoteShip;
	}

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

	shot->last_known_good_timeupdatey =
	shot->last_known_good_timeupdatex = 0;
    shot->frame = 0;
    shot->offset = offset;
    shot->dwtype = SPRITE_SHOT;
	shot->last_known_goodx =
    shot->posx = x;
	shot->last_known_goody =
    shot->posy = y;
    shot->velx = dx;
    shot->vely = dy;
    shot->timeborn = timeGetTime();
	shot->sample_timeupdate =
    shot->timeupdate = shot->timeborn;
    shot->state = UpdateShot;

    shot->spriteset = &g_shotsprite;

	//Place this shot onto the queue
    AddNode ( shot );

    return shot;
}

// Needed for dead navigation
DWORD update_time( LPNODE ship,DWORD* lpdwTime,DWORD* delta )
{
    DWORD	dwTime, dwDelta;

	dwTime = timeGetTime();	// Get the current system time

	// Make sure there's a pointer to the time
	if( lpdwTime )
	{
		*lpdwTime = dwTime;
	}

	// Update ship is nessecary
	if( dwTime > ship->sample_timeupdate )
	{
		dwDelta = dwTime - ship->sample_timeupdate;
		ship->timeupdate = dwTime;

		if( dwDelta >= SAMPLE_RATE )
		{
			ship->sample_timeupdate = dwTime;
			*delta = dwDelta;
			return 1;
		}
		else
		{
			*delta = 0;
			return 0;
		}
	}
	else
	{
		ship->sample_timeupdate = dwTime;
		*delta = 0;
		return 0;
	}
}

// Needed for dead reckoning
void apply_delta( LPNODE ship, DWORD delta )
{
	ship->posx = ship->last_known_goodx + ( ship->velx * ship->last_known_good_timeupdatex / SAMPLE_RATE ) + ( ship->velx * ( delta / SAMPLE_RATE ) );
    ship->posy = ship->last_known_goody + ( ship->vely * ship->last_known_good_timeupdatey / SAMPLE_RATE ) + ( ship->vely * ( delta / SAMPLE_RATE ) );

	if( ship->posx != ship->last_known_goodx )
	{
		ship->last_known_good_timeupdatex += delta;
	}
	else
	{
		ship->last_known_good_timeupdatex = 0;
	}
	
	if( ship->posy != ship->last_known_goody )
	{
		ship->last_known_good_timeupdatey += delta;
	}
	else
	{
		ship->last_known_good_timeupdatey = 0;
	}

    if ( ship->posx > SHIP_X )
    {
		ship->last_known_good_timeupdatex = 0;
		ship->last_known_goodx = SHIP_X;
        ship->posx = SHIP_X;
        ship->velx = -ship->velx;
    }

    if ( ship->posx < 0 )
    {
		ship->last_known_good_timeupdatex = 0;
		ship->last_known_goodx = 0;
        ship->posx = 0;
        ship->velx = -ship->velx;
    }

    if ( ship->posy > SHIP_Y )
    {
		ship->last_known_good_timeupdatey = 0;
		ship->last_known_goody = SHIP_Y;
        ship->posy = SHIP_Y;
        ship->vely = -ship->vely;
    }

    if ( ship->posy < 0 )
    {
		ship->last_known_good_timeupdatey = 0;
		ship->last_known_goody = 0;
        ship->posy = 0;
        ship->vely = -ship->vely;
    }
}

//Update shots fired
void UpdateShot( LPNODE shot )
{
    DWORD dwTime;
	DWORD delta;

	update_time( shot,&dwTime,&delta );

    // shots have a lifetime of SHOTLIFE ms
    if ( ( dwTime - shot->timeborn ) > SHOTLIFE )
    {
        RemoveNode( shot );
        return;
    }

	if( delta != 0 )
	{
		apply_delta( shot,delta );
	}
}

//Update our ship
void UpdateShip( LPNODE ship )
{
    DWORD	dwTime;
	double	x, y, dx, dy;
	DWORD delta;

	update_time( ship,&dwTime,&delta );

	if ( ( dwTime - ship->timeinput ) > INPUT_RATE )
	{
		ship->timeinput = dwTime;

		if( g_byInput & KEY_LEFT )
		{
			ship->frame -= 1;
			if( ship->frame < 0)
				ship->frame = 39;
		}
		if( g_byInput & KEY_RIGHT )
		{
			ship->frame += 1;
			if( ship->frame > 39 )
				ship->frame = 0;
		}

		if( g_byInput & KEY_THRUST )
		{
			ship->velx += Dirx[ship->frame]/10;
			ship->vely += Diry[ship->frame]/10;
		}

		// Only send a control message if one of the
		// bits of interest has changed. We're only interested
		// in maneuvering controls, firing is handled by a different
		// message.

		if ( ( g_byInput & SYNCKEYS ) != ( g_byLastInput & SYNCKEYS ) )
		{
			g_byLastInput = g_byInput;
			SendControlMessage( g_byInput );
		}
	}

	if( delta != 0 )
	{
		apply_delta( ship,delta );
	}

    if( g_byInput & KEY_FIRE )
    {
		// You can't fire if you've been disabled
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
				//Load our shot sound
				LoadStatic( lpds, SHOTWAVE);
				// Play our shot sound
				PlayStatic(SHOTWAVE);

				SendFireMessage( x, y, dx, dy );
				// Store the time the last shot was fired. 
				ship->timeborn = timeGetTime();	
			}
		}
    }

	// See if it's time to re-enable the ship
	if ( ( dwTime - ( ship->timedisabled ) ) > DISABLEDTIME )
	{
		ship->offset = g_byPlayerSlot * 40;
		ship->spriteset = &g_shipsprite;
		ship->timedisabled = 0;
		ship->status = STATUS_OK;
		SendSyncMessage();
	}
    return;
}

// Update a remote ship
void UpdateRemoteShip( LPNODE ship )
{
	DWORD dwTime;
    DWORD delta;

	update_time( ship,&dwTime,&delta );

	if ( ( dwTime - ship->timeinput ) > INPUT_RATE )
	{
		ship->timeinput = dwTime;

		if( ship->byinput & KEY_LEFT )
		{
			ship->frame -= 1;
			if( ship->frame < 0)
				ship->frame = 39;
		}
		if( ship->byinput & KEY_RIGHT )
		{
			ship->frame += 1;
			if( ship->frame > 39 )
				ship->frame = 0;
		}
		if( ship->byinput & KEY_THRUST )
		{
			ship->velx += Dirx[ship->frame] / 10;
			ship->vely += Diry[ship->frame] / 10;
		}
	}

	if( delta != 0 )
	{
		apply_delta( ship,delta );
	}

    return;
}

//Flip surfaces to the screen.
HRESULT FlipSurfaces( void )
{
    HRESULT ddrval;

    // If we are fullscreen, call Flip as usual
    if ( g_bFullScreen )
    {
		ddrval = lpDDSPrimary->Flip( NULL, DDFLIP_WAIT );
	}        
	else
    // If we are in a window, use Blt instead of Flip.  We must use Blt
    // because BltFast does not perform clipping.
    {
        ddrval = lpDDSPrimary->Blt( &g_rcWindow,
                lpDDSBack,
                NULL,
                DDBLT_WAIT,
                NULL );
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
