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
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.SecurityArea.Controls
{
    /// <summary>
    /// System related security policies
    /// </summary>
    public partial class SecurityPoliciesSystemUserControl : UserControl
    {
        #region Fields

        private bool _changesPending = false;
        private Policies _thePolicies = null;
        private Policies.POLICY_SETTING _theCurrentSetting = Policies.POLICY_SETTING.SETTING_CUSTOMER;

        #endregion Fields

        #region Properties

        /// <summary>
        /// IsChangePending: Is there any change pending? 
        /// </summary>
        public bool IsChangePending
        {
            get { return _changesPending; }
            set { _changesPending = value; }
        }

        /// <summary>
        /// CurrentSetting: What is the current value setting?
        /// </summary>
        public Policies.POLICY_SETTING CurrentSetting
        {
            get { return _theCurrentSetting; }
            set { _theCurrentSetting = value; }
        }

        /// <summary>
        /// AutoDownloadCertificate: Automatic down system certificate to client machine? 
        /// </summary>
        public bool AutoDownloadCertificate
        {
            get { return _autoDownloadCheckBox.Checked; }
            set { _autoDownloadCheckBox.Checked = value; }
        }

        /// <summary>
        /// EnforceCertificateExpiry: Enforce certificate expiry? 
        /// </summary>
        public bool EnforceCertificateExpiry
        {
            get { return _enforceCertificateExpiryCheckBox.Checked; }
            set { _enforceCertificateExpiryCheckBox.Checked = value; }
        }

        /// <summary>
        /// AllowDownRevDrivers: Allow driver prior to 2.5 to connect?
        /// </summary>
        public bool AllowDownRevDrivers
        {
            get { return _driverCompatibilityCheckBox.Checked; }
            set { _driverCompatibilityCheckBox.Checked = value; }
        }

        /// <summary>
        /// OptimizeForLocalAccess: optimize for local access?
        /// </summary>
        public bool OptimizeForLocalAccess
        {
            get { return _optForLocalAccessCheckBox.Checked; }
            set { _optForLocalAccessCheckBox.Checked = value; }
        }
            
        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor with a model poicicies object.
        /// </summary>
        /// <param name="thePolicies"></param>
        public SecurityPoliciesSystemUserControl(Policies thePolicies)
        {
            InitializeComponent();
            AddToolTips();
            _thePolicies = thePolicies;
            Reload();
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Refresh all attruibutes. 
        /// Note: this will trigger a connection to the server to fetch the current attribute. 
        /// Note: The caller should have refresh the model object.
        /// </summary>
        public void Reload()
        {
            Reload(_thePolicies, Policies.POLICY_SETTING.SETTING_CUSTOMER);
        }

        /// <summary>
        /// Reload value settings.  
        /// </summary>
        /// <param name="aPolicies"></param>
        /// <param name="aSetting"></param>
        public void Reload(Policies aPolicies, Policies.POLICY_SETTING aSetting)
        {
            LoadAttributes(aPolicies);
            CurrentSetting = aSetting;
            _changesPending = false;
        }

        /// <summary>
        /// Update the policies model object with user's input.
        /// </summary>
        public void Update()
        {
            if (_changesPending  || CurrentSetting == Policies.POLICY_SETTING.SETTING_FACTORY_DEFAULT || CurrentSetting == Policies.POLICY_SETTING.SETTING_MOSE_SECURE)
            {
                SetAttributes(_thePolicies);
            }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Load attributes from the model policies. 
        /// </summary>
        /// <param name="policies"></param>
        private void LoadAttributes(Policies policies)
        {
            AutoDownloadCertificate = policies.AutoDownloadCertificate;
            EnforceCertificateExpiry = policies.EnforceCertificateExpiry;
            AllowDownRevDrivers = policies.AllowDownRevDrivers;
            OptimizeForLocalAccess = policies.OptimizeForLocalAccess;
        }

        /// <summary>
        /// Set the model policies attributes from user's input. 
        /// </summary>
        /// <param name="policies"></param>
        private void SetAttributes(Policies policies)
        {
            policies.AutoDownloadCertificate = AutoDownloadCertificate;
            policies.EnforceCertificateExpiry = EnforceCertificateExpiry;
            policies.AllowDownRevDrivers = AllowDownRevDrivers;
            policies.OptimizeForLocalAccess = OptimizeForLocalAccess;
        }

        /// <summary>
        /// Add tool tips.
        /// </summary>
        private void AddToolTips()
        {
            this._toolTip.SetToolTip(this._autoDownloadCheckBox, Utilities.TrafodionTextWrapper("Specifies whether the certificate is automatically downloaded if not already present when a client connects.", 50));
            this._toolTip.SetToolTip(this._enforceCertificateExpiryCheckBox, Utilities.TrafodionTextWrapper("Specifies whether the certificate expires.", 50));
            this._toolTip.SetToolTip(this._driverCompatibilityCheckBox, Utilities.TrafodionTextWrapper("Specifies whether to allow down-rev drivers to connect. Drivers prior to Release R2.5 do not support password encryption.", 50));
            this._toolTip.SetToolTip(this._optForLocalAccessCheckBox, Utilities.TrafodionTextWrapper("Specifies whether to optimize for local authentication.", 50));
        }

        #endregion Private methods
    }
}
