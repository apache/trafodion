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

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel to display the primary key details of a Sql Object
    /// </summary>
    public class PrimaryKeyPanel : TrafodionObjectPanel
    {
        /// <summary>
        /// Constructs the panel with details from the Primary key object
        /// </summary>
        /// <param name="aTrafodionPrimaryKey">The primary key object, whose details are displayed in the panel</param>
        public PrimaryKeyPanel(TrafodionPrimaryKey aTrafodionPrimaryKey)
            : base(Properties.Resources.PrimaryKey, aTrafodionPrimaryKey)
        {

            PrimaryKeyAttributesDataGridView primaryKeyAttributesDataGridView
                = new PrimaryKeyAttributesDataGridView(null, aTrafodionPrimaryKey);
            primaryKeyAttributesDataGridView.Dock = DockStyle.Top;
            Controls.Add(primaryKeyAttributesDataGridView);

            PrimaryKeyColumnsDataGridView primaryKeyColumnsDataGridView
                = new PrimaryKeyColumnsDataGridView(null, aTrafodionPrimaryKey);
            primaryKeyColumnsDataGridView.Dock = DockStyle.Fill;
            Controls.Add(primaryKeyColumnsDataGridView);

            primaryKeyColumnsDataGridView.BringToFront();

        }

        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        override public Control Clone()
        {
            return new PrimaryKeyPanel(TheTrafodionObject as TrafodionPrimaryKey);
        }

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>A string</returns>
        override public string WindowTitle
        {
            get { return (TheTrafodionObject as TrafodionPrimaryKey).TrafodionTable.VisibleAnsiName + " " + TitleSuffix; }
        }

        #endregion

    }

}
