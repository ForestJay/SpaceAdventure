// dptools.h - for Space Adventure 
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998 - 1999
////////////////////////////
#ifndef DPTOOLS_H
#define DPTOOLS_H

// Define our flags
#define MSG_HOST        1	
#define MSG_SYNC        2
#define MSG_FIRE		3
#define MSG_CONTROL     4

// Generic message structure
typedef struct _GENERICMSG
{
    BYTE        byType;
} GENERICMSG, *LPGENERICMSG;

// Host message structure
typedef struct _HOSTMSG
{
    BYTE        byType;
    BYTE		bySlot;
} HOSTMSG, *LPHOSTMSG;

// The fire message structure
typedef struct _FIREMSG
{
	BYTE		byType;
	double      dPosX;
    double      dPosY;
	double 		dVelX;                        
	double 		dVelY;
} FIREMSG, *LPFIREMSG;

// The control message structure
typedef struct _CONTROLMSG
{
    BYTE        byType;
	BYTE        bySlot;
    BYTE        byState;
} CONTROLMSG, *LPCONTROLMSG;

// The sync message structure
typedef struct _SYNCMSG
{
    BYTE        byType;
    BYTE        bySlot;
    double      dPosX;
    double      dPosY;
	double		dVelX;
	double		dVelY;
	BYTE		byStatus;
    BYTE        byFrame;
} SYNCMSG, *LPSYNCMSG;

// Our directplay function prototypes
BOOL	FAR PASCAL EnumPlayer( DPID, DWORD, LPCDPNAME, DWORD, LPVOID );
BOOL	WINAPI EnumConnection( LPCGUID, LPVOID, DWORD, LPDPNAME, DWORD, LPVOID );
BOOL	WINAPI EnumSession( LPDPSESSIONDESC2, LPDWORD, DWORD, LPVOID );
LRESULT CALLBACK DlgProcEnumPlayers( HWND, UINT, WPARAM, LPARAM );
bool	CreateGamePlayer( void );
BOOL	CALLBACK DlgProcDPStart( HWND, UINT, WPARAM, LPARAM );
void	EvaluateSystemMessage( DPMSG_GENERIC, HWND );
void	EvaluateGameMessage( DPMSG_GENERIC, DPID, HWND );
void	ReceiveMessages( HWND );
DWORD	WINAPI ReceiveThread( LPVOID );
int		StartDPSession( void );
BOOL	ShutDownDPSession( void );
void	SendSyncMessage( void );
void	SendFireMessage( double, double, double, double );
void	SendControlMessage( BYTE byControl );
void	SendGameMessage( LPGENERICMSG, DPID );

extern bool		g_bHost;					// Are we the host?
extern DPID		g_dpPlayerID;           // Local player's ID.
extern char		g_szPlayerName[31];		// Local player's name.
extern BYTE		g_byPlayerSlot;			// Local players slot.
#endif // DPTOOLS_H