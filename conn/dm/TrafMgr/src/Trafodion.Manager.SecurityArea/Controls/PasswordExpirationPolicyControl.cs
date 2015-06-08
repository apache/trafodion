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
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class PasswordExpirationPolicyControl : UserControl
    {
       #region Constructors
        public PasswordExpirationPolicyControl()
        {            
            InitializeComponent();
            _theExpiryDate.Value = DateTime.Now;
            this._theExpiryDateLabel.Text = "Expiry Date";
            this._theExpiryDateLabel.ShowRequired = false;

            this._theExpiresEveryLabel.Text = "Expires Every";
            this._theExpiresEveryLabel.ShowRequired = false;

            setPasswordExpiryUi();
            AddToolTips();
        }

        public PasswordExpirationPolicyControl(string aLabel) : this()
        {
            GroupboxText = aLabel;
        }
        #endregion

        #region Properties

        //Sets/Gets the value of the expiry days
        public int ExpiryDays
        {
            get
            {
                int expirationDays = 0;
                return(int.TryParse(_theExpiryDays.Text, out expirationDays)) ? expirationDays : 0;              
            }
            set
            {
                _theExpiryDays.Text = value.ToString();
            }
        }

        //Sets/Gets the value of the expiry date
        public string ExpiryDate
        {
            get
            {
                if (_theExpiryDate.Checked)
                {
                    //Save  OS client  Culture Info
                    System.Globalization.CultureInfo currentCultureInfo = System.Threading.Thread.CurrentThread.CurrentCulture;

                    //Create English culture info
                    System.Threading.Thread.CurrentThread.CurrentCulture = new System.Globalization.CultureInfo("en-US");

                    string strReturnDate = _theExpiryDate.Value.ToString("MMM dd yyyy");
                    //Restore current cultrue info as OS client.
                    System.Threading.Thread.CurrentThread.CurrentCulture = currentCultureInfo;

                    return strReturnDate;
                }
                else
                {
                    return "";
                }

            }

            set
            {
                try
                {
                    _theExpiryDate.Value = DateTime.Parse(value.Trim());
                    _theExpiryDate.Checked = true;
                }
                catch (Exception ex)
                {
                    _theExpiryDate.Checked = false;
                }
            }
        }

        //Sets the label(Text) property of the groupbox
        public string GroupboxText
        {
            set
            {
                _thePasswordExpirationGroupbox.Text = value;
            }
        }

        //sets/gets the values of the radio buttons
        public bool NoExpiry
        {
            get
            {
                return _theNoExpirationRadio.Checked;
            }

            set
            {
                if (value)
                {
                    _theNoExpirationRadio.Checked = value;
                }
                else
                {
                    _theUseExpirationRadio.Checked = true;
                }
                setPasswordExpiryUi();
            }
        }
 
        #endregion

        #region Private methods
        private void setPasswordExpiryUi()
        {
            if (_theNoExpirationRadio.Checked)
            {
                _theExpiryDate.Enabled = false;
                _theExpiryDays.Text = "";
                _theExpiryDays.Enabled = false;
            }
            else
            {
                _theExpiryDate.Enabled = true;
                _theExpiryDays.Enabled = true;
            }
        }

        private void AddToolTips()
        {
            _toolTip.SetToolTip(_theNoExpirationRadio, Utilities.TrafodionTextWrapper("Specifies that the password never expires.", 50));
            _toolTip.SetToolTip(_theUseExpirationRadio, Utilities.TrafodionTextWrapper("Specifies that the password expires on the schedule indicated by Expires Every n Days or Expiry Date.", 50));
            _toolTip.SetToolTip(_theExpiryDate, Utilities.TrafodionTextWrapper("Specifies the default password expiration date.", 50));
            _toolTip.SetToolTip(_theExpiryDays, Utilities.TrafodionTextWrapper("Specifies the interval that controls password expiration. Valid value is between 0 and 365.", 50));
        }

        private void _theNoExpirationRadio_CheckedChanged(object sender, EventArgs e)
        {
            setPasswordExpiryUi();
        }

        private void _theUseExpirationRadio_CheckedChanged(object sender, EventArgs e)
        {
            setPasswordExpiryUi();
        }
        #endregion

        private void _theExpiryDays_KeyPress(object sender, KeyPressEventArgs e)
        {
            e.Handled = !(Char.IsDigit(e.KeyChar) || Char.IsControl(e.KeyChar));
        }

        #region Public Methods

        #endregion
    }
}
