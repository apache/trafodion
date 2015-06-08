//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//

using System;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    /// <summary>
    /// Wrapper class extending the TabControl class. It has hooks to change the look and feel
    /// when the look and feel of the framework is changed.
    /// A tab control that draws tabs the way One GUI prescribes.
    /// </summary>
    [ToolboxBitmapAttribute(typeof(TabControl))]
    public class TrafodionTabControl : TabControl
    {
        #region Fields
        /// <summary>
        /// 
        /// </summary>
        public event TabControlEventHandler TrafodionTabControlSelectedEvent;
        /// <summary>
        /// 
        /// </summary>
        public event EventHandler TrafodionTabControlSelectedIndexChangedEvent;
        #endregion Fields

        private TrafodionLookAndFeelChangeHandler lookAndFeelChangeHandler = null;

        /// <summary>
        /// Constructor
        /// </summary>
        public TrafodionTabControl()
        {
            //Changes the theme when the theme is changed for the framework and
            //also sets the default theme
            lookAndFeelChangeHandler = new TrafodionLookAndFeelChangeHandler(this);
            this.Font = new Font("Tahoma", 8.25F, FontStyle.Regular);

            // Create the text formatter for the tabs if it hasn't been created yet
            if (theTabTextFormat == null)
            {
                theTabTextFormat = new StringFormat();
                theTabTextFormat.Alignment = StringAlignment.Center;
                theTabTextFormat.LineAlignment = StringAlignment.Center;
            }

            //DrawMode = System.Windows.Forms.TabDrawMode.OwnerDrawFixed;
            Name = "theTabControl";
            SelectedIndex = 0;
            //DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.theTabControl_DrawItem);
            MouseClick += new System.Windows.Forms.MouseEventHandler(this.theTabControl_MouseClick);
            DoubleClick += new EventHandler(TrafodionTabControl_DoubleClick);
            SelectedIndexChanged += new EventHandler(TrafodionTabControl_SelectedIndexChanged);
            Selected += new TabControlEventHandler(TrafodionTabControl_Selected);

            ControlAdded += new ControlEventHandler(TrafodionTabControl_ControlAdded);
            Padding = new Point(10, 5); // Affects the tab size

            Multiline = true;
            HotTrack = true;
        }

        private void TrafodionTabControl_ControlAdded(object sender, ControlEventArgs e)
        {
            if ((TabCount == 1) && (e.Control != null && e.Control is DelayedPopulateTabPage))
            {
                DelayedPopulateTabPage theDelayedPopulateTabPage = e.Control as DelayedPopulateTabPage;
                theDelayedPopulateTabPage.DoPopulate();
            }
        }

        private void TrafodionTabControl_SelectedIndexChanged(object sender, EventArgs e)
        {
            TabPage theSelectedTabPage = SelectedTab;
            if (theSelectedTabPage is DelayedPopulateTabPage)
            {
                DelayedPopulateTabPage theDelayedPopulateTabPage = theSelectedTabPage as DelayedPopulateTabPage;
                theDelayedPopulateTabPage.DoPopulate();
            }

            if (TrafodionTabControlSelectedIndexChangedEvent != null)
                TrafodionTabControlSelectedIndexChangedEvent(sender, e);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TrafodionTabControl_Selected(object sender, TabControlEventArgs e)
        {
            if (TrafodionTabControlSelectedEvent != null)
                TrafodionTabControlSelectedEvent(sender, e);
        }


        /// <summary>
        /// Called if the user double clicks the tab
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TrafodionTabControl_DoubleClick(object sender, EventArgs e)
        {
            DoMouseDoubleClick(e);
        }

        /// <summary>
        /// Called if something needs to be redrawn
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theTabControl_DrawItem(object sender, DrawItemEventArgs e)
        {
            DoDrawItem(e);
        }

        /// <summary>
        /// Process a draw item event
        /// </summary>
        /// <param name="e"></param>
        private void DoDrawItem(DrawItemEventArgs e)
        {
            e.DrawBackground();

            Graphics theGraphics = e.Graphics;

            // This statement was an attempt to change the background color or the tab control itself.  Sometimes
            // it worked, sometimes it didn't and left tandom grey rectangles.  Furthermore, it required that
            //theGraphics.FillRectangle(theBackgroundBrush, 0, 0, Width, Height);

            // Draw the desired tab
            DrawTab(theGraphics, e.Index);

        }

        /// <summary>
        /// Checks if the given page is clonable.
        /// </summary>
        /// <param name="page"></param>
        /// <returns></returns>
        private bool IsTabPageClonable(TabPage page)
        {
            // The tab can be cloned if it contains exactly one control and that one
            // control implements ICloneToWindow
            return ((page.Controls.Count == 1) && (page.Controls[0] is ICloneToWindow));
        }

        /// <summary>
        /// Draw a given tab
        /// </summary>
        /// <param name="aGraphics"></param>
        /// <param name="anIndex">The index of the tab</param>
        private void DrawTab(Graphics aGraphics, int anIndex)
        {
            int theSelectedIndex = SelectedIndex;
            Brush theTabBrush = (anIndex == theSelectedIndex) ? theActiveTabBrush : theInactiveTabBrush;
            Rectangle theTabRectangle = GetTabRect(anIndex);
            Rectangle theTabBackgroundRectangle = theTabRectangle;
            TabPage theTabPage = TabPages[anIndex];
            string theTabText = theTabPage.Text;

            Font theFont = Font;

            aGraphics.FillRectangle(theTabBrush, theTabBackgroundRectangle);

            TrafodionTabPage theTrafodionTabPage = theTabPage as TrafodionTabPage;
            if (((theTrafodionTabPage != null) && (theTrafodionTabPage.CanCloneToWindow)) || IsTabPageClonable(theTabPage))
            {
                theFont = new Font(theFont, theFont.Style | FontStyle.Italic);
            }

            aGraphics.DrawString(theTabText, theFont, theTabTextBrush, theTabRectangle, theTabTextFormat);
        }

        /// <summary>
        /// Usefull contants for drawing tabs
        /// </summary>
        static private StringFormat theTabTextFormat = null;
        static private readonly SolidBrush theActiveTabBrush = new SolidBrush(System.Drawing.Color.FromArgb(((int)(((byte)(117)))), ((int)(((byte)(145)))), ((int)(((byte)(172))))));
        static private readonly SolidBrush theInactiveTabBrush = new SolidBrush(System.Drawing.Color.FromArgb(((int)(((byte)(51)))), ((int)(((byte)(102)))), ((int)(((byte)(153))))));
        static private readonly SolidBrush theTabTextBrush = new SolidBrush(Color.White);
        static private readonly SolidBrush theBackgroundBrush = new SolidBrush(Color.WhiteSmoke);
        static private readonly Pen theTabPen = new Pen(Color.WhiteSmoke);
        private ContextMenu theTabContextMenu = new ContextMenu();

        /// <summary>
        /// Called if the user clicks in a tab
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theTabControl_MouseClick(object sender, MouseEventArgs e)
        {
            DoMouseClick(e);
        }

        /// <summary>
        /// Handle a click in a tab
        /// </summary>
        /// <param name="e"></param>
        private void DoMouseClick(MouseEventArgs e)
        {

            // Check for the right (as opposed to left) mouse button and show the context menu if the
            // tab contains a single control and that control is clonable
            if (e.Button == MouseButtons.Right)
            {
                Point theMouseClickPoint = new Point(e.X, e.Y);

                //If the tab is not the active tab, do not show the context menu.
                //The inactive tab may not be initialized yet.
                int theTabIndex = GetTabIndex(theMouseClickPoint);
                if (SelectedIndex != theTabIndex)
                    return;

                theTabContextMenu.MenuItems.Clear();
                MenuItem theMenuItem = new MenuItem("Clone in Window", new EventHandler(OnTabToWindowClick));
                theTabContextMenu.MenuItems.Add(theMenuItem);

                if ((theTabIndex >= 0) && (TabPages[theTabIndex].Controls.Count == 1))
                {
                    if (IsTabPageClonable(TabPages[theTabIndex]))
                    {
                        Control theControl = TabPages[theTabIndex].Controls[0];
                        theICloneToWindow = theControl as ICloneToWindow;
                        if (theICloneToWindow != null)
                        {
                            theTabContextMenu.Show(this, theMouseClickPoint);
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Called when the user double clicks on a tab to clone its contents if the
        /// tab contains a single control and that control is clonable
        /// </summary>
        /// <param name="e"></param>
        private void DoMouseDoubleClick(EventArgs e)
        {
            if ((SelectedIndex >= 0) && (TabPages[SelectedIndex].Controls.Count == 1))
            {
                if (IsTabPageClonable(TabPages[SelectedIndex]))
                {
                    Control theControl = TabPages[SelectedIndex].Controls[0];
                    theICloneToWindow = theControl as ICloneToWindow;
                    if (theICloneToWindow != null)
                    {
                        WindowsManager.CloneToWindow(Size, theICloneToWindow);
                    }
                }
            }
        }


        /// <summary>
        /// The clonable implementer or not
        /// </summary>
        private ICloneToWindow theICloneToWindow = null;

        /// <summary>
        /// Called if the user selects clone to window on the context menu
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnTabToWindowClick(object sender, System.EventArgs e)
        {
            WindowsManager.CloneToWindow(Size, theICloneToWindow);
        }

        /// <summary>
        /// Get the tab index of the tab containing a given point such as an event might report
        /// </summary>
        /// <param name="aPoint">The opoint as reported by an event</param>
        /// <returns>The tab index or -1 if not in a tab</returns>
        private int GetTabIndex(Point aPoint)
        {
            for (int theIndex = 0, theCount = TabPages.Count; theIndex < theCount; theIndex++)
            {
                if (GetTabRect(theIndex).Contains(aPoint))
                {
                    return theIndex;
                }
            }
            return -1;
        }

    }
}
