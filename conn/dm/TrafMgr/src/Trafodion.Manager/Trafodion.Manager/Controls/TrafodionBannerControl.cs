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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;


namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionBannerControl : UserControl
    {
        private ConnectionDefinition _connectionDefinition;
        public delegate void UpdateBanner();
        Timer _timer;

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
            set
            {
                _connectionDefinition = value;
                if (_connectionDefinition != null)
                {
                    UpdateBannerDisplay();
                }
                else
                {
                    descriptionTextBox.Text = "";
                    ConnectionDefToolTip.SetToolTip(descriptionTextBox, null);
                }
            }

        }
        public string FormattedDescription
        {
            get
            {
                if (_connectionDefinition != null)
                {
                    DateTime serverTime = DateTime.UtcNow + _connectionDefinition.ServerGMTOffset;
                    StringBuilder descBuilder = new StringBuilder();
                    descBuilder.AppendFormat("System : {0}, Host: {1} , Port: {2}", _connectionDefinition.Name, _connectionDefinition.Host, _connectionDefinition.Port);
                    //descBuilder.AppendFormat(", Platform Version : {0}", (string.IsNullOrEmpty(PlatformReleaseVersion) ? DefaultPlatformVersion : _platformReleaseVersion));
                    descBuilder.AppendFormat(string.IsNullOrEmpty(_connectionDefinition.PlatformReleaseVersion) ? "" : ", Platform Version : " + _connectionDefinition.PlatformReleaseVersion);
                    descBuilder.AppendLine();
                    descBuilder.Append("Directory Service User : " + _connectionDefinition.UserName);
                    descBuilder.Append(string.IsNullOrEmpty(_connectionDefinition.DatabaseUserName) ? "" : ", Database User : " + _connectionDefinition.DatabaseUserName);
                    descBuilder.Append(string.IsNullOrEmpty(_connectionDefinition.RoleName) ? "" : ", Role : " + _connectionDefinition.RoleName);
                    descBuilder.AppendLine();
                    //descBuilder.Append(_connectionDefinition.ConnectedDataSource.Length > 0 ? "Data Source : " + _connectionDefinition.ConnectedDataSource : "");
                    //descBuilder.Append(string.IsNullOrEmpty(_connectionDefinition.ConnectedDataSource) ? "" : ", ");
                    //descBuilder.Append(_connectionDefinition.ConnectedServiceName.Length > 0 ? "WMS Service : " + _connectionDefinition.ConnectedServiceName : "");
                    //descBuilder.Append(string.IsNullOrEmpty(_connectionDefinition.ConnectedServiceName) ? "" : ", ");
                    descBuilder.Append((_connectionDefinition.FullyQualifiedDefaultSchema.Length > 0) ? ("Server Local Time: " + serverTime.ToString("yyyy-MM-dd HH':'mm':'ss") + " " + _connectionDefinition.ServerTimeZoneName) : "");
                    return descBuilder.ToString();
                }
                else
                {
                    return "";
                }
            }
        }

        public bool ShowDescription
        {
            get { return descriptionTextBox.Visible; }
            set { descriptionTextBox.Visible = value; }
        }

        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, 
                                                    ConnectionDefinition.Reason aReason)
        {
            if (aReason == ConnectionDefinition.Reason.Tested
                        || aReason == ConnectionDefinition.Reason.Disconnected
                        || aReason == Connections.ConnectionDefinition.Reason.PlatformReleaseVersion)
            {
                if (_connectionDefinition != null && aConnectionDefinition != null)
                {
                    if (aConnectionDefinition.Name.Equals(_connectionDefinition.Name))
                    {
                        //descriptionTextBox.Text = _connectionDefinition.FormattedDescription;
                        //ConnectionDefToolTip.SetToolTip(descriptionTextBox, descriptionTextBox.Text);
                        //descriptionTextBox.SelectionStart = descriptionTextBox.Text.Length;
                        try
                        {
                            this.BeginInvoke(new UpdateBanner(UpdateBannerDisplay), new object[] { });
                        }
                        catch (Exception) { }
                    }
                }
            }
        }

        public void UpdateBannerDisplay()
        {
            if (_connectionDefinition != null)
            {
                int selectionStart = descriptionTextBox.SelectionStart;
                int selectionLength = descriptionTextBox.SelectionLength;
                descriptionTextBox.Text = FormattedDescription;
                descriptionTextBox.SelectionStart = selectionStart;
                descriptionTextBox.SelectionLength = selectionLength;
            }
            ConnectionDefToolTip.SetToolTip(descriptionTextBox, descriptionTextBox.Text);
            //descriptionTextBox.SelectionStart = descriptionTextBox.Text.Length;
        }

        public TrafodionBannerControl()
        {
            InitializeComponent();

            tableLayoutPanel1.BackColor = TrafodionColorTable.GripDarkColor;
            descriptionTextBox.BackColor = TrafodionColorTable.GripDarkColor;
            //tableLayoutPanel1.BackColor = TrafodionColorTable.ToolStripGradientEndColor;
            //descriptionTextBox.BackColor = TrafodionColorTable.ToolStripGradientEndColor;

            descriptionTextBox.ForeColor = TrafodionColorTable.ActiveCaptionText;
            productNameLabel.ForeColor = TrafodionColorTable.ActiveCaptionText;


            descriptionTextBox.Text = "";
            _timer = new Timer();
            _timer.Interval = 1000;
            _timer.Tick += new EventHandler(_timer_Tick);
            _timer.Start();

            ConnectionDefinition.Changed += ConnectionDefinition_Changed;
        }

        void _timer_Tick(object sender, EventArgs e)
        {
            UpdateBannerDisplay();
        }

        void MyDispose()
        {
            if (_timer != null)
            {
                _timer.Stop();
            }
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }
    }
}
