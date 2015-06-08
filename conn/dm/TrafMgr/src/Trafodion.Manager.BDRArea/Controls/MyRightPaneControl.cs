// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Controls;


namespace Trafodion.Manager.BDRArea.Controls
{
    public partial class MyRightPaneControl : UserControl
    {
        #region Fields
        BDRAreaNavigator _navigationControl;
        //Display widget control when system is connected
        MyWidgetControl _myWidgetControl;
        //Display fix system user control when system is not connected
        FixSystemUserControl _fixSystemUserControl;
        MySystemsUserControl _mySystemsUserControl;
        #endregion Fields

        #region Properties
        public BDRAreaNavigator TheNavigationControl
        {
            get { return _navigationControl; }
            set
            {
                //If current _navigationControl is not null,
                //unregister the existing select event handler
                if (_navigationControl != null &&
                _navigationControl.TheNavigationTreeView != null)
                {
                    _navigationControl.TheNavigationTreeView.Selected -=
                            TheNavigationTreeView_Selected;
                }
                _navigationControl = value;
                //Add a listener to the Navigation Tree View
                //selection events
                if (_navigationControl != null &&
                _navigationControl.TheNavigationTreeView != null)
                {
                    _navigationControl.TheNavigationTreeView.Selected +=
                            TheNavigationTreeView_Selected;
                }
            }
        }
        #endregion Properties

        void TheNavigationTreeView_Selected(
                Trafodion.Manager.Framework.Navigation.NavigationTreeNode aTreeNode)
        {
            Controls.Clear();
            if (aTreeNode is NavigationTreeConnectionsFolder)
            {
                //If "My Systems" root folder is selected
                //Display the MySystemsUserControl and show all configured systems
                if (_mySystemsUserControl == null)
                {
                    _mySystemsUserControl = new MySystemsUserControl(true);
                    _mySystemsUserControl.Dock = DockStyle.Fill;
                }
                Controls.Add(_mySystemsUserControl);
            }
            else
            {
                ConnectionDefinition theConnectionDefinition =
                aTreeNode.TheConnectionDefinition;
                if (theConnectionDefinition != null &&
                    theConnectionDefinition.TheState !=
                    ConnectionDefinition.State.TestSucceeded)
                {
                    //If selected system is not connected
                    if (_fixSystemUserControl == null)
                    {
                        _fixSystemUserControl = new FixSystemUserControl();
                        _fixSystemUserControl.Dock = DockStyle.Fill;
                    }
                    _fixSystemUserControl.TheConnectionDefinition = theConnectionDefinition;
                    Controls.Add(_fixSystemUserControl);
                }
                else
                {
                    //System is connected. Display the myWidgetControl
                    if (_myWidgetControl == null)
                    {
                        _myWidgetControl = new MyWidgetControl();
                        _myWidgetControl.Dock = DockStyle.Fill;
                    }
                    _myWidgetControl.TheConnectionDefinition = theConnectionDefinition;
                    Controls.Add(_myWidgetControl);
                }
            }
        }

        public MyRightPaneControl()
        {
            InitializeComponent();
        }
    }
}
