// saser.cpp for Space Adventure Setup
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998 - 1999
//////////////////////////////////////
#define WIN32_LEAN_AND_MEAN

#include <windows.h>	// Windows header
#include <windowsx.h>	// Windows API header
#include <shellapi.h>	// For Shell.dll types and files
#include <shlobj.h>		// For the shell object
#include "saset.h"		// Holds global constants
#include "dsetup.h"		// The direct setup header

#define DIRECTXSPACE	15000000			// Space needed for DirectX
#define SASPACE			500000				// Space needed for Our game
#define DEFAULTDIR		"C:\\Games\\SpaceA"	// Our preferred directory

HANDLE  ghWnd;          // hWnd for callback.
DWORD   gdwFreeBytes;	// Number of client free bytes

char    szTargetDir[MAX_PATH];	// Size of the target directory
char    szSetupDir[MAX_PATH];	// Size of our setup directory
char    szSystemDir[MAX_PATH];	// Size of the client system directory
char	szWindowsDir[MAX_PATH];	// Size of the client Windows directory

// The list of files to copy from the setup directory to the target
static char* copy_list [] =
{
    "SPACEA~2.exe","shot.wav","hit.wav","welcome.wav","readme.txt",
};

// Concatenate file path
void CatPath( char *dst, char *src )
{
        int len = lstrlen( dst );

		// Check for a backslash.
        if ( len > 0 && ( dst[len-1] != '\\' && dst[len-1] != '/') )
		{
			lstrcat( dst,"\\" );
		}

        lstrcat( dst, src );

        // SHFileOperation needs a double null string.
        len = lstrlen( dst );
        dst[len+1] = 0;
}

// Get the source path
void GetSourcePath( HANDLE hInstance, LPSTR szPath )
{
	char *p;
	char *x;

	GetModuleFileName( (HINSTANCE) hInstance, szPath, MAX_PATH );

    for ( x=p=szPath; *p; p = AnsiNext( p ) )
    {
		// Find the last slash in the string.
        if ( ( *p == '\\' ) || ( *p == '/' ) )
		{
			x = p;
		}
    }
	// Set it to null.
    *x = 0;
}

// Get client's number of free bytes
int GetFreeBytes( char* szDir )
{
    DWORD	dwSectorsPerCluster;
    DWORD	dwBytesPerSector;
    DWORD	dwFreeClusters;
    char	szDrive[4];

    memset( szDrive, 0, sizeof( szDrive ) );
    memcpy( szDrive, szDir, 3 );

    if ( GetDiskFreeSpace( szDrive, &dwSectorsPerCluster,
            &dwBytesPerSector, &dwFreeClusters, NULL ) )
    {
        return ( dwBytesPerSector * dwSectorsPerCluster *
                    dwFreeClusters );
    }
    else
    {
        return 0;
    }
}

// Used to print info
void LBPrintf( HWND hWnd, UINT id, LPSTR fmt, ... )
{
    char    buff[256];
    int     iIndex;

    wvsprintf( buff, fmt, ( LPSTR )( &fmt+1 ) );

    iIndex = SendDlgItemMessage( hWnd, id, LB_ADDSTRING, 0, 
                                    ( LONG )( LPSTR )buff );
    SendMessage(hWnd, LB_SETCURSEL, iIndex, 0);

}

// The setup callback function
DWORD CALLBACK SetupCallback( DWORD Reason,	// The reason flag
							 DWORD  MsgType,	// Type of message box
                              LPSTR szMessage,	// Message size
							  LPSTR szName,		// Message name
                              DSETUP_CB_UPGRADEINFO *pUpgradeInfo )	// pointer to the info structure
{
    // Nothing to say, so return.
    if ( Reason == DSETUP_CB_MSG_NOMESSAGE 
                && MsgType == 0 ) return IDOK;

    // Evaluate the status message.
    switch ( Reason )
    {
        case DSETUP_CB_MSG_BEGIN_INSTALL:
            LBPrintf( (HWND) ghWnd, IDC_MESSAGES,
                "Beginning DX install..." );
            break;
        case DSETUP_CB_MSG_BEGIN_INSTALL_RUNTIME:
            LBPrintf( (HWND) ghWnd, IDC_MESSAGES,
                "Installing DX components..." );
            break;
        case DSETUP_CB_MSG_BEGIN_INSTALL_DRIVERS:
            LBPrintf( (HWND) ghWnd, IDC_MESSAGES,
                "Installing DX drivers..." );
            break;
    }

    // If the box is information-only, just return
    // 0 for default, which is "OK".
    if ( MsgType == 0 ) return IDOK;
    if ( MsgType & MB_ICONINFORMATION ) return 0;

    return MessageBox( (HWND) ghWnd, szMessage, 
                        "Space Adventure", MsgType );
}

