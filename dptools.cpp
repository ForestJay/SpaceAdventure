// dptools.cpp for Space Adventure 
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998 - 1999
////////////////////////////
#define WIN32_LEAN_AND_MEAN
// You must include this define to use DirectPlay 3
#define IDIRECTPLAY2_OR_GREATER
// You must include this define to use QueryInterface
#define INITGUID

#include <windows.h>	// Windows header
#include <windowsx.h>	// Windows API header
#include <stdio.h>		// Standard I/O header
#include <objbase.h>	// GUID functions

// Our headers
#include "space.h"		
#include "dplay.h"
#include "dplobby.h"
#include "gameutil.h"
#include "dptools.h"
#include "resource.h"

// Create the CONNECTIONINFO structure
typedef struct
{
	GUID	guidSP;
	LPVOID 	lpConnection;
	DWORD	dwConnectionSize;
}CONNECTIONINFO;
// Create a type for CONNECTIONINFO pointers
typedef CONNECTIONINFO* LPCONNECTIONINFO;

// Message structures

// BAF45162-47DA-46DF-847E-5A5910AEEF84
// This holds our unique identifier
GUID SPACE_GUID = {
    0xbaf45162,
    0x47da,
    0x46df,
    {0x84, 0x7e, 0x5a, 0x59, 0x10, 0xae, 0xef, 0x84}
};

#define TIMER_ID        1	
#define TIMER_RATE      2000 // milliseconds

LPDIRECTPLAY3       lpDP				= NULL;	// Pointer to DirectPlay
LPDIRECTPLAYLOBBY2	lpDPLobby2			= NULL;	// Pointer to DirectLobby
DPSESSIONDESC2      dpDesc;						// Our Session info
LPDPSESSIONDESC2    lpdpDesc			= NULL;	// Pointer to session info
DPCAPS              dpCaps;						// DirectPlay Capabilities
HANDLE              hPlayerEvent		= NULL;	// Event flag
HANDLE              hKillEvent			= NULL;	// End flag
HANDLE              hReceiveThread		= NULL;	// Recieve flag
DWORD               idReceiveThread		= 0;	// The id of the recieve thread
LPVOID              lpReceiveBuffer		= NULL;	// The id of the recieve buffer
DWORD               dwReceiveBufferSize = 0;	// The Size of the recieve buffer
bool				bEnumerating		= FALSE;	// Enumerate flag
bool				bUserCancel			= TRUE;	// Cancel flag

HOSTMSG				MsgHost;	// Message for the host
SYNCMSG				MsgSync;	// Message to synchronize
FIREMSG				MsgFire;	// Fire notification
CONTROLMSG			MsgControl;	// Control message

bool		g_bHost	= FALSE;	// Are we the host?
char		g_szPlayerName[31];		// Local player's name
DPID		g_dpPlayerID;			// Local player's ID
BYTE		g_byPlayerSlot;			// Local player's slot


// Initialize the message buffers
void InitMessageBuffers( void )
{
    MsgHost.byType		= MSG_HOST;
	MsgSync.byType		= MSG_SYNC;
	MsgFire.byType		= MSG_FIRE;
	MsgControl.byType	= MSG_CONTROL;
}

// Enumerate players
BOOL FAR PASCAL EnumPlayer( DPID pidID,	// Player id
                            DWORD dwPlayerType,	// Player type 
							LPCDPNAME lpName,	// Player name
                            DWORD dwFlags,		// Player flags 
							LPVOID lpContext)	// Player context
{
    HWND hWnd = ( HWND ) lpContext;	// Define the window 

	// Ask to be added
    SendMessage( hWnd, LB_ADDSTRING, 0, 
                    ( LPARAM ) lpName->lpszShortNameA );

    return( TRUE );
}

