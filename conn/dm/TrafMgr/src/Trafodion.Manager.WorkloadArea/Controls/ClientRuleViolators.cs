// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using TenTec.Windows.iGridLib;
using System.Collections;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{

    public struct ViolatorData
    {
        public string NodeText;
        public DataRow[] violatorsRow;
        public ArrayList Conditions;
        public ConnectionDefinition System;
        public bool isLiveView;        

        public ViolatorData(string inNodeText, DataRow[] inViolatorsRow, ArrayList inConditions, ConnectionDefinition inSystem,
                            bool liveViewQueries)
        {
            NodeText = inNodeText;
            violatorsRow = inViolatorsRow;
            Conditions = inConditions;
            System = inSystem;
            isLiveView = liveViewQueries;
        }
    }
    
    public partial class ClientRuleViolators : TrafodionForm
    {
        public bool isShown;
        private ArrayList RuleNames;
        private Hashtable Violators;        
        public DataTable opTable;
        private bool loaded;
        private ArrayList selectedSystems = new ArrayList();
        private bool allSelected = true;
        public event EventHandler RuleViolatorCellClick;

        public void OnRuleViolatorCellClick(ViolatorCellClickEventArgs e) 
        {
            EventHandler ruleViolatorCellClick = RuleViolatorCellClick;
            if (ruleViolatorCellClick != null)
                ruleViolatorCellClick(this, e);        
        }

        public class ViolatorCellClickEventArgs : EventArgs 
        {
            public bool IsLiveView { get; set; }
            public string QueryID { get; set; }
            public string QueryStartTime { get; set; }
            public ViolatorCellClickEventArgs(bool isLiveView, string queryID, string queryStartTime) 
            {
                this.IsLiveView = isLiveView;
                this.QueryID = queryID;
                this.QueryStartTime = queryStartTime;
            }
        }


        public bool Loaded
        {
            get { return loaded; }
        }

        private Hashtable violatorTable = new Hashtable();
        public Hashtable ViolatorTable
        {
            get { return violatorTable; }
            set { violatorTable = value; }
        }

        private ArrayList systemHashKeys = new ArrayList();
        public ArrayList SystemHashKeys
        {
            get { return systemHashKeys; }
            set { systemHashKeys = value; }
        }


        public ClientRuleViolators()
        {
            InitializeComponent();
            this.loaded = true;
            this.iGridRuleViolators.GroupObject.Add("RuleName");

            RuleNames = new ArrayList();
            Violators = new Hashtable();
            this.isShown = false;

            selectedSystems = new ArrayList();
            allSelected = true;

            this.treeView_Systems.AfterCheck += new TreeViewEventHandler(treeView_Systems_AfterCheck);            
        }

        public void addNode(ViolatorData inViolatorData)
        {
            lock (ViolatorTable)
            {
                if (!ViolatorTable.ContainsKey(inViolatorData.System.Name))
                    ViolatorTable.Add(inViolatorData.System.Name, new ArrayList());

                ((ArrayList)ViolatorTable[inViolatorData.System.Name]).Add(inViolatorData);
            }

            lock (SystemHashKeys)
            {
                if (!this.SystemHashKeys.Contains(inViolatorData.System.Name))
                    this.SystemHashKeys.Add(inViolatorData.System.Name);
            }

            updateDisplay();
        }

        public void clearTree()
        {
            this.treeView_Systems.Nodes.Clear();
            this.iGridRuleViolators.Rows.Clear();
            //this.treeView1.Nodes.Clear();
            RuleNames.Clear();
            Violators.Clear();
        }

        public void clearSystemEntries(string systemTitle)
        {
            lock (ViolatorTable)
            {
                if (ViolatorTable.ContainsKey(systemTitle))
                {
                    ViolatorTable.Remove(systemTitle);
                }
            }
            lock (SystemHashKeys)
            {
                systemHashKeys.Remove(systemTitle);
            }
            updateDisplay();
        }

        private void updateDisplay()//addNode(ViolatorData inViolatorData)
        {         

            clearTree();
            this.treeView_Systems.Nodes.Add("ALL_SYSTEMS", "All Systems");
            this.treeView_Systems.Nodes["ALL_SYSTEMS"].Checked = this.allSelected;

            foreach (string systemTitle in this.systemHashKeys)
            {
                if (this.violatorTable.ContainsKey(systemTitle))
                {
                    ArrayList vdList = (ArrayList)this.violatorTable[systemTitle];
                    this.treeView_Systems.Nodes["ALL_SYSTEMS"].Nodes.Add(systemTitle, systemTitle + " (" + vdList.Count + ")");
                    bool displaySystem = (this.selectedSystems.Contains(systemTitle) || this.allSelected);

                    this.treeView_Systems.Nodes["ALL_SYSTEMS"].Nodes[systemTitle].Checked = displaySystem;
                    if (displaySystem)
                    {
                        if (!this.selectedSystems.Contains(systemTitle))
                            this.selectedSystems.Add(systemTitle);

                        foreach (ViolatorData vd in vdList)
                        {
                            showViolatorTable(vd);
                        }
                    }
                }
            }
            this.treeView_Systems.Nodes["ALL_SYSTEMS"].ExpandAll();

            for (int i = 0; i < this.selectedSystems.Count; i++)
            {
                if (null == this.treeView_Systems.Nodes["ALL_SYSTEMS"].Nodes[this.selectedSystems[i].ToString()])
                {
                    this.selectedSystems.Remove(this.selectedSystems[i].ToString());
                }
            }
        }

        void treeView_Systems_AfterCheck(object sender, TreeViewEventArgs e)
        {
            if (e.Action == TreeViewAction.ByKeyboard || e.Action == TreeViewAction.ByMouse)
            {
                if (e.Node.Checked)
                {
                    if (!this.selectedSystems.Contains(e.Node.Name))
                    {
                        this.selectedSystems.Add(e.Node.Name);
                    }
                }
                else
                {
                    allSelected = false;
                    if (this.selectedSystems.Contains(e.Node.Name))
                        this.selectedSystems.Remove(e.Node.Name);
                }

                //IF the node checked is a 'header' node then toggle all child nodes
                if (e.Node.Nodes.Count != 0)
                {
                    this.allSelected = e.Node.Checked;
                    if (!this.allSelected)
                        this.selectedSystems.Clear();
                }
                updateDisplay();
            }
            //displayRule(newRule);
        } 


        private void showViolatorTable(ViolatorData inVD)
        {
            foreach (DataRow dr in inVD.violatorsRow)
            {
                TenTec.Windows.iGridLib.iGRow row = this.iGridRuleViolators.Rows.Add();
                row.Cells["RuleName"].Value = inVD.NodeText;
                row.Cells["QueryID"].Value = dr["QUERY_ID"];
                row.Cells["SystemName"].Value = inVD.System.Name;
                row.Cells["WorkloadMonitor"].Value = inVD.isLiveView;

                //In WorkloadMonitor, Column Start Time is QUERY_START_TIME, and in Triage Space, Start Time is START_TIME, 
                if(inVD.isLiveView==true)
                    row.Cells["StartTime"].Value = dr["QUERY_START_TIME"];
                else
                    row.Cells["StartTime"].Value = dr["START_TIME"];
                
                row.Tag = inVD.System;
                foreach (ClientRuleCondition rc in inVD.Conditions)
                {
                    if (rc.isEnabled)
                    {
                        string columnName = rc.QueryProperty.ToString();
                        try
                        {
                            TenTec.Windows.iGridLib.iGCol column = iGridRuleViolators.Cols[columnName];

                        }
                        catch
                        {
                            iGridRuleViolators.Cols.Add(columnName, columnName);
                        }

                        /*
                         * Fix defect 4185: M6:Be incorrect sort way in Client Threshold Violations window.
                         * The cell value should be set with actual data, rather than always a string.(Previous value: dr[columnName].ToString().TrimEnd(' '))
                         * The order rule is different between numbers and strings. 
                         * E.g. an array of integers:           200, 400, 30, 1
                         *          If the values are convert to strings, the sorted result are unexpected:  
                         *                                      1, 200, 30, 400
                         *          If the values are still numbers, the sorted result is what we want:     
                         *                                      1, 30, 200, 400
                         */
                        row.Cells[columnName].Value = dr[columnName];
                    }
                }
            }
            this.iGridRuleViolators.Group();
        }

        private void treeView1_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {            
        }

        private void iGridRuleViolators_SelectionChanged(object sender, System.EventArgs e)
        {
            try
            {
                bool isLiveView = false;
                try
                {
                    isLiveView = bool.Parse(this.iGridRuleViolators.SelectedRows[0].Cells["WorkloadMonitor"].Value.ToString());

                }
                catch (Exception)
                {
                }


                String queryID = this.iGridRuleViolators.SelectedRows[0].Cells["QueryID"].Value.ToString();
                string queryStartTime = this.iGridRuleViolators.SelectedRows[0].Cells["StartTime"].Value.ToString();
                //((ConnectionDefinition)this.iGridRuleViolators.SelectedRows[0].Tag).HighlightQuery(queryID, queryStartTime);
                OnRuleViolatorCellClick(new ViolatorCellClickEventArgs(isLiveView, queryID, queryStartTime));
            }
            catch
            {
                
            }
        }

        private void ClientRuleViolators_FormClosing(object sender, System.Windows.Forms.FormClosingEventArgs e)
        { 
            this.Hide();
            if (e.CloseReason.ToString().Equals("ApplicationExitCall"))
                e.Cancel = false;
            else
                e.Cancel = true;
        }
    }
}
