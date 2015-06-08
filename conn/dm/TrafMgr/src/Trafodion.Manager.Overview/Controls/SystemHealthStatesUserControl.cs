//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class SystemHealthStatesUserControl : UserControl
    {
        #region Fields

        private ConnectionDefinition _theConnectionDefinition = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: ConnectionDefn - the connection definition 
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        /// <summary>
        /// Property: AccessState - the state of the access layer
        /// </summary>
        public int AccessState
        {
            get { return _accessStatusLightUserControl.State; }
            set { _accessStatusLightUserControl.State = value; }
        }

        /// <summary>
        /// Property: AccessLayer - the access layer control 
        /// </summary>
        public TrafodionStatusLightUserControl AccessLayer
        {
            get { return _accessStatusLightUserControl; }
        }

        /// <summary>
        /// Porpety: DatabaseState - the state of the database layer
        /// </summary>
        public int DatabaseState
        {
            get { return _databaseStatusLightUserControl.State; }
            set { _databaseStatusLightUserControl.State = value; }
        }

        /// <summary>
        /// Property: DatabaseLayer - the database layer control itself
        /// </summary>
        public TrafodionStatusLightUserControl DatabaseLayer
        {
            get { return _databaseStatusLightUserControl; }
        }

        /// <summary>
        /// Porpety: FoundationState - the state of the foundation layer
        /// </summary>
        public int FoundationState
        {
            get { return _foundationStatusLightUserControl.State; }
            set { _foundationStatusLightUserControl.State = value; }
        }

        /// <summary>
        /// Property: FoundationLayer - the foundation layer control itself
        /// </summary>
        public TrafodionStatusLightUserControl FoundationLayer
        {
            get { return _foundationStatusLightUserControl; }
        }

        /// <summary>
        /// Porpety: OSState - the state of the OS layer
        /// </summary>
        public int OSState
        {
            get { return _osStatusLightUserControl.State; }
            set { _osStatusLightUserControl.State = value; }
        }

        /// <summary>
        /// Property: OSLayer - the OS layer control itself
        /// </summary>
        public TrafodionStatusLightUserControl OSLayer
        {
            get { return _osStatusLightUserControl; }
        }

        /// <summary>
        /// Porpety: ServerState - the state of the server layer
        /// </summary>
        public int ServerState
        {
            get { return _serverStatusLightUserControl.State; }
            set { _serverStatusLightUserControl.State = value; }
        }

        /// <summary>
        /// Property: ServerLayer - the server layer control itself
        /// </summary>
        public TrafodionStatusLightUserControl ServerLayer
        {
            get { return _serverStatusLightUserControl; }
        }

        /// <summary>
        /// Porpety: StorageState - the state of the storage layer
        /// </summary>
        public int StorageState
        {
            get { return _storageStatusLightUserControl.State; }
            set { _storageStatusLightUserControl.State = value; }
        }

        /// <summary>
        /// Property: StorageLayer - the storage layer control itself
        /// </summary>
        public TrafodionStatusLightUserControl StorageLayer
        {
            get { return _storageStatusLightUserControl; }
        }

        /// <summary>
        /// Property: AnyVisible - Is any of the health & states light visible?
        /// </summary>
        public bool AnyVisible
        {
            get
            {
                foreach(SystemMetricModel.HealthLayer h in Enum.GetValues(typeof(SystemMetricModel.HealthLayer)))
                {
                    if (SystemMetricChartConfigModel.Instance.GetHealthStatesLayerDisplayStatus(ConnectionDefn.Name, h))
                    {
                        return true;
                    }
                }

                return false;
            }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefn"></param>
        public SystemHealthStatesUserControl(ConnectionDefinition aConnectionDefn)
        {
            _theConnectionDefinition = aConnectionDefn;
            InitializeComponent();
            this.Dock = DockStyle.Fill;
            _thePanel.Resize += new EventHandler(_thePanel_Resize);

            foreach(SystemMetricModel.HealthLayer h in Enum.GetValues(typeof(SystemMetricModel.HealthLayer)))
            {
                //the name is like _accessStatusLightUserControl
                string healthStatesStatusLightUserControlName = GetHealthStateControlName(h.ToString());
                try
                {
                    TrafodionStatusLightUserControl statusLightUserControl=(TrafodionStatusLightUserControl)this.Controls.Find(healthStatesStatusLightUserControlName, true)[0];
                    bool displayStatus=SystemMetricChartConfigModel.Instance.GetHealthStatesLayerDisplayStatus(aConnectionDefn.Name, h);
                    statusLightUserControl.Visible=displayStatus;
                }
                catch(Exception ex)
                {
                    if(Logger.IsTracingEnabled)
                        Logger.OutputErrorLog("Failed to initialize HealthStates Panel. Details = " +ex.Message);
                    MessageBox.Show("Failed to initialize HealthStates Panel. Details= " + ex.Message + "\n\n", "Initialize Health States Panel", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        #endregion Constructor

        #region Public methods

        public void ClearHealthState(string healthLayer)
        {
            string healthStateControlName = GetHealthStateControlName(healthLayer);
            Control[] findedControl = this.Controls.Find(healthStateControlName, true);
            if (findedControl != null && findedControl.Length > 0)
            {
                TrafodionStatusLightUserControl healthStateControl = (TrafodionStatusLightUserControl)findedControl[0];
                healthStateControl.State = -1;
                healthStateControl.ToolTipText = Properties.Resources.DetailsNotYetAvailable;
            }
        }

        #endregion Public methods

        #region Private methods

        private string GetHealthStateControlName(string healthLayer)
        {
            return "_" + healthLayer.ToLower() + "StatusLightUserControl";
        }


        /// <summary>
        /// Event handler for panel resized event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _thePanel_Resize(object sender, EventArgs e)
        {
            Control control = sender as Control;
            Size size = new Size((control.Size.Width-70)/6, control.Size.Height - 15);
            _accessStatusLightUserControl.Size = _databaseStatusLightUserControl.Size = _foundationStatusLightUserControl.Size =
                _osStatusLightUserControl.Size = _serverStatusLightUserControl.Size = _storageStatusLightUserControl.Size = size;
        }

        #endregion Private methods
    }
}