// Enumerate DirectPlay Connections ie(IPX,Modem,Serial,TCP/IP)
BOOL WINAPI EnumConnection( LPCGUID lpguidSP,	// The unique id
                            LPVOID lpConnection, // Pointer to connection
							DWORD dwSize,		// Connection size
                            LPDPNAME lpName,	// Connection name
							DWORD dwFlags,		// Connection Flags
                            LPVOID lpContext )	// Pointer to the context
{
    LONG				iIndex;
    HWND hWnd			= ( HWND ) lpContext;
    LPVOID				lpOurConnection = NULL;
	LPCONNECTIONINFO	lpConnectionInfo = NULL;
    LPDIRECTPLAY3		lpDPTemp;

    // Check to see if a connection can be initialized
    if FAILED( CoCreateInstance( CLSID_DirectPlay,
                NULL, CLSCTX_ALL, IID_IDirectPlay3A,
                ( LPVOID* ) &lpDPTemp ) )
    {
        return( FALSE );
    }

    if FAILED( lpDPTemp->InitializeConnection( lpConnection, 0 ) )
    {
        lpDPTemp->Release();
        return( TRUE );
    }
    lpDPTemp->Release();

    // If it was initialized, add it to the list box
    iIndex = SendMessage( hWnd, CB_ADDSTRING, 0, 
                            (LPARAM) lpName->lpszShortNameA );

    // If it got added to the list box, create a copy of the connection
    // info and store a pointer to it in the list box.
    if ( iIndex != LB_ERR )
    {
		lpConnectionInfo = 
			(LPCONNECTIONINFO)malloc( sizeof( CONNECTIONINFO ) );

		memcpy( &lpConnectionInfo->guidSP, lpguidSP, sizeof( GUID ) );

		lpConnectionInfo->lpConnection = malloc( dwSize );
		memcpy( lpConnectionInfo->lpConnection, lpConnection, dwSize );

		lpConnectionInfo->dwConnectionSize = dwSize;

        SendMessage( hWnd, CB_SETITEMDATA, iIndex, 
                            ( LPARAM ) lpConnectionInfo );
    }
    else 
    {
        return FALSE;
    }

    return( TRUE );
}

// Enumerate Sessions
BOOL WINAPI EnumSession( LPDPSESSIONDESC2 lpDPGameDesc,	// Session description
						LPDWORD lpdwTimeOut,	// Pointer to timeout length
                            DWORD dwFlags,		// Session flags
							LPVOID lpContext )	// Session Context
{
    LONG iIndex;
    HWND hWnd = ( HWND ) lpContext;
    LPGUID lpGuid;

    // First check and see if the enumeration timed out.  If so, we
    // could reset it, but we'll just canel it instead.

    if( dwFlags & DPESC_TIMEDOUT )
    {
        return FALSE;
    }

    iIndex = SendMessage( hWnd, LB_ADDSTRING, 0, 
                            (LPARAM) lpDPGameDesc->lpszSessionName );

    // If it got added to the list box, create a copy of the GUID
    // and store a pointer to it in the list box.

    if ( iIndex != LB_ERR )
    {
        lpGuid = ( LPGUID ) malloc( sizeof( GUID ) );
        if ( !lpGuid ) return FALSE;

        *lpGuid = lpDPGameDesc->guidInstance;

        SendMessage( hWnd, LB_SETITEMDATA, iIndex, 
                                ( LPARAM ) lpGuid );
    }

    return(TRUE);
}

// Our callback function
LRESULT CALLBACK DlgProcEnumPlayers( HWND hWnd, // The window
									UINT message, // The message
                                        WPARAM wParam, // Window params
										LPARAM lParam )	// Pointer to pointers 
{
    int iIndex;
    LPGUID lpGuid;

	// Evaluate what type of message we are using
    switch( message )
    {
		case WM_INITDIALOG:

            iIndex = SendDlgItemMessage( GetParent( hWnd ), IDC_SESSIONS, 
                                    LB_GETCURSEL, 0, 0L );
            lpGuid = ( LPGUID )SendDlgItemMessage( GetParent( hWnd ), 
                        IDC_SESSIONS, LB_GETITEMDATA, iIndex, 0 );

            if FAILED( lpDP->EnumPlayers( lpGuid, 
                            ( LPDPENUMPLAYERSCALLBACK2 )EnumPlayer,
                            ( LPVOID )GetDlgItem( hWnd, IDC_PLAYERS ),
                            DPENUMPLAYERS_SESSION ) )
            {
                MessageBox( hWnd, "Error enumerating players.", 
                                        "Error", MB_OK );
                EndDialog( hWnd, FALSE );
            }

            return TRUE;

        case WM_COMMAND:         
            switch( LOWORD( wParam ) )
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog( hWnd, FALSE );
                    return TRUE;
            }

    }
    return FALSE; 
}

