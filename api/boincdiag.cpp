// The contents of this file are subject to the BOINC Public License
// Version 1.0 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://boinc.berkeley.edu/license_1.0.txt
//
// Software distributed under the License is distributed on an "AS IS"
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
// License for the specific language governing rights and limitations
// under the License.
//
// The Original Code is the Berkeley Open Infrastructure for Network Computing.
//
// The Initial Developer of the Original Code is the SETI@home project.
// Portions created by the SETI@home project are Copyright (C) 2002
// University of California at Berkeley. All Rights Reserved.
//
// Contributor(s):
//

#include "boincdiag.h"

///////////////////////////////////////////////////////////////////////////////
//
// Windows 32-bit Platforms
//
///////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32

// Check to see if the MFC/ATL frameworks are included, if so we have to use a
//   different set of diagnostics functions.
#ifdef __AFXWIN_H__


//
// TODO: Determine what needs to happen between the two frameworks and implement it
//


// Well if we are not using the MFC/ATL frameworks, then we are using the CRT.
#else // __AFXWIN_H__

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "app_ipc.h"

#ifdef _DEBUG

void boinc_trace(const char *pszFormat, ...)
{
	static char szBuffer[4096];

	memset(szBuffer, 0, sizeof(szBuffer));

	va_list ptr;
	va_start(ptr, pszFormat);

	BOINCASSERT( -1 != _vsnprintf(szBuffer, sizeof(szBuffer), pszFormat, ptr) );

	va_end(ptr);

	_CrtDbgReport(_CRT_WARN, NULL, NULL, NULL, "%s", szBuffer);
}

#endif // _DEBUG


//
// Function: BoincReportingFunction
//
// Purpose:  Trap ASSERTs from the CRT and spew them to stderr.
//
// Date:     01/29/04
//
int __cdecl BoincReportingFunction( int reportType, char *szMsg, int *retVal ){ 
	(*retVal) = 0; 

	if ( _CRT_ASSERT == reportType ){
		fprintf( stderr, "ASSERT: %s\n", szMsg );
		fflush( stderr );
		(*retVal) = 1;
	} else { 
		fprintf( stderr, "TRACE: %s", szMsg );
		fflush( stderr );
	}

	return(TRUE);
} 


//
// Function: boinc_diag_init
//
// Purpose:  Initialize the diagnostic system
//
// Date:     01/29/04
//
int boinc_diag_init() {	

	// Redirect stderr earlier then boinc_init so we can trap errors earlier.
    freopen(STDERR_FILE, "a", stderr);

	// Define how messages should me formatted to sdterr
	_CrtSetReportHook( BoincReportingFunction );

	return TRUE;
}


//
// Function: boinc_diag_cleanup
//
// Purpose:  Cleanup the diagnostic system
//
// Date:     01/29/04
//
int boinc_diag_cleanup() {	
	return TRUE;
}


#endif // __AFXWIN_H__

#endif // _WIN32
