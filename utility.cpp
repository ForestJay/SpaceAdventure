// utility.cpp - for Space Adventure 1
// East Coast Games
// Forest J. Handford
// Copyright (c) 1998
////////////////////////////
#define WIN32_LEAN_AND_MEAN	//Ask for a lean compilation
#include <windows.h>	//Windows header
#include <windowsx.h>	//Window's API header
#include <stdlib.h>		//The C standard library header
#include <stdio.h>		//The C standard I/O header
#include "utility.h"	//Our utility header

/*
 *  randInt
 *
 *  Generate a random integer between two values
 *
 */

int randInt( 
			int low,	// lower limit, inclusive
			int high	// upper limit, inclusive
			)
{
    int range = high - low;
    int num = rand() % range;
    return( num + low );
}

/*
 *  randDouble
 *
 *  Generate a random double between two values
 *
 */

double randDouble(
				  double low,	// lower limit, inclusive
				  double high	// upper limit, inclusive
				  )
{
    double range = high - low;
    double num = range * (double)rand()/(double)RAND_MAX;
    return( num + low );
}

/*
 *  DebugPrintf
 *
 *  Output a string to the debugger
 *
 */

void DebugPrintf( LPSTR fmt, ... )
{
    char    buff[256];

    sprintf( buff, fmt, (LPSTR)(&fmt+1) );

	OutputDebugString( buff );

}

