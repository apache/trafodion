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

namespace Trafodion.Manager.UniversalWidget.Controls
{
    /// <summary>
    /// This class shall be used to display Universal widgets
    /// </summary>
    public partial class UniversalWidgetCanvas : UserControl
    {
        public UniversalWidgetCanvas()
        {
            InitializeComponent();
        }

        public ILayoutManager LayoutManager
        {
            get { return this._theUniversalWidgetCanvas.LayoutManager; }
            set { this._theUniversalWidgetCanvas.LayoutManager = value; }
        }

        public WidgetContainer AddWidget(GenericUniversalWidget aReportWidget, string name, string title)
        {
            WidgetContainer widgetContainer = new WidgetContainer(this._theUniversalWidgetCanvas, aReportWidget, name);
            widgetContainer.Size = new Size(800, 600);
            widgetContainer.Name = name;
            widgetContainer.Text = title;
            widgetContainer.AllowDelete = false;
            this._theUniversalWidgetCanvas.AddWidget(widgetContainer);
            aReportWidget.WidgetContainer = widgetContainer;
            return widgetContainer;
        }

        public WidgetContainer AddWidget(GenericUniversalWidget aReportWidget, string name, string title, Object aLayoutConstraint, int idx)
        {
            WidgetContainer widgetContainer = new WidgetContainer(this._theUniversalWidgetCanvas, aReportWidget, name);
            widgetContainer.Size = new Size(800, 600);
            widgetContainer.Name = name;
            widgetContainer.Text = title;
            widgetContainer.AllowDelete = false;
            this._theUniversalWidgetCanvas.AddWidget(widgetContainer, aLayoutConstraint, idx);
            aReportWidget.WidgetContainer = widgetContainer;
            return widgetContainer;
        }

        public WidgetContainer AddUserControl(UserControl aUserControl, string name, string title, Object aLayoutConstraint, int idx)
        {
            WidgetContainer widgetContainer = new WidgetContainer(this._theUniversalWidgetCanvas, aUserControl, name);
            widgetContainer.Size = new Size(800, 600);
            widgetContainer.Name = name;
            widgetContainer.Text = title;
            widgetContainer.AllowDelete = false;
            this._theUniversalWidgetCanvas.AddWidget(widgetContainer, aLayoutConstraint, idx);
            return widgetContainer;
        }
    }
}
