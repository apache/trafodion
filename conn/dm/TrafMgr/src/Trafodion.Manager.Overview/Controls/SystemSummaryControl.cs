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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class SystemSummaryControl : UserControl
    {
        SystemAlertsUserControl _systemAlertsControl;
        ConnectionDefinition _connectionDefinition;
        WidgetCanvas widgetCanvas = new WidgetCanvas();
        TrafodionStatusLightUserControl _alarmLight;
        SystemMonitorControl _systemMonitorControl;
        WidgetContainer _alertsDetailContainer;

        public SystemSummaryControl(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            widgetCanvas.Dock = DockStyle.Fill;

            _connectionDefinition = aConnectionDefinition;

            _systemMonitorControl = new SystemMonitorControl();
            _systemMonitorControl.ConnectionDefn = aConnectionDefinition;
            
            _systemAlertsControl = new SystemAlertsUserControl(_connectionDefinition);
            
            //GridLayoutManager gridLayoutManager = new GridLayoutManager(5, 1);
            //gridLayoutManager.CellSpacing = 4;
            //widgetCanvas.LayoutManager = gridLayoutManager;

            //WidgetContainer systemStatusContainer = new WidgetContainer(widgetCanvas, _systemMonitorControl, "System Status");
            //systemStatusContainer.Size = new Size(300, 200);
            //systemStatusContainer.AllowDelete = false;
            //widgetCanvas.AddWidget(systemStatusContainer, new GridConstraint(0,0,1,1), -1);

            //_alertsDetailContainer = new WidgetContainer(widgetCanvas, _systemAlertsControl, Properties.Resources.Alerts);
            //_alertsDetailContainer.Size = new Size(600, 400);
            //_alertsDetailContainer.AllowDelete = false;
            //widgetCanvas.AddWidget(_alertsDetailContainer, new GridConstraint(1, 0, 4, 1), -1);
            //_alertsDetailContainer.Visible = false;

            //this.widgetCanvas.InitializeCanvas();

            //Controls.Add(widgetCanvas);
            _systemMonitorControl.Dock = DockStyle.Fill;
            Controls.Add(_systemMonitorControl);
        }

        void _alarmLight_MouseClickLight(object sender, EventArgs args)
        {
            if (WindowsManager.Exists(TrafodionForm.TitlePrefix + _systemAlertsControl.ConnectionDefn.Name + " : " + Properties.Resources.Alerts))
            {
                WindowsManager.BringToFront(TrafodionForm.TitlePrefix + _systemAlertsControl.ConnectionDefn.Name + " : " + Properties.Resources.Alerts);
            }
            else
            {
                Control alertsClone = _systemAlertsControl.Clone();
                WindowsManager.PutInWindow(new Size(800, 600), alertsClone, Properties.Resources.Alerts, _systemAlertsControl.ConnectionDefn);
            }
        }

    }
}
