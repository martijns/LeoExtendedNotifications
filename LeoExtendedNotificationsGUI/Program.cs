using System;
using System.Linq;
using System.Collections.Generic;
using System.Windows.Forms;

namespace LeoExtendedNotificationsGUI
{
	static class Program
	{
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[MTAThread]
		static void Main()
		{
			Application.Run(new MainForm());
		}
	}
}