// Create a game player
bool CreateGamePlayer( void )
{	
	DPNAME dpPlayerName;	// Players name
	
	ZeroMemory( &dpPlayerName, 
                sizeof( dpPlayerName ) );
    dpPlayerName.dwSize = sizeof( DPNAME );
    dpPlayerName.lpszShortNameA = g_szPlayerName;

	// Create the player
	if FAILED( lpDP->CreatePlayer( &g_dpPlayerID, &dpPlayerName,
							hPlayerEvent, NULL, 0, 0 ) )
	{
		OutputDebugString( 
			"CreateGamePlayer: Player create failed!\n" );
		return FALSE;
	}

	OutputDebugString( 
		"CreateGamePlayer: Player created.\n" );

	return TRUE;
}

// The DirectPlay start callback function
BOOL CALLBACK DlgProcDPStart( HWND hWnd,	// The window 
							 UINT message,	// The message
							 WPARAM wParam, // The windows parameters
                           LPARAM lParam )	// Pointer to parameters
{
    long                i;
	HRESULT				hr;	// The return result
	LPDIRECTPLAYLOBBY	lpDPLobby	= NULL;	// A pointer to the lobby
	LPDIRECTPLAY2		lpDP2		= NULL;	// A pointer to DirectPlay
    LPVOID              lpHeap,	// The heap 
		lpConnection;	// The connection
	LPDPLCONNECTION		lpLConnection;	// Pointer to the local connection
	LPCONNECTIONINFO	lpConnectionInfo;	// Pointer to connection info
    DWORD               dwSessions;	// Number of sessions
    LPDELETEITEMSTRUCT  lpdis;
    char                szSessionName[30];	// Session name
    LPGUID              lpGuid;	// The unique id

	DPCOMPOUNDADDRESSELEMENT	addressChunks[2];
	DWORD						dwChunkCount;
	DWORD						dwSize = 0;

	// Evaluate the message
    switch( message )
    {
        case WM_INITDIALOG:

            if FAILED( CoCreateInstance( CLSID_DirectPlay,
                        NULL, CLSCTX_ALL, IID_IDirectPlay3A,
                        ( LPVOID* ) &lpDP ) )
            {
                MessageBox( hWnd, "Error creating DirectPlay object.", 
                                "Error", MB_OK );
                EndDialog( hWnd, -1 );
            }

			if FAILED( DirectPlayLobbyCreate( NULL, &lpDPLobby, 
												NULL, NULL, 0 ) )
			{
                MessageBox( hWnd, "Error creating DirectPlayLobby object.", 
                                "Error", MB_OK );
                EndDialog( hWnd, -1 );
			}
			// DirectPlayLobbyCreate gives us an IID_IDirectPlayLobby
			// interface. Query for the newer *ANSI* interface.
			else if FAILED( lpDPLobby->QueryInterface( 
						IID_IDirectPlayLobby2A, (LPVOID*) &lpDPLobby2 ) )
			{
				MessageBox( hWnd, "Error getting Lobby interface.", 
                            "Error", MB_OK );
				EndDialog( hWnd, -1 );
			}

			// This version of the interface is no longer needed
			lpDPLobby->Release();

			// Check for lobby launch

			hr = lpDPLobby2->GetConnectionSettings( 0, NULL, &dwSize );

			if ( hr != DPERR_NOTLOBBIED )

			{
				OutputDebugString("Attempting Lobby Launch...\n");

				// We were lobbied. Get the connection settings.

				lpLConnection = (LPDPLCONNECTION) malloc( dwSize );

				lpDPLobby2->GetConnectionSettings( 0,
											lpLConnection, &dwSize );

				// Set the session up the way we want it.

				lpLConnection->lpSessionDesc->dwFlags = DPSESSION_MIGRATEHOST |
														DPSESSION_KEEPALIVE;
				lpLConnection->lpSessionDesc->dwMaxPlayers = 4;
				lpDPLobby2->SetConnectionSettings( 0, 0, lpLConnection );
				
				// Connect to the session.
				lpDPLobby2->Connect( 0, &lpDP2, NULL );

				lpDP2->QueryInterface( IID_IDirectPlay3A, (LPVOID*)&lpDP );

				lpDP2->Release();

				EndDialog( hWnd, 0 );
			}

			// Fill the connections drop down list box
            SendDlgItemMessage( hWnd, IDC_CONNECTIONS,
                                CB_ADDSTRING, 0, 
                                (LPARAM) "<<Select a Connection>>" );

            if FAILED( lpDP->EnumConnections( NULL, 
                        ( LPDPENUMCONNECTIONSCALLBACK )EnumConnection,
                        ( LPVOID )GetDlgItem( hWnd, IDC_CONNECTIONS ),
                        0 ) )
            {
                MessageBox( hWnd, "Couldn't enumerate connections.",
                                "Error", MB_OK );
                EndDialog( hWnd, -1 );
            }
            else
            {
                SendDlgItemMessage( hWnd, IDC_CONNECTIONS, 
                    CB_SETCURSEL, 0, 0L );
            }

            // Start a timer for async session list
            SetTimer( hWnd, TIMER_ID, TIMER_RATE, NULL );

            return TRUE;

        case WM_DELETEITEM:
            // Delete the memory allocated when the list and combo
            // boxes are filled.

            lpdis = ( LPDELETEITEMSTRUCT ) lParam;

			if ( wParam == IDC_CONNECTIONS )
			{
				lpConnectionInfo = ( LPCONNECTIONINFO )lpdis->itemData;
				if ( lpConnectionInfo )
				{
					if ( lpConnectionInfo->lpConnection )
						free( lpConnectionInfo->lpConnection );
					free( lpConnectionInfo );
				}
			}
			else
			{
				lpHeap = ( LPVOID )lpdis->itemData;
				if ( lpHeap ) 
					free( lpHeap );
            }

            return TRUE;

        case WM_COMMAND:         
            switch( LOWORD( wParam ) )
            {
                case IDC_CONNECTIONS:
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                            // Release the existing DP object so we can 
                            // reinitialize it with the selected connection.

                            if ( lpDP ) lpDP->Release();

                            if FAILED( CoCreateInstance( CLSID_DirectPlay,
                                NULL, CLSCTX_ALL, IID_IDirectPlay3A,
                                ( LPVOID* ) &lpDP) )
                            {
                                MessageBox( hWnd, 
                                    "Error creating DirectPlay object.",
                                    "Error", MB_OK );
                                return TRUE;
                            }
                            
                            // Get the currently selected connection from 
                            // the list box.
                            i = SendDlgItemMessage( hWnd, IDC_CONNECTIONS, 
                                                        CB_GETCURSEL,
                                                        0, 0L );
                            lpConnectionInfo = (LPCONNECTIONINFO)
														SendDlgItemMessage(
                                                        hWnd, IDC_CONNECTIONS, 
                                                        CB_GETITEMDATA,
                                                        i, 0 );
							
							lpConnection = lpConnectionInfo->lpConnection;

							// If the connection uses the TCP/IP service 
							// provider, fill in chunks for the provider
							// and TCP/IP address.
							if ( IsEqualGUID( lpConnectionInfo->guidSP, 
														DPSPGUID_TCPIP ) )
							{
								dwChunkCount = 0;

								addressChunks[ dwChunkCount ].guidDataType = 
														DPAID_ServiceProvider;
								addressChunks[ dwChunkCount ].dwDataSize = 
															sizeof(GUID);
								addressChunks[ dwChunkCount ].lpData = 
													(LPVOID) &DPSPGUID_TCPIP;
								dwChunkCount++;

								// An empty address will trigger a
								// local enumeration.
								addressChunks[dwChunkCount].guidDataType = 
																	DPAID_INet;
								addressChunks[dwChunkCount].dwDataSize = 1;
								addressChunks[dwChunkCount].lpData = "";
								dwChunkCount++;

								// Find out how much space we'll need for
								// the connection data and allocate.
								lpDPLobby2->CreateCompoundAddress(
													addressChunks, 
													dwChunkCount, 
													NULL, &dwSize );

								lpConnection = malloc( dwSize );

								// Create the connection buffer.
								if FAILED( lpDPLobby2->CreateCompoundAddress(
														addressChunks, 
														dwChunkCount,
														lpConnection, 
														&dwSize ) )
								{
									OutputDebugString(
										"Couldn't create compound address.\n");
								}

							}

                            if FAILED( lpDP->InitializeConnection( 
														lpConnection, 0 ) )
                            {
                                MessageBox( hWnd, 
                                    "Error initializing DirectPlay object.",
                                    "Error", MB_OK );
                                EnableWindow( GetDlgItem( hWnd, IDCREATE ),
                                                FALSE );
                                EnableWindow( GetDlgItem( hWnd, IDPLAYERS ),
                                                FALSE );
                                EnableWindow( GetDlgItem( hWnd, IDJOIN ),
                                                FALSE );
                            }
                            else
                            {
                                EnableWindow( GetDlgItem( hWnd, IDCREATE ),
                                                TRUE );
								bUserCancel = FALSE;
                            }
                            return TRUE;
                    }
                    return TRUE;

                case IDPLAYERS:
                    DialogBox( (HINSTANCE)g_hInstance, MAKEINTRESOURCE( IDD_PLAYERS ), hWnd,
                                ( DLGPROC )DlgProcEnumPlayers );
                    return TRUE;

                case IDJOIN:
					OutputDebugString("*********Joining!\n");
                    i = SendDlgItemMessage( hWnd, IDC_SESSIONS, 
                                LB_GETCURSEL, 0, 0L );
                    lpGuid = ( LPGUID )SendDlgItemMessage( hWnd, 
                                IDC_SESSIONS, LB_GETITEMDATA, i, 0 );

                    ZeroMemory( &dpDesc, sizeof( dpDesc ) );
                    dpDesc.dwSize = sizeof( dpDesc );
                    dpDesc.guidInstance = *lpGuid;
                    dpDesc.guidApplication = SPACE_GUID;

                    if FAILED( lpDP->Open( &dpDesc, DPOPEN_JOIN ) )
                    {         
                        MessageBox( hWnd, "Could not join session.", 
                                        "Error", MB_OK );
                        EndDialog( hWnd, -1 );
                    }
                    else
                    {
						EndDialog( hWnd, 0 );
                    }
                    return TRUE;

                case IDCREATE:
                    // Create the session with the supplied parameters
                    GetDlgItemText( hWnd, IDC_SESSIONNAME, 
                        szSessionName, 30 );

                    ZeroMemory( &dpDesc, sizeof( dpDesc ) );
                    dpDesc.dwSize = sizeof( dpDesc );
                    dpDesc.guidApplication = SPACE_GUID;
                    dpDesc.dwFlags = DPSESSION_MIGRATEHOST |
                                        DPSESSION_KEEPALIVE;
					dpDesc.dwMaxPlayers = 4;
                    dpDesc.lpszSessionNameA = szSessionName;

                    if FAILED( lpDP->Open( &dpDesc, DPOPEN_CREATE ) )
                    {         
                        MessageBox( hWnd, "Could not create session.", 
                                        "Error", MB_OK );
                        EndDialog( hWnd, -1 );
                    }
                    else
                    {
						EndDialog( hWnd, 0 );
                    }
                    return TRUE;

                case IDCANCEL:
                    EndDialog( hWnd, -2 );
                    return TRUE;
            }
            break;

        case WM_TIMER:
			if ( bEnumerating || bUserCancel ) return TRUE;

            // Display the current session list if
            // there is a valid DPlay object

            SendDlgItemMessage(hWnd, IDC_SESSIONS, 
                                LB_RESETCONTENT, 0, 0L);

            ZeroMemory( &dpDesc, sizeof( dpDesc ) );
            dpDesc.dwSize = sizeof( dpDesc );
            dpDesc.guidApplication = SPACE_GUID;

            dwSessions = DPENUMSESSIONS_AVAILABLE | 
                            DPENUMSESSIONS_ASYNC;

			bEnumerating = TRUE;

            // Start the enumeration
            hr = lpDP->EnumSessions( &dpDesc, 0, 
					( LPDPENUMSESSIONSCALLBACK2 )EnumSession, 
					( LPVOID ) GetDlgItem( hWnd, IDC_SESSIONS ), 
					dwSessions );

			if ( hr == DPERR_USERCANCEL ) {
				bUserCancel = TRUE;
			}

			bEnumerating = FALSE;

            // Adjust the user interface accordingly if there
            // are sessions available
            if ( SendDlgItemMessage( hWnd, IDC_SESSIONS, 
                                        LB_GETCOUNT, 0, 0 ) > 0 )
            {
                EnableWindow( GetDlgItem( hWnd, IDPLAYERS ),
                    TRUE );
                EnableWindow( GetDlgItem( hWnd, IDJOIN ),
                    TRUE );
                SendDlgItemMessage( hWnd, IDC_SESSIONS,
                                        LB_SETCURSEL, 0, 0 );
            }
            else
            {
                EnableWindow( GetDlgItem( hWnd, IDPLAYERS ),
                    FALSE );
                EnableWindow( GetDlgItem( hWnd, IDJOIN ),
                    FALSE );
            }
            return TRUE;
                    
        case WM_DESTROY:
			// Save the player name for later
            GetDlgItemText( hWnd, IDC_PLAYER,
                            g_szPlayerName, 30 );

            KillTimer( hWnd, TIMER_ID );
            break;

    }
    return FALSE; 

}

