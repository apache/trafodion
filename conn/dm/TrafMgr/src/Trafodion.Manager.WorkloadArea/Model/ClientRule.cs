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
using System.Linq;
using System.Text;

//XML stuff
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using System.Data;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework.Connections;
using System.Collections;
using Trafodion.Manager.WorkloadArea.Controls;
using System.Text.RegularExpressions;

namespace Trafodion.Manager.WorkloadArea.Model
{
    [XmlRoot("ClientRule")]
    public class ClientRule: ICloneable
    {
        public ClientRule() 
        {        
        }

        #region Properties        
                
        private bool applyToAllSystems = false;

        [XmlElement("ApplyToAllSystems")]
        public bool ApplyToAllSystems
        {
            get { return applyToAllSystems; }
            set { applyToAllSystems = value; }
        }

        private ArrayList appliedSystems = new ArrayList();
        [XmlElement("AppliedSystems", typeof(String))]
        public ArrayList AppliedSystems
        {
            get { return appliedSystems; }
            set { appliedSystems = value; }
        }

        private ClientRuleAlert clientRuleAlerts = new ClientRuleAlert();
        [XmlElement("Alerts")]
        public ClientRuleAlert Alerts
        {
            get { return clientRuleAlerts; }
            set { clientRuleAlerts = value; }
        }

        [XmlElement("Popup")]
        public bool Popup
        {
            get {return (clientRuleAlerts.ShowAlertBox);}
        }

        [XmlElement("RuleName")]
        private string ruleName;
        public string RuleName
        {
            get{return ruleName;}
            set{ruleName=value;}
        }

        [XmlElement("Enabled")]
        private bool mbEnabled;
        public bool Enabled
        {
            get { return mbEnabled; }
            set { mbEnabled = value; }
        }

        private ArrayList alConditions = new ArrayList();
        [XmlElement("Conditions",typeof(ClientRuleCondition))]
        public ArrayList Conditions
        {
            get { return alConditions; }
            set { alConditions = value; }
        }

        private string expression;
        public string Expression
        {
            get { return expression; }
            set { expression = value; }
        }

        private bool advancedRule;
        public bool AdvancedRule
        {
            get { return advancedRule; }
            set { advancedRule = value; }
        }

        private string expressionOverride;
        public string ExpressionOverride
        {
            get { return expressionOverride; }
            set { expressionOverride = value; }
        }

        private string detailsCreatorName;
        public string DetailsCreatorName
        {
            get { return detailsCreatorName; }
            set { detailsCreatorName = value; }
        }

        private string detailsCreatorEmail;
        public string DetailsCreatorEmail
        {
            get { return detailsCreatorEmail; }
            set { detailsCreatorEmail = value; }
        }

        private string detailsVersion;
        public string DetailsVersion
        {
            get { return detailsVersion; }
            set { detailsVersion = value; }
        }

        private string detailsComments;
        public string DetailsComments
        {
            get { return detailsComments; }
            set { detailsComments = value; }
        }

        public bool IsComplete
        {
            get
            {
                return true;
            }
        }

        #endregion

        public bool Evaluate()
        {
            return true;            
        }

        public bool BuildAlertDictionary() 
        {
            return true;
        }

        public bool Execute(DataRow[] affectedRows, string ruleName) 
        {
            return clientRuleAlerts.Execute(affectedRows, ruleName);
        }

        public bool Execute(DataRow[] affectedRows, ArrayList conditions) 
        {
            return clientRuleAlerts.Execute(affectedRows, conditions);
        }

        public void replaceCondition(ClientRuleCondition inRCtoReplace, bool state) 
        {
            ClientRuleCondition rc = FindByFormat(inRCtoReplace);
            if (rc != null) 
            {
                if (state) 
                {
                    foreach (string str in inRCtoReplace.Threshold) 
                    {
                        if(!rc.Threshold.Contains(str))
                            rc.Threshold.Add(str);
                        rc.isComplete=true;
                    }
                }
                else
                {
                    foreach (string str in inRCtoReplace.Threshold) 
                    {
                        if (rc.Threshold.Contains(str) || str == "")
                            rc.Threshold.Remove(str);
                        rc.isComplete = true;

                        if (rc.Threshold.Count == 0)
                            rc.isComplete = false;
                    }
                }
                return;
            }
            //Create new rule
            ClientRuleCondition newRule = (ClientRuleCondition)inRCtoReplace.Clone();
            newRule.isEnabled = true;
            this.alConditions.Add(newRule);
        }

