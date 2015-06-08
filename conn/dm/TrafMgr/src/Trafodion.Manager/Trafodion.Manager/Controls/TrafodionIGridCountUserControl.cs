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
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// This is a panel that shows the count of rows in a TrafodionIGrid.
    /// </summary>
    public partial class TrafodionIGridCountUserControl : UserControl
    {

        private TrafodionIGrid _TrafodionIGrid;
        private string _theFormat;
        private InvalidateEventHandler _TrafodionIGridInvalidateEventHandler;

        /// <summary>
        /// Property to get/set associated TrafodionIGrid
        /// </summary>
        public TrafodionIGrid TheTrafodionIGrid
        {
            get { return _TrafodionIGrid; }
            set
            {
                RemoveEventHandler();
                _TrafodionIGrid = value;
                UpdateControls();
                AddEventHandler();
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
        /// Constructs the user control that displays the count message for the TrafodionIGrid
        /// </summary>
        /// <param name="aTrafodionIGrid"></param>
        /// <param name="aFormat"></param>
        public TrafodionIGridCountUserControl(TrafodionIGrid aTrafodionIGrid, string aFormat)
        {
            InitializeComponent();
            
            this.theCountToolStrip.Renderer = new TrafodionToolStripRenderer(false);

            TheTrafodionIGrid = aTrafodionIGrid;
            _theFormat = aFormat;

            theCountToolStrip.BackColor = TrafodionColorTable.Highlight;
            theCountToolStrip.GripStyle = ToolStripGripStyle.Hidden;
            theCountToolStrip.ForeColor = TrafodionColorTable.HighlightText;
         }
        
        private void MyDispose(bool disposing)
        {
            RemoveEventHandler();
            _TrafodionIGrid = null;
        }

        private void AddEventHandler()
        {
            if (TheTrafodionIGrid != null)
            {
                _TrafodionIGridInvalidateEventHandler = new InvalidateEventHandler(TheTrafodionDataGridView_Invalidated);
                TheTrafodionIGrid.Invalidated += _TrafodionIGridInvalidateEventHandler;
            }
        }

        void TheTrafodionDataGridView_Invalidated(object sender, InvalidateEventArgs e)
        {
            UpdateControls();
        }

        private void RemoveEventHandler()
        {
            if (TheTrafodionIGrid != null)
            {
                TheTrafodionIGrid.Invalidated -= _TrafodionIGridInvalidateEventHandler;
                _TrafodionIGridInvalidateEventHandler = null;
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
                theText = String.Format(Properties.Resources.CountUserControlFormat, TheTrafodionIGrid.Rows.Count);
            }

            theLabel.Text = theText;
        }


        /// <summary>
        /// This method only works for English. For Other languages, get it from the resource. 
        /// </summary>
        private string TextToDisplayInEnglish()
        {
            string theText = "";

            if ((TheTrafodionIGrid != null) && (TheFormat != null))
            {
                switch (TheTrafodionIGrid.Rows.Count)
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
                            theText = String.Format(TheFormat, TheTrafodionIGrid.Rows.Count);
                            break;
                        }

                } // -switch
            } // if -on- TheTrafodionIGrid

            return   theText;

        }
    }
}
