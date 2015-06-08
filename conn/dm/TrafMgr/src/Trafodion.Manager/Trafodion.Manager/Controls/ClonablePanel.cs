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

using System.ComponentModel;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.Framework.Controls
{
    /// <summary>
    /// Clonable panel to help DelayedPopulateClonableTabPage to 
    /// defer time-consuming trip for loading data. This shows the panel-name in Italics to indicate that panel can be Cloned. 
    /// </summary>
    [ToolboxItem(false)]
    public class ClonablePanel : TrafodionPanel, ICloneToWindow
    {

        ConnectionDefinition _connectionDefinition = null;

        /// <summary>
        /// Default constructor for the UI designer.
        /// </summary>
        public ClonablePanel()
        {
        }


        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        virtual public Control Clone()
        {
            return new ClonablePanel();
        }

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>A string</returns>
        virtual public string WindowTitle
        {
            get { return "Dummy"; } 
        }



        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
        }

        #endregion
    }
}
