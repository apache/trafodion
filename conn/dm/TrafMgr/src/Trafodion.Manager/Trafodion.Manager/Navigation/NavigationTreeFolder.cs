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

namespace Trafodion.Manager.Framework.Navigation
{
    /// <summary>
    /// Abstract class for all folder nodes in a Navigation Tree.
    /// </summary>
    abstract public class NavigationTreeFolder : NavigationTreeNode
    {
        private const int TREE_NODE_REFRESH_CAPACITY = 100;
        private System.ComponentModel.BackgroundWorker _backgroundWorker;
        int refreshcount = 0;
        int completecount = 0;
        private DummyTreeNode _dummyTreeNode;
        protected bool isPopulating = false;

        /// <summary>
        /// Constructs the folder node
        /// </summary>
        public NavigationTreeFolder()
        {
            ImageKey = NavigationTreeView.FOLDER_CLOSED_ICON;
            SelectedImageKey = NavigationTreeView.FOLDER_CLOSED_ICON;
            _dummyTreeNode = new DummyTreeNode();
            Nodes.Add(_dummyTreeNode); // So that a plus sign will show up initially.  Will never be seen.
            InitializeBackgoundWorker();
        }

        /// <summary>
        /// Set up the BackgroundWorker object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorker()
        {
            _backgroundWorker = new System.ComponentModel.BackgroundWorker();
            _backgroundWorker.DoWork += new DoWorkEventHandler(BackgroundWorker_DoWork);
            _backgroundWorker.RunWorkerCompleted +=
                new RunWorkerCompletedEventHandler(BackgroundWorker_RunWorkerCompleted);
        }

        private void BackgroundWorker_DoWork(object sender,
            DoWorkEventArgs e)
        {
            // Get the BackgroundWorker that raised this event.
            BackgroundWorker worker = sender as BackgroundWorker;

            // Assign the result of the computation
            // to the Result property of the DoWorkEventArgs
            // object. This is will be available to the 
            // RunWorkerCompleted eventhandler.
            DoWork(worker, e);
        }

        private void BackgroundWorker_RunWorkerCompleted(
            object sender, RunWorkerCompletedEventArgs e)
        {
            // First, handle the case where an exception was thrown.
            RemoveListeners();
            if (e.Result is TreeNode)
            {
                Nodes.Remove((TreeNode)e.Result);
            }
            if (e.Error != null)
            {
                IsPopulated = false;
            }
            else if (e.Cancelled)
            {
                // Next, handle the case where the user canceled 
                // the operation.
                // Note that due to a race condition in 
                // the DoWork event handler, the Cancelled
                // flag may not have been set, even though
                // CancelAsync was called.
                IsPopulated = false;
            }
            else
            {
                // Finally, handle the case where the operation 
                // succeeded.
                try
                {
                    Populate(null);
                    IsPopulated = true;
                    if (this.TreeView != null && this.TreeView is NavigationTreeView)
                    {
                        ((NavigationTreeView)this.TreeView).TreeNodePopulated(this);
                    }
                }
                catch (Exception ex)
                {
                    IsPopulated = false;
                }
            }
        }

        /// <summary>
        /// This event handler updates the progress bar and appends the DDL text to the output textbox
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>

        private void BackgroundWorker_ProgressChanged(object sender,
            ProgressChangedEventArgs e)
        {

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
                e.Result = e.Argument as TreeNode;
        }

        virtual protected void RemoveListeners()
        {

        }

        /// <summary>
        /// Populates the folder 
        /// </summary>
        /// <param name="aNameFilter"></param>
        public void DoPopulate(NavigationTreeNameFilter aNameFilter)
        {
            DoPopulate(aNameFilter, false);
        }

        public void DoPopulateAsync(NavigationTreeNameFilter aNameFilter)
        {
            if (!_backgroundWorker.IsBusy)
            {
                DoPopulate(aNameFilter, true);
            }
        }

