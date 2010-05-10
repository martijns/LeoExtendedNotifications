#include "StdAfx.h"
#include "MessageBoxTimed.h"

MessageBoxTimed::MessageBoxTimed(void)
{
	m_hThread = NULL;
}

MessageBoxTimed::~MessageBoxTimed(void)
{
}

int MessageBoxTimed::Show(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, UINT uTimeoutMs)
{
	if (m_hThread)
		return IDABORT;

	m_lpText = lpText;
	m_lpCaption = lpCaption;
	m_uTimeoutMs = uTimeoutMs;
	m_hThread = CreateThread(NULL, 0, MessageBoxTimed::ThreadStart, this, 0, NULL);
	SetThreadPriority(m_hThread, THREAD_PRIORITY_HIGHEST);
	return ::MessageBox(hWnd, lpText, lpCaption, uType);
}

/* static */ DWORD MessageBoxTimed::ThreadStart(LPVOID param)
{
	MessageBoxTimed *timed = (MessageBoxTimed*)param;
	Sleep(timed->m_uTimeoutMs);
	HWND hWnd = ::FindWindow(NULL, timed->m_lpCaption);
	if (hWnd == NULL)
		goto end;

	EndDialog(hWnd, IDOK);

end:
	timed->m_hThread = NULL;
	return 0;
}