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
using System.Collections;
using System.Text;
using TenTec.Windows.iGridLib;
using System.Data;
using System.Drawing;
using System.Windows.Forms;

//XML stuff
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using Trafodion.Manager.WorkloadArea.Controls;

namespace Trafodion.Manager.WorkloadArea.Model
{
    public class ClientRuleAlert : ICloneable
    {        
        public ClientRuleAlert()
        {
            this.foregroundColor = null;
            this.backgroundColor = null;
            this.ShowAlertBox = false;
            this.logToFile = false;
            this.fileToLog = "";

            m_ActiveAlertDict = new Dictionary<AlertType, bool>();
            foreach (AlertType alert in Enum.GetValues(typeof(AlertType)))
            {
                m_ActiveAlertDict[alert] = false;
            }
        }

        private IDictionary<AlertType, bool> m_ActiveAlertDict;
        [XmlIgnore()]
        public IDictionary<AlertType, bool> ActiveAlertDict
        {
            get { return m_ActiveAlertDict; }
            set { m_ActiveAlertDict = value; }
        }

        private FileStream logStream;
        private StreamWriter logWriter;

        private const string ForeColorColName = "ForegroundColor";
        private const string BackColorColName = "BackgroundColor";
        private const string CellColorColName = "ViolatorColor";
        private const string CellNamesColName = "ViolatorNames";

        //Action Vars
        private string foregroundColor;
        private string backgroundColor;
        private string violatorColor;
        private bool logToFile;
        private string fileToLog = "";


        //Might not need
        private ArrayList violatorNames;

        //****** Properties Begin *************
        public string ForegroundColor
        {
            get { return foregroundColor; }
            set { foregroundColor = value; }
        }

        public bool LogToFile
        {
            get { return logToFile; }
            set { logToFile = value; }
        }

        public string FileToLog
        {
            get { return fileToLog; }
            set { fileToLog = value; }
        }

        public string BackgroundColor
        {
            get { return backgroundColor; }
            set { backgroundColor = value; }
        }

        public string ViolatorColor
        {
            get { return violatorColor; }
            set { violatorColor = value; }
        }

        public bool ShowAlertBox = false;
        public bool CustomBackground = false;
        public bool CustomForeground = false;
        public bool CustomCellBackground = false;

        //****** Properties End *************

        private bool bComplete = false;
        public bool isComplete
        {
            get { return bComplete; }
            set { bComplete = value; }
        }

        public bool isAlertActive(AlertType at)
        {
            return m_ActiveAlertDict[at];
        }


        public bool LoadAlertDictionary()
        {
            m_ActiveAlertDict[AlertType.BACK_COL] = CustomBackground;
            m_ActiveAlertDict[AlertType.FORE_COL] = CustomForeground;
            m_ActiveAlertDict[AlertType.CELL_COL] = CustomCellBackground;
            m_ActiveAlertDict[AlertType.POPUP] = ShowAlertBox;
            m_ActiveAlertDict[AlertType.LOG] = LogToFile;
            return true;
        }

        public bool saveAlertDictionary()
        {
            CustomBackground = m_ActiveAlertDict[AlertType.BACK_COL];
            CustomForeground = m_ActiveAlertDict[AlertType.FORE_COL];
            CustomCellBackground = m_ActiveAlertDict[AlertType.CELL_COL];
            ShowAlertBox = m_ActiveAlertDict[AlertType.POPUP];
            LogToFile = m_ActiveAlertDict[AlertType.LOG];
            return true;
        }

        public bool AlertTypeIsComplete(AlertType at)
        {
            switch (at)
            {
                case AlertType.BACK_COL:
                    return (this.backgroundColor != null && !this.backgroundColor.Equals(""));
                case AlertType.FORE_COL:
                    return (this.foregroundColor != null && !this.foregroundColor.Equals(""));
                case AlertType.CELL_COL:
                    return (this.violatorColor != null && !this.violatorColor.Equals(""));
                case AlertType.LOG:
                    return (this.fileToLog != null && !this.fileToLog.Equals(""));

                default:
                    return true;
            }
        }



        public string getValue(AlertType at)
        {
            switch (at)
            {
                case AlertType.BACK_COL:
                    return MatchKnownColor(Color.FromArgb(int.Parse(this.backgroundColor))).ToString();
                case AlertType.FORE_COL:
                    return MatchKnownColor(Color.FromArgb(int.Parse(this.foregroundColor))).ToString();
                case AlertType.CELL_COL:
                    return MatchKnownColor(Color.FromArgb(int.Parse(this.violatorColor))).ToString();
                case AlertType.POPUP:
                    return this.ShowAlertBox.ToString();
                case AlertType.LOG:
                    return this.fileToLog.ToString();
                default:
                    return "";
            }
        }

        public static KnownColor MatchKnownColor(Color c)
        {
            int mindist = int.MaxValue;
            int dist = 0;
            int best = 28;

            // loop through all known colors and try to find the best match
            // using eucledian distance (R^2+G^2+B^2)
            for (int i = 28; i < 168; i++)
            {
                Color knc = Color.FromKnownColor((KnownColor)i);
                dist = (knc.R - c.R) * (knc.R - c.R) + (knc.G - c.G) * (knc.G - c.G) + (knc.B - c.B) * (knc.B - c.B);
                if (dist < mindist)
                {
                    mindist = dist;
                    best = i;
                }
            }

            return (KnownColor)best;
        }


        public bool SetAlert(AlertType at, bool checkState)
        {
            if (at.Equals(AlertType.LOG))
            {
                this.logToFile = checkState;
            }
            if (at.Equals(AlertType.POPUP))
            {
                this.ShowAlertBox = checkState;
            }
            return (m_ActiveAlertDict[at] = checkState);//!ActiveAlertDict[at]);
        }



