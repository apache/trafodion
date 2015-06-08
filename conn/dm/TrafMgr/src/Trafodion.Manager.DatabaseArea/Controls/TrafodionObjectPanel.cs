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

using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel that displays information about a SQL object.
    /// </summary>
    public class TrafodionObjectPanel : TrafodionPanel, ICloneToWindow
    {
        #region Fields

        private TrafodionObject _sqlMxObject;
        private string _titleSuffix;

        #endregion

        #region Properties

        /// <summary>
        /// Text that is to follow the object's name in the window title.
        /// </summary>
        public string TitleSuffix
        {
            get { return _titleSuffix; }
            set { _titleSuffix = value; }
        }

        /// <summary>
        /// The object whose information is to be displayed.
        /// </summary>
        protected TrafodionObject TheTrafodionObject
        {
            get { return _sqlMxObject; }
            set { _sqlMxObject = value; }
        }

        #endregion

        /// <summary>
        /// Default constructor used by the UI designer.
        /// </summary>
        public TrafodionObjectPanel()
        {
            TitleSuffix = "";
            TheTrafodionObject = null;
        }

        /// <summary>
        /// Create a panel that displays information about a SQL object.
        /// </summary>
        /// <param name="aTitleSuffix">Text will be appendeded to the object's name in a window title.</param>
        /// <param name="aTrafodionObject">The object whose information is to be displayed.</param>
        public TrafodionObjectPanel(string aTitleSuffix, TrafodionObject aTrafodionObject)
        {
            TitleSuffix = aTitleSuffix;
            TheTrafodionObject = aTrafodionObject;
        }

        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        virtual public Control Clone()
        {
            Label theLabel = new Label();
            theLabel.Text = "TrafodionObjectPanel";
            return theLabel;
        }

        #endregion

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>A string</returns>
        virtual public string WindowTitle
        {
            get { return TheTrafodionObject.VisibleAnsiName + " " + TitleSuffix; }
        }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return TheTrafodionObject.ConnectionDefinition; }
        }
    }
}
