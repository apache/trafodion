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
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class RenameCodeFile : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        private Regex _validCharacters = new Regex(@"^[a-zA-Z0-9_.]+$");
        private Regex _validExtension = new Regex(@"^[a-zA-Z0-9_]+(\.(jar))*$");

        public string CodeFileName
        {
            get { return this._theCodeFileName.Text;}
        }


        public RenameCodeFile()
        {
            InitializeComponent();
        }

        private void _theCreateButton_Click(object sender, EventArgs e)
        {

        }

         private void _theCancelButton_Click(object sender, EventArgs e)
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }

        private void _theCodeFileName_TextChanged(object sender, EventArgs e)
        {
            string codeFileName = _theCodeFileName.Text.Trim();
            this._theRenameButton.Enabled = (codeFileName.Length > 0 && IsNameValid());
        }

        private bool IsNameValid()
        {
            if (_validCharacters.IsMatch(_theCodeFileName.Text.Trim()))
            {
                if(_validExtension.IsMatch(_theCodeFileName.Text.Trim()))
                {
                    _errorLabel.Text = "";
                    return true;
                }
                else
                {
                    _errorLabel.Text = Properties.Resources.InvalidExtensionForCodeFile;
                    return false;
                }
            }
            else
            {
                _errorLabel.Text = Properties.Resources.InvalidCharactersInCodeFileName;
                return false;
            }
        }
    }
}
