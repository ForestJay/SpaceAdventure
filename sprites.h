// sprites.h - for Space Adventure 1
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998
////////////////////////////
#ifndef SPRITES_H
#define SPRITES_H

//our spriteset structure
typedef struct _spriteset
{
	LPDIRECTDRAWSURFACE		surface;
	int						stride;
	int						height;
	int						width;
}SPRITESET;
typedef SPRITESET* LPSPRITESET;

typedef void (*LPSTATE) (struct _NODE* node);

//Our node struct
typedef struct _NODE
{
	struct _NODE*	next;                         
 	struct _NODE*	prev;   
    DWORD           dwtype;
	BYTE			status;
	double 			posx;                         
	double 			posy;                            
	double 			velx;                        
	double 			vely;            
	int 			frame;     
	BYTE			offset;
	DWORD			timedisabled;
	DWORD		 	timeborn;
    DWORD           timeupdate;
	DWORD			timeinput;
	BYTE			byinput;
	LPSTATE 		state;               
	SPRITESET* 		spriteset;
}NODE;
typedef NODE* LPNODE;

//Constants
#define SPRITE_SHIP 1
#define SPRITE_SHOT 2

#define STATUS_OK	1
#define STATUS_HIT	2

// Prototypes

void	InitLinkedList( void );
void	CloseLinkedList( void );
void	UpdateStates( void );
bool	CheckHit( LPNODE, LPNODE );
bool	CheckForHits( LPNODE );
HRESULT DrawSprites( LPDIRECTDRAWSURFACE, bool );
HRESULT DrawSprite( LPNODE, LPDIRECTDRAWSURFACE );
void	AddNode ( LPNODE  );
void	RemoveNode ( LPNODE );

#endif // SPRITES_H
