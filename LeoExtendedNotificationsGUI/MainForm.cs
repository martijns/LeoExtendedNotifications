using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using StedySoft.SenseSDK;
using StedySoft.SenseSDK.DrawingCE;
using System.Text.RegularExpressions;

namespace LeoExtendedNotificationsGUI
{
	public partial class MainForm : Form
    {
        #region Private fields

        private SenseListControl list;

        private SensePanelColoredLabelItem itemStatus;

        private SensePanelMoreItem itemDuration;
        private SensePanelTextboxItem txtDuration;
        
        private SensePanelMoreItem itemSequence;
        private SensePanelTextboxItem txtSequence;

        private SensePanelMoreItem itemBlinkSleep;
        private SensePanelTextboxItem txtBlinkSleep;

        #endregion

        public MainForm()
		{
			InitializeComponent();

            SuspendLayout();
            InitializeSenseComponents();
            ResumeLayout();
		}

        private void InitializeSenseComponents()
        {
            SenseHeaderControl header = new SenseHeaderControl();
            header.Dock = DockStyle.Top;
            header.Text = "LEN Configurator";

            list = new SenseListControl();
            list.Dock = DockStyle.Fill;

            SensePanelDividerItem divStatus = new SensePanelDividerItem();
            divStatus.Text = "Status";

            itemStatus = new SensePanelColoredLabelItem();
            HandleTimerAppRunningTick(this, EventArgs.Empty);

            SensePanelDividerItem divNotify = new SensePanelDividerItem();
            divNotify.Text = "Notify on...";

            SensePanelOnOffItem itemCalls = new SensePanelOnOffItem();
            itemCalls.PrimaryText = "Missed calls";
            itemCalls.Status = ToItemStatus(Settings.Instance.NotifyOnMissedCalls);
            itemCalls.OnStatusChanged += new SensePanelOnOffItem.StatusEventHandler(HandleItemCallsStatusChanged);

            SensePanelOnOffItem itemSms = new SensePanelOnOffItem();
            itemSms.PrimaryText = "Unread SMS";
            itemSms.Status = ToItemStatus(Settings.Instance.NotifyOnSms);
            itemSms.OnStatusChanged += new SensePanelOnOffItem.StatusEventHandler(HandleItemSmsStatusChanged);

            SensePanelOnOffItem itemMms = new SensePanelOnOffItem();
            itemMms.PrimaryText = "Unread MMS";
            itemMms.Status = ToItemStatus(Settings.Instance.NotifyOnMms);
            itemMms.OnStatusChanged += new SensePanelOnOffItem.StatusEventHandler(HandleItemMmsStatusChanged);

            SensePanelOnOffItem itemEmail = new SensePanelOnOffItem();
            itemEmail.PrimaryText = "Unread e-mail";
            itemEmail.Status = ToItemStatus(Settings.Instance.NotifyOnEmail);
            itemEmail.OnStatusChanged += new SensePanelOnOffItem.StatusEventHandler(HandleItemEmailStatusChanged);

            SensePanelOnOffItem itemVmail = new SensePanelOnOffItem();
            itemVmail.PrimaryText = "Voicemail messages";
            itemVmail.Status = ToItemStatus(Settings.Instance.NotifyOnVoicemail);
            itemVmail.OnStatusChanged += new SensePanelOnOffItem.StatusEventHandler(HandleItemVmailStatusChanged);

            SensePanelOnOffItem itemReminder = new SensePanelOnOffItem();
            itemReminder.PrimaryText = "Reminders";
            itemReminder.Status = ToItemStatus(Settings.Instance.NotifyOnReminders);
            itemReminder.OnStatusChanged += new SensePanelOnOffItem.StatusEventHandler(HandleItemReminderStatusChanged);

            SensePanelDividerItem divBehaviour = new SensePanelDividerItem();
            divBehaviour.Text = "Behaviour options";

            SensePanelOnOffItem itemStopDuringCall = new SensePanelOnOffItem();
            itemStopDuringCall.PrimaryText = "Stop blinking during call";
            itemStopDuringCall.SecondaryText = "Continues after call finished";
            itemStopDuringCall.Status = ToItemStatus(Settings.Instance.DoNotNotifyDuringCall);
            itemStopDuringCall.OnStatusChanged += new SensePanelOnOffItem.StatusEventHandler(HandleItemStopDuringCallStatusChanged);

            SensePanelOnOffItem itemStopAfterUnlock = new SensePanelOnOffItem();
            itemStopAfterUnlock.PrimaryText = "Stop blinking after unlock";
            itemStopAfterUnlock.SecondaryText = "Until new notifications are available";
            itemStopAfterUnlock.Status = ToItemStatus(Settings.Instance.StopBlinkingAfterUnlock);
            itemStopAfterUnlock.OnStatusChanged += new SensePanelOnOffItem.StatusEventHandler(HandleItemStopAfterUnlockStatusChanged);

            // Duration
            int curDuration = Settings.Instance.NotificationTimeoutInSeconds;
            itemDuration = new SensePanelMoreItem();
            itemDuration.PrimaryText = "Stop blinking after X seconds";
            itemDuration.SecondaryText = "Currently " + (curDuration > 0 ? "set to " + curDuration + " seconds" : "disabled");

            txtDuration = new SensePanelTextboxItem();
            txtDuration.LabelText = "Set a new time? (0 = forever)";
            txtDuration.Text = curDuration + "";
            txtDuration.GotFocus += new EventHandler(HandleTextboxGotFocus);
            txtDuration.LostFocus += new EventHandler(HandleTextboxLostFocus);
            itemDuration.Children.Add(txtDuration);

            SensePanelButtonItem btnDuration = new SensePanelButtonItem();
            btnDuration.Text = "Save";
            btnDuration.OnClick += new SensePanelButtonItem.ClickEventHandler(HandleDurationClick);
            itemDuration.Children.Add(btnDuration);

            // Blinking sequence
            string curSequence = Settings.Instance.BlinkingSequence;
            itemSequence = new SensePanelMoreItem();
            itemSequence.PrimaryText = "Set the blinking sequence";
            itemSequence.SecondaryText = "Currently: " + curSequence;

            txtSequence = new SensePanelTextboxItem();
            txtSequence.LabelText = "Set new sequence (off-on-off-on-etc...)";
            txtSequence.Text = curSequence;
            txtSequence.GotFocus += new EventHandler(HandleTextboxGotFocus);
            txtSequence.LostFocus += new EventHandler(HandleTextboxLostFocus);
            itemSequence.Children.Add(txtSequence);

            SensePanelButtonItem btnSequence = new SensePanelButtonItem();
            btnSequence.Text = "Save";
            btnSequence.OnClick += new SensePanelButtonItem.ClickEventHandler(HandleSequenceClick);
            itemSequence.Children.Add(btnSequence);

            // Blinking sleep
            int curBlinkSleep = Settings.Instance.SleepTimeBetweenSequencesInMilliseconds;
            itemBlinkSleep = new SensePanelMoreItem();
            itemBlinkSleep.PrimaryText = "Time between blinking sequences";
            itemBlinkSleep.SecondaryText = "Currently set to " + curBlinkSleep + " ms";

            txtBlinkSleep = new SensePanelTextboxItem();
            txtBlinkSleep.LabelText = "Set new time between blinking sequences (in ms)";
            txtBlinkSleep.Text = curBlinkSleep + "";
            txtBlinkSleep.GotFocus += new EventHandler(HandleTextboxGotFocus);
            txtBlinkSleep.LostFocus += new EventHandler(HandleTextboxLostFocus);
            itemBlinkSleep.Children.Add(txtBlinkSleep);

            SensePanelButtonItem btnBlinkSleep = new SensePanelButtonItem();
            btnBlinkSleep.Text = "Save";
            btnBlinkSleep.OnClick += new SensePanelButtonItem.ClickEventHandler(HandleBlinkSleepClick);
            itemBlinkSleep.Children.Add(btnBlinkSleep);

            list.AddItem(divStatus, true);
            list.AddItem(itemStatus, true);
            list.AddItem(divNotify, true);
            list.AddItem(itemCalls, true);
            list.AddItem(itemSms, true);
            list.AddItem(itemMms, true);
            list.AddItem(itemEmail, true);
            list.AddItem(itemVmail, true);
            list.AddItem(itemReminder, true);
            list.AddItem(divBehaviour, true);
            list.AddItem(itemStopDuringCall, true);
            list.AddItem(itemStopAfterUnlock, true);
            list.AddItem(itemDuration, true);
            list.AddItem(itemSequence, true);
            list.AddItem(itemBlinkSleep, true);

            Controls.Add(list);
            Controls.Add(header);
        }