        public ClientRuleCondition FindByFormat(ClientRuleCondition inRC)
        {
            foreach (ClientRuleCondition rc in this.alConditions)
            {
                if (inRC.Text.Equals(rc.Text) && inRC.ListText.Equals(rc.ListText))
                {
                    return rc;
                }
            }
            return null;
        }

        public void SetCondition(ClientRuleCondition inRC, bool state) 
        {
            ClientRuleCondition rc = FindByFormat(inRC);
            if (rc != null) 
            {
                rc.isEnabled = state;
                return;
            }
            //Create new rule
            ClientRuleCondition newRule = (ClientRuleCondition)inRC.Clone();
            newRule.isEnabled = true;
            this.alConditions.Add(newRule);
        }

        //Add a varible "triageSpecial" because some columns with the same name, but different type in liveview/triagespace
        public string GetExpression(bool triageSpecial) 
        {
            if (this.advancedRule)
                return this.expressionOverride;
            string tempEx = "";
            bool firstRC = true;
            foreach (ClientRuleCondition rc in this.Conditions) 
            {
                if (rc.isEnabled) 
                {
                    if (!firstRC)
                        tempEx += " AND ";
                    if (rc.Threshold.Count > 1)
                        tempEx += " ( ";
                    for (int i = 0; i < rc.Threshold.Count; i++) 
                    {
                        if (i > 0)
                            tempEx += " OR ";                        

                        if (rc.QueryProperty == "ELAPSED_TIME")
                            tempEx += " (ElapsedTimeTicks " + rc.Comparator + " " + rc.Threshold[i].ToString() + ") ";
                        else if (rc.QueryProperty == "WAIT_TIME")
                            tempEx += " (WaitTimeTicks " + rc.Comparator + " " + rc.Threshold[i].ToString() + ") ";
                        else if (rc.QueryProperty == "HOLD_TIME" || rc.QueryProperty=="PROCESS_CREATE_TIME"
                            || rc.QueryProperty == "OPEN_TIME" || rc.QueryProperty == "TOTAL_QUERY_TIME" || rc.QueryProperty == "LAST_INTERVAL_PROCESSOR_TIME")
                        {                            
                            TimeSpan ts = TimeSpan.FromMilliseconds(Double.Parse(rc.Threshold[i].ToString()));
                            if (rc.QueryProperty == "HOLD_TIME")
                            {
                                // This is because HOLD_TIME is special hhh:mm:ss format, not normal hh:mm:ss format.
                                if(ts.Hours < 99&&!triageSpecial)
                                    tempEx += " ([" + rc.QueryProperty + "] " + rc.Comparator + "'0" + ts.ToString() + "'" + ") ";
                                //In triageSpace HOLD_TIME is in seconds, for example 20. so not convert to TimeSpan format;
                                if(triageSpecial)
                                    tempEx += " ([" + rc.QueryProperty + "] " + rc.Comparator + "" + rc.Threshold[i] + ") ";
                            }  
                            else
                                tempEx += " ([" + rc.QueryProperty + "] " + rc.Comparator + "'" + ts.ToString() + "'" + ") ";
                        }
                        else 
                        {
                            string theValue = rc.Threshold[i].ToString();
                            if ((LinkType.STATE == rc.LinkFieldType) && "'Any'".Equals(theValue))
                                tempEx += " (1=1) ";
                            else if (rc.QueryProperty == "SQL_TEXT" || rc.QueryProperty == "QUERY_TEXT" || rc.QueryProperty=="ERROR_TEXT")
                            {
                                //SQL_TEXT added %% to enable like search SQL_TEXT value is like
                                // 'SELECT COL1 FROM T1', make it as '%SELECT COL1 FROM T1%'
                                theValue = "'%" + theValue.Trim('\'') + "%'";
                                theValue = ConvertSpecialCharacters(theValue);
                                tempEx += " ([" + rc.QueryProperty + "] " + rc.Comparator + "" + theValue + ") ";
                            }
                            else if (rc.QueryProperty == "ERROR_CODE" && triageSpecial)
                            {
                                int error_code = 0;
                                if (Int32.TryParse(theValue, out error_code))
                                    tempEx += " ([" + rc.QueryProperty + "] " + rc.Comparator + "'" + error_code + "') ";
                            }
                            else 
                            {
                                if (rc.Comparator.Equals("LIKE"))
                                {
                                    string tvalue = rc.Threshold[i].ToString();
                                    tvalue = ConvertSpecialCharacters(theValue);
                                    tempEx += " ([" + rc.QueryProperty + "] " + rc.Comparator + "" + tvalue + ") ";
                                }
                                else 
                                {
                                    tempEx += " ([" + rc.QueryProperty + "] " + rc.Comparator + "" + theValue + ") ";
                                }
                                
                            }
                                
                        }
                    }
                    if (rc.Threshold.Count > 1)
                        tempEx += " ) ";
                    firstRC = false;
                }
            }
            return tempEx;
        }   

