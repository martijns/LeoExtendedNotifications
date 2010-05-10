using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using Microsoft.Win32;

namespace LeoExtendedNotificationsGUI
{
	public class Settings : IDisposable
	{
		#region Singleton

		private static Settings _instance;
		public static Settings Instance
		{
			get
			{
				if (_instance == null)
					_instance = new Settings();
				return _instance;
			}
		}

		private Settings()
		{
			_reg = Registry.LocalMachine.CreateSubKey("Software\\LeoExtendedNotifications");
		}

		#endregion

		#region Private fields

		private RegistryKey _reg;

		#endregion

		#region Private helpers

		private bool GetBool(string name, bool defaultValue)
		{
			return (int)_reg.GetValue(name, defaultValue ? 1 : 0) > 0;
		}

		private int GetInt(string name, int defaultValue)
		{
			return (int)_reg.GetValue(name, defaultValue);
		}

        private string GetString(string name, string defaultValue)
        {
            return (string)_reg.GetValue(name, defaultValue);
        }

		private void SetBool(string name, bool newValue)
		{
			_reg.SetValue(name, newValue ? 1 : 0, RegistryValueKind.DWord);
		}

		private void SetInt(string name, int newValue)
		{
			_reg.SetValue(name, newValue, RegistryValueKind.DWord);
		}

        private void SetString(string name, string newValue)
        {
            _reg.SetValue(name, newValue, RegistryValueKind.String);
        }

		#endregion

		#region Public properties

		public bool NotifyOnSms
		{
			get
			{
				return GetBool("SMS", true);
			}
			set
			{
				SetBool("SMS", value);
			}
		}

		public bool NotifyOnEmail
		{
			get
			{
				return GetBool("MAIL", true);
			}
			set
			{
				SetBool("MAIL", value);
			}
		}

		public bool NotifyOnVoicemail
		{
			get
			{
				return GetBool("VMAIL", true);
			}
			set
			{
				SetBool("VMAIL", value);
			}
		}

		public bool NotifyOnMissedCalls
		{
			get
			{
				return GetBool("CALL", true);
			}
			set
			{
				SetBool("CALL", value);
			}
		}

		public bool NotifyOnMms
		{
			get
			{
				return GetBool("MMS", true);
			}
			set
			{
				SetBool("MMS", value);
			}
		}

        public bool NotifyOnReminders
        {
            get
            {
                return GetBool("REMINDER", true);
            }
            set
            {
                SetBool("REMINDER", value);
            }
        }

		public bool DoNotNotifyDuringCall
		{
			get
			{
				return GetBool("STOP", true);
			}
			set
			{
				SetBool("STOP", value);
			}
		}

		public bool StopBlinkingAfterUnlock
		{
			get
			{
				return GetBool("UNLOCK", false);
			}
			set
			{
				SetBool("UNLOCK", value);
			}
		}

		/// <summary>
		/// Stops notifications after X seconds since the last time there were new notifications available. Use 0 for infinite.
		/// </summary>
		public int NotificationTimeoutInSeconds
		{
			get
			{
				return GetInt("DURATION", 300);
			}
			set
			{
				SetInt("DURATION", value);
			}
		}

        public int SleepTimeBetweenSequencesInMilliseconds
        {
            get
            {
                return GetInt("BLINKSLEEP", 3000);
            }
            set
            {
                SetInt("BLINKSLEEP", value);
            }
        }

        public string BlinkingSequence
        {
            get
            {
                return GetString("SEQ", "75-125-400-50-50-50-50-50-50-50");
            }
            set
            {
                SetString("SEQ", value);
            }
        }

		#endregion

		#region IDisposable Members

		public void Dispose()
		{
			if (_reg != null)
			{
				_reg.Close();
				_reg = null;
			}
			_instance = null;
		}

		#endregion
	}
}