        #region Event handlers

        void HandleItemCallsStatusChanged(object sender, ItemStatus status)
        {
            Settings.Instance.NotifyOnMissedCalls = FromItemStatus(status);
        }

        void HandleItemSmsStatusChanged(object sender, ItemStatus status)
        {
            Settings.Instance.NotifyOnSms = FromItemStatus(status);
        }

        void HandleItemMmsStatusChanged(object sender, ItemStatus status)
        {
            Settings.Instance.NotifyOnMms = FromItemStatus(status);
        }

        void HandleItemEmailStatusChanged(object sender, ItemStatus status)
        {
            Settings.Instance.NotifyOnEmail = FromItemStatus(status);
        }

        void HandleItemVmailStatusChanged(object sender, ItemStatus status)
        {
            Settings.Instance.NotifyOnVoicemail = FromItemStatus(status);
        }

        void HandleItemReminderStatusChanged(object sender, ItemStatus status)
        {
            Settings.Instance.NotifyOnReminders = FromItemStatus(status);
        }

        void HandleItemStopDuringCallStatusChanged(object sender, ItemStatus status)
        {
            Settings.Instance.DoNotNotifyDuringCall = FromItemStatus(status);
        }

        void HandleItemStopAfterUnlockStatusChanged(object sender, ItemStatus status)
        {
            Settings.Instance.StopBlinkingAfterUnlock = FromItemStatus(status);
        }

