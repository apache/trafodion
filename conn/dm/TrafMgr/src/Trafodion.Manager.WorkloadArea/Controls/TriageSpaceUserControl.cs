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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class TriageSpaceUserControl : UserControl, ICloneToWindow
    {
        #region Constructors
        public TriageSpaceUserControl()
        {
            InitializeComponent();
            _theTriageHelper = new TriageHelper();
            showFilterPanel(true);
        }

        public TriageSpaceUserControl(ConnectionDefinition aConnectionDefinition)
            : this()
        {
            _connectionDefinition = aConnectionDefinition;            
            _theTriageHelper.TheConnectionDefinition = aConnectionDefinition;            

            _theFilterpanel = new TriageFilterPropertyGrid(_theTriageHelper);
            _theFilterpanel.Dock = DockStyle.Fill;
            _theFilterContainer.Controls.Add(_theFilterpanel);

            _theTriageGridUserControl = new TriageGridUserControl(aConnectionDefinition);
            _theTriageGridUserControl.TriageHelper = _theTriageHelper;
            _theTriageGridUserControl.Dock = DockStyle.Fill;
            _theTriageGridContainer.Controls.Add(_theTriageGridUserControl);

            //Test code -- must be refactored
            _theTriageChartControl.TriageHelper = _theTriageHelper;
            _theTriageGridUserControl.ChartControl = _theTriageChartControl;

            _theTriageHelper.TriageGridUserControl = _theTriageGridUserControl;
            _theTriageHelper.TriageFilterPropertyGrid = _theFilterpanel;
        }
        #endregion

        #region private member variables
        private ConnectionDefinition _connectionDefinition;
        TriageFilterPropertyGrid _theFilterpanel;
        TriageGridUserControl _theTriageGridUserControl;
        TriageHelper _theTriageHelper;
        #endregion private member variables

        #region Public Properties

        public ConnectionDefinition TheConnectionDefinition
        {
            get { return _connectionDefinition; }
            set
            {
                if (_connectionDefinition != value)
                {
                _connectionDefinition = value;
                    _theTriageGridUserControl.Dispose();
                    _theTriageGridUserControl = new TriageGridUserControl(_connectionDefinition);
                    _theTriageGridUserControl.TriageHelper = _theTriageHelper;
                    _theTriageGridUserControl.Dock = DockStyle.Fill;
                    _theTriageGridContainer.Controls.Add(_theTriageGridUserControl);
                    _theTriageGridUserControl.ChartControl = _theTriageChartControl;
                _theTriageHelper.TheConnectionDefinition = _connectionDefinition;
                    _theTriageHelper.TriageGridUserControl = _theTriageGridUserControl;
                }
            }
        }

        public void loadSelectedLiveViewQueries(TrafodionIGrid theIGrid) 
        {
            _theTriageGridUserControl.loadSelectedLiveViewQueries(theIGrid);
        }

        public void GetSession(TrafodionIGrid theIGrid) 
        {
            //the first parameter determines whether the iGrid is from LiveView iGrid or not,
            //Now it's true
            _theTriageGridUserControl.getSessionQueries(true, theIGrid);
        }

        #endregion Public Properties

        #region private methods
        private void toolStripButton1_Click(object sender, System.EventArgs e)
        {
            showFilterPanel(_theFilterAndGridSplitContainer.Panel1Collapsed);
        }

        private void showFilterPanel(bool show)
        {
            _theFilterAndGridSplitContainer.Panel1Collapsed = !show;
            toolStripButton1.Image = (show) ? Properties.Resources._2leftarrow : Properties.Resources._2rightarrow;
        }
        #endregion

        #region ICloneToWindow Members

        public Control Clone()
        {
            return new TriageSpaceUserControl(_connectionDefinition);
        }

        public string WindowTitle
        {
            get { return Properties.Resources.TriageSpace; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
        }

        #endregion ICloneToWindow Members
    }
}