// Evaluate a system message
void EvaluateSystemMessage( DPMSG_GENERIC *pGeneric, HWND hWnd )
{    
    switch( pGeneric->dwType )
    {
        // The message comes to us cast as DPMSG_GENERIC.  We'll examine
        // dwType to determine the type of message, then cast to the new
        // type and evaluate the rest of the message.

        case DPSYS_CREATEPLAYERORGROUP:
        {
            DPMSG_CREATEPLAYERORGROUP *pMsg;
            pMsg = (DPMSG_CREATEPLAYERORGROUP *) pGeneric;

			OutputDebugString("A player has joined.\n");

			if ( g_bHost )
			{
				MsgHost.bySlot = FindPlayerSlot();
				SendGameMessage( (LPGENERICMSG)&MsgHost, pMsg->dpId );
				OutputDebugString( "Sending welcome message.\n" );
			}

            break;
        }

        case DPSYS_DESTROYPLAYERORGROUP:
        {
            DPMSG_DESTROYPLAYERORGROUP *pMsg;
            pMsg = (DPMSG_DESTROYPLAYERORGROUP *) pGeneric;

			OutputDebugString("A player has left.\n");

			// Remove the player from the game
			EmptyPlayerSlot( pMsg->dpId );
        }

        case DPSYS_HOST:
        {
			g_bHost = TRUE;
			OutputDebugString( "This machine is now the session host.\n" );
            break;
        }       
    }      
}

