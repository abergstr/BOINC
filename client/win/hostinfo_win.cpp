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

#include "boinc_win.h"

#include "client_types.h"
#include "filesys.h"
#include "util.h"
#include "hostinfo_network.h"
#include "hostinfo.h"

HINSTANCE g_hIdleDetectionDll;

// Returns the number of seconds difference from UTC
//
int get_timezone(void) {
	TIME_ZONE_INFORMATION tzi;
	ZeroMemory(&tzi, sizeof(TIME_ZONE_INFORMATION));
	GetTimeZoneInformation(&tzi);
	return -(tzi.Bias * 60);
}

// Gets windows specific host information (not complete)
//
int HOST_INFO::get_host_info() {

    ZeroMemory(os_name, sizeof(os_name));
    ZeroMemory(os_version, sizeof(os_version));

    // This code snip-it was copied straight out of the MSDN Platform SDK
    //   Getting the System Version example and modified to dump the output
    //   into os_name.

    OSVERSIONINFOEX osvi;
    BOOL bOsVersionInfoEx;

    // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
    // If that fails, try using the OSVERSIONINFO structure.

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
    {
        osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        GetVersionEx ( (OSVERSIONINFO *) &osvi );
    }


    switch (osvi.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_NT:

            if ( osvi.dwMajorVersion >= 6 )
                strcpy(os_name, "Microsoft Windows Longhorn" );

            if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
                strcpy(os_name, "Microsoft Windows 2003" );

            if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
                strcpy(os_name, "Microsoft Windows XP" );

            if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
                strcpy(os_name, "Microsoft Windows 2000" );

            if ( osvi.dwMajorVersion <= 4 )
                strcpy(os_name, "Microsoft Windows NT" );

            break;

        case VER_PLATFORM_WIN32_WINDOWS:

            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
                strcpy(os_name, "Microsoft Windows 95" );

            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
                strcpy( os_name, "Microsoft Windows 98" );

            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
                strcpy( os_name, "Microsoft Windows Millennium" );

            break;

        case VER_PLATFORM_WIN32s:

            strcpy( os_name, "Microsoft Win32s" );
            break;
    }


    char szVersion[128];
    char szSKU[128];
    char szServicePack[128];


    ZeroMemory( szVersion, sizeof(szVersion) );
    ZeroMemory( szSKU, sizeof(szSKU) );
    ZeroMemory( szServicePack, sizeof(szServicePack) );


    snprintf( szVersion, sizeof(szVersion), ", (%.2u.%.2u.%.4u.%.2u)",
        osvi.dwMajorVersion, osvi.dwMinorVersion, (osvi.dwBuildNumber & 0xFFFF), 0 );


    switch (osvi.dwPlatformId)
    {
        // Test for the Windows NT product family.
        case VER_PLATFORM_WIN32_NT:

            // Test for specific product on Windows NT 4.0 SP6 and later.
            if( bOsVersionInfoEx )
            {
                // Test for the workstation type.
                if ( osvi.wProductType == VER_NT_WORKSTATION )
                {
                    if( osvi.dwMajorVersion == 4 )
                        strcpy( szSKU, "Workstation Edition" );
                    else if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
                        strcpy( szSKU, "Home Edition" );
                    else
                        strcpy( szSKU, "Professional Edition" );
                }
            
                // Test for the server type.
                else if ( (osvi.wProductType == VER_NT_SERVER) || (osvi.wProductType == VER_NT_DOMAIN_CONTROLLER) )
                {
                    if( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 )
                    {
                        if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                            strcpy( szSKU, "Datacenter Server Edition" );
                        else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                            strcpy( szSKU, "Enterprise Server Edition" );
                        else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
                            strcpy( szSKU, "Web Server Edition" );
                        else
                            strcpy( szSKU, "Standard Server Edition" );
                    }
                    else if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
                    {
                        if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                            strcpy( szSKU, "Datacenter Server Edition" );
                        else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                            strcpy( szSKU, "Enterprise Server Edition" );
                        else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
                            strcpy( szSKU, "Web Server Edition" );
                        else
                            strcpy( szSKU, "Standard Server Edition" );
                    }
                    else if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
                    {
                        if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                            strcpy( szSKU, "Datacenter Server Edition" );
                        else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                            strcpy( szSKU, "Advanced Server Edition" );
                        else
                            strcpy( szSKU, "Standard Server Edition" );
                    }
                    else  // Windows NT 4.0 
                    {
                        if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                            strcpy( szSKU, "Enterprise Server Edition" );
                        else
                            strcpy( szSKU, "Server Edition" );
                    }
                }
            }
            else  // Test for specific product on Windows NT 4.0 SP5 and earlier
            {
                HKEY hKey;
                char szProductType[80];
                DWORD dwBufLen=sizeof(szProductType);
                LONG lRet;

                lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                    "SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
                    0, KEY_QUERY_VALUE, &hKey );
                if( lRet != ERROR_SUCCESS )
                    return FALSE;

                lRet = RegQueryValueEx( hKey, "ProductType", NULL, NULL,
                    (LPBYTE) szProductType, &dwBufLen);
                if( (lRet != ERROR_SUCCESS) || (dwBufLen > 80) )
                    return FALSE;

                RegCloseKey( hKey );

                if ( lstrcmpi( "WINNT", szProductType) == 0 )
                    strcpy( szSKU, "Workstation Edition" );
                if ( lstrcmpi( "LANMANNT", szProductType) == 0 )
                    strcpy( szSKU, "Server Edition" );
                if ( lstrcmpi( "SERVERNT", szProductType) == 0 )
                    strcpy( szSKU, "Advanced Server Edition" );

            }

            // Display service pack (if any) and build number.

            if( osvi.dwMajorVersion == 4 && lstrcmpi( osvi.szCSDVersion, "Service Pack 6" ) == 0 )
            {
                HKEY hKey;
                LONG lRet;

                // Test for SP6 versus SP6a.
                lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009",
                    0, KEY_QUERY_VALUE, &hKey );
                if( lRet == ERROR_SUCCESS )
                {
                    strcpy( szServicePack, ", " );
                    strcat( szServicePack, "Service Pack 6a" );
                }
                else // Windows NT 4.0 prior to SP6a
                {
                    if ( strlen(osvi.szCSDVersion) > 0 )
                    {
                        strcpy( szServicePack, ", " );
                        strcat( szServicePack, osvi.szCSDVersion );
                    }
                }

                RegCloseKey( hKey );
            }
            else // Windows NT 3.51 and earlier or Windows 2000 and later
            {
                if ( strlen(osvi.szCSDVersion) > 0 )
                {
                    strcpy( szServicePack, ", " );
                    strcat( szServicePack, osvi.szCSDVersion );
                }
            }

            break;

        // Test for the Windows 95 product family.
        case VER_PLATFORM_WIN32_WINDOWS:

            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
            {
                if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
                    strcpy( szServicePack, "OSR2" );
            } 

            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
            {
                if ( osvi.szCSDVersion[1] == 'A' )
                    strcpy( szServicePack, "SE" );
            } 

            break;
    }

    snprintf( os_version, sizeof(os_version), "%s%s%s",
        szSKU, szServicePack, szVersion );


    // Detect the number of CPUs
    SYSTEM_INFO SystemInfo;
    memset( &SystemInfo, NULL, sizeof( SystemInfo ) );
    ::GetSystemInfo( &SystemInfo );

    p_ncpus = SystemInfo.dwNumberOfProcessors;

    // Detect the filesystem information
	get_filesystem_info(d_total, d_free);
    
	// Open the WinSock dll so we can get host info
    WORD    wVersionRequested;
	WSADATA wsdata;
	wVersionRequested = MAKEWORD(1, 1);
	WSAStartup(wVersionRequested, &wsdata);

	// Get host name/ip info
    get_local_network_info(
        domain_name, sizeof(domain_name), ip_addr, sizeof(ip_addr)
    );

	// Close the WinSock dll
	WSACleanup();

	timezone = get_timezone();

	MEMORYSTATUS mStatus;
	ZeroMemory(&mStatus, sizeof(MEMORYSTATUS));
	mStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&mStatus);
	m_nbytes = (double)mStatus.dwTotalPhys;
	m_swap = (double)mStatus.dwTotalPageFile;
	
	// gets processor vendor name and model name from registry, works for intel
	char vendorName[256], processorName[256], identifierName[256];
	HKEY hKey;
	LONG retval;
	DWORD nameSize, procSpeed;
	bool gotIdent = false, gotProcName = false, gotMHz = false, gotVendIdent = false;

	retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE, &hKey);
	if(retval == ERROR_SUCCESS) {
        // Win9x and WinNT store different information in these field.
        // NT Examples:
        // ProcessorNameString: Intel(R) Xeon(TM) CPU 3.06GHz
        // Identifier: x86 Family 15 Model 2 Stepping 7
        // VendorIdentifier: GenuineIntel
        // ~MHz: 3056
        // 9X Examples:
        // ProcessorNameString: <Not Defined>
        // Identifier: Pentium(r) Processor
        // ~MHz: <Not Defined>
        // VendorIdentifier: GenuineIntel

        // Look in various places for processor information, add'l
		// entries suggested by mark mcclure
		nameSize = sizeof(vendorName);
		retval = RegQueryValueEx(hKey, "VendorIdentifier", NULL, NULL, (LPBYTE)vendorName, &nameSize);
		if (retval == ERROR_SUCCESS) gotVendIdent = true;

		nameSize = sizeof(identifierName);
		retval = RegQueryValueEx(hKey, "Identifier", NULL, NULL, (LPBYTE)identifierName, &nameSize);
		if (retval == ERROR_SUCCESS) gotIdent = true;

		nameSize = sizeof(processorName);
		retval = RegQueryValueEx(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)processorName, &nameSize);
		if (retval == ERROR_SUCCESS) gotProcName = true;

		nameSize = sizeof(DWORD);
		retval = RegQueryValueEx(hKey, "~MHz", NULL, NULL, (LPBYTE)&procSpeed, &nameSize);
		if (retval == ERROR_SUCCESS) gotMHz = true;
	}

    if (gotVendIdent) safe_strncpy( p_vendor, vendorName, sizeof(p_vendor) );
    else safe_strncpy( p_vendor, "Unknown", sizeof(p_vendor) );

    if (gotProcName) {
        safe_strncpy( p_model, processorName, sizeof(p_model) );
    } else if (gotIdent && gotMHz) {
        sprintf( p_model, "%s %dMHz", identifierName, procSpeed );
    } else if (gotVendIdent && gotMHz) {
        sprintf( p_model, "%s %dMHz", vendorName, procSpeed );
    } else if (gotIdent) {
        safe_strncpy( p_model, identifierName, sizeof(p_model) );
    } else if (gotVendIdent) {
        safe_strncpy( p_model, vendorName, sizeof(p_model) );
    } else {
        safe_strncpy( p_model, "Unknown", sizeof(p_model) );
    }

	RegCloseKey(hKey);
    return 0;
}

bool HOST_INFO::host_is_running_on_batteries() {
	SYSTEM_POWER_STATUS pStatus;
	ZeroMemory(&pStatus, sizeof(SYSTEM_POWER_STATUS));
	if (!GetSystemPowerStatus(&pStatus)) {
		return false;
	}

	return (pStatus.ACLineStatus != 1);
}

bool HOST_INFO::users_idle(bool check_all_logins, double idle_time_to_run) {
    typedef DWORD (CALLBACK* GetFn)();
    static GetFn fn = (GetFn)GetProcAddress(g_hIdleDetectionDll, "IdleTrackerGetLastTickCount");

    if (g_hIdleDetectionDll) {
        if (fn) {
            return (GetTickCount() - fn()) / 1000 > 60 * idle_time_to_run;
        }
    }

    return false;
}

const char *BOINC_RCSID_37fbd07edd = "$Id$";
