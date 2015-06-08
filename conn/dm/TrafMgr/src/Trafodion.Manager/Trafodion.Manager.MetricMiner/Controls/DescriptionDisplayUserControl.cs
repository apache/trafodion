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

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class DescriptionDisplayUserControl : UserControl
    {
        private UniversalWidgetConfig _theConfig;
        public DescriptionDisplayUserControl()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Displays widget description in RTF format
        /// </summary>
        public void SetDescription(UniversalWidgetConfig aConfig)
        {
            _theConfig = aConfig;
            showDescription();
        }


        private void showDescription()
        {
            if (_theConfig != null)
            {
                int row = 0;

                StringBuilder sb = new StringBuilder();
                sb.AppendLine("<html><head></head><body><table width=\"100%\">");
                sb.AppendLine(getTableHeader());
                sb.AppendLine(getTableRow("Name", _theConfig.Name, row++));
                sb.AppendLine(getTableRow("Author", _theConfig.Author, row++));
                sb.AppendLine(getTableRow("Query Version", _theConfig.WidgetVersion, row++));
                sb.AppendLine(getTableRow("Server Version", _theConfig.ServerVersion, row++));
                sb.AppendLine(getTableRow("Description", GetHtmlFormattedString(_theConfig.Description), row++));
                sb.AppendLine(getTableRow("Last Execution Time", _theConfig.LastExecutionTime, row++));
                sb.AppendLine(getColumnsRow(row++));
                sb.AppendLine(getAssociatedReportRow(row++));
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
            return string.Format("<tr><th colspan=\"2\" style=\"font-family:Tahoma; font-weight:bold; font-size:12px; background-color:Silver; \" width=\"100%\" align=\"left\" valign=\"top\" >{0}</th></tr>", "Report Details");
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

        private string getColumnsRow(int row)
        {
            StringBuilder sb = new StringBuilder();
            string bgColor = "";
            if ((row % 2) == 0)
            {
            }
            else
            {
                bgColor = "background-color:WhiteSmoke;";
            }
            sb.AppendLine("<tr>");
            sb.AppendLine(string.Format("<td style=\"font-family:Tahoma; font-weight:bold; font-size:12px; {0}\" width=\"30%\" align=\"left\" valign=\"top\" > Columns </td>", bgColor));
            sb.AppendLine(string.Format("<td style=\"font-family:Tahoma; font-size:12px;  {0}\"  width=\"70%\" align=\"left\" valign=\"top\" >", bgColor));
            if (_theConfig.DataProviderConfig.ColumnMappings != null)
            {
                sb.AppendLine("<ul>");
                foreach (ColumnMapping cm in _theConfig.DataProviderConfig.ColumnMappings)
                {
                    if ((cm.ExternalName != null) && (cm.ExternalName.Trim().Length > 0))
                    {
                        sb.AppendLine(string.Format("<li>{0}", cm.ExternalName));
                    }
                }
                sb.AppendLine("</ul>");
            }
            else
            {
                sb.AppendLine("&nbsp;");
            }
            sb.AppendLine("</td></tr>");
            return sb.ToString();
        }


        private string getAssociatedReportRow(int row)
        {
            if ((_theConfig.AssociatedWidgets != null) && (_theConfig.AssociatedWidgets.Count > 0))
            {
                StringBuilder sb = new StringBuilder();
                string bgColor = "";
                if ((row % 2) == 0)
                {
                }
                else
                {
                    bgColor = "background-color:WhiteSmoke;";
                }
                sb.AppendLine("<tr>");
                sb.AppendLine(string.Format("<td style=\"font-family:Tahoma; font-weight:bold; font-size:12px; {0}\" width=\"30%\" align=\"left\" valign=\"top\" > Linked Reports </td>", bgColor));
                sb.AppendLine(string.Format("<td style=\"font-family:Tahoma; font-size:12px;  {0}\"  width=\"70%\" align=\"left\" valign=\"top\" >", bgColor));
                    sb.AppendLine("<ul>");
                    foreach (AssociatedWidgetConfig awc in _theConfig.AssociatedWidgets)
                    {
                        sb.AppendLine(string.Format("<li>{0}", WidgetRegistry.GetWidgetDisplayName(awc.CalledWidgetName)));
                    }
                    sb.AppendLine("</ul>");
                sb.AppendLine("</td></tr>");
                return sb.ToString();
            }
            return "";
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
        //The RTF capability is very basic. Hence for display only functions its
        //better to use HTML formatted text in the browser controls

        //private void showDescription()
        //{
        //    if (_theConfig != null)
        //    {
        //        StringBuilder sb = new StringBuilder();
        //        sb.AppendLine("{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033");
        //        sb.AppendLine("{\\fonttbl {\\f0\\fnil\\fcharset1 Tahoma;}}");
        //        sb.AppendLine("{\\colortbl ;\\red255\\green0\\blue0;\\red0\\green0\\blue0;}");

        //        sb.AppendLine(getTableRow("Name", _theConfig.Name));
        //        sb.AppendLine(getTableRow("Author", _theConfig.Author));
        //        sb.AppendLine(getTableRow("Version", _theConfig.WidgetVersion));
        //        sb.AppendLine(getTableRow("Description", _theConfig.Description));

        //        sb.Append(" }");
        //        _theDescriptionTextBox.Rtf = sb.ToString();
        //        _theDescriptionTextBox.ReadOnly = true;
        //    }
        //}

        //private string getTableRow(string key, string value)
        //{
        //    StringBuilder sb = new StringBuilder();
        //    sb.AppendLine();
        //    sb.Append(" \\trowd\\clbrdrt\\brdrs\\clbrdrl\\brdrs\\clbrdrb\\brdrs\\clbrdrr\\brdrs\\cellx"); sb.Append( _theDescriptionTextBox.Width / 3); sb.Append(" \\clbrdrt\\brdrs\\clbrdrl\\brdrs\\clbrdrb\\brdrs\\clbrdrr\\brdrs\\cellx"); sb.Append( _theDescriptionTextBox.Width * 2 / 3);
        //    sb.Append("{ \\f0\\b\\fs20 "); sb.Append(key); sb.Append(": \\intbl\\cell}"); sb.Append("{ \\f0\\fs20 "); sb.Append(value); sb.Append(" \\intbl\\cell}");
        //    sb.Append(" \\row ");
        //    return sb.ToString(); 
        //}
    }
}
