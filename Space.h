// Space.h
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998 - 1999
////////////////////////////
#define NAME "Space Adventure"	//The name of the game
#define TITLE "Space Adventure"	//The title of our game

#define WIN32_LEAN_AND_MEAN	//Have the preproccessor compile compactly

#include <windows.h>	//The windows header
#include <windowsx.h>	//The API header
#include <ddraw.h>		//The direct draw header
#include <dplay.h>		//The direct play header
#include <dplobby.h>	//The direct play lobby header
#include <stdlib.h>		//The C standard library
#include <stdio.h>		//The C standard I/O library
#include <stdarg.h>		//This allows variable number of arguments to be passed to a function
#include <mmsystem.h>	//This header is for multimedia APIs

//Our headers
#include "resource.h"
#include "ddutil.h"
#include "ddtools.h"
#include "dptools.h"
#include "static.h"
#include "utility.h"
#include "sprites.h"
#include "gameutil.h"

// Externals

extern LPDIRECTSOUND			lpds;			//Pointer to DirectSound
extern LPDIRECTDRAW				lpDD;			//Pointer to Direct Draw
extern LPDIRECTDRAWSURFACE		lpDDSPrimary;   //Pointer to the Primary surface
extern LPDIRECTDRAWSURFACE		lpDDSBack;      //Pointer to the back buffer
extern LPDIRECTDRAWSURFACE		lpDDSOverlay;	//Pointer to the overlay
extern LPDIRECTDRAWSURFACE		lpDDSShips;		//Pointer to the ships
extern LPDIRECTDRAWSURFACE		lpDDSShots;		//Pointer to the shots
extern LPDIRECTDRAWSURFACE		lpDDSGhost;		//Pointer to the ghost ships
extern LPDIRECTDRAWPALETTE		lpDDPalette;	//Pointer to the pallette
extern LPDIRECTDRAWCLIPPER		lpDDClipper;	//Pointer to the clipper

extern HANDLE					g_hInstance;	//Our instance
extern HWND						g_hwnd;			//Our window
extern RECT						g_rcWindow;		//The coordinates of our rectangle
extern bool						g_bFullScreen;	//Do they want full screen?
extern bool						g_bAllowWindowed;	//Do they want to allow a windowed app
extern DWORD					g_dwRenderSetup;	//The setup for rendering
extern bool						g_bReInitialize;	//Should we reinitialize?
extern bool						g_bTryOverlays;		//Should we use overlays?
extern BYTE						g_byInput;			//The input
extern BYTE						g_byLastInput;		//The last input
extern SPRITESET				g_shipsprite;		//The ship sprite
extern SPRITESET				g_shotsprite;		//The shot sprite
extern SPRITESET				g_ghostsprite;		//The ghost sprite
extern char						*g_szShipBitmap;	//The size of the ship
extern char						*g_szShotBitmap;	//The size of the shot
extern char						*g_szGhostBitmap;	//The size of the ghost

extern PLAYERINFO 				g_Players[4];	//Info about players

// Keyboard commands

#define KEY_THRUST 0x0001 // Thrust with the "5" key
#define KEY_LEFT   0x0004 // Turn left with left arrow
#define KEY_RIGHT  0x0008 // Turn right with right arrow
#define KEY_FIRE   0x0020 // Fire with space bar

#define SYNCKEYS   0x000D

// Rendering setups

#define MODE_OVERLAY	1
#define MODE_FULL		2
#define MODE_WINDOWED	3

// Game constants

#define SCREEN_X    639
#define SCREEN_Y    479
#define SHIP_X      ( SCREEN_X - 32 )
#define SHIP_Y      ( SCREEN_Y - 32 )
#define SHOT_X      ( SCREEN_X - 3 )
#define SHOT_Y      ( SCREEN_Y - 3 )

#define FRAME_RATE		25		// 40 frames per second (ms)
#define SAMPLE_RATE		25		// 40 frames per second (ms)
#define SHOTLIFE		2000	// lifetime of shots (ms)
#define SHOTFREQ		500		// time between shots (ms)
#define DISABLEDTIME	4000	// time disabled after hit
#define SYNC_INTERVAL	1000	// time between sync messages
#define INPUT_RATE		25		// 40 times per second (ms)

// Prototypes

bool DDInit( void );


