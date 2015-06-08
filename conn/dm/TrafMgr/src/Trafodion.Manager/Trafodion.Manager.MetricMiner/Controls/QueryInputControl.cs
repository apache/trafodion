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
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.MetricMiner.Controls
{
    /// <summary>
    /// The user control for Query Input Text
    /// </summary>
    public partial class QueryInputControl : UserControl
    {
        #region Fields

        public delegate void QueryInputTextChanged(object aSender, EventArgs e);
        public event QueryInputTextChanged OnQueryTextChanged;

        #endregion Fields

        #region Constructors

        public QueryInputControl()
        {
            InitializeComponent();
            _theQueryText.BackColor = _theQueryText.ReadOnly ? Color.LightGray : Color.White;
        }

        #endregion Constructors

        #region Properties

        public bool ReadOnly
        {
            get { return _theQueryText.ReadOnly; }
            set
            {
                _theQueryText.ReadOnly = value;
                _theQueryText.BackColor = _theQueryText.ReadOnly ? Color.LightGray : Color.White;
            }
        }

        public string QueryText
        {
            get { return _theQueryText.Text; }
            set { _theQueryText.Text = value; }
        }

        #endregion Properties

        #region Private methods

        private void _theQueryText_TextChanged(object sender, EventArgs e)
        {
            FireOnTextChanged(sender, e);
        }

        private void FireOnTextChanged(object sender, EventArgs e)
        {
            if (OnQueryTextChanged != null)
            {
                OnQueryTextChanged(this, e);
            }
        }

        #endregion Private methods

        #region Public metrods

        public void RedoTheQueryText()
        {
            _theQueryText.Redo();
        }

        public void UndoTheQueryText()
        {
            _theQueryText.Undo();
        }

        public bool CanTheQueryTextRedo()
        {
            return _theQueryText.CanRedo;
        }

        public bool CanTheQueryTextUndo()
        {
            return _theQueryText.CanUndo;
        }

        #endregion Public metrods
    }
}
