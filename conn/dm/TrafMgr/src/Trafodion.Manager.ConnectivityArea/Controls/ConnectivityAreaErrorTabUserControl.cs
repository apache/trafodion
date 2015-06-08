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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.ConnectivityArea.Model;
using System.Data;
using System.Collections.Generic;


namespace Trafodion.Manager.ConnectivityArea.Controls
{
 
    public partial class ConnectivityAreaErrorTabUserControl : UserControl, ICloneToWindow
    {
        #region Fields
        NDCSObject _NdcsObject;
        #endregion Fields

        #region Properties
        public NDCSObject NdcsObject
        {
            get { return _NdcsObject; }
            set { _NdcsObject = value; }
        }
        #endregion Properties
        
        public ConnectivityAreaErrorTabUserControl(NDCSObject ndcsObject)
        {
            this._NdcsObject = ndcsObject;
            InitializeComponent();
        }

        public ConnectivityAreaErrorTabUserControl(NDCSObject ndcsObject, string errorText)
        {
            this._NdcsObject = ndcsObject;
            InitializeComponent();
            //this.TrafodionLabel1.Text = errorText;
            this.TrafodionTextBox1.Text = errorText;
        }

        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            return new ConnectivityAreaErrorTabUserControl(this._NdcsObject);

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get { return Properties.Resources.ActiveSystemSummary + " - " + this._NdcsObject.ConnectionDefinition.Name; }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return this._NdcsObject.ConnectionDefinition; }
        }

        #endregion

    }
}