// Create a shortcut to our game
HRESULT CreateShortcut( LPCSTR lpszPathObj, LPSTR lpszPathLink ) 
{     
	HRESULT hres; 
    IShellLink* psl;
    
	hres = CoCreateInstance( CLSID_ShellLink, NULL, 
				CLSCTX_INPROC_SERVER, IID_IShellLink, ( LPVOID * )&psl ); 
    
	if ( SUCCEEDED( hres ) ) 
	{         
		IPersistFile* ppf;  
                
		psl->SetPath( lpszPathObj ); 
		psl->SetIconLocation( lpszPathObj, 0 );

		// Query IShellLink for the IPersistFile interface for saving the 
		// shortcut in persistent storage. 
        hres = psl->QueryInterface( IID_IPersistFile, ( LPVOID * )&ppf );          
		if ( SUCCEEDED( hres ) ) 
		{ 
            WORD wsz[MAX_PATH];
            MultiByteToWideChar( CP_ACP, 0, lpszPathLink, -1, 
                wsz, MAX_PATH );  
            hres = ppf->Save( wsz, TRUE ); 
            ppf->Release();         
		} 
        psl->Release();     
	}     	
	return hres; 
}

// Copy the files
HRESULT CopyFiles( HWND hWnd )
{
	int i;
	char szSrc[MAX_PATH];
    char szDst[MAX_PATH];
    SHFILEOPSTRUCT fileop;
	HRESULT result;

	fileop.hwnd     = hWnd;
	fileop.wFunc    = FO_COPY;
	fileop.fFlags   = FOF_SILENT | FOF_NOCONFIRMATION;

	for ( i = 0; i < sizeof( copy_list )/sizeof( copy_list[0] ); i++ )
	{
		lstrcpy( szSrc, szSetupDir );
		CatPath( szSrc, copy_list[i] );

		lstrcpy( szDst, szTargetDir );
		CatPath( szDst, copy_list[i] );

		fileop.pFrom    = szSrc;
		fileop.pTo      = szDst;

		LBPrintf( hWnd, IDC_MESSAGES,
                                "From: %s", szSrc );
		LBPrintf( hWnd, IDC_MESSAGES,
                                "To: %s", szDst );
		
        result = SHFileOperation( &fileop );
                            
        if ( result == 0 )
        {
			SetFileAttributes( szDst, FILE_ATTRIBUTE_NORMAL );
        }
		else
		{
			return result;
        }
	}
	return result;
}

// Add an entry in the registry
void DPRegister( HWND hWnd, LPSTR szDirectory )
{
	DIRECTXREGISTERAPP	regentries;

	// BAF45162-47DA-46DF-847E-5A5910AEEF84
	// This holds our unique identifier
	GUID SPACE_GUID = {
		0xbaf45162,
		0x47da,
		0x46df,
		{0x84, 0x7e, 0x5a, 0x59, 0x10, 0xae, 0xef, 0x84}
	};

	// Prepare a DIRECTXREGISTERAPP structure.
	ZeroMemory( &regentries,
					sizeof( DIRECTXREGISTERAPP ) );
	regentries.dwSize = sizeof( DIRECTXREGISTERAPP );

	regentries.dwFlags = 0; 
	regentries.lpszApplicationName = "Space Adventure"; 
	regentries.lpGUID = &SPACE_GUID;
	regentries.lpszFilename = "Space Adventure3.exe"; 
    regentries.lpszCommandLine = "";

	regentries.lpszPath = szDirectory; 
	regentries.lpszCurrentDirectory = szDirectory; 

	// Register the application.
	DirectXRegisterApplication( hWnd, &regentries );	
}

