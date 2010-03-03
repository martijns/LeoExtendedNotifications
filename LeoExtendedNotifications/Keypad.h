#pragma once

#include "stdafx.h"

class Keypad
{
public:
	Keypad();
	~Keypad();
	bool Initialize();
	void TurnOn();
	void TurnOff();
private:
	HANDLE LedOnEvent;
	HANDLE LedOffEvent;

	static DWORD LedRefreshThreadStart(LPVOID);
	HANDLE LedRefreshThread;
	HANDLE LedRefreshThreadEvent;
};
