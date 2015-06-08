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

using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class GenericDataDisplayControl : UserControl, IDataDisplayControl
    {
        #region Fields
        
        private UniversalWidgetConfig _theConfig = null;
        private DataProvider _theDataProvider = null;
        private IDataDisplayHandler _theDataDisplayHandler = null;
        private Control _theControl = null;

        #endregion Fields

        #region Constructors

        public GenericDataDisplayControl()
        {
            InitializeComponent();

            // Froce control/handler & all child controls/handles creation.
            // Thus, enable to call "Invoke" method easily. 
            this.CreateControl();
        }

        public GenericDataDisplayControl(Control aControl)
            :this()
        {
            _thePanel.Controls.Clear();
            _theControl = aControl;
            _theControl.Dock = DockStyle.Fill;
            _thePanel.Controls.Add(_theControl);
        }

        #endregion Constructors

        #region Properties

        /// <summary>
        /// Property: DataProvider
        /// </summary>
        public DataProvider DataProvider
        {
            get { return _theDataProvider; }
            set { _theDataProvider = value; }
        }

        /// <summary>
        /// Property: UniversalWidgetConfiguration
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _theConfig; }
            set { _theConfig = value; }
        }

        /// <summary>
        /// Property: DataDisplayHandler
        /// </summary>
        public IDataDisplayHandler DataDisplayHandler
        {
            get { return _theDataDisplayHandler; }
            set { _theDataDisplayHandler = value; }
        }

        public Control TheControl
        {
            set
            {
                if (value != null)
                {
                    _thePanel.Controls.Clear();
                    _theControl = value;
                    _theControl.Dock = DockStyle.Fill;
                    _thePanel.Controls.Add(_theControl);
                }
            }
            get { return _theControl; }
        }

        /// <summary>
        /// Property: DrillDownManager
        /// </summary>
        public DrillDownManager DrillDownManager
        {
            get { return null; }
            set { }
        }

        #endregion Properties 

        #region Public methods

        /// <summary>
        /// To persist if any 
        /// </summary>
        public void PersistConfiguration()
        {
            //do nothing
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Anything need to be disposed when the object is disposed
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Dispose the result set control to free up resources.
                if (_theControl != null)
                {
                    _theControl.Dispose();
                    _theControl = null;
                }
            }
        }

        #endregion Private methods
    }
}