// Evaluate a game message
void EvaluateGameMessage( LPGENERICMSG lpGeneric, DPID pidFrom )
{   
	NODE	node;	// The node

    switch( lpGeneric->byType )
    {
        case MSG_HOST:
			// If we're getting this message, we must be a new
			// player. Get the slot from the slot number, fill it.
			OutputDebugString("Processing welcome message.\n");
			LPHOSTMSG       lpHost;
			lpHost = (LPHOSTMSG) lpGeneric;
			FillPlayerSlot( lpHost->bySlot, g_dpPlayerID );
			g_byPlayerSlot = lpHost->bySlot;
            break;

		case MSG_FIRE:
			OutputDebugString("Getting fire!\n");
			LPFIREMSG	lpFire;
			lpFire = (LPFIREMSG) lpGeneric;

			CreateShot ( lpFire->dPosX, lpFire->dPosY,
							lpFire->dVelX, lpFire->dVelY, 0 );
			break;

		case MSG_SYNC:
			LPSYNCMSG	lpSync;
			lpSync = (LPSYNCMSG) lpGeneric;

			if ( g_Players[lpSync->bySlot].lpNode != NULL )
			{
				// The slot is occupied, update the player

				node.frame = lpSync->byFrame;
				node.posx = lpSync->dPosX;
				node.posy = lpSync->dPosY;
				node.velx = lpSync->dVelX;
				node.vely = lpSync->dVelY;
				node.status = lpSync->byStatus;

				SyncData( &node, g_Players[lpSync->bySlot].lpNode, 
							lpSync->bySlot );
			}
			else
			{
				// The slot is not occupied, we must be seeing this
				// player for the first time.

				g_Players[lpSync->bySlot].lpNode = CreateShip( 
										lpSync->dPosX, lpSync->dPosY,
										0, 0, 
										lpSync->bySlot * 40, 
										lpSync->byFrame,
										FALSE );

				g_Players[lpSync->bySlot].dwStatus = 1;
				g_Players[lpSync->bySlot].dpID = pidFrom;
			}
			break;

		case MSG_CONTROL:
			LPCONTROLMSG	lpControl;
			lpControl = (LPCONTROLMSG) lpGeneric;

			// Only process the message if we've heard from that
			// remote player before.
			if ( g_Players[lpControl->bySlot].lpNode != NULL )
			{
				NodeInputData( lpControl->byState,
					g_Players[lpControl->bySlot].lpNode );
				OutputDebugString( "Got input message.\n" );
			}
			break;
    }  
}

