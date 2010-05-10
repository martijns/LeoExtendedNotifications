using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using StedySoft.SenseSDK;

namespace LeoExtendedNotificationsGUI
{
    public static class SenseListControlExtensions
    {
        private static int _counter = 1;

        public static void AddItem(this SenseListControl control, SenseListControl.ISenseListItem item, bool giveUniqueName)
        {
            if (giveUniqueName)
            {
                item.Name = "UniqueName" + _counter++;
            }
            control.AddItem(item);
        }
    }
}
