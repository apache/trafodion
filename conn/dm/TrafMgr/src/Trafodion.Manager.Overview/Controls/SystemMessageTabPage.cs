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
using System.Text;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.OverviewArea.Models;

namespace Trafodion.Manager.OverviewArea.Controls
{
    class SystemMessageTabPage : Trafodion.Manager.Framework.Controls.DelayedPopulateClonableTabPage
    {
        private SystemMessageControl _systemMessageControl;

        public SystemMessageControl SystemMessageControl
        {
            get { return _systemMessageControl; }
        }
        private ConnectionDefinition _connectionDefinition;

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
            set 
            { 
                _connectionDefinition = value;

                if (_systemMessageControl != null)
                {
                    _systemMessageControl.ConnectionDefn = value;
                }
            }
        }
        

        public SystemMessageTabPage()
        :this(Properties.Resources.SystemMessage)
        {
        }

        public SystemMessageTabPage(string aTitle)
        {
            this.Text = aTitle;
            Controls.Clear();
            _systemMessageControl = new SystemMessageControl();
            _systemMessageControl.Dock = System.Windows.Forms.DockStyle.Fill;
            Controls.Add(_systemMessageControl);
        }

        protected override void Populate()
        {
            _systemMessageControl.SystemMessage = SystemMessage.GetSystemMessage(this._connectionDefinition);
        }
    }
}
