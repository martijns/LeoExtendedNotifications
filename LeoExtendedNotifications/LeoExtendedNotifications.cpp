// LeoExtendedNotifications.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <regext.h>
#include <snapi.h>
#include <pmpolicy.h>
#include "keypad.h"
#include "MessageBoxTimed.h"

#define VERSION L"LeoExtendedNotifications v0.92"

// Change this according to what you want...
#define LEN_MSGBOX 0

// own NotifyEntryPoints
#define SN_REMINDER_ROOT HKEY_LOCAL_MACHINE
#define SN_REMINDER_PATH TEXT("System\\State\\Reminder\\Count")
#define SN_REMINDER_VALUE TEXT("APPT")

// Application Settings
const LPCTSTR appRegPath = TEXT("Software\\LeoExtendedNotifications");
#if DEBUG
const LPCTSTR debugRegPath = TEXT("Software\\LeoExtendedNotifications\\debug");
#endif

// blinkTypes
#define SEQ_TYPES 7
const int SEQ_UNSET = -1;
const int SEQ_MASTER = 0;
const int SEQ_SMS = 1;
const int SEQ_MMS = 2;
const int SEQ_CALL = 3;
const int SEQ_MAIL = 4;
const int SEQ_VMAIL = 5;
const int SEQ_REMINDER = 6;
const TCHAR STR_SEQ_KEYS[SEQ_TYPES][10] = {
	L"SEQ",
	L"SEQsms",
	L"SEQmms",
	L"SEQcall",
	L"SEQmail",
	L"SEQvmail",
	L"SEQrem"
};
const int SEQSIZE = 100;

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
void blinkFunky(Keypad keypad);
void tokenizeBlinkSequence();
void setBlinkType(int i);
void readSeq(int type);

DWORD getSMSCount();
DWORD getMMSCount();
DWORD getMissedCalls();
DWORD getMailCount();
DWORD getVMailCount();
DWORD getReminderCount();

void KeypadBlinkStart();
void KeypadBlinkStop();
DWORD KeypadBlinkThreadStart(LPVOID);
HANDLE KeypadBlinkThread = NULL;
HANDLE KeypadBlinkThreadEvent = NULL;

SYSTEMTIME stBlinkStart, stBlinkStop;
FILETIME ftBlinkStart, ftBlinkStop;
LARGE_INTEGER liBlinkStart, liBlinkStop;
const int tDivider = 10000000;

DWORD blinkTimeOut = NULL;
DWORD timeOutCount = 1;

DWORD notifyBySMS = 1;
DWORD notifyByMMS = 1;
DWORD notifyByMail = 1;
DWORD notifyByCall = 1;
DWORD notifyByVmail = 1;
DWORD notifyByReminder = 1;
DWORD stopInCall = 1;
DWORD clearByUnlock = 0;
DWORD notifications = 0;
DWORD sleepBetweenBlinks = 3000;

int blinkType = SEQ_UNSET;
int blinkSize[SEQ_TYPES] = { 0 };
DWORD blinks[SEQ_TYPES][20] = { 0 };
TCHAR strSeq[SEQ_TYPES][SEQSIZE] = { NULL };

DWORD fakeParam_tOut = 1;
DWORD fakeParam_notify = 2;
HREGNOTIFY hMyNotify;
HREGNOTIFY hSmsNotify;
HREGNOTIFY hMmsNotify;
HREGNOTIFY hEmailNotify;
HREGNOTIFY hCallsNotify;
HREGNOTIFY hVmailNotify;
HREGNOTIFY hReminderNotify;
HREGNOTIFY hInCallStatus;
HREGNOTIFY hkeyLockStatus;
HREGNOTIFY hASyncStatus;
HREGNOTIFY hPowertStatus;

HREGNOTIFY hZeroParam = 0;

HANDLE appEvent = NULL;

bool bUsingUnattendedMode = false;
bool isLocked = false;
//bool timeOut = false;

