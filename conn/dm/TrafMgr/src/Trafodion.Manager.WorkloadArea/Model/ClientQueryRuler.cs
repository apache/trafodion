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
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.WorkloadArea.Controls;
using System.Data;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using System.IO;

namespace Trafodion.Manager.WorkloadArea.Model
{
    sealed class ClientQueryRuler
    {
        private static Hashtable _queryRulerColumnNames_ht = new Hashtable();
        public static readonly ClientQueryRuler Instance = new ClientQueryRuler();

        ClientRuleManager ruleManager = new ClientRuleManager();
        private DataTable m_dOperatingTable;

        public DataTable OperatingTable
        {
            get { return m_dOperatingTable; }
            //set { m_dOperatingTable = value; }
        }

        private Hashtable systemHash = new Hashtable();
        public Hashtable SystemHash
        {
            get 
            {
                if (null != Trafodion.Manager.Framework.Connections.ConnectionDefinition.ConnectionDefinitions) 
                {
                    systemHash.Clear();
                    foreach (ConnectionDefinition connectionDefn in Trafodion.Manager.Framework.Connections.ConnectionDefinition.ConnectionDefinitions)
                    {                        
                        systemHash.Add(connectionDefn.Name, connectionDefn);
                    }
                }
                
                return systemHash; 
            }
            set
            { 
                systemHash = value;                
            }
        }

        private ArrayList m_aRulesList = new ArrayList();
        public ArrayList RulesList
        {
            get { return m_aRulesList; }
            set { m_aRulesList = value; }
        }

        public ClientRuleViolators RuleViolatorPopup;

        static ClientQueryRuler() {
			_queryRulerColumnNames_ht.Add("ForegroundColor", "ForegroundColor");
			_queryRulerColumnNames_ht.Add("BackgroundColor", "BackgroundColor");
			_queryRulerColumnNames_ht.Add("ViolatorColor", "ViolatorColor");
			_queryRulerColumnNames_ht.Add("ViolatorNames", "ViolatorNames");
            _queryRulerColumnNames_ht.Add("ElapsedTimeTicks", "ElapsedTimeTicks");
            _queryRulerColumnNames_ht.Add("WaitTimeTicks", "WaitTimeTicks");            
		}

        public ClientQueryRuler()
        {
            RulesList = new ArrayList();
            RuleViolatorPopup = new ClientRuleViolators();
        }

        public static bool isAQueryRulerColumn(String colName)
        {
            return _queryRulerColumnNames_ht.ContainsKey(colName);
        }

        public bool SetupOperatingTable(ConnectionDefinition in_System, DataTable in_OpTable)
        {
            //Add then hide Color Columns
            foreach (DictionaryEntry de in _queryRulerColumnNames_ht)
            {
                try
                {
                    String colName = de.Key.ToString();
                    if (in_OpTable.Columns.Contains(colName))
                        continue;
                    if (colName.Equals("ElapsedTimeTicks") || colName.Equals("WaitTimeTicks"))
                        in_OpTable.Columns.Add(colName, typeof(Int64));
                    else
                        in_OpTable.Columns.Add(new DataColumn(colName));                    
                    
                    in_OpTable.Columns[colName].ColumnMapping = MappingType.Hidden;

                }
                catch (Exception e)
                {
                    if (Logger.IsTracingEnabled)
                        Logger.OutputToLog("An error occured while initializing Rules/Alerts for system" +
                                                    in_System.Name + "\r\n" + e.Message);

                }
            }


            bool haveWaitTimeColumn = in_OpTable.Columns.Contains("WAIT_TIME");
            foreach (DataRow dr in in_OpTable.Rows)
            {
                try
                {
                    string tempString = dr["ELAPSED_TIME"].ToString();
                    TimeSpan tempSpan = TimeSpan.Parse(tempString);
                    dr["ElapsedTimeTicks"] = tempSpan.TotalMilliseconds.ToString();
                }
                catch
                {
                    dr["ElapsedTimeTicks"] = "0";
                }

                try
                {
                    dr["WaitTimeTicks"] = "0";

                    if (haveWaitTimeColumn)
                    {
                        String waitTimeStr = dr["WAIT_TIME"].ToString();
                        if ((null != waitTimeStr) && (0 < waitTimeStr.Length))
                        {
                            TimeSpan ts=new TimeSpan(0) ;                            
                            double temp;
                            if(Double.TryParse(waitTimeStr, out temp))
                                ts = TimeSpan.FromSeconds(temp);
                            else
                                ts = TimeSpan.Parse(waitTimeStr);
                            dr["WaitTimeTicks"] = ts.TotalMilliseconds.ToString();
                        }

                    }

                }
                catch
                {
                    dr["WaitTimeTicks"] = "0";
                }

            }

            this.m_dOperatingTable = in_OpTable;
            RuleViolatorPopup.opTable = in_OpTable;

            this.ruleManager.ruleWizard.AddDTCols_To_Hash(in_System, in_OpTable);


            return true;
        }


