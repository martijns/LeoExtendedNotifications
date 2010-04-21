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

void NotificationChanged(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData);
void SetUnattended(bool on);
bool IsKeypadLedControlRunning();
bool ShouldNotify();

void KeypadBlinkStart();
void KeypadBlinkStop();
DWORD KeypadBlinkThreadStart(LPVOID);
HANDLE KeypadBlinkThread = NULL;
HANDLE KeypadBlinkThreadEvent = NULL;

DWORD notifyBySMS = 1;
DWORD notifyByMail = 1;
DWORD notifyByCall = 1;
DWORD notifyByVmail = 1;
DWORD stopInCall = 1;

HREGNOTIFY hSmsNotify;
HREGNOTIFY hEmailNotify;
HREGNOTIFY hCallsNotify;
HREGNOTIFY hVmailNotify;
HREGNOTIFY hInCallStatus;

int _tmain(int argc, _TCHAR* argv[])
{
	RegistryGetDWORD(HKEY_LOCAL_MACHINE, TEXT("Software\\LeoExtendedNotifications"), TEXT("SMS"), &notifyBySMS);
	RegistryGetDWORD(HKEY_LOCAL_MACHINE, TEXT("Software\\LeoExtendedNotifications"), TEXT("MAIL"), &notifyByMail);
	RegistryGetDWORD(HKEY_LOCAL_MACHINE, TEXT("Software\\LeoExtendedNotifications"), TEXT("CALL"), &notifyByCall);
	RegistryGetDWORD(HKEY_LOCAL_MACHINE, TEXT("Software\\LeoExtendedNotifications"), TEXT("VMAIL"), &notifyByVmail);
	RegistryGetDWORD(HKEY_LOCAL_MACHINE, TEXT("Software\\LeoExtendedNotifications"), TEXT("STOP"), &stopInCall);

	// Create an event for ourselves, so that we can be turned off as well
	HANDLE appEvent = CreateEvent(NULL, TRUE, FALSE, L"LedExtendedNotificationsApp");
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

	// Request notifications
	if (stopInCall == 1) {
		::RegistryNotifyCallback(SN_PHONEACTIVECALLCOUNT_ROOT, SN_PHONEACTIVECALLCOUNT_PATH, SN_PHONEACTIVECALLCOUNT_VALUE,
			NotificationChanged, 0, NULL, &hInCallStatus);
	}

//#if LEN_NOTIFY_SMS
	if (notifyBySMS == 1) {
		::RegistryNotifyCallback(SN_MESSAGINGSMSUNREAD_ROOT, SN_MESSAGINGSMSUNREAD_PATH, SN_MESSAGINGSMSUNREAD_VALUE,
			NotificationChanged, 0, NULL, &hSmsNotify);
	}
//#endif
//#if LEN_NOTIFY_CALLS
	if (notifyByCall == 1) {
		::RegistryNotifyCallback(SN_PHONEMISSEDCALLS_ROOT, SN_PHONEMISSEDCALLS_PATH, SN_PHONEMISSEDCALLS_VALUE,
			NotificationChanged, 0, NULL, &hCallsNotify);
	}
//#endif
//#if LEN_NOTIFY_EMAIL
	if (notifyByMail == 1) {
		::RegistryNotifyCallback(SN_MESSAGINGTOTALEMAILUNREAD_ROOT, SN_MESSAGINGTOTALEMAILUNREAD_PATH, SN_MESSAGINGTOTALEMAILUNREAD_VALUE,
			NotificationChanged, 0, NULL, &hEmailNotify);
	}
//#endif

	// Check initial state
	if (ShouldNotify())
	{
		SetUnattended(true);
		KeypadBlinkStart();
	}

#if LEN_MSGBOX
	if (IsKeypadLedControlRunning())
	{
		MessageBox(NULL, L"You have KeypadLedControl running as well. It will work, but you may get some strange blinking behaviour while there are notifications.", L"Warning", MB_OK | MB_TOPMOST);
	}

	MessageBox(NULL, L"LeoExtendedNotifications started", L"Info", MB_OK | MB_TOPMOST);
#endif

	// Wait until signal for exit
	WaitForSingleObject(appEvent, INFINITE);

	// Cleanup
	if (stopInCall == 1) {
		RegistryCloseNotification(hInCallStatus);
	}

//#if LEN_NOTIFY_SMS
	if (notifyBySMS == 1) {
		RegistryCloseNotification(hSmsNotify);
	}
//#endif
//#if LEN_NOTIFY_CALLS
	if (notifyByCall == 1) {
		RegistryCloseNotification(hCallsNotify);
	}
//#endif
//#if LEN_NOTIFY_EMAIL
	if (notifyByMail == 1) {
		RegistryCloseNotification(hEmailNotify);
	}
//#endif

	if (notifyByVmail == 1) {
		RegistryCloseNotification(hVmailNotify);
	}

	SetUnattended(false);
	KeypadBlinkStop();
	CloseHandle(appEvent);
	CloseHandle(KeypadBlinkThreadEvent);

//#if LEN_MSGBOX
	MessageBox(NULL, L"bye bye!", L"LeoExtendedNotifications", MB_OK | MB_TOPMOST);
//#endif

	return 0;
}

