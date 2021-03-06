// gameutil.h - for Space Adventure 
// gameutil.h
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998 - 1999
////////////////////////////
#ifndef GAMEUTIL_H
#define GAMEUTIL_H

//our playerinfo structure
typedef struct
{  
    DWORD           dwStatus;
	DPID			dpID;
	LPNODE			lpNode;
}PLAYERINFO;
typedef PLAYERINFO* LPPLAYERINFO;

// Prototypes

void	DDRelease( void );
void	CleanUp( void );
bool	GameInit( void );
BYTE	FindPlayerSlot( void );
void	FillPlayerSlot( BYTE i, DPID dpID );
BYTE	EmptyPlayerSlot( DPID dpID );
//bool	CreateSprite(  LPSPRITESET, LPDIRECTDRAW lpDD,
//						LPDIRECTDRAWSURFACE, int, int, int, LPCSTR,
//						LPDDCOLORKEY, DWORD, LPDDPIXELFORMAT );
HRESULT RestoreSprite( LPSPRITESET, LPCSTR );
HRESULT RestoreSurfaces( void );
void	AttemptRestore( void );
bool	CreatePalette( LPDIRECTDRAW, LPDIRECTDRAWPALETTE,
								LPDIRECTDRAWSURFACE, LPCSTR );
bool	LoadGameArt( DWORD, LPDDPIXELFORMAT );
HRESULT UpdateFrame( bool );
void	UpdateShot( LPNODE );
LPNODE	CreateShip( double, double, double, double, int, int, bool );
LPNODE 	CreateShot( double, double, double, double, int );
void	UpdateShip( LPNODE );
void	UpdateRemoteShip( LPNODE  );
HRESULT FlipSurfaces( void );
HWND	CreateDesktopWindow( HANDLE, WNDPROC, DWORD, DWORD );
HWND	CreateFullScreenWindow( HANDLE, WNDPROC );

// Frame rate variables
extern DWORD					g_dwFrameTime;
extern DWORD					g_dwFrames;
extern DWORD					g_dwFrameCount;

#endif // GAMEUTIL_H
