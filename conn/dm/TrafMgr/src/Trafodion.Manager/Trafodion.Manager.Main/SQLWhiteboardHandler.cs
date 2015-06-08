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
using System.Windows.Forms;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using System.Drawing;

namespace Trafodion.Manager.Main
{
    /// <summary>
    /// This class will manage the launching of the SQL Whiteboard
    /// </summary>
    class SQLWhiteboardHandler : IOptionsProvider
    {
        #region Fields

        private Control _sqlWhiteboardControl = null;

        #endregion Fields

        #region public methods

        /// <summary>
        /// Launch the SQL Whiteboard and loaded it with a list of statements.
        /// </summary>
        /// <param name="aListOfStatements"></param>
        /// <param name="from"></param>
        public void LaunchSQLWhiteboard(ArrayList aListOfStatements, string from, Size clientSize)
        {
            if (WindowsManager.Exists(TrafodionForm.TitlePrefix + global::Trafodion.Manager.Properties.Resources.SQLWhiteboard))
            {
                WindowsManager.Restore(TrafodionForm.TitlePrefix + global::Trafodion.Manager.Properties.Resources.SQLWhiteboard);
                WindowsManager.BringToFront(TrafodionForm.TitlePrefix + global::Trafodion.Manager.Properties.Resources.SQLWhiteboard);
            }
            else
            {
                Size size = clientSize;
                size.Width -= 50;
                size.Height -= 50;

                FloatingQueryWhiteboardControl floatingQueryWhiteboardControl = new FloatingQueryWhiteboardControl();
                TrafodionForm sqlWhiteBoard = WindowsManager.PutInWindow(size, floatingQueryWhiteboardControl,
                                global::Trafodion.Manager.Properties.Resources.SQLWhiteboard, true, TrafodionContext.Instance.CurrentConnectionDefinition);
                WindowsManager.ReplaceManagedWindowTitle(sqlWhiteBoard.Text, global::Trafodion.Manager.Properties.Resources.SQLWhiteboard);
                _sqlWhiteboardControl = (Control)floatingQueryWhiteboardControl;
                sqlWhiteBoard.Disposed += new EventHandler(SQLWhiteBoard_Disposed);
            }

            if (aListOfStatements != null)
            {
                ((FloatingQueryWhiteboardControl)_sqlWhiteboardControl).LoadStatements(aListOfStatements, from);
            }
        }

        /// <summary>
        /// Event handler for SQL Whiteboard disposed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void SQLWhiteBoard_Disposed(object sender, EventArgs e)
        {
            _sqlWhiteboardControl = null;
        }

        #endregion

        #region IOptionsProvider

        /// <summary>
        /// Property that the framework reads to get the options control
        /// </summary>
        public List<IOptionControl> OptionControls
        {
            get
            {
                return null;
            }
        }

        public Dictionary<String, IOptionObject> OptionObjects
        {
            get
            {
                Dictionary<string, IOptionObject> optionObjects = new Dictionary<string, IOptionObject>();
                optionObjects.Add(SQLWhiteboardOptions.OptionsKey, SQLWhiteboardOptions.GetOptions());
                return optionObjects;
            }
        }
        #endregion

    }
}