        //Some rule threshold values contain special characters, like 'SELECT COUNT(*) FROM'
        // (, *, ) are all special characters, DataTable.Select method needs them to be coverted 
        //from ( to [(], * to [*] 
        private string ConvertSpecialCharacters(string rawString) 
        {
            
            //Add process about "[", "]" characters, and it needs to be first processed;
            //Because for other special characters like "(", it will be converted as [(], so is hard to tell "[" was in string or is just added;
            //To make it simple we assume "[" and "]" will appear as a pair, and there is no embedded format like [ab[c]].
            Regex re = new Regex("\\[.*?\\]", RegexOptions.None);
            MatchCollection mc = re.Matches(rawString);

            foreach (var match in mc)
            {
                string replaceString = "[[]" + match.ToString().Substring(1, match.ToString().Length - 2) + "[]]";
                rawString = rawString.Replace(match.ToString(), replaceString);
            }  

            StringBuilder rawStringBuilder = new StringBuilder(rawString);
            string[] specialChars = { "(", ")", "*", "\n", "\t", "\r", "~", "#", "&", "^" };
            foreach (string c in specialChars.ToArray())
            {
                if (rawStringBuilder.ToString().Contains(c))
                    rawStringBuilder.Replace(c, "[" + c + "]");
            }      
            return rawStringBuilder.ToString();
        }


        public bool IsConditionEnabled(ClientRuleCondition inRC) 
        {
            ClientRuleCondition tempCondition = FindByFormat(inRC);
            return (tempCondition != null && tempCondition.isEnabled);
        }

        private ArrayList CloneConditions()
        {
            ArrayList tempList = new ArrayList();
            foreach (ClientRuleCondition con in this.Conditions)
            {
                tempList.Add(con.Clone());
            }
            return tempList;
        }

        public object Clone()
        {
            ClientRule QR = new ClientRule();
            QR.ApplyToAllSystems = this.ApplyToAllSystems;
            QR.RuleName = this.RuleName;
            QR.AppliedSystems = this.AppliedSystems;
            //QR.AssociatedSystem = this.AssociatedSystem;
            QR.Conditions = (ArrayList)this.CloneConditions();

            QR.Alerts = (ClientRuleAlert)this.Alerts.Clone();
            QR.Enabled = this.Enabled;
            QR.Expression = this.expression;
            QR.AdvancedRule = this.AdvancedRule;
            QR.ExpressionOverride = this.expressionOverride;
            QR.DetailsComments = this.detailsComments;
            QR.DetailsCreatorEmail = this.detailsCreatorEmail;
            QR.DetailsCreatorName = this.detailsCreatorName;
            QR.DetailsVersion = this.detailsVersion;
            return QR;
        }

        //public override bool Equals(object obj)
        //{
        //    //Check for null and compare run-time types.
        //    if (obj == null || GetType() != obj.GetType()) return false;
        //    ClientRule QR = (ClientRule)obj;
        //    return (ApplyToAllSystems == QR.ApplyToAllSystems) && (RuleName == QR.RuleName) && (AppliedSystems == QR.AppliedSystems)
        //        && (Conditions == QR.Conditions) && (Alerts == QR.Alerts) && (Enabled == QR.Enabled) && (Expression == QR.Expression)
        //        && (AdvancedRule == QR.AdvancedRule) && (ExpressionOverride == QR.ExpressionOverride) && (DetailsComments == QR.DetailsComments)
        //        && (DetailsCreatorEmail == QR.DetailsCreatorEmail) && (DetailsCreatorName == QR.DetailsCreatorName) && (DetailsVersion == QR.DetailsVersion);
        //}
    }
}
