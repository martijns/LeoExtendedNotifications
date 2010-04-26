// LeoExtendedNotifications.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <regext.h>
#include <snapi.h>
#include <pmpolicy.h>
#include "keypad.h"

// Change this according to what you want...
#define LEN_MSGBOX 0
/*
#define LEN_NOTIFY_SMS 1
#define LEN_NOTIFY_EMAIL 1
#define LEN_NOTIFY_CALLS 1
*/

void ZeroNotificationChanged();
void ConfigChanged(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData);
void LockModeChanged(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData);
void NotificationChanged(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData);
void SyncChargeStatusChanged(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData);

void SetUnattended(bool on);
void readConfig();
void createNotifyHooks();
void clearNotifiyHooks();
void createConfigHook();
void clearConfigHook();
bool IsKeypadLedControlRunning();
bool ShouldNotify();
void exitProgram();
bool isSyncincOrCharging();
void updateStartTime();
void updateStopTime();
bool getLockStatus();

void KeypadBlinkStart();
void KeypadBlinkStop();
DWORD KeypadBlinkThreadStart(LPVOID);
HANDLE KeypadBlinkThread = NULL;
HANDLE KeypadBlinkThreadEvent = NULL;

SYSTEMTIME stBlinkStart, stBlinkStop;
FILETIME ftBlinkStart, ftBlinkStop;
LARGE_INTEGER liBlinkStart, liBlinkStop;
const int tDivider = 10000000;
const LPCTSTR appRegPath = TEXT("Software\\LeoExtendedNotifications");

DWORD blinkTimeOut = 300;
DWORD timeOutCount = 1;

DWORD notifyBySMS = 1;
DWORD notifyByMMS = 1;
DWORD notifyByMail = 1;
DWORD notifyByCall = 1;
DWORD notifyByVmail = 1;
DWORD stopInCall = 1;
DWORD clearByUnlock = 1;
DWORD notifications = 0;

HREGNOTIFY hMyNotify;
HREGNOTIFY hSmsNotify;
HREGNOTIFY hMmsNotify;
HREGNOTIFY hEmailNotify;
HREGNOTIFY hCallsNotify;
HREGNOTIFY hVmailNotify;
HREGNOTIFY hInCallStatus;
HREGNOTIFY hkeyLockStatus;
HREGNOTIFY hASyncStatus;
HREGNOTIFY hPowertStatus;

HREGNOTIFY hZeroParam = 0;

HANDLE appEvent = NULL;

bool bUsingUnattendedMode = false;
bool isLocked = false;
//bool dontBlink = true;
bool timeOut = true;

int _tmain(int argc, _TCHAR* argv[]) {
	// Create an event for ourselves, so that we can be turned off as well
	appEvent = CreateEvent(NULL, TRUE, FALSE, L"LedExtendedNotificationsApp");
	if (appEvent == NULL)
	{
		MessageBox(NULL, L"Couldn't create an event for ourselves", L"Error", MB_OK);
		return 1;
	}

	// If the app is already running, we set the event so that it will close itself.
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		SetEvent(appEvent);
		CloseHandle(appEvent);
		return 1;
	}

	// Initialize the keypad blink thread event, so we can stop it
	KeypadBlinkThreadEvent = CreateEvent(NULL, TRUE, FALSE, L"LedExtendedNotificationsApp\\{43D2E54D-461B-4d07-B8DF-2F0947B9FDD8}");
	if (KeypadBlinkThreadEvent == NULL)
	{
		MessageBox(NULL, L"Fatal error: couldn't create KeypadBlinkThreadEvent", L"Error", MB_OK | MB_TOPMOST);
		CloseHandle(appEvent);
		return 1;
	}

	readConfig();
	createNotifyHooks();

	// Check initial state
	if (ShouldNotify())
	{
		SetUnattended(true);
		KeypadBlinkStart();
	}

#if LEN_MSGBOX
	if (IsKeypadLedControlRunning()) {
		MessageBox(NULL, L"You have KeypadLedControl running as well. It will work, but you may get some strange blinking behaviour while there are notifications.", L"Warning", MB_OK | MB_TOPMOST);
	}

	MessageBox(NULL, L"LeoExtendedNotifications started", L"Info", MB_OK | MB_TOPMOST);
#endif

	// Wait until signal for exit
	WaitForSingleObject(appEvent, INFINITE);

	exitProgram();

	return 0;
}

