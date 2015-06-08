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
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Globalization;
using System.Threading;
using Trafodion.Manager.Framework;


namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// This is a panel that shows the count of rows in a TrafodionDataGridView.
    /// </summary>
    public partial class DataGridViewCountUserControl : UserControl
    {

        private TrafodionDataGridView _theTrafodionDataGridView;
        private string _theFormat;
        private DataGridViewRowsAddedEventHandler   _theDataGridViewRowsAddedEventHandler;
        private DataGridViewRowsRemovedEventHandler _theDataGridViewRowsRemovedEventHandler;

        /// <summary>
        /// Property to get/set associated TrafodionDataGridView
        /// </summary>
        public TrafodionDataGridView TheTrafodionDataGridView
        {
            get { return _theTrafodionDataGridView; }
            set
            {
                RemoveEventHandlers();
                _theTrafodionDataGridView = value;
                UpdateControls();
                AddEventHandlers();
            }
        }

        /// <summary>
        /// The format string for the count message
        /// </summary>
        public string TheFormat
        {
            get { return _theFormat; }
            set 
            {
                _theFormat = value;
                UpdateControls();
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aTrafodionDataGridView">An assoicated TrafodionDataGridView</param>
        /// <param name="aFormat">format string</param>
        public DataGridViewCountUserControl(TrafodionDataGridView aTrafodionDataGridView, string aFormat)
        {
            InitializeComponent();

            this.theCountToolStrip.Renderer = new TrafodionToolStripRenderer(false);

            TheTrafodionDataGridView = aTrafodionDataGridView;
            TheFormat = aFormat;

            theCountToolStrip.BackColor = TrafodionColorTable.Highlight;
            theCountToolStrip.GripStyle = ToolStripGripStyle.Hidden;
            theCountToolStrip.ForeColor = TrafodionColorTable.HighlightText;
        }

        private void theTrafodionDataGridViewRowsAdded(object sender, DataGridViewRowsAddedEventArgs e)
        {
            UpdateControls();
        }

        private void theTrafodionDataGridViewRowsRemoved(object sender, DataGridViewRowsRemovedEventArgs e)
        {
            UpdateControls();
        }

        private void AddEventHandlers()
        {
            if (TheTrafodionDataGridView != null)
            {
                _theDataGridViewRowsAddedEventHandler = new DataGridViewRowsAddedEventHandler(theTrafodionDataGridViewRowsAdded);
                TheTrafodionDataGridView.RowsAdded += _theDataGridViewRowsAddedEventHandler;
                _theDataGridViewRowsRemovedEventHandler = new DataGridViewRowsRemovedEventHandler(theTrafodionDataGridViewRowsRemoved);
                TheTrafodionDataGridView.RowsRemoved += _theDataGridViewRowsRemovedEventHandler;
            }
        }

        private void RemoveEventHandlers()
        {
            if (TheTrafodionDataGridView != null)
            {
                TheTrafodionDataGridView.RowsAdded -= _theDataGridViewRowsAddedEventHandler;
                _theDataGridViewRowsAddedEventHandler = null;
                TheTrafodionDataGridView.RowsRemoved -= _theDataGridViewRowsRemovedEventHandler;
                _theDataGridViewRowsRemovedEventHandler = null;
            }
        }



        private void UpdateControls()
        {
            string theText = "";

            // The following is Language-grammer and Culture dependant text displayed to the user
            // e.g. This schema has 69 Tables
            // For non-English, get it from the resource
            // 

            if (Utilities.CurrentLanguageIsEnglish())
            {
                theText = TextToDisplayInEnglish();
            }
            else
            {
                // Note that the type of object e.g. "Tables" is not known to this object 
                // get from resource and format: CountUserControlFormat
                
                // TODO: SINCE WE DO NOT HAVE OTHER RESOURCES AVAILABLE FOR NON-ENGLISH SYSTEMS, 
                // COMMENT THIS OUT FOR NOW, and just display English
                // theText = String.Format(Properties.Resources.CountUserControlFormat, TheTrafodionDataGridView.Rows.Count);
                theText = TextToDisplayInEnglish();

                // TODO: 
                // OR -- Use format as supplied to this object ?
                // theText = String.Format(TheFormat, TheTrafodionDataGridView.Rows.Count);

            }

            theLabel.Text = theText;
        }


        /// <summary>
        /// This method only works for English. For Other languages, get it from the resource. 
        /// </summary>
        private string TextToDisplayInEnglish()
        {
            string theText = "";

            if ((TheTrafodionDataGridView != null) && (TheFormat != null))
            {
                switch (TheTrafodionDataGridView.Rows.Count)
                {
                    // No rows
                    case 0:
                        {
                            // Replace the number with "no".
                            theText = String.Format(TheFormat, "no");
                            break;
                        }

                    // One row (singular); try to fix the plural text
                    case 1:
                        {

                            // Make plural into singular
                            theText = String.Format(TheFormat, 1).Replace(" are ", " is ");

                            // If the plural ends with "ies" make it "y" for singular or if the plural simply
                            // ends is "s" remove it.
                            if (theText.EndsWith("ies"))
                            {
                                theText = theText.Substring(0, theText.Length - 3) + "y";
                            }
                            else if (theText.EndsWith("xes"))
                            {
                                theText = theText.Substring(0, theText.Length - 2);
                            }
                            else if (theText.EndsWith("s"))
                            {
                                theText = theText.Substring(0, theText.Length - 1);
                            }

                            break;
                        }

                    // More than one row (plural)
                    default:
                        {

                            // Use format as supplied
                            theText = String.Format(TheFormat, TheTrafodionDataGridView.Rows.Count);
                            break;
                        }

                } // -switch
            } // if -on- TheTrafodionDataGridView

            return   theText;

        }
    }
}
