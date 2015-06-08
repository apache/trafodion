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

using System;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.LiveFeedFramework.Models;

namespace Trafodion.Manager.LiveFeedFramework.Controls
{
    /// <summary>
    /// The dialog for change livefeed broker properties.
    /// </summary>
    public partial class LiveFeedBrokerConfigDialog : TrafodionForm
    {
        #region Fields

        private LiveFeedConnection _theLiveFeedConnection = null;
        private ConnectionDefinition _theConnectionDefinition = null;
        private string _originalPortNumber = null;
        private string _originalSessionMointorTimer = null;

        #endregion Fields

        #region Properties

        public string Message
        {
            get { return _theMessagePanel.Text.Trim(); }
            set
            {
                _theMessagePanel.Text = value;
                _theMessagePanel.Visible = !string.IsNullOrEmpty(value);
            }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public LiveFeedBrokerConfigDialog(LiveFeedConnection aLiveFeedConnection)
        {
            _theLiveFeedConnection = aLiveFeedConnection;
            _theConnectionDefinition = aLiveFeedConnection.ConnectionDefn;
            InitializeComponent();
            _brokerIPAddress.Text = _theConnectionDefinition.LiveFeedHostName;
            _originalPortNumber = _theConnectionDefinition.LiveFeedPort;
            if (_originalPortNumber.Equals("-1") || string.IsNullOrEmpty(_originalPortNumber))
            {
                _portNumberComboBox.SelectedIndex = 0;
            }
            else
            {
                _portNumberComboBox.Text = _originalPortNumber;
            }

            _originalSessionMointorTimer = _sessionRetryTimer.Text = _theConnectionDefinition.LiveFeedRetryTimer;
            _applyButton.Enabled = false;

            this._portNumberComboBox.TextChanged += new System.EventHandler(this._brokerPortNumber_TextChanged);
            this._sessionRetryTimer.TextChanged += new System.EventHandler(this._sessionRetryTimer_TextChanged);

            Message = "";
        }

        #endregion Constructor

        #region Private methods

        /// <summary>
        /// event handler for apply button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _applyButton_Click(object sender, EventArgs e)
        {
            //if (_brokerPortNumber.Text.Trim() != _theConnectionDefinition.LiveFeedPort)
            //{
                //_theConnectionDefinition.LiveFeedPort = _brokerPortNumber.Text.Trim();
            //}

            //if (sessionMonitorTimer != _theLiveFeedConnection.BrokerConfiguration.SessionRetryTimer)
            //{
                _theConnectionDefinition.LiveFeedRetryTimer = _sessionRetryTimer.Text.Trim();
            //}

                if (_portNumberComboBox.Text.StartsWith("Default"))
                {
                    _theConnectionDefinition.LiveFeedPort = "-1";
                }
                else
                {
                    _theConnectionDefinition.LiveFeedPort = _portNumberComboBox.Text;
                }

            _theLiveFeedConnection.ReConfigure();

            DialogResult = System.Windows.Forms.DialogResult.OK;
            Close();
        }

        /// <summary>
        /// For cancel button 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _cancelButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private string DoValidate()
        {
            int port = 0;
            int sessionMonitorTimer = 0;
            string message = null;

            try
            {
              if (_portNumberComboBox.Text.StartsWith("Default"))
                {
                    port = -1;
                }
                else
                {
                    port = int.Parse(_portNumberComboBox.Text.Trim());
                    if (port <= 0 || port > 65535)
                    {
                        message = "Invalid port number, please enter a number between 1 and 65535.";
                    }
                }
            }
            catch (Exception ex)
            {
                message = "Invalid port number, please enter a number between 1 and 65535.";
            }

            try
            {
                sessionMonitorTimer = int.Parse(_sessionRetryTimer.Text.Trim());
                if (sessionMonitorTimer < 0)
                {
                    message = "Invalid session timeout value, please specify a postive number.";
                }
            }
            catch (Exception ex)
            {
                message = "Invalid session timeout value, please specify a postive number.";
            }

            if (string.IsNullOrEmpty(message) && 
                 (!_originalPortNumber.Equals(_portNumberComboBox.Text.Trim()) ||
                  !_originalSessionMointorTimer.Equals(_sessionRetryTimer.Text.Trim())))
            {
                _applyButton.Enabled = true;
            }
            else
            {
                 _applyButton.Enabled = false;
            }

            return message;
        }

        private void _brokerPortNumber_TextChanged(object sender, EventArgs e)
        {
            Message = DoValidate();
        }


        private void _sessionRetryTimer_TextChanged(object sender, EventArgs e)
        {
            Message = DoValidate();
        }

        /// <summary>
        /// When help button clicked.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.LiveFeedBrokerConfigurationHelper);
        }
        #endregion Private methods
    }
}
