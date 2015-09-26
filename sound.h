/*==========================================================================
 *
 *  Copyright (C) 1997 Microsoft Corporation. All Rights Reserved.
 *
 *  File:       sound.h
 *  Content:    DirectSound routines include file
 *
 ***************************************************************************/


#include <dsound.h>

#define PANLEFT (DSBPAN_LEFT / 10)
#define PANRIGHT (DSBPAN_RIGHT / 10)

bool PlayBuffer(void);
bool SetupStreamBuffer(LPSTR);
void PanStreaming(int);

bool  InitDSound(HWND, HINSTANCE, GUID*);
void  Cleanup(void);

bool CALLBACK EnumCallback(LPGUID, LPCSTR, LPCSTR, LPVOID);          
