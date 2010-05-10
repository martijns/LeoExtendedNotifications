using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using StedySoft.SenseSDK;
using System.Drawing;

namespace LeoExtendedNotificationsGUI
{
    public class SensePanelColoredLabelItem : SenseListControl.ISenseListItem
    {
        #region Dummy implementation of SensePanelBase

        private class MySensePanelBase : SensePanelBase
        {
            public MySensePanelBase()
                : base()
            {
            }
        }

        #endregion

        #region Private fields

        private SensePanelBase _base;

        #endregion

        #region Constructor

        public SensePanelColoredLabelItem()
            : base()
        {
            _base = new MySensePanelBase();
            _base.RequestAction += new SenseListControl.RequestActionHandler(HandleBaseRequestAction);
            _base.RequestParent += new SenseListControl.RequestParentHandler(HandleBaseRequestParent);
            Text = "Dummy";
            TextColor = Color.DarkGreen;
            //BackColor = Color.DarkGreen;
            Font = new Font(FontFamily.GenericSerif, 12F, FontStyle.Bold);
        }

        #endregion

        #region Public properties

        public string Text { get; set; }

        public Font Font { get; set; }

        public Color TextColor { get; set; }

        //public Color BackColor { get; set; }

        #endregion

        #region Wrapping SensePanelBase

        public List<SenseListControl.ISenseListItem> Children { get { return _base.Children; } set { _base.Children = value; } }
        public System.Drawing.Rectangle ClientRectangle { get { return _base.ClientRectangle; } set { _base.ClientRectangle = value; } }
        public int Height { get { return _base.Height; } set { _base.Height = value; } }
        public int Index { get { return _base.Index; } set { _base.Index = value; } }
        public string Name { get { return _base.Name; } set { _base.Name = value; } }
        public object Tag { get { return _base.Tag; } set { _base.Tag = value; } }
        public bool Visible { get { return _base.Visible; } set { _base.Visible = value; } }
        public int Width { get { return _base.Width; } set { _base.Width = value; } }

        public void OnGotFocus(int x, int y)
        {
            _base.OnGotFocus(x, y);
        }

        public void OnHandleCreated()
        {
            _base.OnHandleCreated();
        }

        public void OnHandleDisposed()
        {
            _base.OnHandleDisposed();
        }

        public void OnLostFocus(int x, int y)
        {
            _base.OnLostFocus(x, y);
        }

        public void OnMouseDown(int x, int y, ref bool cancel)
        {
            _base.OnMouseDown(x, y, ref cancel);
        }

        public void OnMouseMove(int x, int y)
        {
            _base.OnMouseMove(x, y);
        }

        public void OnMouseUp(int x, int y)
        {
            _base.OnMouseUp(x, y);
        }

        public void OnRender(Graphics g)
        {
            _base.OnRender(g);
            MyRender(g);
        }

        public void OnResize()
        {
            _base.OnResize();
        }

        public event SenseListControl.RequestActionHandler RequestAction;

        public event SenseListControl.RequestParentHandler RequestParent;

        void HandleBaseRequestParent(SenseListControl.ISenseListItem sender)
        {
            if (RequestParent != null)
                RequestParent(sender);
        }

        void HandleBaseRequestAction(SenseListControl.ISenseListItem sender, SenseListControl.RequestActionEventArgs e)
        {
            if (RequestAction != null)
                RequestAction(sender, e);
        }

        #endregion

        #region Custom render

        private void MyRender(Graphics g)
        {
            SizeF size = g.MeasureString(Text, Font);

            int x = ClientRectangle.X + (int)Math.Round((Width / 2.0) - (size.Width / 2.0));
            int y = ClientRectangle.Y + (int)Math.Round((Height / 2.0) - (size.Height / 2.0));

            g.DrawString(Text, Font, new SolidBrush(TextColor), new RectangleF(x, y, size.Width, size.Height));
        }

        #endregion
    }
}