// Recieve messages
void ReceiveMessages( HWND hWnd )
{
    DPID            fromID;	// Who the message is from
    DPID            toID;	// Who the message is to
    DWORD           nBytes;	
    DPMSG_GENERIC   *pGeneric;
	LPGENERICMSG	pGameMsg;
    HRESULT         dprval;

    // Don't let Receive work use the global value directly,
    // as it changes it.
    nBytes = dwReceiveBufferSize;

    while( TRUE )
    {   
		dprval = lpDP->Receive( &fromID, &toID,
							DPRECEIVE_ALL, lpReceiveBuffer, &nBytes);


		if ( dprval == DPERR_BUFFERTOOSMALL )
		// The recieve buffer size must be adjusted.
		{
			if ( lpReceiveBuffer == NULL)
			{
				// We haven't allocated any buffer yet -- do it.
				lpReceiveBuffer = malloc( nBytes );
				if ( lpReceiveBuffer == NULL ) {
					OutputDebugString( "Couldn't allocate memory.\n" );
					return;
				}
			}
			else
			{
				// The buffer's been allocated, but it's too small so
				// it must be enlarged.
				free( lpReceiveBuffer );
				lpReceiveBuffer = malloc( nBytes );
				if ( lpReceiveBuffer == NULL ) {
					OutputDebugString( "Couldn't allocate memory.\n" );
					return;
				}
			}
			// Update our global to the new buffer size.
			dwReceiveBufferSize = nBytes;
		}
		else if ( dprval == DP_OK )
		// A message was successfully retrieved.
		{	
			if ( fromID == DPID_SYSMSG )
			{
				pGeneric = (DPMSG_GENERIC *) lpReceiveBuffer;
				OutputDebugString( "Processing system message.\n" );
				EvaluateSystemMessage ( pGeneric, hWnd );
			}
			else
			{
				pGameMsg = (LPGENERICMSG) lpReceiveBuffer;
				OutputDebugString("Processing game message.\n");
				EvaluateGameMessage( pGameMsg, fromID );
			}
		}
		else
		{
			return;
		}
    }
}