        public bool Execute(DataRow[] affectedRows, string RuleName)
        {
            if ((this.m_ActiveAlertDict[AlertType.LOG] && !string.IsNullOrEmpty(fileToLog)))
            {
                logStream = new FileStream(this.fileToLog, FileMode.Append);
                logWriter = new StreamWriter(logStream);
            }

            for (int i = 0; i < affectedRows.Length; i++)
            {
                if (this.m_ActiveAlertDict[AlertType.FORE_COL] && null != foregroundColor)
                    affectedRows[i][ForeColorColName] = foregroundColor;
                if (this.m_ActiveAlertDict[AlertType.BACK_COL] && null != backgroundColor)
                    affectedRows[i][BackColorColName] = backgroundColor;

                //log queries
                if (this.m_ActiveAlertDict[AlertType.LOG] &&  !string.IsNullOrEmpty(fileToLog))
                {
                    LogQuery(affectedRows[i], RuleName);
                }
            }

            if ((this.m_ActiveAlertDict[AlertType.LOG] && !string.IsNullOrEmpty(fileToLog)))
            {
                logWriter.Close();
            }

            //*****************************************************************
            return true;
        }

        public bool Execute(DataRow[] affectedRows, ArrayList Conditions)
        {
            if (this.m_ActiveAlertDict[AlertType.CELL_COL] && null != violatorColor && Conditions.Count > 0)
            {
                string ViolatorColumns = "|";
                foreach (ClientRuleCondition rc in Conditions)
                {
                    if (rc.isEnabled)
                        ViolatorColumns += rc.QueryProperty.ToString() + ",";
                }
                for (int i = 0; i < affectedRows.Length; i++)
                {
                    affectedRows[i][CellColorColName] += "|" + violatorColor;
                    affectedRows[i][CellNamesColName] += ViolatorColumns;
                }
            }
            return true;
        }


        private void LogQuery(DataRow dr, string RuleName)
        {
            try
            {
                // write some text to the file
                logWriter.WriteLine(dr["QUERY_ID"].ToString() + "," + DateTime.Now + "," + RuleName);
                logWriter.Flush();
            }
            catch
            {
                this.m_ActiveAlertDict[AlertType.LOG] = false;
                MessageBox.Show("\nError: Unable to log rule violation to file.\n" +
                                "Problem: \t Failed to log details about rule violations to the log file.\n\n" +
                                "Solution: \t Please see error details for recovery information.\n\n" +
                                "Details: \t LogFile=" + this.fileToLog + "\n\n",
                                "Error Logging Violations", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }


        public bool updateValue(AlertType at)
        {
            bool isSuccess = false;

            if (at == AlertType.LOG)
            {
                SaveFileDialog saveFileDialog1 = new SaveFileDialog();
                saveFileDialog1.Title = "Select Log File";

                saveFileDialog1.DefaultExt = "txt";
                saveFileDialog1.Filter = "Text Files (*.txt)|*.txt";

                if (this.fileToLog != null && this.fileToLog != "")
                {
                    saveFileDialog1.FileName = this.fileToLog;
                }
                DialogResult result = saveFileDialog1.ShowDialog();
                if (result == DialogResult.OK)
                {
                    this.fileToLog = saveFileDialog1.FileName;
                    isSuccess = true;
                }
            }

            if (at == AlertType.FORE_COL || at == AlertType.BACK_COL || at == AlertType.CELL_COL)
            {
                string CurrentCol = this.foregroundColor;
                if (at == AlertType.BACK_COL)
                    CurrentCol = this.backgroundColor;
                if (at == AlertType.CELL_COL)
                    CurrentCol = this.violatorColor;

                ColorDialog cd = new ColorDialog();
                cd.SolidColorOnly = true;
                cd.AnyColor = false;
                cd.AllowFullOpen = false;
                if (null != CurrentCol)
                    cd.Color = Color.FromArgb(int.Parse(CurrentCol));

                if (cd.ShowDialog() == DialogResult.OK)
                {
                    CurrentCol = cd.Color.ToArgb().ToString();
                    if (at == AlertType.BACK_COL)
                        this.backgroundColor = CurrentCol;
                    else if (at == AlertType.FORE_COL)
                        this.foregroundColor = CurrentCol;
                    else
                        this.violatorColor = CurrentCol;

                    isSuccess = true;
                }
            }

            if (isSuccess)
                this.m_ActiveAlertDict[at] = true;

            return isSuccess;
        }

        private IDictionary CloneDictionary()
        {
            IDictionary tempDict = new Dictionary<AlertType, bool>();
            foreach (AlertType alert in Enum.GetValues(typeof(AlertType)))
            {
                tempDict[alert] = m_ActiveAlertDict[alert];
            }
            return tempDict;
        }

        public object Clone()
        {
            ClientRuleAlert RA = new ClientRuleAlert();
            RA.isComplete = this.isComplete;
            RA.FileToLog = this.fileToLog;
            RA.LogToFile = this.logToFile;
            RA.BackgroundColor = this.backgroundColor;
            RA.ForegroundColor = this.foregroundColor;
            RA.ShowAlertBox = this.ShowAlertBox;
            RA.ActiveAlertDict = (Dictionary<AlertType, bool>)CloneDictionary();
            RA.CustomBackground = this.CustomBackground;
            RA.CustomCellBackground = this.CustomCellBackground;
            RA.CustomForeground = this.CustomForeground;
            RA.ViolatorColor = this.violatorColor;
            RA.violatorNames = this.violatorNames;

            return RA;
        }
    }
}

