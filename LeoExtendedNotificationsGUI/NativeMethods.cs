using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace LeoExtendedNotificationsGUI
{
    public static class NativeMethods
    {
        private const int ERROR_ALREADY_EXISTS = 183;

        [DllImport("coredll.dll", SetLastError = true)]
        private static extern IntPtr CreateEvent(IntPtr lpEventAttributes, bool bManualReset, bool bInitialState, string lpName);

        [DllImport("coredll.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr hObject);

        public static bool EventAlreadyExists(string eventName)
        {
            IntPtr ptr = CreateEvent(IntPtr.Zero, true, false, eventName);
            if (ptr == IntPtr.Zero)
                return false;

            if (Marshal.GetLastWin32Error() == ERROR_ALREADY_EXISTS)
            {
                CloseHandle(ptr);
                return true;
            }
            CloseHandle(ptr);
            return false;
        }
    }
}