bool bUsingUnattendedMode = false;
void SetUnattended(bool on)
{
	if (on && !bUsingUnattendedMode)
	{
		PowerPolicyNotify(PPN_UNATTENDEDMODE, TRUE);
		bUsingUnattendedMode = true;
	}
	else if (!on && bUsingUnattendedMode)
	{
		PowerPolicyNotify(PPN_UNATTENDEDMODE, FALSE);
		bUsingUnattendedMode = false;
	}
}

bool IsKeypadLedControlRunning()
{
	HANDLE testEvent = CreateEvent(NULL, TRUE, FALSE, L"KeypadLedControlApp");
	if (testEvent != NULL)
	{
		CloseHandle(testEvent);
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			return true;
		}
	}
	return false;
}

bool ShouldNotify()
{
	if (stopInCall == 1) {
		DWORD hIsInCall = 0;
		::RegistryGetDWORD(SN_PHONEACTIVECALLCOUNT_ROOT, SN_PHONEACTIVECALLCOUNT_PATH, SN_PHONEACTIVECALLCOUNT_VALUE, &hIsInCall);
		if (hIsInCall > 0) {
			return false;
		}
	}

//#if LEN_NOTIFY_SMS
	if (notifyBySMS == 1) {
		DWORD unreadSms = 0;
		::RegistryGetDWORD(SN_MESSAGINGSMSUNREAD_ROOT, SN_MESSAGINGSMSUNREAD_PATH, SN_MESSAGINGSMSUNREAD_VALUE, &unreadSms);
		if (unreadSms > 0)
			return true;
	}
//#endif
//#if LEN_NOTIFY_CALLS
	if (notifyByCall == 1) {
		DWORD missedCalls = 0;
		::RegistryGetDWORD(SN_PHONEMISSEDCALLS_ROOT, SN_PHONEMISSEDCALLS_PATH, SN_PHONEMISSEDCALLS_VALUE, &missedCalls);
		if (missedCalls > 0)
			return true;
	}
//#endif
//#if LEN_NOTIFY_EMAIL
	if (notifyByMail == 1) {
		DWORD unreadEmail = 0;
		::RegistryGetDWORD(SN_MESSAGINGTOTALEMAILUNREAD_ROOT, SN_MESSAGINGTOTALEMAILUNREAD_PATH, SN_MESSAGINGTOTALEMAILUNREAD_VALUE, &unreadEmail);
		if (unreadEmail > 0)
			return true;
	}
//#endif

	if (notifyByVmail == 1) {
		DWORD vMails = 0;
		::RegistryGetDWORD(SN_MESSAGINGVOICEMAILTOTALUNREAD_ROOT, SN_MESSAGINGVOICEMAILTOTALUNREAD_PATH, SN_MESSAGINGVOICEMAILTOTALUNREAD_VALUE, &vMails);
		if (vMails > 0)
			return true;
	}

	return false;
}

void NotificationChanged(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData)
{
	if (ShouldNotify())
	{
		SetUnattended(true);
		KeypadBlinkStart();
	}
	else
	{
		SetUnattended(false);
		KeypadBlinkStop();
	}
}


#pragma region KeypadBlinkThread

void KeypadBlinkStart()
{
	if (KeypadBlinkThread == NULL)
	{
		ResetEvent(KeypadBlinkThreadEvent);
		KeypadBlinkThread = CreateThread(NULL, 0, KeypadBlinkThreadStart, NULL, 0, NULL);
		SetThreadPriority(KeypadBlinkThread, THREAD_PRIORITY_HIGHEST);
	}
}

void KeypadBlinkStop()
{
	if (KeypadBlinkThread != NULL)
	{
		SetEvent(KeypadBlinkThreadEvent);
		WaitForSingleObject(KeypadBlinkThread, INFINITE);
		KeypadBlinkThread = NULL;
	}
}

DWORD KeypadBlinkThreadStart(LPVOID data)
{
	Keypad keypad;
	if (!keypad.Initialize())
	{
		MessageBox(NULL, L"Cannot initialize Keypad", L"Error", MB_OK | MB_TOPMOST);
		return 1;
	}

	while (true)
	{
		// Blink for a few times
		for (int i=0; i < 7; i++)
		{
			keypad.TurnOff();
			Sleep(75);
			keypad.TurnOn();
			Sleep(75);
		}
		keypad.TurnOff();

		// Keep looping until we're instructed to stop the thread
		if (WaitForSingleObject(KeypadBlinkThreadEvent, 3000) == WAIT_OBJECT_0)
		{
			break;
		}
	}

	return 0;
}

#pragma endregion // KeypadBlinkThread