int _tmain(int argc, _TCHAR* argv[]) {
	// Create an event for ourselves, so that we can be turned off as well
	appEvent = CreateEvent(NULL, TRUE, FALSE, L"LedExtendedNotificationsApp");
	if (appEvent == NULL) {
		MessageBox(NULL, L"Couldn't create an event for ourselves", VERSION, MB_OK | MB_TOPMOST);
		return 1;
	}

	// If the app is already running, we set the event so that it will close itself.
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		SetEvent(appEvent);
		CloseHandle(appEvent);
		return 1;
	}

	// Initialize the keypad blink thread event, so we can stop it
	KeypadBlinkThreadEvent = CreateEvent(NULL, TRUE, FALSE, L"LedExtendedNotificationsApp\\{43D2E54D-461B-4d07-B8DF-2F0947B9FDD8}");
	if (KeypadBlinkThreadEvent == NULL) {
		MessageBox(NULL, L"Fatal error: couldn't create KeypadBlinkThreadEvent", VERSION, MB_OK | MB_TOPMOST);
		CloseHandle(appEvent);
		return 1;
	}

	readConfig();
	createNotifyHooks();

	// Check initial state
	if (ShouldNotify()) {
		SetUnattended(true);
		KeypadBlinkStart();
	}

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

	MessageBoxTimed msgBox;
	msgBox.Show(NULL, L"Closing!", VERSION, MB_OK | MB_TOPMOST, 1000);
}

void clearNotifiyHooks() {
	// Cleanup

	RegistryCloseNotification(hMyNotify);
	RegistryCloseNotification(hASyncStatus);
	RegistryCloseNotification(hPowertStatus);

	if (clearByUnlock == 1)    { RegistryCloseNotification(hkeyLockStatus); }
	if (stopInCall == 1)       { RegistryCloseNotification(hInCallStatus); }
	if (notifyBySMS == 1)      { RegistryCloseNotification(hSmsNotify); }
	if (notifyByMMS == 1)      { RegistryCloseNotification(hMmsNotify); }
	if (notifyByCall == 1)     { RegistryCloseNotification(hCallsNotify); }
	if (notifyByMail == 1)     { RegistryCloseNotification(hEmailNotify); }
	if (notifyByVmail == 1)    { RegistryCloseNotification(hVmailNotify); }
	if (notifyByReminder == 1) { RegistryCloseNotification(hReminderNotify); }
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

	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("_tOut"), NotificationChanged, fakeParam_tOut, NULL, &hMyNotify);

	if (stopInCall == 1) {
		::RegistryNotifyCallback(SN_PHONEACTIVECALLCOUNT_ROOT, SN_PHONEACTIVECALLCOUNT_PATH, SN_PHONEACTIVECALLCOUNT_VALUE,
			NotificationChanged, fakeParam_notify, NULL, &hInCallStatus);
	}

	if (notifyBySMS == 1) {
		::RegistryNotifyCallback(SN_MESSAGINGSMSUNREAD_ROOT, SN_MESSAGINGSMSUNREAD_PATH, SN_MESSAGINGSMSUNREAD_VALUE,
			NotificationChanged, fakeParam_notify, NULL, &hSmsNotify);
	}

	if (notifyByMMS == 1) {
		::RegistryNotifyCallback(SN_MESSAGINGMMSUNREAD_ROOT, SN_MESSAGINGMMSUNREAD_PATH, SN_MESSAGINGMMSUNREAD_VALUE,
			NotificationChanged, fakeParam_notify, NULL, &hMmsNotify);
	}

	if (notifyByCall == 1) {
		::RegistryNotifyCallback(SN_PHONEMISSEDCALLS_ROOT, SN_PHONEMISSEDCALLS_PATH, SN_PHONEMISSEDCALLS_VALUE,
			NotificationChanged, fakeParam_notify, NULL, &hCallsNotify);
	}

	if (notifyByMail == 1) {
		::RegistryNotifyCallback(SN_MESSAGINGTOTALEMAILUNREAD_ROOT, SN_MESSAGINGTOTALEMAILUNREAD_PATH, SN_MESSAGINGTOTALEMAILUNREAD_VALUE,
			NotificationChanged, fakeParam_notify, NULL, &hEmailNotify);
	}

	if (notifyByReminder == 1) {
		::RegistryNotifyCallback(SN_REMINDER_ROOT, SN_REMINDER_PATH, SN_REMINDER_VALUE,
			NotificationChanged, fakeParam_notify, NULL, &hReminderNotify);
	}
}

void readConfig() {
	::RegistrySetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("_tOut"), 0x00000000);

	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("SMS"), &notifyBySMS);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("MMS"), &notifyByMMS);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("MAIL"), &notifyByMail);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("CALL"), &notifyByCall);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("VMAIL"), &notifyByVmail);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("REMINDER"), &notifyByReminder);

	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("STOP"), &stopInCall);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("UNLOCK"), &clearByUnlock);
	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("DURATION"), &blinkTimeOut);

	::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("BLINKSLEEP"), &sleepBetweenBlinks);

	readSeq(SEQ_MASTER);
	readSeq(SEQ_SMS);
	readSeq(SEQ_MMS);
	readSeq(SEQ_CALL);
	readSeq(SEQ_MAIL);
	readSeq(SEQ_VMAIL);
	readSeq(SEQ_REMINDER);
	tokenizeBlinkSequence();

	isLocked = getLockStatus();
}

