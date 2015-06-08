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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.NCC;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// User Dialog for managing session control settings
    /// </summary>
    public partial class CQSettingDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {

        #region Fields

        private List<ReportControlStatement> _theControlStatements = null;
        private List<ReportControlStatement> _tempList = new List<ReportControlStatement>();
        private const String TRACE_SUB_AREA_NAME = "CQ Setting";

        #endregion Fields

        #region Constructors

        /// <summary>
        /// The constructor
        /// </summary>
        /// <param name="theControlStatements"></param>
        public CQSettingDialog(List<ReportControlStatement> theControlStatements)
        {
            _theControlStatements = theControlStatements;
            InitializeComponent();
            CenterToParent();
            Setup();
        }

        #endregion Constructors

        #region Private methods

        /// <summary>
        /// Set up all handlers and the grid
        /// </summary>
        private void Setup()
        {
            ncccqSettingControl1.rowAdded += new NCCCQSettingControl.rowAddedDelegate(On_cqSettings_rowAdded);
            ncccqSettingControl1.rowDeleted += new NCCCQSettingControl.rowDeletedDelegate(On_cqSettings_rowDeleted);
            ncccqSettingControl1.rowChanged += new NCCCQSettingControl.rowChangedDelegate(On_cqSettings_rowChanged);
            ncccqSettingControl1.SetCQGridValues(_theControlStatements);
        }

        /// <summary>
        /// Called when a row is added to the cqSettings grid
        /// </summary>
        private void On_cqSettings_rowAdded()
        {
            UpdateTempControlStatements();
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG, 
                    TraceOptions.TraceArea.SQLWhiteboard,
                    TRACE_SUB_AREA_NAME,
                    "CQSettingDialog:: Row added to cq Settings");
            }
        }

        /// <summary>
        /// Called when a row is removed from the cqSettings grid
        /// </summary>
        /// <param name="rra"></param>
        private void On_cqSettings_rowDeleted(RemoveRowArgs rra)
        {
            UpdateTempControlStatements();
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG, 
                    TraceOptions.TraceArea.SQLWhiteboard,
                    TRACE_SUB_AREA_NAME,
                    "CQSettingDialog:: Row Deleted index: " + rra.DeletedIndex());
            }
        }

        /// <summary>
        /// Called when a row is changed
        /// </summary>
        private void On_cqSettings_rowChanged()
        {
            UpdateTempControlStatements();
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG, 
                    TraceOptions.TraceArea.SQLWhiteboard,
                    TRACE_SUB_AREA_NAME,
                    "CQSettingDialog:: Row changed: ");
            }
        }

        /// <summary>
        /// Update the current temp list
        /// </summary>
        private void UpdateTempControlStatements()
        {
            try
            {
                // Clear the list before loading from the grid
                _tempList.Clear();

                List<ReportControlStatement> csList = ncccqSettingControl1.GetCQGridValues();
                for (int i = 0; csList.Count > i; i++)
                {
                    ReportControlStatement cso = (ReportControlStatement)csList[i];
                    try
                    {
                        _tempList.Add(cso);
                    }
                    catch (Exception)
                    {
                    }
                }
            }
            catch
            {
                //No rows found
            }
        }

        /// <summary>
        /// Invoked when user click on the apply button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _applyButton_Click(object sender, EventArgs e)
        {
            // First clear the current entries and then copy everything from the temp list.
            _theControlStatements.Clear();
            for (int i = 0; i < _tempList.Count; i++)
            {
                _theControlStatements.Add((ReportControlStatement)_tempList[i]);
            }

            DialogResult = DialogResult.OK;
        }

        /// <summary>
        /// User has cancel the dialog
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _cancelButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        #endregion Private methods

        private void _helpButton_Click(object sender, EventArgs e)
        {

        }
    }
}
