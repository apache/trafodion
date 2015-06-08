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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.DatabaseArea.Queries;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class PrametersDisplayUserControl : UserControl
    {
        public PrametersDisplayUserControl()
        {
            InitializeComponent();
        }



        public void ShowParameters(List<ReportParameter> parameters)
        {
            if (parameters != null)
            {
                int row = 0;

                StringBuilder sb = new StringBuilder();
                sb.AppendLine("<html><head></head><body><table width=\"100%\">");
                sb.AppendLine(getTableHeader());
                foreach (ReportParameter param in parameters)
                {
                    if (! param.IsInternal)
                    {
                        sb.AppendLine(getTableRow(param.DisplayName, param.Value.ToString(), row++));
                    }
                }
                sb.AppendLine("</table></body></html>");

                //clear the last browser and dispose it
                this.Controls.Clear();
                if (_theBrowser != null)
                {
                    _theBrowser.Dispose();
                }

                //Repopulate the control with a new browser
                _theBrowser = new WebBrowser();
                this._theBrowser.AllowNavigation = false;
                this._theBrowser.Dock = System.Windows.Forms.DockStyle.Fill;
                this._theBrowser.Location = new System.Drawing.Point(0, 0);
                this._theBrowser.MinimumSize = new System.Drawing.Size(20, 20);
                this._theBrowser.Name = "_theBrowser";
                this._theBrowser.ScrollBarsEnabled = true;
                this._theBrowser.Size = new System.Drawing.Size(263, 495);
                this._theBrowser.TabIndex = 1;
                _theBrowser.DocumentText = sb.ToString();
                this.Controls.Add(_theBrowser);
            }
        }

        private string getTableHeader()
        {
            return string.Format("<tr><th colspan=\"2\" style=\"font-family:Tahoma; font-weight:bold; font-size:12px; background-color:Silver; \" width=\"100%\" align=\"left\" valign=\"top\" >{0}</th></tr>", "Report Parameters");
        }

        private string getTableRow(string key, string value, int row)
        {
            value = ((value != null) && (value.Trim().Length > 0)) ? value : "&nbsp;";
            if ((row % 2) == 0)
            {
                return string.Format("<tr style=\"border-bottom:dotted; \"><td style=\"font-family:Tahoma; font-weight:bold; font-size:12px;\" width=\"30%\" align=\"left\" valign=\"top\" >{0}</td><td style=\"font-family:Tahoma; font-size:12px; \"  width=\"70%\" align=\"left\" valign=\"top\" >{1}</td></tr>", key, value);
            }
            else
            {
                return string.Format("<tr style=\"border-bottom:dotted; \"><td style=\"font-family:Tahoma; font-weight:bold; font-size:12px; background-color:WhiteSmoke;\" width=\"30%\" align=\"left\" valign=\"top\" >{0}</td><td style=\"font-family:Tahoma; font-size:12px;  background-color:WhiteSmoke;\"  width=\"70%\" align=\"left\" valign=\"top\" >{1}</td></tr>", key, value);
            }

        }

        private string GetHtmlFormattedString(string str)
        {
            if (str != null)
            {
                str = str.Replace("\r\n", "<br>");
                str = str.Replace("\r", "<br>");
                str = str.Replace("\n", "<br>");
                return str;
            }
            return "&nbsp;";
        }
    }
}
