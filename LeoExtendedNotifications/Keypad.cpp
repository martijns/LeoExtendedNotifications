#include "stdafx.h"
#include <windows.h>
#include "keypad.h"

Keypad::Keypad()
{
	LedRefreshThreadEvent = CreateEvent(NULL, TRUE, FALSE, L"Keypad\\{6FA329C8-8C74-42a7-93F5-F9D55CF4C54E}");
}

Keypad::~Keypad()
{
}

bool Keypad::Initialize()
{
	// Get the keyped led event (this one is used for Leo at least)
	LedOnEvent = CreateEvent(NULL, TRUE, FALSE, L"IconLedOnEvent");
	if (LedOnEvent == NULL)
	{
		return false;
	}
	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
		// This even didn't exist yet. Let's see if we can try another event (it's IconLedEvent for Rhodium)
		CloseHandle(LedOnEvent);
		LedOnEvent = CreateEvent(NULL, TRUE, FALSE, L"IconLedEvent");
		if (LedOnEvent == NULL)
		{
			return false;
		}
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			// This event didn't exist either.
			CloseHandle(LedOnEvent);
			return false;
		}
	}

	// Get the keypad led event to turn it off
	LedOffEvent = CreateEvent(NULL, TRUE, FALSE, L"IconLedOffEvent");
	if (LedOffEvent == NULL)
	{
		// Oops, error creating the event?
		return false;
	}
	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
		// This event didn't exist yet. This means this device does not use this event. Which means we will just ignore it and hope
		// the keypad lights goes off automatically when it goes into standby.
		CloseHandle(LedOffEvent);
		return false;
	}

	// If we reach here, all went well
	return true;
}

void Keypad::TurnOn()
{
	// Start the thread to keep the keypad light on
	if (!LedRefreshThread)
	{
		ResetEvent(LedRefreshThreadEvent);
		LedRefreshThread = CreateThread(NULL, 0, Keypad::LedRefreshThreadStart, this, 0, NULL);
		SetThreadPriority(LedRefreshThread, THREAD_PRIORITY_HIGHEST);
	}
}

DWORD Keypad::LedRefreshThreadStart(LPVOID data)
{
	Keypad *keypad = (Keypad*)data;
	while (true)
	{
		SetEvent(keypad->LedOnEvent);
		if (WaitForSingleObject(keypad->LedRefreshThreadEvent, 5000) == WAIT_OBJECT_0)
		{
			break;
		}
	}
	return 0;
}

void Keypad::TurnOff()
{
	// Stop the thread if running
	if (LedRefreshThread)
	{
		SetEvent(LedRefreshThreadEvent);
		WaitForSingleObject(LedRefreshThread, INFINITE);
		LedRefreshThread = NULL;
	}

	// Turn off the led
	SetEvent(LedOffEvent);
}
