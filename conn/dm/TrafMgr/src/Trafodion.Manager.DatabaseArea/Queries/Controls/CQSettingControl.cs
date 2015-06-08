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
using Trafodion.Manager.Framework;
using Trafodion.Manager.DatabaseArea.NCC;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// User Control for managing session control settings
    /// </summary>
    public partial class CQSettingControl : UserControl
    {

        #region Fields

        private List<ReportControlStatement> _tempList = new List<ReportControlStatement>();
        private const String TRACE_SUB_AREA_NAME = "CQ Setting";
        private ConnectionDefinition _theConnectionDefinition = null;
        private bool _isChanged = false;

        public delegate void CQSettingChanged(object sender, EventArgs e);
        public event CQSettingChanged OnCQSettingChanged;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: TheControlStatements - Return the current control statement list if there is any
        /// </summary>
        public List<ReportControlStatement> CurrentControlStatements
        {
            get
            {
                return ncccqSettingControl1.GetCQGridValues();
            }
        }

        /// <summary>
        /// Property: ConnectionDefn - the selected connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set
            {
                if (_theConnectionDefinition == null && value == null)
                {
                    return;
                }
                else if (_theConnectionDefinition == null || 
                         value == null ||
                         !_theConnectionDefinition.Equals(value))
                {
                    _theConnectionDefinition = value;
                    ncccqSettingControl1.InitializeCQSettingsControl(_theConnectionDefinition);
                }
            }
        }

        /// <summary>
        /// Property: IsChanged - if the control settings have been changed. 
        /// </summary>
        public bool IsChanged
        {
            get { return _isChanged; }
        }

        #endregion Properties

        #region Constructors
        /// <summary>
        /// The default constructor
        /// </summary>
        public CQSettingControl()
        {
            InitializeComponent();
            Setup();
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Set the CQ Grid with a list of Control Statements
        /// </summary>
        /// <param name="theControlStatements"></param>
        public void SetCQGridValues(List<ReportControlStatement> theControlStatements)
        {
            //TheControlStatements = theControlStatements;
            ncccqSettingControl1.SetCQGridValues(theControlStatements);
            _isChanged = false;
        }

        /// <summary>
        /// Reset the CQ Grid to null
        /// </summary>
        public void Reset()
        {
            ncccqSettingControl1.SetCQGridValues(null);
            _isChanged = false;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Set up all handlers and the grid
        /// </summary>
        private void Setup()
        {
            ncccqSettingControl1.rowAdded += new NCCCQSettingControl.rowAddedDelegate(On_cqSettings_rowAdded);
            ncccqSettingControl1.rowDeleted += new NCCCQSettingControl.rowDeletedDelegate(On_cqSettings_rowDeleted);
            ncccqSettingControl1.rowChanged += new NCCCQSettingControl.rowChangedDelegate(On_cqSettings_rowChanged);
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
            _isChanged = true;
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
                FireOnCQSettingChanged(new EventArgs());
            }
            catch (Exception)
            {
                //No rows found
            }
        }

        /// <summary>
        /// Fire a CQ Setting change event.
        /// </summary>
        /// <param name="e"></param>
        private void FireOnCQSettingChanged(EventArgs e)
        {
            if (OnCQSettingChanged != null)
            {
                OnCQSettingChanged(this, e);
            }
        }

        #endregion Private methods
    }
}
