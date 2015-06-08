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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class FloatingTableSampledStatsControl : UserControl, IMenuProvider
    {
        private SampledStatsSummaryControl _sampledStatsSummaryControl;
        private SampledStatsFreqValuesControl _sampledStatsFreqValuesControl;
        private SampledStatsIntervalControl _sampledStatsIntervalControl;
        private SampledStatsGraphControl _sampledStatsGraphControl;
        private System.Drawing.Size _theCurrentSize;
        private TrafodionTableColumn _sqlMxTableColumn = null;
        private static readonly string TableStatsPersistenceKey = "TableStatistics";
        private System.ComponentModel.BackgroundWorker _backgroundWorker;

        /// <summary>
        /// Constructs the user control to display the statistics for the table column
        /// </summary>
        /// <param name="sqlMxTableColumn">The SQL table column model</param>
        public FloatingTableSampledStatsControl(TrafodionTableColumn sqlMxTableColumn)
        {
            InitializeComponent();
            _sqlMxTableColumn = sqlMxTableColumn;
            widgetCanvas.ThePersistenceKey = TableStatsPersistenceKey;

            GridLayoutManager gridLayoutManager = new GridLayoutManager(4, 2);
            gridLayoutManager.CellSpacing = 4;
            widgetCanvas.LayoutManager = gridLayoutManager;

            GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 2);
            _sampledStatsSummaryControl = new SampledStatsSummaryControl();
            WidgetContainer widgetContainer = new WidgetContainer(widgetCanvas, _sampledStatsSummaryControl, "Summary");
            widgetContainer.Name = "Summary";
            widgetContainer.AllowDelete = false;
            this.widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            gridConstraint = new GridConstraint(1, 0, 1, 2);
            _sampledStatsFreqValuesControl = new SampledStatsFreqValuesControl();
            widgetContainer = new WidgetContainer(widgetCanvas, _sampledStatsFreqValuesControl, "Top 10 Values");
            widgetContainer.Name = "Top 10 Values";
            widgetContainer.AllowDelete = false;
            this.widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            gridConstraint = new GridConstraint(2, 0, 2, 1);
            _sampledStatsIntervalControl = new SampledStatsIntervalControl();
            widgetContainer = new WidgetContainer(widgetCanvas, _sampledStatsIntervalControl, "Intervals");
            widgetContainer.Name = "Intervals";
            widgetContainer.AllowDelete = false;
            this.widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            gridConstraint = new GridConstraint(2, 1, 2, 1);
            _sampledStatsGraphControl = new SampledStatsGraphControl();
            widgetContainer = new WidgetContainer(widgetCanvas, _sampledStatsGraphControl, "Distribution By Intervals");
            widgetContainer.Name = "Distribution By Intervals";
            widgetContainer.AllowDelete = false;
            this.widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            //Display the screen first and then populate. So process the enter event
            this.Enter += new System.EventHandler(TableSampledStatsControl_Enter);

            // Before, let's check if the newly added widgetCanvas needs to be locked.
            // Note, this has to be done after all of the components in the widgetCanvas
            // have been created.  If it locked before that, nothing can be added to the
            // canvas. 
            this.widgetCanvas.Locked = LockManager.Locked;
            LockManager.LockHandlers += new LockManager.LockHandler(TableFloatingSampledStatsControl_Lock);

            //[TBD] Resize when the container resized?
            // Now, it's time to register for resize event.
            this._theCurrentSize = this.Size;

            this.widgetCanvas.InitializeCanvas();
        }

        #region IMenuProvider implementation
        /// <summary>
        /// Implemeting the IMenuProvider interface
        /// </summary>
        /// <returns></returns>
        public Trafodion.Manager.Framework.Controls.TrafodionMenuStrip GetMenuItems(ImmutableMenuStripWrapper aMenuStripWrapper)
        {
            //get the menu items from the canvas
            TrafodionToolStripMenuItem theResetLayoutMenuItem = widgetCanvas.ResetLayoutMenuItem;
            TrafodionToolStripMenuItem theLockStripMenuItem = widgetCanvas.LockMenuItem;
            System.Windows.Forms.ToolStripSeparator toolStripSeparator1 = new TrafodionToolStripSeparator();

            //Obtain the index of the exit menu because we want to insert the
            //menus just above the exit menu
            int exitIndex = aMenuStripWrapper.getMenuIndex(global::Trafodion.Manager.Properties.Resources.MenuExit);


            //Set the properties of the menu items correctly
            theResetLayoutMenuItem.MergeAction = System.Windows.Forms.MergeAction.Insert;
            theResetLayoutMenuItem.MergeIndex = exitIndex;
            theLockStripMenuItem.MergeAction = System.Windows.Forms.MergeAction.Insert;
            theLockStripMenuItem.MergeIndex = exitIndex;
            toolStripSeparator1.MergeAction = System.Windows.Forms.MergeAction.Insert;
            toolStripSeparator1.MergeIndex = exitIndex;

            //Create the same menu structure as we have for main
            ToolStripMenuItem fileMenuItem = new ToolStripMenuItem();
            fileMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            toolStripSeparator1,
            theResetLayoutMenuItem,
            theLockStripMenuItem});

            //Appropriately set the menu name and text for the file menu
            fileMenuItem.MergeAction = System.Windows.Forms.MergeAction.MatchOnly;
            fileMenuItem.Name = global::Trafodion.Manager.Properties.Resources.MenuFile;
            fileMenuItem.Text = global::Trafodion.Manager.Properties.Resources.MenuFile;

            //Create the menu strip
            Trafodion.Manager.Framework.Controls.TrafodionMenuStrip menus = new Trafodion.Manager.Framework.Controls.TrafodionMenuStrip();
            menus.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            fileMenuItem});

            return menus;
        }
        #endregion


        #region Private Methods

        /// <summary>
        /// The Resize event handler.
        /// </summary>
        private void TableFloatingSampledStatsControl_Resize(object sender, EventArgs e)
        {
            System.Drawing.Size newSize = ((Control)sender).Size;

            if (this.Parent != null)
            {
                // We don't want to shrunk too small or grow bigger than the container.
                if (newSize.Width >= this.DefaultSize.Width &&
                    newSize.Height >= this.DefaultSize.Height &&
                    newSize.Width <= this.Parent.Size.Width &&
                    newSize.Height <= this.Parent.Size.Height)
                {
                    double wf = (newSize.Width / 1.0F) / (this._theCurrentSize.Width / 1.0F);
                    double hf = (newSize.Height / 1.0F) / (this._theCurrentSize.Height / 1.0F);
                    this.widgetCanvas.ResizeWidgets(wf, hf);

                    // Remember the current size.
                    this._theCurrentSize = newSize;
                }
            }
        }

        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_backgroundWorker != null)
                {
                    _backgroundWorker.CancelAsync();
                }
                _sampledStatsSummaryControl.Dispose();
                _sampledStatsFreqValuesControl.Dispose();
                _sampledStatsIntervalControl.Dispose();
                _sampledStatsGraphControl.Dispose();
                widgetCanvas.Dispose();
            }
        }
        /// <summary>
        /// The lock event handler.
        /// </summary>
        private void TableFloatingSampledStatsControl_Lock(LockManager.LockOperation aLockOperation)
        {
            switch (aLockOperation)
            {
                case LockManager.LockOperation.Lock:
                    {
                        this.widgetCanvas.Locked = true;
                        break;
                    }
                case LockManager.LockOperation.Unlock:
                    {
                        this.widgetCanvas.Locked = false;
                        break;
                    }
                default:
                    {
                        throw new ApplicationException("Unknown lock op");
                    }
            }
        }


        /// <summary>
        /// When the focus enters this control, populate with data.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TableSampledStatsControl_Enter(object sender, EventArgs e)
        {
            try
            {
                _sqlMxTableColumn.SampledStatistics = null;//reset the earlier sample
                InitializeBackgoundWorker();
                _sampledStatsSummaryControl.InitializeControl(_sqlMxTableColumn);
                _sampledStatsFreqValuesControl.InitializeControl();
                _sampledStatsIntervalControl.InitializeControl();
                Cursor = Cursors.WaitCursor;
                _backgroundWorker.RunWorkerAsync(_sqlMxTableColumn);
            }
            catch (Exception ex)
            {
                string header = String.Format(Properties.Resources.ErrorWhileSampling, _sqlMxTableColumn.TrafodionSchemaObject.VisibleAnsiName);
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, header, MessageBoxButtons.OK, MessageBoxIcon.Error);
                Cursor = Cursors.Default;
            }
        }

        /// <summary>
        /// Set up the BackgroundWorker object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorker()
        {
            _backgroundWorker = new System.ComponentModel.BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.WorkerSupportsCancellation = true;
            _backgroundWorker.DoWork += new DoWorkEventHandler(BackgroundWorker_DoWork);
            _backgroundWorker.RunWorkerCompleted +=
                new RunWorkerCompletedEventHandler(BackgroundWorker_RunWorkerCompleted);
            _backgroundWorker.ProgressChanged +=
                new ProgressChangedEventHandler(BackgroundWorker_ProgressChanged);
        }

        /// <summary>
        /// This event handler is where the actual,
        /// potentially time-consuming DDL work is done.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_DoWork(object sender,
            DoWorkEventArgs e)
        {
            // Get the BackgroundWorker that raised this event.
            BackgroundWorker worker = sender as BackgroundWorker;

            // Assign the result of the computation
            // to the Result property of the DoWorkEventArgs
            // object. This is will be available to the 
            // RunWorkerCompleted eventhandler.
            RunSample((TrafodionTableColumn)e.Argument, worker, e);
        }


        /// <summary>
        /// This event handler deals with the results of the
        /// background operation.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_RunWorkerCompleted(
            object sender, RunWorkerCompletedEventArgs e)
        {
            // First, handle the case where an exception was thrown.
            if (e.Error != null)
            {
                _sampledStatsSummaryControl.SetStatusOnError();
                MessageBox.Show(Utilities.GetForegroundControl(), e.Error.Message, Properties.Resources.Error, MessageBoxButtons.OK);
            }
            else if (e.Cancelled)
            {
                _sampledStatsSummaryControl.SetStatusOnError();
            }
            else
            {
                TrafodionTableColumn sqlMxTableColumn = e.Result as TrafodionTableColumn;
                if (sqlMxTableColumn != null)
                {
                    _sampledStatsSummaryControl.Populate(sqlMxTableColumn);
                    _sampledStatsFreqValuesControl.Populate(sqlMxTableColumn);
                    _sampledStatsIntervalControl.Populate(sqlMxTableColumn);
                    _sampledStatsGraphControl.DrawChart(_sampledStatsIntervalControl.BoundaryDataGridView);
                }
            }
            Cursor = Cursors.Default;
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

        void RunSample(TrafodionTableColumn sqlMxTableColumn, BackgroundWorker worker, DoWorkEventArgs e)
        {
            // Abort the operation if the user has canceled.
            // Note that a call to CancelAsync may have set 
            // CancellationPending to true just after the
            // last invocation of this method exits, so this 
            // code will not have the opportunity to set the 
            // DoWorkEventArgs.Cancel flag to true. This means
            // that RunWorkerCompletedEventArgs.Cancelled will
            // not be set to true in your RunWorkerCompleted
            // event handler. This is a race condition.

            if (worker.CancellationPending)
            {
                e.Cancel = true;
            }
            else
            {
                int count = sqlMxTableColumn.SampledStatistics.FrequentValues.Count;
                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                    return;
                }

                count = sqlMxTableColumn.SampledStatistics.SampledIntervals.Count;
                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                }
                e.Result = sqlMxTableColumn;
            }
        }

        #endregion /* end of Private Methods region */

    }
}