void readSeq(int type) {
	HRESULT hRes = ::RegistryGetString(HKEY_LOCAL_MACHINE, appRegPath, STR_SEQ_KEYS[type], (LPTSTR) &strSeq[type], SEQSIZE);
	if (hRes != S_OK) { blinkSize[type] = SEQ_UNSET; }

#if DEBUG
	::RegistrySetDWORD(HKEY_LOCAL_MACHINE, debugRegPath, STR_SEQ_KEYS[type], blinkSize[type]);
#endif
}

void tokenizeBlinkSequence() {
	if (strSeq == NULL) {
		return;
	}

	wchar_t *delims = L"-";
	for (int type = 0; type < SEQ_TYPES; type++) {
		if (strSeq[type] == NULL || blinkSize[type] < 0) {
			continue;
		}

		wchar_t *token = NULL;
		DWORD dTok = 0;
		int i = 0, j = 0, size = 0;
		do {
			token = wcstok(i == 0 ? strSeq[type] : NULL, delims);
			if (token != NULL) {
				size += sizeof(token)+1;
				if (size >= SEQSIZE) {
					break;
				}

				dTok = _wtol(token);
				if (dTok == 0) { continue; }

				blinks[type][i++] = dTok;
			}
		} while (token != NULL && i < 20 && j++ < 40);

		if (i > 0) {
			blinkSize[type] = i;
#if DEBUG
			::RegistrySetDWORD(HKEY_LOCAL_MACHINE, debugRegPath, STR_SEQ_KEYS[type], blinkSize[type]);
#endif
		}
	}
}

void ConfigChanged(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData) {
	readConfig();
	ZeroNotificationChanged();

	MessageBoxTimed msgbox;
	msgbox.Show(NULL, L"New settings applied!", VERSION, MB_OK | MB_TOPMOST, 500);
}

