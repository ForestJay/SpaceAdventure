// static.h - for Space Adventure 
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998 - 1999
////////////////////////////
#ifndef __STATIC_INCLUDED__
#define __STATIC_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

#define SHOTWAVE "shot.wav"
#define HITWAVE "hit.wav"
#define WELCOMEWAVE "welcome.wav"
#include <dsound.h>		//The DirectSound library

bool LoadStatic(LPDIRECTSOUND lpds, LPSTR lpzFileName);
void PlayStatic(LPSTR);

#ifdef __cplusplus
}
#endif

#endif