void exitProgram() {
	clearNotifiyHooks();

	SetUnattended(false);
	KeypadBlinkStop();
	CloseHandle(appEvent);
	CloseHandle(KeypadBlinkThreadEvent);

//#if LEN_MSGBOX
	MessageBox(NULL, L"bye bye!", L"LeoExtendedNotifications v0.6", MB_OK | MB_TOPMOST);
//#endif
}

void clearNotifiyHooks() {
	// Cleanup

	RegistryCloseNotification(hMyNotify);
	RegistryCloseNotification(hASyncStatus);
	RegistryCloseNotification(hPowertStatus);

	if (clearByUnlock == 1) {
		RegistryCloseNotification(hkeyLockStatus);
	}

	if (stopInCall == 1) {
		RegistryCloseNotification(hInCallStatus);
	}
	
	if (notifyBySMS == 1) {
		RegistryCloseNotification(hSmsNotify);
	}

	if (notifyByMMS == 1) {
		RegistryCloseNotification(hMmsNotify);
	}

	if (notifyByCall == 1) {
		RegistryCloseNotification(hCallsNotify);
	}

	if (notifyByMail == 1) {
		RegistryCloseNotification(hEmailNotify);
	}

	if (notifyByVmail == 1) {
		RegistryCloseNotification(hVmailNotify);
	}
}

void createNotifyHooks() {
	// Request notifications
	createConfigHook();

	::RegistryNotifyCallback(SN_ACTIVESYNCSTATUS_ROOT, SN_ACTIVESYNCSTATUS_PATH, SN_ACTIVESYNCSTATUS_VALUE,
		SyncChargeStatusChanged, 0, NULL, &hASyncStatus);

	::RegistryNotifyCallback(SN_POWERBATTERYSTATE_ROOT, SN_POWERBATTERYSTATE_PATH, SN_POWERBATTERYSTATE_VALUE,
		SyncChargeStatusChanged, 0, NULL, &hPowertStatus);

	if (clearByUnlock == 1) {
		::RegistryNotifyCallback(SN_LOCK_ROOT, SN_LOCK_PATH, SN_LOCK_VALUE, LockModeChanged, 0, NULL, &hkeyLockStatus);
	}

	DWORD fakeParam = 1;
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("_tOut"),
		NotificationChanged, fakeParam, NULL, &hMyNotify);
	
	fakeParam = 2;
	if (stopInCall == 1) {
		::RegistryNotifyCallback(SN_PHONEACTIVECALLCOUNT_ROOT, SN_PHONEACTIVECALLCOUNT_PATH, SN_PHONEACTIVECALLCOUNT_VALUE,
			NotificationChanged, fakeParam, NULL, &hInCallStatus);
	}

	if (notifyBySMS == 1) {
		::RegistryNotifyCallback(SN_MESSAGINGSMSUNREAD_ROOT, SN_MESSAGINGSMSUNREAD_PATH, SN_MESSAGINGSMSUNREAD_VALUE,
			NotificationChanged, fakeParam, NULL, &hSmsNotify);
	}

	if (notifyByMMS == 1) {
		::RegistryNotifyCallback(SN_MESSAGINGMMSUNREAD_ROOT, SN_MESSAGINGMMSUNREAD_PATH, SN_MESSAGINGMMSUNREAD_VALUE,
			NotificationChanged, fakeParam, NULL, &hMmsNotify);
	}

	if (notifyByCall == 1) {
		::RegistryNotifyCallback(SN_PHONEMISSEDCALLS_ROOT, SN_PHONEMISSEDCALLS_PATH, SN_PHONEMISSEDCALLS_VALUE,
			NotificationChanged, fakeParam, NULL, &hCallsNotify);
	}

	if (notifyByMail == 1) {
		::RegistryNotifyCallback(SN_MESSAGINGTOTALEMAILUNREAD_ROOT, SN_MESSAGINGTOTALEMAILUNREAD_PATH, SN_MESSAGINGTOTALEMAILUNREAD_VALUE,
			NotificationChanged, fakeParam, NULL, &hEmailNotify);
	}
}

void readConfig() {
	::RegistrySetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("_tOut"), 0x00000000);

	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("SMS"), &notifyBySMS);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("MMS"), &notifyByMMS);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("MAIL"), &notifyByMail);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("CALL"), &notifyByCall);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("VMAIL"), &notifyByVmail);

	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("STOP"), &stopInCall);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("UNLOCK"), &clearByUnlock);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("DURATION"), &blinkTimeOut);

	isLocked = getLockStatus();
}

void ConfigChanged(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData) {
	readConfig();
	ZeroNotificationChanged();

	MessageBox(NULL, L"New settings applied!", L"LeoExtendedNotifications v0.6", MB_OK | MB_TOPMOST);
}

