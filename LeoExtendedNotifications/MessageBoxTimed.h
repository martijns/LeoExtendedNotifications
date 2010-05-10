#pragma once

class MessageBoxTimed
{
public:
	MessageBoxTimed(void);
	~MessageBoxTimed(void);

	// Calling parameters are identical to MessageBoxW(). Note that lpCaption is used
	// to find and terminate the dialog, so it must be unique!
	int Show(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, UINT uTimeoutMs);

private:
	HANDLE m_hThread;
	UINT m_uTimeoutMs;
	LPCWSTR m_lpText;
	LPCWSTR m_lpCaption;
	static DWORD ThreadStart(LPVOID);
};