void createConfigHook() {
	DWORD fakeParam = 0;

	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("SMS"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("MMS"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("MAIL"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("CALL"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("VMAIL"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("REMINDER"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("STOP"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("UNLOCK"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("DURATION"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("BLINKSLEEP"), ConfigChanged, fakeParam, NULL, &hZeroParam);
	//::RegistryNotifyCallback(HKEY_LOCAL_MACHINE, appRegPath, TEXT("SEQ"), ConfigChanged, fakeParam, NULL, &hZeroParam);
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
	setBlinkType(SEQ_UNSET);

	notifications = 0;
	notifications += getSMSCount();
	notifications += getMMSCount();
	notifications += getMissedCalls();
	notifications += getMailCount();
	notifications += getVMailCount();
	notifications += getReminderCount();

#if DEBUG
		::RegistrySetDWORD(HKEY_LOCAL_MACHINE, debugRegPath, TEXT("_notifications"), notifications);
#endif
	if (blinkTimeOut != NULL && blinkTimeOut > 0) {
		DWORD htOut = 0;
		::RegistryGetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("_tOut"), &htOut);
		if (notifications == 0) {
			::RegistrySetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("_tOut"), 0x00000000);
		} else if (htOut == notifications) {
			return false;
		}
	}

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

	return (notifications > 0);
}

void setBlinkType(int i) {
	blinkType = (blinkType == SEQ_UNSET || i == SEQ_UNSET ? i : SEQ_MASTER);
}

DWORD getSMSCount() {
	if (notifyBySMS == 1) {
		DWORD returnVal = 0;
		::RegistryGetDWORD(SN_MESSAGINGSMSUNREAD_ROOT, SN_MESSAGINGSMSUNREAD_PATH, SN_MESSAGINGSMSUNREAD_VALUE, &returnVal);
		if (returnVal > 0 ) { setBlinkType(SEQ_SMS); }

		return returnVal;
	}

	return 0;
}

DWORD getMMSCount() {
	if (notifyByMMS == 1) {
		DWORD returnVal = 0;
		::RegistryGetDWORD(SN_MESSAGINGMMSUNREAD_ROOT, SN_MESSAGINGMMSUNREAD_PATH, SN_MESSAGINGMMSUNREAD_VALUE, &returnVal);
		if (returnVal > 0 ) { setBlinkType(SEQ_MMS); }

		return returnVal;
	}

	return 0;
}

DWORD getMissedCalls() {
	if (notifyByCall == 1) {
		DWORD returnVal = 0;
		::RegistryGetDWORD(SN_PHONEMISSEDCALLS_ROOT, SN_PHONEMISSEDCALLS_PATH, SN_PHONEMISSEDCALLS_VALUE, &returnVal);
		if (returnVal > 0 ) { setBlinkType(SEQ_CALL); }

		return returnVal;
	}

	return 0;
}

DWORD getMailCount() {
	if (notifyByMail == 1) {
		DWORD returnVal = 0;
		::RegistryGetDWORD(SN_MESSAGINGTOTALEMAILUNREAD_ROOT, SN_MESSAGINGTOTALEMAILUNREAD_PATH, SN_MESSAGINGTOTALEMAILUNREAD_VALUE, &returnVal);
		if (returnVal > 0 ) { setBlinkType(SEQ_MAIL); }

		return returnVal;
	}

	return 0;
}

DWORD getVMailCount() {
	if (notifyByVmail == 1) {
		DWORD returnVal = 0;
		::RegistryGetDWORD(SN_MESSAGINGVOICEMAILTOTALUNREAD_ROOT, SN_MESSAGINGVOICEMAILTOTALUNREAD_PATH, SN_MESSAGINGVOICEMAILTOTALUNREAD_VALUE, &returnVal);
		if (returnVal > 0 ) { setBlinkType(SEQ_VMAIL); }

		return returnVal;
	}

	return 0;
}

DWORD getReminderCount() {
	if (notifyByReminder == 1) {
		DWORD returnVal = 0;
		::RegistryGetDWORD(SN_REMINDER_ROOT, SN_REMINDER_PATH, SN_REMINDER_VALUE, &returnVal);
		if (returnVal > 0 ) { setBlinkType(SEQ_REMINDER); }

		return returnVal;
	}

	return 0;
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
	if (dwUserData != NULL && blinkTimeOut != NULL && blinkTimeOut > 0) {
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

bool timedOut() {
	if (blinkTimeOut == NULL || blinkTimeOut == 0) {
		return false;
	}

	updateStopTime();
	if (&liBlinkStart == NULL ||
	   &liBlinkStop == NULL ||
	   liBlinkStop.QuadPart == NULL ||
	   liBlinkStart.QuadPart == NULL) {
		return false;
	}

	DWORD diff = (DWORD) (liBlinkStop.QuadPart - liBlinkStart.QuadPart) / tDivider;
	if (diff >= blinkTimeOut) {
		return true;
	}

	return false;
}

void updateStartTime() {
//	timeOut = false;

	GetSystemTime(&stBlinkStart);
	if (&stBlinkStart == NULL) {
		return;
	}

	SystemTimeToFileTime(&stBlinkStart, &ftBlinkStart);
	if (&ftBlinkStart == NULL) {
		return;
	}

	liBlinkStart.LowPart = ftBlinkStart.dwLowDateTime;
	liBlinkStart.HighPart = ftBlinkStart.dwHighDateTime;
}

void updateStopTime() {
	GetSystemTime(&stBlinkStop);
	if (&stBlinkStop == NULL) {
//		timeOut = false;
		return;
	}

	SystemTimeToFileTime(&stBlinkStop, &ftBlinkStop);
	if (&ftBlinkStop == NULL) {
		return;
	}

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
		MessageBox(NULL, L"Cannot initialize Keypad", VERSION, MB_OK | MB_TOPMOST);
		return 1;
	}

	while (true) {
		// Blink for a few times
		blinkFunky(keypad);

		if (timedOut()) {
			::RegistrySetDWORD(HKEY_LOCAL_MACHINE, appRegPath, TEXT("_tOut"), notifications);
		}

		// Keep looping until we're instructed to stop the thread
		if (WaitForSingleObject(KeypadBlinkThreadEvent, sleepBetweenBlinks) == WAIT_OBJECT_0) {
			break;
		}
	}

	return 0;
}

void blinkFunky(Keypad keypad) {
	int type = (blinkSize[blinkType] != SEQ_UNSET ? blinkType : SEQ_MASTER);
	if (type != SEQ_UNSET) {
		for (int i = 0; i < blinkSize[type]; i++) {
			int slTime = blinks[type][i];
			if (slTime == 0) {
				continue;
			}

			if (i % 2 == 0) {
				keypad.TurnOff();
			} else {
				keypad.TurnOn();
			}

			Sleep(slTime);
		}

	} else {
		for (int i = 0; i < 7; i++) {
			keypad.TurnOff();
			Sleep(75);
			keypad.TurnOn();
			Sleep(75);
		}
	}

	keypad.TurnOff();
}

#pragma endregion // KeypadBlinkThread