void createConfigHook() {
	DWORD fakeParam = 0;

	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("SMS"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("MMS"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("MAIL"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("CALL"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("VMAIL"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("STOP"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("UNLOCK"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("DURATION"), ConfigChanged, fakeParam, NULL, &hZeroParam);
}

void clearConfigHook() {
	RegistryCloseNotification(hZeroParam);
}

void SetUnattended(bool on) {
	if (on && !bUsingUnattendedMode) {
		PowerPolicyNotify(PPN_UNATTENDEDMODE, TRUE);
		bUsingUnattendedMode = true;

	} else if (!on && bUsingUnattendedMode) {
		PowerPolicyNotify(PPN_UNATTENDEDMODE, FALSE);
		bUsingUnattendedMode = false;
	}
}

bool IsKeypadLedControlRunning() {
	HANDLE testEvent = CreateEvent(NULL, TRUE, FALSE, L"KeypadLedControlApp");
	if (testEvent != NULL) {
		CloseHandle(testEvent);
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			return true;
		}
	}

	return false;
}

bool ShouldNotify() {
	notifications = 0;

	if (stopInCall == 1) {
		DWORD hIsInCall = 0;
		::RegistryGetDWORD(SN_PHONEACTIVECALLCOUNT_ROOT, SN_PHONEACTIVECALLCOUNT_PATH, SN_PHONEACTIVECALLCOUNT_VALUE, &hIsInCall);
		if (hIsInCall > 0) {
			return false;
		}
	}

	// isSyncincOrCharging returns true if syncing or charging == (battery < 100%)
	//  && !isSyncincOrCharging()
	if (clearByUnlock == 1 && !isLocked) {
		return false;
	}

	if (notifyBySMS == 1) {
		DWORD unreadSms = 0;
		::RegistryGetDWORD(SN_MESSAGINGSMSUNREAD_ROOT, SN_MESSAGINGSMSUNREAD_PATH, SN_MESSAGINGSMSUNREAD_VALUE, &unreadSms);
		notifications += unreadSms;
	}

	if (notifyByMMS == 1) {
		DWORD unreadMms = 0;
		::RegistryGetDWORD(SN_MESSAGINGMMSUNREAD_ROOT, SN_MESSAGINGMMSUNREAD_PATH, SN_MESSAGINGMMSUNREAD_VALUE, &unreadMms);
		notifications += unreadMms;
	}

	if (notifyByCall == 1) {
		DWORD missedCalls = 0;
		::RegistryGetDWORD(SN_PHONEMISSEDCALLS_ROOT, SN_PHONEMISSEDCALLS_PATH, SN_PHONEMISSEDCALLS_VALUE, &missedCalls);
			notifications += missedCalls;
	}

	if (notifyByMail == 1) {
		DWORD unreadEmail = 0;
		::RegistryGetDWORD(SN_MESSAGINGTOTALEMAILUNREAD_ROOT, SN_MESSAGINGTOTALEMAILUNREAD_PATH, SN_MESSAGINGTOTALEMAILUNREAD_VALUE, &unreadEmail);
		notifications += unreadEmail;
	}

	if (notifyByVmail == 1) {
		DWORD vMails = 0;
		::RegistryGetDWORD(SN_MESSAGINGVOICEMAILTOTALUNREAD_ROOT, SN_MESSAGINGVOICEMAILTOTALUNREAD_PATH, SN_MESSAGINGVOICEMAILTOTALUNREAD_VALUE, &vMails);
		notifications += vMails;
	}

	if (blinkTimeOut > 0) {
		DWORD htOut = 0;
		::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("_tOut"), &htOut);
		if (htOut > notifications) {
			return false;
		}
	}

	return (notifications > 0);
}

bool timedOut() {
	if (blinkTimeOut == 0) {
		return false;
	}

	updateStopTime();
	if(liBlinkStop.QuadPart == NULL || liBlinkStart.QuadPart == NULL) {
		return false;
	}

	DWORD diff = (liBlinkStop.QuadPart - liBlinkStart.QuadPart) / tDivider;
	if (diff >= blinkTimeOut) {
		return true;
	}

	return false;
}

bool isSyncincOrCharging() {
	DWORD hIsSyncing = 0;
	::RegistryGetDWORD(SN_ACTIVESYNCSTATUS_ROOT, SN_ACTIVESYNCSTATUS_PATH, SN_ACTIVESYNCSTATUS_VALUE, &hIsSyncing);

	DWORD hIsCharging = 0;
	::RegistryGetDWORD(SN_POWERBATTERYSTATE_ROOT, SN_POWERBATTERYSTATE_PATH, SN_POWERBATTERYSTATE_VALUE, &hIsCharging);
	hIsCharging = SN_POWERBATTERYSTATE_BITMASK & hIsCharging;

/*
	DWORD hBattPerc = 0;
	::RegistryGetDWORD(SN_POWERBATTERYSTRENGTH_ROOT, SN_POWERBATTERYSTRENGTH_PATH, SN_POWERBATTERYSTRENGTH_VALUE, &hBattPerc);
	hBattPerc = SN_POWERBATTERYSTRENGTH_BITMASK & hBattPerc;
 || hBattPerc  >= 5308000
*/

	return (hIsSyncing == 1 || hIsCharging == SN_POWERBATTERYSTATE_FLAG_CHARGING);
}

void SyncChargeStatusChanged(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData) {
	ZeroNotificationChanged();
}

void LockModeChanged(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData) {
	if (clearByUnlock == 0) {
		return;
	}

	/*
	DWORD lockStatus = 0;
	::RegistryGetDWORD(SN_LOCK_ROOT, SN_LOCK_PATH, SN_LOCK_VALUE, &lockStatus);
	if (isLocked && lockStatus == 0) {
		isLocked = false;
		//dontBlink = !(isLocked = false);
		ZeroNotificationChanged();

	} else if (lockStatus == SN_LOCK_BITMASK_KEYLOCKED) {
		isLocked = true;
		//dontBlink = !(isLocked = true);
		ZeroNotificationChanged();
	}
	*/

	isLocked = getLockStatus();
	ZeroNotificationChanged();
}

bool getLockStatus() {
	DWORD lockStatus = 0;
	::RegistryGetDWORD(SN_LOCK_ROOT, SN_LOCK_PATH, SN_LOCK_VALUE, &lockStatus);

	return (lockStatus == SN_LOCK_BITMASK_KEYLOCKED);
}

void ZeroNotificationChanged() {
	NotificationChanged(NULL, NULL, NULL, NULL);
}

void NotificationChanged(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData) {
	if (dwUserData != NULL && blinkTimeOut > 0) {
		updateStartTime();
	}
	
	if (ShouldNotify()) {
		SetUnattended(true);
		KeypadBlinkStart();

	} else {
		SetUnattended(false);
		KeypadBlinkStop();
	}
}

void updateStartTime() {
	timeOut = false;

	GetSystemTime(&stBlinkStart);
	SystemTimeToFileTime(&stBlinkStart, &ftBlinkStart);
	liBlinkStart.LowPart = ftBlinkStart.dwLowDateTime;
	liBlinkStart.HighPart = ftBlinkStart.dwHighDateTime;
}

void updateStopTime() {
	GetSystemTime(&stBlinkStop);
	SystemTimeToFileTime(&stBlinkStop, &ftBlinkStop);
	liBlinkStop.LowPart = ftBlinkStop.dwLowDateTime;
	liBlinkStop.HighPart = ftBlinkStop.dwHighDateTime;
}

#pragma region KeypadBlinkThread

void KeypadBlinkStart() {
	if (KeypadBlinkThread == NULL) {
		ResetEvent(KeypadBlinkThreadEvent);
		KeypadBlinkThread = CreateThread(NULL, 0, KeypadBlinkThreadStart, NULL, 0, NULL);
		SetThreadPriority(KeypadBlinkThread, THREAD_PRIORITY_HIGHEST);
	}
}

void KeypadBlinkStop() {
	if (KeypadBlinkThread != NULL) {
		SetEvent(KeypadBlinkThreadEvent);
		WaitForSingleObject(KeypadBlinkThread, INFINITE);
		KeypadBlinkThread = NULL;
	}
}

DWORD KeypadBlinkThreadStart(LPVOID data) {
	Keypad keypad;
	if (!keypad.Initialize()) {
		MessageBox(NULL, L"Cannot initialize Keypad", L"Error", MB_OK | MB_TOPMOST);
		return 1;
	}

	while (true) {
		// Blink for a few times
		for (int i=0; i < 7; i++) {
			keypad.TurnOff();
			Sleep(75);
			keypad.TurnOn();
			Sleep(75);
		}
		keypad.TurnOff();

		if (timedOut()) {
			::RegistrySetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("_tOut"), notifications+1);
		}

		// Keep looping until we're instructed to stop the thread
		if (WaitForSingleObject(KeypadBlinkThreadEvent, 3000) == WAIT_OBJECT_0) {
			break;
		}
	}

	return 0;
}

#pragma endregion // KeypadBlinkThread