// sprites.cpp - for Space Adventure 1
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998
////////////////////////////
#define WIN32_LEAN_AND_MEAN	//Have the code made lean
#include <windows.h>	//Windows header
#include <windowsx.h>	//Windows API header
#include <mmsystem.h>	//Header for vraible arguments in functions
#include <ddraw.h>		//Direct Draw header

//Our headers
#include "sprites.h"	
#include "space.h"

// Globals
LPNODE g_bottom_node;
LPNODE g_top_node;

//Setup the Queue
void InitLinkedList( void )
{
    g_bottom_node = NULL;
    g_top_node = NULL;
}

//Close the queue
void CloseLinkedList( void )
{
	LPNODE next_node = (LPNODE) NULL;	//The next node
    LPNODE thenode;						//Current node

	for ( thenode=g_bottom_node; thenode != (LPNODE)NULL; thenode=next_node )
    {
        next_node = thenode->next;
		free( thenode );
    }

	g_bottom_node = NULL;
	g_top_node = NULL;
}

//Update node states
void UpdateStates( void )
{
	// Go through each node in the list,
	// invoking its state function.

    LPNODE next_node = (LPNODE) NULL;	//The next node
    LPNODE thenode;						//current node

    for ( thenode=g_bottom_node; thenode!=(LPNODE)NULL; thenode=next_node )
    {
        next_node = thenode->next;
        if ( thenode->state ) thenode->state( thenode );
    }

}

//Check if the ship was shot
bool CheckHit( LPNODE ship, LPNODE shot )
{
	// Make a crude check to see if a ship is
	// occupying the same place as a shot

	if ( shot->posx < ship->posx ) return FALSE;
	if ( shot->posy < ship->posy ) return FALSE;
	if ( shot->posx > (ship->posx + 32.0) ) return FALSE;
	if ( shot->posy > (ship->posy + 32.0) ) return FALSE;

	return TRUE;
}

//Check for collisions
bool CheckForHits( LPNODE ship )
{
	// Go through all the nodes checking shot nodes for
	// collisions with the passed ship.

    LPNODE next_node = (LPNODE) NULL;	//The next node
    LPNODE thenode;						//The current node

    for ( thenode=g_bottom_node; thenode!=(LPNODE)NULL; thenode=next_node )
    {
        next_node = thenode->next;
		if ( thenode->dwtype == SPRITE_SHOT )
		{
			if ( CheckHit( ship,  thenode ) )
			{
				// Disable the ship
				ship->offset = 0;
				ship->spriteset = &g_ghostsprite;
				ship->status = STATUS_HIT;
				ship->timedisabled = timeGetTime();
				// No need to check further
				return TRUE;		
			}
		}
    }
	return FALSE;
}

//Draw the ships and shots
HRESULT DrawSprites( 
                 LPDIRECTDRAWSURFACE lpDDSSurface,  // destination surface
                 bool bDrawOrder                    // drawing order
                 )
{
	// Draw all the sprites in the list,
	// top down or bottom up.

    LPNODE	next_node = (LPNODE) NULL;	//The next node
    LPNODE	thenode;					//The current node
    HRESULT ddrval;						//The return value

    if ( bDrawOrder )

    // start at the bottom of the list and work up -- items added most
    // recently will be drawn first
    {
        for ( thenode=g_top_node; thenode!=(LPNODE)NULL; thenode=next_node )
        {
            next_node = thenode->prev;
            ddrval = DrawSprite( thenode, lpDDSSurface );
            if FAILED( ddrval ) return ddrval;
        }
    }
    else
    // start at the top of the list and work down -- items will be drawn in the
    // order they were added
    {
        for ( thenode=g_bottom_node; thenode!=(LPNODE)NULL; thenode=next_node )
        {
            next_node = thenode->next;
            ddrval = DrawSprite( thenode, lpDDSSurface );
            if FAILED( ddrval ) return ddrval;
        }
    }

    return TRUE;
}

//Draw a sprite
HRESULT DrawSprite( 
                LPNODE drawnode,                    // the sprite node
                LPDIRECTDRAWSURFACE lpDDSSurface    // destination surface
                )
{
    HRESULT ddrval;	//The return value
    RECT    src;	//The source rectangle

	//coordinates
    src.left = ( ( drawnode->frame+drawnode->offset ) %
                        drawnode->spriteset->stride ) *
                        drawnode->spriteset->width;
    src.top = ( ( drawnode->frame+drawnode->offset ) /
                        drawnode->spriteset->stride ) *
                        drawnode->spriteset->height;
    src.right = src.left + drawnode->spriteset->width;
    src.bottom = src.top + drawnode->spriteset->height;

    drawnode->timeupdate = timeGetTime();

	//Blit the ship
    ddrval = lpDDSSurface->BltFast(
                        (DWORD)drawnode->posx,
                        (DWORD)drawnode->posy,
                        drawnode->spriteset->surface,
                        &src,
                        DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT );

    if FAILED( ddrval )
    {
        OutputDebugString( "DrawSprite: BltFast failed.\n" );
        return ddrval;
    } 

    return ddrval;
}

//Add a new node to the queue
void AddNode ( 
              LPNODE newNode    // the node to be added
              )
{
    if (g_bottom_node == (LPNODE) NULL )
    {
        g_bottom_node = newNode;
        newNode->prev = (LPNODE) NULL ;
    }
    else
    {
        newNode->prev = g_top_node;
        newNode->prev->next = newNode;
    }
    g_top_node = newNode;
    newNode->next = (LPNODE) NULL;
}

//Delete a node
void RemoveNode(
                LPNODE node     // the node to be removed
                )
{
    if (node == g_bottom_node)
    {
        g_bottom_node = node->next;
        if ( g_bottom_node != (LPNODE) NULL )
        {
            g_bottom_node->prev = (LPNODE) NULL;
        }
    }
    else if (node == g_top_node)
    {
        g_top_node = node->prev;
        g_top_node->next = (LPNODE) NULL;
    }
    else
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    free( node );
}

