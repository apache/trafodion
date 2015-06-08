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
    class ViewAttributesPanel : TrafodionObjectPanel
    {
        #region Fields

        private ViewAttributesDataGridView _dataGridView;

        #endregion

        /// <summary>
        /// Create an Attributes panel that displays a view's attributes.
        /// </summary>
        /// <param name="aTrafodionView">The view whose attributes are to be displayed.</param>
        public ViewAttributesPanel(TrafodionView aTrafodionView)
            : base(Properties.Resources.Attributes, aTrafodionView)
        {
            Load();
        }

        private void Load()
        {
            Controls.Clear();
            _dataGridView = new ViewAttributesDataGridView(null, TheTrafodionObject as TrafodionView);
            _dataGridView.TheHeaderText = WindowTitle;
            _dataGridView.Dock = DockStyle.Fill;
            Controls.Add(_dataGridView);
            _dataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }

        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        public override System.Windows.Forms.Control Clone()
        {
            return new ViewAttributesPanel(TheTrafodionObject as TrafodionView);
        }

        #endregion
    }
}
