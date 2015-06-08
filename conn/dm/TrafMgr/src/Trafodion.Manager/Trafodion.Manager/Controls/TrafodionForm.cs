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
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// Wrapper class extending the TrafodionForm class. It has hooks to change the look and feel
    /// when the look and feel of the framework is changed.
    /// Base class for all One GUI forms
    /// </summary>
    public partial class TrafodionForm : Form
    {
        private TrafodionLookAndFeelChangeHandler lookAndFeelChangeHandler = null;
        [DllImport("user32")]
        public static extern int DestroyIcon(IntPtr hIcon);
        IntPtr hIcon;
        
        /// <summary>
        /// Constructor
        /// </summary>
        public TrafodionForm()
            : base()
        {
            //Changes the theme when the theme is changed for the framework and
            //also sets the default theme
            lookAndFeelChangeHandler = new TrafodionLookAndFeelChangeHandler(this);
            BackColor = Color.WhiteSmoke;            
            hIcon = Properties.Resources.trafodion_form_icon.GetHicon();
            this.Icon = Icon.FromHandle(hIcon);
        }

        /// <summary>
        /// Property for the window title.  The standard prefix will be prepended by the "getter" if not already there.
        /// </summary>
        public override string Text
        {
            get
            {
                // Save it as supplie dby the caller
                return base.Text;
            }

            set
            {

                // Return it with the prefix
                if (!value.StartsWith(TitlePrefix))
                {
                    base.Text = TitlePrefix + value;
                }
                else
                {
                    base.Text = value;
                }
            }
        }

        /// <summary>
        /// Read only property that returns the standard prefix for window titles.
        /// </summary>
        public static string TitlePrefix
        {
            get { return TrafodionForm.theTitlePrefix; }
        }

        /// <summary>
        /// Read only property indicating whether or not this form's content can be cloned to a window
        /// </summary>
        public bool CanCloneToWindow
        {
            get
            {

                return (GetICloneToWindow() != null);

            }
        }

        public ICloneToWindow GetICloneToWindow()
        {
            // The tab can be cloned if it contains exactly one control (other than menu bars) and that one
            // control implements ICloneToWindow
            ICloneToWindow theICloneToWindow = null;
            foreach (Control theControl in Controls)
            {
                if (theControl is ICloneToWindow)
                {
                    if (theICloneToWindow != null)
                    {
                        return null; // More than 1
                    }
                    theICloneToWindow = theControl as ICloneToWindow;
                    continue; // Found one
                }

                if (theControl is MenuStrip)
                {
                    continue; // Ignore main menu
                }

                if (theControl is TrafodionBannerControl)
                {
                    continue;
                }
                if (theControl is ToolStrip)
                {
                    continue;
                }

                return null; // Found something else
            }

            // Found one or zero
            return theICloneToWindow;

        }

        /// <summary>
        /// The standard prefix for window titles.
        /// </summary>
        private static readonly string theTitlePrefix = Properties.Resources.ProductName + " - ";

    }
}