// The recieve thread
DWORD WINAPI ReceiveThread( LPVOID lpParameter )
{
    HWND        hWnd = (HWND) lpParameter;
    HANDLE      eventHandles[2];
    eventHandles[0] = hPlayerEvent;
    eventHandles[1] = hKillEvent;

    // Wait for either the player or kill event to fire.  If it
    // is the player event (WAIT_OBJECT_0), process the messages
    // and wait again.  If it's the kill event, shut down the
    // thread and exit

    while (WaitForMultipleObjects( 2, eventHandles, FALSE,
                INFINITE) == WAIT_OBJECT_0 )
    {
        OutputDebugString( "Thread awakened.\n" );
		ReceiveMessages( hWnd );
    }

    ExitThread( 0 );

    return ( 0 );
}

// Start a DirectPlay session
int StartDPSession( void )
{
	int rc;

	// Call the dialog for establishing a connection
	rc = DialogBox( (HINSTANCE)g_hInstance, MAKEINTRESOURCE(IDD_CONNECT), 
                            g_hwnd, (DLGPROC)DlgProcDPStart );

	if ( !rc )
	{
		InitMessageBuffers();

		// Create an event that will be used to signal when
		// the player has messages
		hPlayerEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

		hKillEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

		hReceiveThread = CreateThread(NULL,	// security attributes
                                   0,           //  initial thread size
                                   ReceiveThread,	// Thread function
                                   g_hwnd,	// argument for thread function
                                   0,           // creation flags
                                   &idReceiveThread);	// pointer to thread ID

		 // Get the caps for the session
		ZeroMemory( &dpCaps, sizeof( dpCaps ) );
		dpCaps.dwSize = sizeof( dpCaps );
		lpDP->GetCaps( &dpCaps, 0 );

		// Are we the session host?
		if ( dpCaps.dwFlags & DPCAPS_ISHOST  )
		{
			g_bHost = TRUE;
			OutputDebugString( "We are the session host.\n" );
		}
	}
	return rc;

}

