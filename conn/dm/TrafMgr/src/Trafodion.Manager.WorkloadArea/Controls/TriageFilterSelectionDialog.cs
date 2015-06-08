//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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

using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using System.Windows.Forms;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class TriageFilterSelectionDialog : TrafodionFilterSelection
    {
        #region Constants
        private const string LABEL_CLIENT_ID = "Clients";
        private const string LABEL_APP_ID = "Applications";
        private const string LABEL_DATASOURCE = "DataSources";
        private const string LABEL_USER_NAME = "Users";

#if UNUSED_CODE
		private const string LABEL_ERROR = "Error";
		private const string LABEL_TYPE = "Type";
		private const string LABEL_STATE = "State";
		private const string LABEL_ELAPSED_TIME = "Elapsed";
		private const string LABEL_START_TIME = "Start";
		private const string LABEL_END_TIME = "End";
#endif

        #endregion Constants

        #region Members

        TriageHelper _theTriageHelper = null;

        #endregion

        #region Properties

        #endregion Properties

        public TriageFilterSelectionDialog(string title, TriageHelper aTriageHelper, object value)
            : base(title, null, value)
        {
            InitializeComponent();
            _theTriageHelper = aTriageHelper;
            setupComponents();
        }


        private void setupComponents()
        {
            TriageFilterQueryData queryData = null;
            if (null != _theTriageHelper)
                queryData = _theTriageHelper.FilterQueryData;


            if (Title.Equals(LABEL_APP_ID))
            {
                if ((null != queryData) && (null != queryData.ApplicationIDs))
                    addObjectsAvail(queryData.ApplicationIDs.ToArray());

                //addObjectsSelected();
            }
            else if (Title.Equals(LABEL_CLIENT_ID))
            {
                if ((null != queryData) && (null != queryData.ClientIDs))
                    addObjectsAvail(queryData.ClientIDs.ToArray());

                //addObjectsSelected();
            }
            else if (Title.Equals(LABEL_DATASOURCE))
            {
                if ((null != queryData) && (null != queryData.DataSources))
                    addObjectsAvail(queryData.DataSources.ToArray());

                //addObjectsSelected();
            }
            else if (Title.Equals(LABEL_USER_NAME))
            {
                if ((null != queryData) && (null != queryData.UserNames))
                    addObjectsAvail(queryData.UserNames.ToArray());

                //addObjectsSelected();
            }
        }

        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TriageFilterSelectionDialog));
            this.SuspendLayout();
            // 
            // TriageFilterSelectionDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.ClientSize = new System.Drawing.Size(594, 298);
            this.Name = "TriageFilterSelectionDialog";
            this.ResumeLayout(false);

        }

        protected override bool IsValid()
        {
            if (SelectedListCount > 100)
            {
                MessageBox.Show("You cannot select more than 100 items. Plese remove some items.", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return false;
            }
            return true;
        }
    }
}