        public void OpenRuleManager()
        {
            if (ruleManager.WindowState == FormWindowState.Minimized)
                ruleManager.WindowState = FormWindowState.Normal;
            Refresh();
            Utilities.SetCenterParent(ruleManager);
            ruleManager.Show();
            ruleManager.BringToFront();            
            
        }

        public void Refresh()
        {            
            ruleManager.LoadExistingRules(ruleManager.RulesList);
        }

        public void LoadRulesFromFolder() 
        {
            ruleManager.LoadRulesFromFolder();
        }

        public void AddRule(ClientRule in_rRule)
        {
            RulesList.Add(in_rRule);
        }

        public bool CreateRule(string in_sExpression)
        {
            RulesList.Add(new ClientRule());
            return true;
        }

        public void ClearRules()
        {
            this.RulesList.Clear();
        }

        public bool EvaluateEnabledRules(ConnectionDefinition activeSystem, bool isLiveView)
        {
            this.RulesList = ruleManager.RulesList;
            RuleViolatorPopup.clearSystemEntries(activeSystem.Name);            

            foreach (ClientRule rule in RulesList) 
            {
                if ((rule.ApplyToAllSystems||rule.AppliedSystems.Contains(activeSystem.Name))&& (rule.IsComplete && rule.Enabled))
                {
                    try
                    {
                        DataRow[] foundRows;
                        //Check whether all column conditions exist in DataTable
                        List<string> nonExistColumns = new List<string>();                        
                        bool columnExist = checkRuleColumnExistInDataTable(m_dOperatingTable, rule, isLiveView, out nonExistColumns);                        

                        if (columnExist)
                        {
                            foundRows = m_dOperatingTable.Select(rule.GetExpression(!isLiveView));
                            if (foundRows.Length > 0)
                            {
                                if (rule.Alerts.ShowAlertBox && foundRows.Length > 0)
                                {
                                    RuleViolatorPopup.Show();
                                    RuleViolatorPopup.addNode(new ViolatorData(rule.RuleName, foundRows, rule.Conditions,
                                                                               activeSystem, isLiveView));
                                    RuleViolatorPopup.BringToFront(); 
                                }
                                rule.Execute(foundRows, rule.RuleName);
                                if (rule.Alerts.ActiveAlertDict[AlertType.CELL_COL])
                                {
                                    rule.Execute(foundRows, rule.Conditions);
                                }
                            }
                        }
                        else 
                        {
                            string nonExistColumnNames = string.Join(",", nonExistColumns.ToArray());
                            String errMsg = "Rule " + rule.RuleName + " specified nonexistent columns: " + nonExistColumnNames;                            
                            Logger.OutputErrorLog(errMsg);
                        }   
                    }
                    catch (Exception exc)
                    {
                        String errMsg = "Error applying threshold rule '" + rule.RuleName +
                                        "'.\n   Details = " + exc.Message;
                        if (Logger.IsTracingEnabled)
                            Logger.OutputToLog(errMsg);

                        MessageBox.Show(errMsg, "Error applying Threshold Rule '" + rule.RuleName + "'", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }                
                } 
            }
            return true;
        }


        private bool checkRuleColumnExistInDataTable(DataTable opTable, ClientRule aRule, bool isLiveView, out List<string> nonExistColumn) 
        {
            bool flag = false;
            nonExistColumn=new List<string>();
            for (int i = 0; i < aRule.Conditions.Count; i++) 
            {
                ClientRuleCondition rc = (ClientRuleCondition)aRule.Conditions[i];
                columnNameConvert(rc, isLiveView);
                if (!opTable.Columns.Contains(rc.QueryProperty)&& rc.isEnabled)
                {  
                    nonExistColumn.Add(rc.QueryProperty);
                }            
            }
            if (nonExistColumn.Count > 0)
                return false;
            else
                return true;            
        }

        ////Fix QC defect: 4186,For "Most Used" columns, there are some needed to be changed in LiveView
        //For example, CARDINALITY_ESTIMATE named as EST_CANDINALITY in LiveView
        private void columnNameConvert(ClientRuleCondition rc, bool isLiveView) 
        {
            if (isLiveView)
            {
                if (rc.QueryProperty.Equals("CARDINALITY_ESTIMATE")) 
                {
                    rc.QueryProperty = "EST_CARDINALITY";
                }
            }
            else 
            {
                if (rc.QueryProperty.Equals("EST_CARDINALITY")) 
                {
                    rc.QueryProperty = "CARDINALITY_ESTIMATE";
                }
            }
        }

    }
}