// End a DirectPlay Session
BOOL ShutDownDPSession( void )
{
	// Destroy the local player
	if ( g_dpPlayerID )
	{
		lpDP->DestroyPlayer( g_dpPlayerID );
		g_dpPlayerID = 0;
	}

	// Shut down the recieve thread
	if ( hReceiveThread )
	{
		// Signal event to kill receive thread
		SetEvent( hKillEvent );

		// Wait for the thread to shut down
		WaitForSingleObject( hReceiveThread, INFINITE );

		CloseHandle( hReceiveThread); 
		hReceiveThread = NULL;
	}

	// Close the other events
	if ( hKillEvent ) 
	{
		CloseHandle( hKillEvent );
		hKillEvent = NULL;
	}
	if ( hPlayerEvent )
	{
		CloseHandle( hPlayerEvent );
		hPlayerEvent = NULL;
	}

	// Free the receive buffer.
	if ( lpReceiveBuffer )
	{
		free( lpReceiveBuffer );
		lpReceiveBuffer = NULL;
	}

	// Release the DirectPlay object and the Lobby object
	if ( lpDP )
	{
		OutputDebugString( "Releasing the DP object.\n" );
		lpDP->Release();
		lpDP = NULL;
	}
	if ( lpDPLobby2 )
	{
		OutputDebugString( "Releasing the lobby object.\n" );
		lpDPLobby2->Release();
		lpDPLobby2 = NULL;
	}
	return TRUE;
}

// Send a message to synchronize
void SendSyncMessage( void )
{
	// The player might receive this message before the
	// slot is initialized.
	if ( g_Players[g_byPlayerSlot].lpNode == NULL ) return;

	MsgSync.bySlot = g_byPlayerSlot;
	( g_Players[g_byPlayerSlot].lpNode )->last_known_good_timeupdatey =
	( g_Players[g_byPlayerSlot].lpNode )->last_known_good_timeupdatex = 0;
    ( g_Players[g_byPlayerSlot].lpNode )->last_known_goodx =
	MsgSync.dPosX = ( g_Players[g_byPlayerSlot].lpNode )->posx;
	( g_Players[g_byPlayerSlot].lpNode )->last_known_goody =
	MsgSync.dPosY = ( g_Players[g_byPlayerSlot].lpNode )->posy;
	MsgSync.dVelX = ( g_Players[g_byPlayerSlot].lpNode )->velx;
	MsgSync.dVelY = ( g_Players[g_byPlayerSlot].lpNode )->vely;
	MsgSync.byStatus = ( g_Players[g_byPlayerSlot].lpNode )->status;
	MsgSync.byFrame = (BYTE)( g_Players[g_byPlayerSlot].lpNode )->frame;

	SendGameMessage( (LPGENERICMSG)&MsgSync, DPID_ALLPLAYERS );
}

// Send a fire message
void SendFireMessage( double x, double y, double dx, double dy )
{
//	OutputDebugString( "Sending fire!\n" );
	MsgFire.dPosX = x;
	MsgFire.dPosY = y;
	MsgFire.dVelX =	dx;
	MsgFire.dVelY = dy;

	SendGameMessage( (LPGENERICMSG)&MsgFire, DPID_ALLPLAYERS );
}

// Send a Control Message
void SendControlMessage( BYTE byControl )
{
//	OutputDebugString( "Sending control!\n" );
	MsgSync.bySlot = g_byPlayerSlot;
	MsgControl.byState = byControl;
	SendGameMessage( (LPGENERICMSG)&MsgControl, DPID_ALLPLAYERS );
}

// Send a game message
void SendGameMessage( LPGENERICMSG lpMsg, DPID idTo )
{
    int             nBytes;
	DWORD			dwFlags = 0;

    switch( lpMsg->byType )
    {
		case MSG_HOST:
			nBytes = sizeof( HOSTMSG );
			dwFlags = DPSEND_GUARANTEED;
			break;

		case MSG_SYNC:
			nBytes = sizeof( SYNCMSG );
			break;

		case MSG_FIRE:
			nBytes = sizeof( FIREMSG );
			break;

		case MSG_CONTROL:
			nBytes = sizeof( CONTROLMSG );
			break;

		default:
			return;
	}

	// Send the message buffer

	lpDP->Send( g_dpPlayerID, idTo, dwFlags, (LPVOID)lpMsg, nBytes);	
}