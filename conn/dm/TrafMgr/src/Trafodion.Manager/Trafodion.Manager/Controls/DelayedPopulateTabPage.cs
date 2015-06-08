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
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// Base class for a TabPage that should not populate itself in its constructor.  Such a page assumes that
    /// someone will call the DoPopulate method which calls the Populate method that the TabPage has supplied.
    /// <para/>
    /// This is especially useful in a tab control that has several tabs that are expensive to populate.  The tab
    /// control would call DoPopulate on each tab page only as it becomes selected.  The code in DoPopulate is a no-op 
    /// after the first time that it is called.
    /// <para/>
    /// For an example, look at the way the Database Area handles the display of a schema.  The display of the schema's
    /// contents is a tab control with tab for each kind of schema object.  Since each of these can have many entries
    /// and be expensive to populate, the tab pages are only populated the first time that the user could see them.
    /// </summary>
    public class DelayedPopulateTabPage : TrafodionTabPage
    {
        /// <summary>
        /// Whether or not the page has already been populated.
        /// </summary>
        private bool _isPopulated = false;
        private TrafodionProgressUserControl _progressUserControl = null;

        /// <summary>
        /// Default constructor for the UI designer
        /// </summary>
        public DelayedPopulateTabPage()
        {

        }

        /// <summary>
        /// Constuctor
        /// </summary>
        /// <param name="aTabName">The name to appear on the tab</param>
        public DelayedPopulateTabPage(string aTabName)
            : base(aTabName)
        {

        }

        private void BackgroundWorker_RunWorkerCompleted(
            object sender, RunWorkerCompletedEventArgs e)
        {
            // First, handle the case where an exception was thrown.
            if (e.Error != null)
            {
                _isPopulated = false;
                if (e.Error is Connections.MostRecentConnectionTestFailedException || e.Error is Connections.PasswordNotSetException)
                {
                    // This is an OK exception ... we handle it by not populating
                }
                else
                {
                    string errorMessage = (e.Error.InnerException != null) ? e.Error.InnerException.Message : e.Error.Message;
                    MessageBox.Show(Utilities.GetForegroundControl(), errorMessage, Properties.Resources.Error, MessageBoxButtons.OK);
                }
            }
            else if (e.Cancelled)
            {
                // Next, handle the case where the user canceled 
                // the operation.
                // Note that due to a race condition in 
                // the DoWork event handler, the Cancelled
                // flag may not have been set, even though
                // CancelAsync was called.
                _isPopulated = false;
            }
            else
            {
                // Finally, handle the case where the operation 
                // succeeded.
                Populate();
                _isPopulated = true;
            }
        }

        /// <summary>
        /// This method is invoked by the worker thread to fetch DDL for the selected objects
        /// The fetched DDL is reported back in a progress event
        /// </summary>
        /// <param name="sqlMxObjectList"></param>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        void DoWork(BackgroundWorker worker, DoWorkEventArgs e)
        {
            PrepareForPopulate();
        }

        public override void Refresh()
        {
            base.Refresh();
            DoRefresh();
        }
        /// <summary>
        /// The page will be repopulated even if it was already populated.
        /// </summary>
        public void DoRefresh()
        {
            _isPopulated = false;
            DoPopulate();
        }

        /// <summary>
        /// The page will be populated if it hasn't already been populated.
        /// </summary>
        public void DoPopulate()
        {

            // Check to see if it has been populated
            if (!_isPopulated)
            {
                Cursor = Cursors.WaitCursor;
                try
                {
                    Controls.Clear();
                    TrafodionProgressArgs args = new TrafodionProgressArgs("Fetching data...", this, "PrepareForPopulate", new object[0]);
                    _progressUserControl = new TrafodionProgressUserControl(args);
                    _progressUserControl.ProgressCompletedEvent += _progressUserControl_ProgressCompletedEvent;
                    _progressUserControl.Dock = DockStyle.Fill;
                    Controls.Add(_progressUserControl);
                }
                catch (Connections.MostRecentConnectionTestFailedException mrctfe)
                {
                    // This is an OK exception ... we handle it by not populating
                }
                catch (Connections.PasswordNotSetException pnse)
                {
                    // This is an OK exception ... we handle it by not populating
                }
                catch (Exception e)
                {
                    //string errorMessage = (e.InnerException != null) ? e.InnerException.Message : e.Message;
                    //MessageBox.Show(Utilities.GetForegroundControl(), errorMessage, Properties.Resources.Error, MessageBoxButtons.OK);
                }
                finally
                {
                    Cursor = Cursors.Default;
                }

            }
        }

        void _progressUserControl_ProgressCompletedEvent(object sender, TrafodionProgressCompletedArgs e)
        {
            _progressUserControl.ProgressCompletedEvent -= _progressUserControl_ProgressCompletedEvent;
            Controls.Clear();
            // First, handle the case where an exception was thrown.
            if (e.Error != null)
            {
                _isPopulated = false;
                if (e.Error is Connections.MostRecentConnectionTestFailedException || e.Error is Connections.PasswordNotSetException)
                {
                    // This is an OK exception ... we handle it by not populating
                }
                else
                {
                    string errorMessage = (e.Error.InnerException != null) ? e.Error.InnerException.Message : e.Error.Message;
                    TrafodionTextBox theMessageTextBox = new TrafodionTextBox();
                    theMessageTextBox.ReadOnly = true;
                    theMessageTextBox.WordWrap = true;
                    theMessageTextBox.Multiline = true;
                    theMessageTextBox.Text = errorMessage;
                    theMessageTextBox.Dock = DockStyle.Fill;
                    Controls.Add(theMessageTextBox);
                }
            }
            else
            {
                // Finally, handle the case where the operation 
                // succeeded.
                Controls.Clear();
                Populate();
                _isPopulated = true;
            }
        }

        /// <summary>
        /// The derived class overrides this and populates the page.  The override must completely 
        /// destroy and reconstruct the page's contents for refresh to work correctly.  Note that
        /// the concept of refresh also assumes that any relevant model has been refreshed by
        /// some other means.
        /// </summary>
        virtual protected void Populate()
        {
        }

        public virtual void PrepareForPopulate()
        {

        }

    }

}
