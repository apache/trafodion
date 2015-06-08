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

using System.Data.Odbc;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{

    /// <summary>
    /// A panel that shows DDL info for a SQL object.
    /// </summary>
    public class TrafodionObjectDDLPanel : TrafodionObjectPanel
    {
        private Trafodion.Manager.DatabaseArea.Queries.Controls.SqlStatementTextBox _ddlTextBox;
        /// <summary>
        /// Constructs that panel to display DDL information for the SQL object.
        /// </summary>
        /// <param name="aTrafodionObject">The object whose information is to be displayed.</param>
        public TrafodionObjectDDLPanel(TrafodionObject aTrafodionObject)
            : base(Properties.Resources.DDL, aTrafodionObject)
        {
            InitializeComponent();
            Load();
        }

        /// <summary>
        /// Tell the panel to populate or repopulate itself.
        /// </summary>
        virtual protected void Load()
        {
            //Get the DDL Text from the object and set it in the text box control
            try
            {
                _ddlTextBox.Text = TheTrafodionObject.DDLText;
            }
            catch (OdbcException ex)
            {
                _ddlTextBox.Text = ex.Message;
            }
        }

        /// <summary>
        /// Clone the DDL panel
        /// </summary>
        /// <returns></returns>
        override public Control Clone()
        {
            return new TrafodionObjectDDLPanel(TheTrafodionObject);
        }

        private void InitializeComponent()
        {
            this._ddlTextBox = new Trafodion.Manager.DatabaseArea.Queries.Controls.SqlStatementTextBox();
            this.SuspendLayout();
            // 
            // _ddlTextBox
            // 
            this._ddlTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._ddlTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._ddlTextBox.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._ddlTextBox.Location = new System.Drawing.Point(0, 0);
            this._ddlTextBox.Name = "_ddlTextBox";
            this._ddlTextBox.ReadOnly = true;
            this._ddlTextBox.Size = new System.Drawing.Size(200, 100);
            this._ddlTextBox.TabIndex = 0;
            this._ddlTextBox.Text = "";
            this._ddlTextBox.WordWrap = true;
            // 
            // TrafodionObjectDDLPanel
            // 
            this.Controls.Add(this._ddlTextBox);
            this.ResumeLayout(false);

        }
    }

}