        private void DoPopulate(NavigationTreeNameFilter aNameFilter, bool isBackground)
        {
            if (isBackground)
            {
                if (IsPopulateRequired)
                {
                    try
                    {
                        foreach (TreeNode aNode in Nodes)
                        {
                            if (aNode is NavigationTreeConnectionFolder)
                            {
                                ((NavigationTreeConnectionFolder)aNode).RemoveHandlers();
                            }
                        }
                        RemoveListeners();

                        if (!_backgroundWorker.IsBusy)
                        {
                            _backgroundWorker.RunWorkerAsync(_dummyTreeNode);
                        }
                    }
                    catch (Exception e)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Framework, "Error running folder populate async : " + e.Message);
                    }
                    finally
                    {
                    }
                }
            }
            else
            {
                if (IsPopulateRequired)
                {
                    try
                    {
                        if (this.TreeView != null)
                        {
                            this.TreeView.Cursor = Cursors.WaitCursor;
                            this.TreeView.BeginUpdate();
                        }
                        Populate(aNameFilter);
                        IsPopulated = true;
                    }
                    catch (Exception)
                    {
                        IsPopulated = false;
                    }
                    finally
                    {
                        if (this.TreeView != null)
                        {
                            this.TreeView.EndUpdate();
                            this.TreeView.Cursor = Cursors.Default;
                        }
                    }
                }
            }
        }

        protected virtual bool IsPopulateRequired
        {
            get { return !IsPopulated; }
        }
        
        /// <summary>
        /// Refreshes the folder by clearing and populating the child nodes
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void DoRefresh(NavigationTreeNameFilter aNameFilter)
        {
            if (this._backgroundWorker.IsBusy) return;

            RemoveListeners();
            Nodes.Clear();
            Nodes.Add(_dummyTreeNode);
            IsPopulated = false;

            try
            {
                Refresh(aNameFilter);

                /*
                 * Both folder node actions of "Refresh" action "Expand" need to fetch data(e.g. Tables) to populated tree nodes.
                 * As of now, the difference is, when fetching all Tables, "Expand" is asynchronous, but "Refresh" is still synchronous:
                 *           - "asynchronous" means the action of adding large number of node can periodicaly respond UI by calling a special build-in method "Application.DoEvents()".
                 *           - But "synchronous" cannot leverage "Application.DoEvents()".
                 * Here the change is to replace DoPopulate() method with DoPopulateAsync() to make fetching data process asynchronous which is called when doing "Refresh" 
                 * and later, the tree can renew UI periodically, such as every 100 Table nodes is added.
                 *
                 */
                DoPopulateAsync(aNameFilter);
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
            }
            if (TreeView != null)
            {
                TreeView.SelectedNode = Parent;
                TreeView.SelectedNode = this;
            }
        }

        protected virtual void Populate(NavigationTreeNameFilter nameFilter)
        {
            // The flag "isPopulating" is to avoid clearing nodes while populating nodes
            if (this.isPopulating) return;
            this.isPopulating = true;

            this.TreeView.Cursor = Cursors.WaitCursor;
            Nodes.Clear();
            int i = 0;
            if ((nameFilter == null) || (nameFilter.IsNoOp))
            {
                AddNodes();
            }
            else
            {
                AddNodes(nameFilter);
            }

            this.TreeView.Cursor = Cursors.Default;
            this.isPopulating = false;
        }

        protected virtual void AddNodes()
        {
        }

        protected virtual void AddNodes(NavigationTreeNameFilter nameFilter)
        {
        }

        protected void RespondUI(NavigationTreeFolder folderNode)
        {
            if (folderNode.Nodes.Count % TREE_NODE_REFRESH_CAPACITY == 0) Application.DoEvents();
        }

        virtual protected void PrepareForPopulate()
        {
        }


        private bool isPopulated = false;

        /// <summary>
        /// Indicates if the folder has been populated
        /// </summary>
        public bool IsPopulated
        {
            get { return isPopulated; }
            set { isPopulated = value; }
        }
    }
}