// The procedure that corresponds with the rc file
LRESULT CALLBACK DlgProc( HWND hWnd,	// The window
						 UINT message,	// The message
                          WPARAM wParam,	
						  LPARAM lParam)	
{
	char szPath[MAX_PATH];
	char szPath2[MAX_PATH];

    switch ( message )
    {
        case WM_INITDIALOG:

            ghWnd = hWnd;

            LBPrintf( hWnd, IDC_MESSAGES,
                                "Source Dir: %s", szSetupDir );

			GetWindowsDirectory( szWindowsDir,
							sizeof( szWindowsDir ) );

			// Get the free space on the system directory drive,
			// where DirectX will be installed.

            GetSystemDirectory( szSystemDir, 
                            sizeof( szSystemDir ) );

            LBPrintf( hWnd, IDC_MESSAGES,
                                "System Dir: %s", szSystemDir );

            gdwFreeBytes = GetFreeBytes( szSystemDir );

            LBPrintf( hWnd, IDC_MESSAGES,
                                "    Space: %d", gdwFreeBytes );

            if ( gdwFreeBytes < DIRECTXSPACE )
            {
                MessageBox( hWnd, 
                            "There's not enough space free on your "
                            "system drive to install Microsoft "
                            "DirectX,  which is required for Space "
                            "Adventure. Please free up at least "
                            "15 megabytes and run setup again.",
                            "Space Adventure",
                            MB_OK | MB_ICONWARNING );

                EndDialog( hWnd, FALSE );
            }

            // Limit the path edit box to the maximum path length.
            SendDlgItemMessage( hWnd, IDC_PATH, EM_LIMITTEXT, 
									MAX_PATH, 0L );
            // Set the text to the default directory.
            SetDlgItemText( hWnd, IDC_PATH, DEFAULTDIR );
            // Select all the text by sending 0, -1.
            SendDlgItemMessage( hWnd, IDC_PATH, EM_SETSEL, 0, 
										MAKELONG( 256, 256 ) );
            // Set the focus to the edit box.
            SetFocus( GetDlgItem( hWnd, IDC_PATH ) );

            // We've already set the focus.
            return FALSE;

        case WM_COMMAND:
            
            switch ( LOWORD( wParam ) )
            {
                case IDOK:
                    int iResult;
                    DWORD dwFlags;
                    dwFlags = 0;
					DWORD dwVersion;

                    // Clear the message log.

                    SendDlgItemMessage( hWnd, IDC_MESSAGES, 
                                          LB_RESETCONTENT, 0, 0L );

					// Get the target directory and check its drive
					// for adequate space.

					GetWindowText( GetDlgItem( hWnd,IDC_PATH ), 
								szTargetDir, sizeof( szTargetDir ));

					gdwFreeBytes = GetFreeBytes( szTargetDir );

					LBPrintf( hWnd, IDC_MESSAGES,
										"Target Space: %d", gdwFreeBytes );

					if ( gdwFreeBytes < SASPACE )
					{
						MessageBox( hWnd, 
									"There's not enough space free on your "
									"destination drive to install Space "
									"Adventure. Please free up at least "
									"500K bytes and try again.",
									"Space Adventure",
									MB_OK | MB_ICONWARNING );

						return TRUE;
					}

					// Now attempt to install the application file(s).
					// A real application would have more robust error
					// checking here.

					LBPrintf( hWnd, IDC_MESSAGES,
                                "Installing application files." );

					if ( FAILED( CopyFiles( hWnd ) ) )
					{
						MessageBox( hWnd, 
							"Installation failed.",
							"Space Adventure",
							MB_OK | MB_ICONWARNING );
						return TRUE;
					}

					// Create a shortcut on the Programs menu.

					LBPrintf( hWnd, IDC_MESSAGES,
                                "Creating menu shortcut." );
					lstrcpy( szPath, szTargetDir );
					CatPath( szPath, "Space Adventure3.exe" );
					lstrcpy( szPath2, szWindowsDir );
					CatPath( szPath2,
							"start menu\\programs\\Space Adventure.lnk" );

					CreateShortcut( szPath, szPath2 );

					// Register the application with DirectPlay.

					LBPrintf( hWnd, IDC_MESSAGES,
                                "Registering for DirectPlay." );

					DPRegister( hWnd, szTargetDir );

					// See what the DX situation is, just for the sake
					// of demonstration.

					LBPrintf( hWnd, IDC_MESSAGES,
                                "Checking DX version." );

					if ( DirectXSetupGetVersion( &dwVersion, NULL ) )
					{
						if ( dwVersion >= 0x00040005 )
						{
							iResult = MessageBox( hWnd, 
								"Space Adventure requires Microsoft "
								"DirectX version 5.0 or greater. "
								"Your system already has a sufficent "
								"version of DirectX installed, "
								"would you like to re-install DirectX "
								"anyway?",
								"Space Adventure",
								MB_YESNO | MB_ICONQUESTION );
							if ( iResult == IDNO )
							{
								LBPrintf( hWnd, IDC_MESSAGES,
									"Installation completed." );
								return TRUE;
							}
						}
					}

					// Install DirectX.
					LBPrintf( hWnd, IDC_MESSAGES,
                                "Installing DX." );

                    DirectXSetupSetCallback( (DSETUP_CALLBACK) 
                                            SetupCallback );

                    iResult = DirectXSetup( hWnd, NULL, dwFlags );

                    switch ( iResult )
                    {
                        case DSETUPERR_SUCCESS:
                            LBPrintf( hWnd, IDC_MESSAGES,
                                "DX: Success, no reboot required." );
                            break;
                        case DSETUPERR_SUCCESS_RESTART:
                            LBPrintf( hWnd, IDC_MESSAGES,
                                "DX: Success, reboot required." );
                            break;
                        case DSETUPERR_OUTOFDISKSPACE:
                            LBPrintf( hWnd, IDC_MESSAGES,
                                "DX: Out of disk space." );
                            break;
                        case DSETUPERR_USERHITCANCEL :
                            LBPrintf( hWnd, IDC_MESSAGES,
                                "DX: User cancelled." );
                            break;
						case DSETUPERR_NOTPREINSTALLEDONNT:
                            LBPrintf( hWnd, IDC_MESSAGES,
                                "DX: Not preinstalled." );
							MessageBox( hWnd, 
									"Microsoft DirectX must be installed "
									"in a Windows NT release or service "
									"pack. You must install an updated "
									"version of DirectX before Space "
									"Adventure can run.",
									"Space Adventure",
									MB_OK | MB_ICONWARNING );
							break;
                        default:
                            LBPrintf( hWnd, IDC_MESSAGES,
                                "Failure Code: %d", iResult );
                    }
					
					// The DirectX installation failed.
					if ( iResult < 0 )
					{
						MessageBox( hWnd, 
							"Microsoft DirectX was not successfully "
							"installed. DirectX is required for "
							"Space Adventure.",
							"Space Adventure",
							MB_OK | MB_ICONWARNING );
					}

					// The DirectX installation succeeded, but a reboot
					// is required.
					if ( iResult == DSETUPERR_SUCCESS_RESTART )
					{
						iResult = MessageBox( hWnd, 
							"Your system must be restarted to complete "
							"the Microsoft DirectX installation. Would "
							"you like to restart?",
							"Space Adventure",
							MB_YESNO | MB_ICONQUESTION );

						if ( iResult == IDYES )
						{
							ExitWindowsEx( EWX_REBOOT, 0 );
							EndDialog( hWnd, TRUE );
							return TRUE;
						}
					}
					LBPrintf( hWnd, IDC_MESSAGES,
									"Installation completed." );
					return TRUE;
                    break;

                case IDC_BROWSE:
                    BROWSEINFO bi;
                    LPITEMIDLIST pidl;
                    IMalloc *lpSHMalloc;

                    // Prepare to use the shell for browsing.
                    bi.hwndOwner      = hWnd;
                    bi.pidlRoot       = NULL;
                    bi.pszDisplayName = szPath;
                    bi.lpszTitle      = "Select an existing "
                                        "destination folder "
                                        "for your SpaceAdventure "
                                        "installation.";
                    bi.ulFlags        = BIF_RETURNONLYFSDIRS;
                    bi.lpfn           = NULL;
                    bi.lParam         = 0;
                    bi.iImage         = 0;

                    // Get the item identifier list for the folder.
                    pidl = SHBrowseForFolder( &bi );

                    // Convert the IDL to a path string.
                    if ( pidl )
                    {
                        SHGetPathFromIDList( pidl, szPath );
                        SetDlgItemText( hWnd, IDC_PATH, szPath );
                        // Free the IDL.
                        SHGetMalloc( &lpSHMalloc );
                        if ( lpSHMalloc )
                        {
                            lpSHMalloc->Free( pidl );
                            lpSHMalloc->Release();
                        }
                    }
                    return TRUE;

                case IDC_README:
					lstrcpy( szPath, "notepad.exe " );
					lstrcat( szPath, szSetupDir );
					CatPath( szPath, "readme.txt" );
                    WinExec( szPath, SW_SHOW );
                    return TRUE;

				case IDC_PLAY:
					lstrcpy( szPath, szSetupDir );
					CatPath( szPath, "SPACEA~2.exe" );
                    WinExec( szPath, SW_SHOW );
					EndDialog( hWnd, TRUE );
                    return TRUE;

                case IDCANCEL:
                    EndDialog( hWnd, FALSE );
                    return TRUE;
            }
            break;
    }
    return FALSE; 
}

int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow)
{
    HANDLE hMutex;	// The mutex handle
    
    GetSourcePath( hInstance, szSetupDir );	// Get the current directory

    CoInitialize( NULL );

    hMutex = CreateMutex( NULL, FALSE, "There can be only one" );	// Create a mutex

	// Check for mutex, if there is one close silently
    if ( ( hMutex == NULL ) || ( GetLastError() == ERROR_ALREADY_EXISTS ) )
    {
        CloseHandle( hMutex );
        return FALSE;
    }

	// Create the dialog box that calls DlgProc
    DialogBox( hInstance, MAKEINTRESOURCE( IDD_MAIN ), NULL, ( DLGPROC )DlgProc );

    CloseHandle( hMutex );	// Close the mutex
    
    CoUninitialize();

    return TRUE;
}