        void HandleDurationClick(object Sender)
        {
            int newDuration;
            try
            {
                newDuration = int.Parse(txtDuration.Text);
            }
            catch (Exception)
            {
                SenseAPIs.SenseMessageBox.Show("Invalid text entered. Please enter a valid number.", "Error", SenseMessageBoxButtons.OK);
                return;
            }
            Settings.Instance.NotificationTimeoutInSeconds = newDuration;
            itemDuration.SecondaryText = "Currently " + (newDuration > 0 ? "set to " + newDuration + " seconds" : "disabled");
            txtDuration.Text = newDuration + "";
            list.ShowParent();
        }

        void HandleSequenceClick(object Sender)
        {
            string newSequence = txtSequence.Text;
            string regex = @"^(\d+-)+\d+$";
            if (!Regex.IsMatch(newSequence, regex))
            {
                SenseAPIs.SenseMessageBox.Show("Invalid sequence entered. Please enter a valid sequence.", "Error", SenseMessageBoxButtons.OK);
                return;
            }
            Settings.Instance.BlinkingSequence = newSequence;
            itemSequence.SecondaryText = "Currently: " + newSequence;
            txtSequence.Text = newSequence;
            list.ShowParent();
        }
        
        void HandleBlinkSleepClick(object Sender)
        {
            int newBlinkSleep;
            try
            {
                newBlinkSleep = int.Parse(txtBlinkSleep.Text);
            }
            catch (Exception)
            {
                SenseAPIs.SenseMessageBox.Show("Invalid text entered. Please enter a valid number.", "Error", SenseMessageBoxButtons.OK);
                return;
            }
            Settings.Instance.SleepTimeBetweenSequencesInMilliseconds = newBlinkSleep;
            itemBlinkSleep.SecondaryText = "Currently set to " + newBlinkSleep + " ms";
            txtBlinkSleep.Text = newBlinkSleep + "";
            list.ShowParent();
        }

        void HandleTextboxGotFocus(object sender, EventArgs e)
        {
            inputPanel1.Enabled = true;
        }

        void HandleTextboxLostFocus(object sender, EventArgs e)
        {
            inputPanel1.Enabled = false;
        }

        private void HandleTimerAppRunningTick(object sender, EventArgs e)
        {
            if (NativeMethods.EventAlreadyExists("LedExtendedNotificationsApp"))
            {
                itemStatus.TextColor = Color.Green;
                itemStatus.Text = "Notify is Active";
            }
            else
            {
                itemStatus.TextColor = Color.Red;
                itemStatus.Text = "Notify NOT Active";
            }
            list.Invalidate(itemStatus.ClientRectangle);
        }

        private void HandleMenuItemAboutClick(object sender, EventArgs e)
        {
            SenseAPIs.SenseMessageBox.Show("LeoExtendedNotifications by NetRipper & 3dfx", "About", SenseMessageBoxButtons.OK);
        }

        #endregion

        #region Private helpers

        ItemStatus ToItemStatus(bool from)
        {
            return from ? ItemStatus.On : ItemStatus.Off;
        }

        bool FromItemStatus(ItemStatus from)
        {
            return from == ItemStatus.Off ? false : true;
        }

        #endregion
    }
}