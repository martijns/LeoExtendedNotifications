// LeoExtendedNotificationsSetupActions.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "ce_setup.h"


BOOL APIENTRY DllMain( HANDLE hModule, 
					   DWORD  ul_reason_for_call, 
					   LPVOID lpReserved
					 )
{
	return TRUE;
}

// Use this method to close the app.
bool CloseLeoExtendedNotificationsApp(void)
{
	// Before (un)installing, we need to shut down the existing LeoExtendedNotifications. We'll use the Event for this which we have since the first version.
	HANDLE appEvent = CreateEvent(NULL, TRUE, FALSE, L"LedExtendedNotificationsApp");
	if (appEvent == NULL)
	{
		// I won't understand why we couldn't create this event. Weird out-of-memory maybe. We failed.
		return false;
	}

	// See if the event already existed
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// Event exists, means the app is running! Signal the event to close the app.
		SetEvent(appEvent);
		CloseHandle(appEvent);

		// Wait a second to allow the app to exit.
		Sleep(1000);
	}
	else
	{
		// The event doesn't exist. The app isn't running.
		CloseHandle(appEvent);
	}
	return true;
}

codeINSTALL_INIT
Install_Init(
			 HWND		hwndParent,
			 BOOL		fFirstCall,	 // is this the first time this function is being called?
			 BOOL		fPreviouslyInstalled,
			 LPCTSTR	 pszInstallDir
			 )
{
	if (CloseLeoExtendedNotificationsApp())
	{
		return codeINSTALL_INIT_CONTINUE;
	}
	return codeINSTALL_INIT_CANCEL;
}



codeINSTALL_EXIT
Install_Exit(
			 HWND	hwndParent,
			 LPCTSTR pszInstallDir,
			 WORD	cFailedDirs,
			 WORD	cFailedFiles,
			 WORD	cFailedRegKeys,
			 WORD	cFailedRegVals,
			 WORD	cFailedShortcuts
			 )
{
	return codeINSTALL_EXIT_DONE;
}

codeUNINSTALL_INIT
Uninstall_Init(
			   HWND		hwndParent,
			   LPCTSTR	 pszInstallDir
			   )
{
	if (CloseLeoExtendedNotificationsApp())
	{
		return codeUNINSTALL_INIT_CONTINUE;
	}
	return codeUNINSTALL_INIT_CANCEL;
}

codeUNINSTALL_EXIT
Uninstall_Exit(
			   HWND	hwndParent
			   )
{
	return codeUNINSTALL_EXIT_DONE;
}
