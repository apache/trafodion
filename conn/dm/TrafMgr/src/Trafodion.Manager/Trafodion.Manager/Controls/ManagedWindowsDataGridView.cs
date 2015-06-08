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

using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing.Design;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// A grid that displays all of the managed windows
    /// </summary>
    [ToolboxItem(false)]
    public class ManagedWindowsDataGridView : TrafodionDataGridView
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public ManagedWindowsDataGridView()
        {

            // Customize
            RowTemplate.ReadOnly = true;
            Enabled = true;
            AllowUserToAddRows = false;
            RowHeadersVisible = true;

            // Ceate the window title column.  We will actually be storing a refernce to the managed window
            // in this column rather than merely its name.
            theWindowTitleColumn = new DataGridViewTextBoxColumn();
            theWindowTitleColumn.HeaderText = "Window Title";
            theWindowTitleColumn.Name = "theWindowTitleColumn";
            theWindowTitleColumn.ReadOnly = true;
            Columns.Add(theWindowTitleColumn);

            // Remember its index
            theWindowTitleColumnIndex = Columns.IndexOf(theWindowTitleColumn);

            // Create and add handlers for managed windows opening and closing
            theManagedWindowOpenedHandler = new WindowsManager.ManagedWindowOpenedHandler(ManagedWindow_Opened);
            theManagedWindowClosedHandler = new WindowsManager.ManagedWindowClosedHandler(ManagedWindow_Closed);
            WindowsManager.ManagedWindowOpened += theManagedWindowOpenedHandler;
            WindowsManager.ManagedWindowClosed += theManagedWindowClosedHandler;

            // Add all of the managed windows in existence as of now to the grid
            foreach (ManagedWindow theManagedWindow in WindowsManager.ManagedWindows)
            {
                AddManagedWindowRow(theManagedWindow);
            }

        }

        /// <summary>
        /// Called when going away
        /// </summary>
        /// <param name="disposing"></param>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {

                // Remove our event handlers
                WindowsManager.ManagedWindowOpened -= theManagedWindowOpenedHandler;
                WindowsManager.ManagedWindowClosed -= theManagedWindowClosedHandler;

            }
            base.Dispose(disposing);
        }

        /// <summary>
        /// Called when a managed windows ceases to exist
        /// </summary>
        /// <param name="aManagedWindow">The managed window</param>
        void ManagedWindow_Closed(ManagedWindow aManagedWindow)
        {

            // Remove it from the grid
            RemoveManagedWindowRow(aManagedWindow);

        }

        /// <summary>
        /// Called when a managed windows comes into existence
        /// </summary>
        /// <param name="aManagedWindow">The managed window</param>
        void ManagedWindow_Opened(ManagedWindow aManagedWindow)
        {

            // Add it to the grid
            AddManagedWindowRow(aManagedWindow);

        }

        /// <summary>
        /// Attempts to find a grid row containing a given managed window
        /// </summary>
        /// <param name="aManagedWindow">The managed window</param>
        /// <returns>-1 if not found else the zero-based row index of its entry</returns>
        int FindManagedWindowRow(ManagedWindow aManagedWindow)
        {

            // Loop over all of the rows
            for (int theRowIndex = 0, theRowCount = Rows.Count; theRowIndex < theRowCount; theRowIndex++)
            {

                // The window title column actually contains a refernece to a managed window
                ManagedWindow theManagedWindow = Rows[theRowIndex].Cells[theWindowTitleColumnIndex].Value as ManagedWindow;

                // Check to see if this is the desired one
                if (theManagedWindow == aManagedWindow)
                {

                    // It is; return the row index
                    return theRowIndex;

                }
            }

            // Never found it; return -1
            return -1;

        }

        /// <summary>
        /// Add a managed window to the grid if it is not already there
        /// </summary>
        /// <param name="aManagedWindow">The managed window</param>
        private void AddManagedWindowRow(ManagedWindow aManagedWindow)
        {

            // Is it already there?
            int theRowIndex = FindManagedWindowRow(aManagedWindow);
            if (theRowIndex < 0)
            {

                // No; add it
                Rows.Add(RowData(aManagedWindow));

            }
        }

        /// <summary>
        /// Remove a managed window from the grid if it is there
        /// </summary>
        /// <param name="aManagedWindow">The managed window</param>
        private void RemoveManagedWindowRow(ManagedWindow aManagedWindow)
        {

            // Is it there?
            int theRowIndex = FindManagedWindowRow(aManagedWindow);
            if (theRowIndex >= 0)
            {

                // Yes; remove it
                Rows.RemoveAt(theRowIndex);

            }
        }

        /// <summary>
        /// Given a managed window, return a array of objects that make up its row data for the grid
        /// </summary>
        /// <param name="aManagedWindow">The managed window</param>
        /// <returns>The array of objects</returns>
        private object[] RowData(ManagedWindow aManagedWindow)
        {
            return new object[] { aManagedWindow };
        }

        /// <summary>
        /// Read only property to get a list of all of the managed windows in the grid
        /// </summary>
        public List<ManagedWindow> ManagedWindows
        {
            get
            {

                // Create an empty list
                List<ManagedWindow> theManagedWindows = new List<ManagedWindow>();

                // Loop over all of the rows in the grid
                foreach (DataGridViewRow theRow in Rows)
                {

                    // Get the managed window form the current row
                    ManagedWindow theManagedWindow = theRow.Cells[theWindowTitleColumnIndex].Value as ManagedWindow;

                    // Make sure that the entry is not null
                    if (theManagedWindow != null)
                    {

                        // Add the current managed window to the lsit
                        theManagedWindows.Add(theManagedWindow);

                    }
                }

                // Pass back the list
                return theManagedWindows;

            }
        }

        /// <summary>
        /// Read only property to get a list of all of the managed windows that are selected in the grid
        /// </summary>
        public List<ManagedWindow> SelectedManagedWindows
        {
            get
            {

                // Create an empty list
                List<ManagedWindow> theManagedWindows = new List<ManagedWindow>();

                // Loop over all of the rows in the grid
                foreach (DataGridViewRow theRow in Rows)
                {

                    // Checkl to see if the current row is selected
                    if (theRow.Selected)
                    {

                        // It is; get the managed window form the current row
                        ManagedWindow theManagedWindow = theRow.Cells[theWindowTitleColumnIndex].Value as ManagedWindow;

                        // Make sure that the entry is not null
                        if (theManagedWindow != null)
                        {

                            // Add the current managed window to the lsit
                            theManagedWindows.Add(theManagedWindow);

                        }
                    }
                }

                // Pass back the list
                return theManagedWindows;
            }
        }

        /// <summary>
        /// The window title column
        /// </summary>
        private DataGridViewTextBoxColumn theWindowTitleColumn;

        /// <summary>
        /// The index of the window title column
        /// </summary>
        private int theWindowTitleColumnIndex;

        // The handlers for adding and removing managed windows while we are in existence
        private WindowsManager.ManagedWindowOpenedHandler theManagedWindowOpenedHandler;
        private WindowsManager.ManagedWindowClosedHandler theManagedWindowClosedHandler;

    }


}
