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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Drawing;
using System.IO;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using System.Xml.Serialization;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public struct Alert
    {
        public string AlertText;
        public string ListText;
        public LinkType LinkFieldType;
        public Bitmap managerIcon;

        public Alert(string inAlertText, string inListText, LinkType inLinkType)
        {
            AlertText = inAlertText;
            ListText = inListText;
            LinkFieldType = inLinkType;
            managerIcon = null;
        }

        public Alert(string inAlertText, string inListText, LinkType inLinkType, Bitmap inBitmap)
        {
            AlertText = inAlertText;
            ListText = inListText;
            LinkFieldType = inLinkType;
            managerIcon = inBitmap;
        }
    }

    public enum AlertType
    {
        BACK_COL, FORE_COL, CELL_COL, POPUP, LOG
    }

    public enum WizardStages
    {
        RULES, CONDITIONS, ALERTS, FINISH
    }

    public partial class ClientRuleWizard : TrafodionForm
    {
        XmlSerializer ruleSerializer;
        private WizardStages currentStage = WizardStages.RULES;
        private ClientRule newRule = new ClientRule();
        private IDictionary<LinkType, string> ConDict;
        private bool editExistingRule = false;
        private bool CommentsFieldOpen;
        private Hashtable DefaultColNames = new Hashtable();
        private ArrayList ActiveColNames = new ArrayList();
        private ArrayList dateTimeColumnList = new ArrayList();
        private List<string> boolColumnList=new List<string>();
        private List<string> stringColumnList = new List<string>();
        private List<string> durationColumnList = new List<string>();
        private List<string> valueTimeTypeColumnList = new List<string>();
        public bool advancedMode;
        private ClientRuleManager clientRuleManager;
        private Hashtable preDefinedValues = new Hashtable();

        private Hashtable htSystemTables = new Hashtable();
        public Hashtable SystemTables
        {
            get { return htSystemTables; }
            set { htSystemTables = value; }
        }

        internal ClientRule NewRule
        {
            get { return newRule; }
        }

        private bool applyToAllSystem = false;
        public bool ApplyToAllSystem
        {
            get { return applyToAllSystem; }
            set { applyToAllSystem = value; }
        }

        private ArrayList aMasterCons;
        public ArrayList MasterCons
        {
            set { aMasterCons = value; }
            get { return aMasterCons; }
        }

        private ArrayList aCannedCons;
        public ArrayList CannedCons
        {
            set { aCannedCons = value; }
            get { return aCannedCons; }

        }

        private ArrayList aDateTimeCons;
        public ArrayList DateTimeCons
        {
            set { aDateTimeCons = value; }
            get { return aDateTimeCons; }
        }

        private ArrayList aMiscCons;
        public ArrayList MiscCons
        {
            set { aMiscCons = value; }
            get { return aMiscCons; }
        }

        private ArrayList aMasterAlerts;
        public ArrayList MasterAlerts
        {
            set { aMasterAlerts = value; }
            get { return aMasterAlerts; }
        }

        private ConnectionDefinition activeSystem;
        public ConnectionDefinition ActiveSystem
        {
            get { return activeSystem; }
            set { activeSystem = value; }
        }

        private IDictionary<AlertType, Alert> AlertDict;

        public ClientRuleWizard(ClientRuleManager argClientRuleManager)
        {
            ruleSerializer = new XmlSerializer(typeof(ClientRule));
            this.clientRuleManager = argClientRuleManager;
            InitializeComponent();
            this.ruleConditionsTreeView.BeforeCheck += new TreeViewCancelEventHandler(treeView1_BeforeCheck);
            this.treeView_Systems.AfterCheck += new TreeViewEventHandler(treeView_Systems_AfterCheck);
            setupConDict();
            setupAlertDict();
            InitializeDefaulDataLists();    
        
            BuildInitialConMasters();
            BuildInitialAlertMasters();
            
        }


        private void setupConDict()
        {
            ConDict = new Dictionary<LinkType, string>();
            ConDict.Add(LinkType.ALERT, "Alert");
            ConDict.Add(LinkType.AMOUNT, "Amount");
            ConDict.Add(LinkType.COLOR, "A Custom Color");
            ConDict.Add(LinkType.EXCEED, "Exceed");
            ConDict.Add(LinkType.USER_ID, "User ID");
            ConDict.Add(LinkType.QUERY_ID, "Query ID");
            ConDict.Add(LinkType.FLOAT_VALUE, "Float Value");

            ConDict.Add(LinkType.VALUE, "Value");
            ConDict.Add(LinkType.PROPERTY, "Property");
            ConDict.Add(LinkType.APPLICATION, "Application");
            ConDict.Add(LinkType.TOGGLE, "");

            ConDict.Add(LinkType.CLIENT_ID, "Client ID");
            ConDict.Add(LinkType.USER_NAME, "User name");
            ConDict.Add(LinkType.SESSION_ID, "Session ID");
            ConDict.Add(LinkType.SQL_TEXT, "SQL Text");
            ConDict.Add(LinkType.ERROR_CODE, "Error Code");
            ConDict.Add(LinkType.DATASOURCE, "Datasource name");

            ConDict.Add(LinkType.STRING, "Text");
            ConDict.Add(LinkType.DATE_TIME, "Date/Time");
            ConDict.Add(LinkType.STATE, "State");
            ConDict.Add(LinkType.FILE_NAME, "File name");
            ConDict.Add(LinkType.DURATION, "Duration");
            ConDict.Add(LinkType.BOOLEAN, "True or False");
        }

        private void setupAlertDict()
        {
            AlertDict = new Dictionary<AlertType, Alert>();
            AlertDict.Add(AlertType.BACK_COL, new Alert("Change the background color to {0}", "Change the background color", LinkType.COLOR));
            AlertDict.Add(AlertType.FORE_COL, new Alert("Change the text color to {0}", "Change the text color", LinkType.COLOR));
            AlertDict.Add(AlertType.CELL_COL, new Alert("Change the cell color to {0}", "Change the cell color", LinkType.COLOR));
            AlertDict.Add(AlertType.POPUP, new Alert("Open the Alert window", "Open the Alert window", LinkType.TOGGLE));
            AlertDict.Add(AlertType.LOG, new Alert("Log violation to file: {0}", "Log violation to text file", LinkType.FILE_NAME));
        }

        private void InitializeDefaulDataLists() 
        {
            BuildDefaultConArray();
            BuildDateTimeColumnList();
            BuildBoolColumnList();
            BuildDurationTypeColumnListFromDateTimeType();
            BuildStringTypeColumnListFromIntType();
            BuildValueTypeColumnListFromDateTimeType();
        }

        private void BuildDefaultConArray()
        {
            //Build the default condition array
            DefaultColNames = new Hashtable();
            DefaultColNames.Add("APPLICATION_NAME", "APPLICATION_NAME");
            DefaultColNames.Add("USER_ID", "USER_ID");
            DefaultColNames.Add("CLIENT_ID", "CLIENT_ID");
            DefaultColNames.Add("USER_NAME", "USER_NAME");
            DefaultColNames.Add("QUERY_ID", "QUERY_ID");
            DefaultColNames.Add("SESSION_ID", "SESSION_ID");
            DefaultColNames.Add("SQL_TEXT", "SQL_TEXT");
            DefaultColNames.Add("ERROR_CODE", "ERROR_CODE");
            DefaultColNames.Add("DATASOURCE", "DATASOURCE");
            DefaultColNames.Add("STATE", "STATE");

            DefaultColNames.Add("CARDINALITY_ESTIMATE", "CARDINALITY_ESTIMATE");
            DefaultColNames.Add("ESTIMATED_COST", "ESTIMATED_COST");
            DefaultColNames.Add("DISK_READS", "DISK_READS");
            DefaultColNames.Add("MESSAGES_TO_DISK", "MESSAGES_TO_DISK");
            DefaultColNames.Add("ROWS_ACCESSED", "ROWS_ACCESSED");
            DefaultColNames.Add("ROWS_RETRIEVED", "ROWS_RETRIEVED");
        }

        private void BuildInitialConMasters()
        {
            //Build the initial master list of Default Conditions
            //Load these from an XML file
            CannedCons = new ArrayList();
            MasterCons = new ArrayList();
            DateTimeCons = new ArrayList();
            MiscCons = new ArrayList();

            //Pure String Constraints
            MasterCons.Add(new ClientRuleCondition("Is run by {0}", (String)DefaultColNames["APPLICATION_NAME"], 1,
                                                LinkType.APPLICATION, "APPLICATION_NAME", "LIKE", ""));
            MasterCons.Add(new ClientRuleCondition("Is run by {0}", (String)DefaultColNames["CLIENT_ID"], 1,
                                                LinkType.CLIENT_ID, "CLIENT_ID", "LIKE", ""));
            MasterCons.Add(new ClientRuleCondition("Is run by {0}", (String)DefaultColNames["USER_NAME"], 1,
                                                LinkType.USER_NAME, "USER_NAME", "LIKE", ""));
            MasterCons.Add(new ClientRuleCondition("Is run with the Query ID {0}", (String)DefaultColNames["QUERY_ID"], 1,
                                                LinkType.QUERY_ID, "QUERY_ID", "LIKE", ""));
            MasterCons.Add(new ClientRuleCondition("Is run with the Session ID {0}", (String)DefaultColNames["SESSION_ID"], 1,
                                                LinkType.SESSION_ID, "SESSION_ID", "LIKE", ""));
            MasterCons.Add(new ClientRuleCondition("Has SQL text containing {0}", (String)DefaultColNames["SQL_TEXT"], 1,
                                                LinkType.SQL_TEXT, "SQL_TEXT", "LIKE", ""));
            MasterCons.Add(new ClientRuleCondition("Returns the error code {0}", (String)DefaultColNames["ERROR_CODE"], 1,
                                                LinkType.ERROR_CODE, "ERROR_CODE", "=", ""));
            MasterCons.Add(new ClientRuleCondition("Is run on the datasource {0}", (String)DefaultColNames["DATASOURCE"], 1,
                                                LinkType.DATASOURCE, "DATASOURCE", "LIKE", ""));
            MasterCons.Add(new ClientRuleCondition("Has reached the state of {0}", (String)DefaultColNames["STATE"], 1,
                                                LinkType.STATE, "STATE", "LIKE", ""));

            //Pure Value Constraints
            MasterCons.Add(new ClientRuleCondition("Has an estimated cardinality greater than {0}", "CARDINALITY_ESTIMATE", 1,
                                                LinkType.VALUE, "CARDINALITY_ESTIMATE", ">", ""));
            MasterCons.Add(new ClientRuleCondition("Has an estimated cost greater than {0}", "ESTIMATED_COST", 1,
                                                LinkType.FLOAT_VALUE, "ESTIMATED_COST", ">", ""));
            MasterCons.Add(new ClientRuleCondition("Performs more than {0} disk reads", "DISK_READS", 1, LinkType.VALUE,
                                                "DISK_READS", ">", ""));
            MasterCons.Add(new ClientRuleCondition("Sends more than {0} messages to the disk process", "MESSAGES_TO_DISK", 1,
                                                LinkType.VALUE, "MESSAGES_TO_DISK", ">", ""));
            MasterCons.Add(new ClientRuleCondition("Accesses more than {0} rows", "ROWS_ACCESSED", 1, LinkType.VALUE,
                                                "ROWS_ACCESSED", ">", ""));
            MasterCons.Add(new ClientRuleCondition("Retrieves more than {0} rows", "ROWS_RETRIEVED", 1, LinkType.VALUE,
                                                "ROWS_RETRIEVED", ">", ""));

            //load caned rules
            LoadRulesFromFolder();
        }

        private void BuildInitialAlertMasters()
        {
            //Time Constraintst            
            MasterAlerts = new ArrayList();
            MasterAlerts.Add(new ClientRuleAlert());
        }


        


        public void Reset()
        {
            newRule = new ClientRule();
            this.advancedMode = false;
            StartSimpleMode();

            this.textBoxExpression.Text = "";
            this.ruleText.Height = 190;
            this.ruleText.Location = new Point(0, 0);
            this.buttonAdvancedMode.Enabled = true;
            CommentsFieldOpen = false;

            ResetComments();
            CloseCommentsField();

            this.clientRuleWizardTabControl.SelectTab(0);
            this.ruleText.Controls.Clear();
            this.textBox_QueryName.Clear();
            this.checkBox_RuleEnabled.Checked = true;
            this.editExistingRule = false;

            this.checkBox_allConnect.Checked = false;
            this.checkBox_allSystem.Checked = false;

            RebuildMasters();
            ListSystems();
            ListConditions();
            ListAlerts();
        }

        public void SetListSystemValue() 
        {
            string checkValue=this.clientRuleManager.KeyOfRuleIGridSelectRow;
            if(checkValue.Equals("All"))
            {
                this.checkBox_allConnect.Checked = false;
                this.checkBox_allSystem.Checked = true;
                CheckToggleAllSystems(true);
                this.treeView_Systems.Enabled = false;
            }
            //else if (!string.IsNullOrEmpty(checkValue))
            //{
            //    foreach (TreeNode tn in this.treeView_Systems.Nodes)
            //    {
            //        if (tn.Text.Equals(checkValue))
            //        {
            //            tn.Checked = true;
            //            return;
            //        }
            //    }
            //}
            else
            {
                foreach (TreeNode tn in this.treeView_Systems.Nodes)
                {
                    if (!string.IsNullOrEmpty(checkValue) && tn.Text.Equals(checkValue))                    
                        tn.Checked = true; 
                    else
                        tn.Checked = false;
                }
            }
            
        }

        private void StartSimpleMode()
        {
            if (this.newRule.AdvancedRule)
            {
                this.newRule.ExpressionOverride = "";
                this.newRule.AdvancedRule = false;
            }

            this.ruleConditionsTreeView.Enabled = true;
            this.advancedMode = false;

            this.textBoxExpression.TextChanged -= new System.EventHandler(textBoxExpression_TextChanged);
            this.textBoxExpression.Text = this.newRule.ExpressionOverride;
            this.ruleText.Height = 190;
            this.ruleText.Location = new Point(0, 0);

            this.buttonAdvancedMode.Text = "Advanced mode";

            //Hide the wizard screen
        }

        private void StartAdvancedMode() 
        {
            if (!this.newRule.AdvancedRule)
                newRule.ExpressionOverride = this.newRule.GetExpression(false);

            this.ruleConditionsTreeView.Enabled = false;
            this.advancedMode = true;
            this.textBoxExpression.Text = this.newRule.ExpressionOverride;

            if (!this.newRule.AdvancedRule)
                this.textBoxExpression.TextChanged += new System.EventHandler(textBoxExpression_TextChanged);

            this.ruleText.Height = 78;// = false;
            this.ruleText.Location = new Point(0, 112);// = false;

            //this.buttonAdvancedMode.Enabled = false;
            this.buttonAdvancedMode.Text = "Simple mode";

            if (this.newRule.AdvancedRule)
                this.buttonAdvancedMode.Enabled = false;
        }

        public void associateWithSystem(ConnectionDefinition in_System)
        {
            this.newRule.ApplyToAllSystems = false;
            this.newRule.AppliedSystems.Add(in_System.Name);
        }

        private void ResetComments()
        {
            this.textAdditionalComments.Text = "";
            this.textEmail.Text = "";
            this.textName.Text = "";
            this.textPlatform.Text = "";
        }

        private void CloseCommentsField()
        {
            this.panelComments.Visible = false;
            this.CommentsFieldOpen = false;
            this.Height = 414;
        }

        private void OpenCommentsField()
        {
            this.panelComments.Visible = true;
            this.CommentsFieldOpen = true;
            this.Height = 575;
        }        

        //private void RebuildMasters()
        //{
        //    BuildInitialConMasters();
        //}


        public void RebuildMasters()
        {

            //Load rules from folder

            this.BuildInitialConMasters();

            this.ActiveColNames.Clear();
            foreach (TreeNode tn in this.treeView_Systems.Nodes)
            {
                if (tn.Checked)
                {
                    ConnectionDefinition ws = (ConnectionDefinition)tn.Tag;
                    if (SystemTables.Contains(ws.Name))
                    {
                        foreach (Hashtable ht in (ArrayList)SystemTables[ws.Name])
                        {
                            String ColumnName = (String)ht["Name"];
                            if (!this.ActiveColNames.Contains(ColumnName))
                            {
                                this.ActiveColNames.Add(ColumnName);
                                if (!DefaultColNames.ContainsValue(ColumnName))
                                    DefaultColNames.Add(ColumnName, ColumnName);

                                Type conType = (Type)ht["Type"];
                                addNewCondition(ColumnName, conType);
                            }
                        }
                    }
                }
            }
        }

        public Alert GetAlertDetailsByType(AlertType at)
        {
            return AlertDict[at];
        }

        public void editRule(object ruleToEdit)
        {
            this.wizardFinish.Enabled = true;

            newRule = (ClientRule)ruleToEdit;
            if (this.newRule.AdvancedRule)
            {
                this.StartAdvancedMode();
            }
            else
            {
                this.StartSimpleMode();
            }
            this.ruleText.Controls.Clear();
            this.textBox_QueryName.Text = newRule.RuleName;
            this.checkBox_RuleEnabled.Checked = newRule.Enabled;
            this.editExistingRule = true;

            ListSystems();
            this.checkBox_allSystem.Checked = newRule.ApplyToAllSystems;

            ListConditions();
            ListAlerts();
            this.clientRuleWizardTabControl.SelectTab(0);// = 1;

            this.textAdditionalComments.Text = newRule.DetailsComments;
            this.textEmail.Text = newRule.DetailsCreatorEmail;
            this.textName.Text = newRule.DetailsCreatorName;
            this.textPlatform.Text = newRule.DetailsVersion;

            this.DisplayRule(newRule);
        }

        private void ListSystems() 
        {
            this.treeView_Systems.Nodes.Clear();
            foreach (DictionaryEntry de in ClientQueryRuler.Instance.SystemHash)
            {
                ConnectionDefinition ws = (ConnectionDefinition)de.Value;
                TreeNode newNode = new TreeNode();
                newNode.Text = ws.Name;
                newNode.Tag = ws;
                //If its set as the rule to apply
                newNode.Checked = false;
                if (this.newRule.AppliedSystems.Contains(ws.Name))
                    newNode.Checked = true;
    
                this.treeView_Systems.Nodes.Add(newNode);
            }
        }

        public void ListConditions()
        {
            currentStage = WizardStages.CONDITIONS;

            //Don't adjust the Rule Text box on selection...only for RULES display
            //this.listView1.SelectedIndexChanged -= new System.EventHandler(this.listView1_SelectedIndexChanged);           
            this.ruleConditionsTreeView.Nodes.Clear();

            foreach (ClientRuleCondition ruleCon in aCannedCons)
            {
                DisplayCondition("Pre-Configured Rules", ruleCon);
            }

            foreach (ClientRuleCondition ruleCon in aMasterCons)
            {
                DisplayCondition("Most Used", ruleCon);
            }

            foreach (ClientRuleCondition ruleCon in aDateTimeCons)
            {
                DisplayCondition("Date/Time", ruleCon);
            }

            foreach (ClientRuleCondition ruleCon in aMiscCons)
            {
                DisplayCondition("Miscellaneous", ruleCon);
            }

            if (!this.editExistingRule)
            {
                this.ruleConditionsTreeView.Nodes[0].Expand();
            }
        }

        private void ListAlerts()
        {
            /**
             *  Redundant code to remove all warnings!! Well only set to Alerts if currentStage is not Alerts.
             *  And currentStage is _not_ really used -- set but not used.
             */
            if (WizardStages.ALERTS != currentStage)
                currentStage = WizardStages.ALERTS;


            this.listView2.Items.Clear();
            foreach (AlertType alert in Enum.GetValues(typeof(AlertType)))
            {
                string alertText = AlertDict[alert].ListText;
                ListViewItem listViewAlert = new ListViewItem();
                listViewAlert.Checked = this.newRule.Alerts.isAlertActive(alert);

                if (alertText.Contains("{0}"))
                    alertText = alertText.Replace("{0}", ConDict[(AlertDict[alert]).LinkFieldType]);

                listViewAlert.Text = alertText;
                listViewAlert.Tag = alert;
                this.listView2.Items.Add(listViewAlert);
            }
        }


        private void textBoxExpression_TextChanged(object sender, System.EventArgs e)
        {

            DialogResult dr = MessageBox.Show("\nWarning: Manually modifying the text in Advanced Mode will \n" +
                                              "  \t permanently disable simple mode for this rule.\n\n",
                                              "Advanced Mode Edit Warning", MessageBoxButtons.OKCancel);
            if (dr.Equals(DialogResult.OK))
            {
                this.buttonAdvancedMode.Enabled = false;
                this.textBoxExpression.TextChanged -= new System.EventHandler(textBoxExpression_TextChanged);
                this.newRule.AdvancedRule = true;
            }
            else
            {
                this.textBoxExpression.Text = this.newRule.ExpressionOverride;
            }
        }

        private void treeView1_BeforeCheck(object sender, TreeViewCancelEventArgs e)
        {
            bool checkState = false;
            if (!e.Node.Checked.Equals(true))
                checkState = true;
            //IF the node checked is a 'header' node then toggle all child nodes
            if (e.Node.Nodes.Count != 0)
            {
                if (this.advancedMode)
                    return;
                foreach (TreeNode tn in e.Node.Nodes)
                {
                    newRule.SetCondition((ClientRuleCondition)tn.Tag, checkState);
                    tn.Checked = checkState;
                }
            }
            else
            {
                ClientRuleCondition rc = (ClientRuleCondition)e.Node.Tag;
                if (rc.IsAPredefinedRule)
                {
                    foreach (ClientRuleCondition rcon in rc.PredefinedRule.Conditions)
                    {
                        if (rcon.isEnabled)
                        {
                            TreeNode[] tna = this.ruleConditionsTreeView.Nodes.Find(rcon.QueryProperty.Trim(), true);
                            if (tna.Length == 0)
                            {
                                AddNewCondition(rcon);
                                tna = this.ruleConditionsTreeView.Nodes.Find(rcon.QueryProperty.Trim(), true);
                            }
                            if (tna.Length > 0)
                            {
                                newRule.replaceCondition(rcon, checkState);
                                if (tna[0].Checked != checkState)
                                    tna[0].Checked = checkState;
                            }
                        }
                    }
                }
                else
                {
                    ApplyConditionToRule(newRule, rc, checkState);
                }
            }
            DisplayRule(newRule);
        }

        //private void treeView_Systems_AfterCheck(object sender, TreeViewEventArgs e)
        //{
        //    if (!e.Node.Checked)
        //    {
        //        this.checkBox_allSystem.Checked = false;
        //    }
        //    RebuildMasters();
        //    this.ListConditions();
        //}

        private void LoadRulesFromFolder()
        {
            try
            {
                string homeDirectory = "";
                try
                {
                    homeDirectory = Persistence.HomeDirectory;
                }
                catch (Exception)
                {
                    homeDirectory = Environment.ExpandEnvironmentVariables("%HOMEDRIVE%%HOMEPATH%");
                }

                string[] filePaths = Directory.GetFiles(homeDirectory,
                    "*." + Trafodion.Manager.Properties.Resources.DefaultClientRuleFileExtention, SearchOption.AllDirectories);

                for (int i = 0; i < filePaths.Length; i++)
                {
                    string rulePathToLoad = filePaths[i];
                    ClientRule ruleToLoad = new ClientRule();
                    TextReader r = new StreamReader(rulePathToLoad);
                    try
                    {
                        ruleToLoad = (ClientRule)ruleSerializer.Deserialize(r);
                        ruleToLoad.BuildAlertDictionary();
                        r.Close();
                        //Generate the alert dictionary
                        if (ruleToLoad.Alerts.LoadAlertDictionary())
                            this.CannedCons.Add(new ClientRuleCondition(ruleToLoad));
                    }
                    catch
                    {
                    }                    
                }
            }
            catch (Exception ex)
            {
                //Error loading canned rules. no directory exists
            }            
        }

        private void AddNewCondition(ClientRuleCondition inRuleCon) 
        {
            if (inRuleCon.LinkFieldType == LinkType.DURATION || inRuleCon.LinkFieldType == LinkType.DATE_TIME)
            {
                if (!this.DateTimeCons.Contains(inRuleCon))
                {
                    //this.DateTimeCons.Add(inRuleCon);
                    DisplayCondition("Date/Time", (ClientRuleCondition)inRuleCon.Clone());
                }
            }
            else
            {
                if (!this.MiscCons.Contains(inRuleCon))
                {
                    //this.MiscCons.Add(inRuleCon);
                    DisplayCondition("Miscelaneous", (ClientRuleCondition)inRuleCon.Clone());
                }
            }
        }

        private void BuildDateTimeColumnList() 
        {
            //dateTimeColumnList.Add("START_TS");
            //dateTimeColumnList.Add("ENTRY_TS");
            //dateTimeColumnList.Add("COMP_START_TIME");
            //dateTimeColumnList.Add("COMP_END_TIME");
            //dateTimeColumnList.Add("EXEC_START_TIME");
            //dateTimeColumnList.Add("EXEC_END_TIME");
            //dateTimeColumnList.Add("FIRST_ROW_RETURNED_TIME");
            dateTimeColumnList.Add("CMP_LAST_UPDATED");
            dateTimeColumnList.Add("EXEC_LAST_UPDATED");
            
            dateTimeColumnList.Add("LAST_UPDATED");            
            dateTimeColumnList.Add("TOTAL_PROCESSOR_TIME");

        }

        private void BuildBoolColumnList() 
        {
            boolColumnList.Add("CMP_TXN_NEEDED");
            boolColumnList.Add("CMP_MANDATORY_X_PROD");
            boolColumnList.Add("CMP_MISSING_STATS");
            boolColumnList.Add("CMP_FULL_SCAN_ON_TABLE");
            boolColumnList.Add("AGGR_QUERY");
        }

        private void BuildStringTypeColumnListFromIntType() 
        {
            stringColumnList.Add("SQL_ERROR_CODE");
            stringColumnList.Add("LAST_ERROR_BEFORE_AQR");
            stringColumnList.Add("STATS_ERROR_CODE");
            //stringColumnList.Add("TRANSACTION_ID");
            stringColumnList.Add("QUERY_PRIORITY");
            stringColumnList.Add("ERROR_CODE");
            stringColumnList.Add("INSTANCE_ID");
            stringColumnList.Add("NODE_ID");
            stringColumnList.Add("PNID_ID");
            stringColumnList.Add("PROCESS_ID");
        }

        private void BuildDurationTypeColumnListFromDateTimeType()
        {
            durationColumnList.Add("ELAPSED_TIME");
            durationColumnList.Add("ESTIMATED_TIME");
            durationColumnList.Add("WAIT_TIME");
            durationColumnList.Add("HOLD_TIME");            
            durationColumnList.Add("TOTAL_QUERY_TIME");            
            durationColumnList.Add("OPEN_TIME");
            durationColumnList.Add("PROCESS_CREATE_TIME");         
            durationColumnList.Add("LAST_INTERVAL_PROCESSOR_TIME");
            durationColumnList.Add("DELTA_PROCESSOR_TIME");
        }

        private void BuildValueTypeColumnListFromDateTimeType() 
        {
            valueTimeTypeColumnList.Add("SQL_CPU_TIME");
            valueTimeTypeColumnList.Add("UDR_PROCESS_BUSY_TIME");
            valueTimeTypeColumnList.Add("DISK_PROCESS_BUSY_TIME");
            valueTimeTypeColumnList.Add("OPEN_BUSY_TIME");
            valueTimeTypeColumnList.Add("DISK_PROCESS_BUSYTIME");
            valueTimeTypeColumnList.Add("PROCESS_BUSYTIME_SEC");
            valueTimeTypeColumnList.Add("DELAY_TIME_BEFORE_AQR_SEC");
            valueTimeTypeColumnList.Add("PROCESS_CREATE_TIME_SEC");
            valueTimeTypeColumnList.Add("OPEN_TIME_SEC");
            valueTimeTypeColumnList.Add("SQL_CPU_TIME_SEC");  
            valueTimeTypeColumnList.Add("EST_IO_TIME");
            valueTimeTypeColumnList.Add("EST_MSG_TIME");
            valueTimeTypeColumnList.Add("EST_IDLE_TIME");
            valueTimeTypeColumnList.Add("EST_CPU_TIME");
            valueTimeTypeColumnList.Add("EST_TOTAL_TIME");
            valueTimeTypeColumnList.Add("PROCESSES_CREATED");
            valueTimeTypeColumnList.Add("MASTER_EXECUTION_TIME_SEC");
            valueTimeTypeColumnList.Add("MASTER_EXECUTOR_TIME");
            valueTimeTypeColumnList.Add("QUERY_ELAPSED_TIME");
            valueTimeTypeColumnList.Add("AGGR_SECS_TOTAL_TIME");
            valueTimeTypeColumnList.Add("MSG_TIME_ESTIMATE");
            valueTimeTypeColumnList.Add("DISK_PROCESS_BUSY_TIME_SEC");
            valueTimeTypeColumnList.Add("COMPILATION_TIME");
            valueTimeTypeColumnList.Add("IO_TIME_ESTIMATE");
            valueTimeTypeColumnList.Add("MSG_TIME_ESTIMATE");
            valueTimeTypeColumnList.Add("IDLE_TIME_ESTIMATE");
            valueTimeTypeColumnList.Add("ESTIMATED_TOAL_TIME");
            valueTimeTypeColumnList.Add("PROCESS_CREATION_TIME");
            valueTimeTypeColumnList.Add("COMPILE_ELAPSED_TIME");
            valueTimeTypeColumnList.Add("ESTIMATED_TOTAL_TIME");            
        }


        private void BuildPreDefinedValueHashTable() 
        {
            //preDefinedValues.Add(States.Any
            foreach(States s in Enum.GetValues(typeof(States)))
            {
                preDefinedValues.Add(s.ToString(), "States");
            }

            //preDefinedValues.Add(

        }


        private void addNewCondition(String ColumnName, Type conType)
        {
            if (ClientQueryRuler.isAQueryRulerColumn(ColumnName))
                return;

            //Data Type get from WMS Command DataTable needs to change so that value input windows can display appropriately

            //Condition 1: Duration type
            if (durationColumnList.Contains(ColumnName))
            {
                this.DateTimeCons.Add(new ClientRuleCondition("The query's " + ColumnName + " is longer than {0}",
                                                           ColumnName, 1, LinkType.DURATION, ColumnName, ">", ""));
                return;
            }

            //Condition 2: Datetime type
            if (conType.Equals(typeof(System.DateTime)) || (ColumnName.Contains("TIME") && !valueTimeTypeColumnList.Contains(ColumnName)) || ColumnName.Contains("_TS") || dateTimeColumnList.Contains(ColumnName))
            {
                //LinkType theLinkType = LinkType.VALUE;
                //String theText = "exceeds";
                //if (conType.Equals(typeof(DateTime)) || dateTimeColumnList.Contains(ColumnName))
                //{
                //    theLinkType = LinkType.DATE_TIME;
                //    theText = "is after";
                //}

                LinkType theLinkType = LinkType.DATE_TIME;
                String theText = "is after";

                this.DateTimeCons.Add(new ClientRuleCondition("The query's " + ColumnName + " " + theText + " {0}",
                                                           ColumnName, 1, theLinkType, ColumnName, ">", ""));
                return;
            }

            //equal value type
            if (stringColumnList.Contains(ColumnName)) 
            {
                this.MiscCons.Add(new ClientRuleCondition("The query's " + ColumnName + " is {0}", ColumnName, 1,
                                                      LinkType.VALUE, ColumnName, "=", ""));
                return;
            }

            //string Type
            if (conType.Equals(typeof(System.String)) ) 
            {
                this.MiscCons.Add(new ClientRuleCondition("Query property " + ColumnName + " equals {0}", ColumnName, 1,
                                                       LinkType.STRING, ColumnName, "LIKE", ""));
                return;
            }

            if (boolColumnList.Contains(ColumnName))
            {
                this.MiscCons.Add(new ClientRuleCondition("The query's " + ColumnName + " is {0}",
                                                           ColumnName, 1, LinkType.BOOLEAN, ColumnName, "=", ""));
                return;
            }


            if (conType.Equals(typeof(System.Int32))
                || conType.Equals(typeof(System.Int64))
                || conType.Equals(typeof(System.Int16)))
            {
                this.MiscCons.Add(new ClientRuleCondition("The query's " + ColumnName + " exceeds {0}", ColumnName, 1,
                                                       LinkType.VALUE, ColumnName, ">", ""));
                return;
            }
            if (conType.Equals(typeof(System.Decimal))
                || conType.Equals(typeof(System.Double)))
            {
                this.MiscCons.Add(new ClientRuleCondition("The query's " + ColumnName + " exceeds {0}", ColumnName, 1,
                                                       LinkType.FLOAT_VALUE, ColumnName, ">", ""));
                return;
            }

            this.MiscCons.Add(new ClientRuleCondition("The query's " + ColumnName + " exceeds {0}", ColumnName, 1,
                                                   LinkType.FLOAT_VALUE, ColumnName, "LIKE", ""));
        }


        private void ApplyConditionToRule(ClientRule newRule, ClientRuleCondition rc, bool checkState)
        {
            newRule.SetCondition(rc, checkState);
            if (this.advancedMode)
            {
                string ruleExp = rc.QueryProperty + " " + rc.Comparator + " " + rc.Threshold;
                string suffix = " '*'";
                if (rc.LinkFieldType == LinkType.VALUE
                    || rc.LinkFieldType == LinkType.FLOAT_VALUE
                    || rc.LinkFieldType == LinkType.AMOUNT)
                    suffix = " ";


                if (this.textBoxExpression.Text != "")
                    this.textBoxExpression.Text = "(" + this.textBoxExpression.Text + ") AND " + ruleExp + suffix;
                else
                    this.textBoxExpression.Text = ruleExp + suffix;
            }
        }

        private void DisplayRule(ClientRule ruleToDisplay)
        {
            int startY = 4;
            int startX = 4;
            int index = 0;

            this.ruleText.SuspendLayout();
            this.ruleText.Controls.Clear();
            //this.advancedMode = ruleToDisplay.advancedRule; 

            if (!this.advancedMode)
            {
                string displayHeader = "When a query:";
                Label lbl = new Label();
                lbl.Text = displayHeader;
                lbl.AutoSize = true;
                this.ruleText.Controls.Add(lbl);
                lbl.SetBounds(2, startX, lbl.Width, lbl.Height);
                startY += lbl.Height + 4;
                startX = 12;

                DisplayConditions(ruleToDisplay, ref startX, ref startY, ref index);
            }

            DisplayAlerts(ruleToDisplay, ref startX, ref startY, ref index);

            this.ruleText.ResumeLayout();
            this.wizardFinish.Enabled = true;
        }


        //This function is for displaying Conditions in the
        //Rule Summary area.
        private void DisplayConditions(ClientRule qr, ref int Xcord, ref int Ycord, ref int index)
        {
            string prefix = "";
            foreach (ClientRuleCondition rc in qr.Conditions)
            {
                if (rc.isEnabled)
                {
                    try
                    {
                        string linkText = ConDict[rc.LinkFieldType];
                        string condText = "";
                        int adjXcord = Xcord;
                        int adj2Xcord = Xcord;
                        for (int i = 0; i < rc.Threshold.Count; i++)
                        {
                            condText = rc.Text;

                            if (prefix != "" && i == 0)
                                condText = prefix + condText.Substring(0, 1).ToLower() + condText.Substring(1);

                            LinkLabel ll = new LinkLabel();
                            ll.Font = new Font("Calibri", 9.75F, FontStyle.Regular);
                            ll.AutoSize = true;

                            if (rc.isComplete)
                                linkText = GetFormattedLinkText(rc, i);

                            if (i > 0)
                            {
                                condText = "Or {0}";
                                adj2Xcord = adjXcord;
                            }

                            if (i == 0)
                            {
                                //Must fix this logic
                                Size textSize = TextRenderer.MeasureText(condText.Substring(0, condText.IndexOf('{')), ll.Font);
                                adjXcord = textSize.Width += 3 + adj2Xcord - TextRenderer.MeasureText(" Or", ll.Font).Width;
                            }

                            ll.Text = condText.Replace("{0}", linkText);
                            ll.LinkArea = new LinkArea(condText.LastIndexOf('{'), linkText.Length);
                            ll.Click += new EventHandler(labelCondition_Click);

                            ll.SetBounds(adj2Xcord, Ycord, ll.Width, ll.Height);
                            ll.Tag = rc;
                            this.ruleText.Controls.Add(ll);
                            Ycord += ll.Height;
                        }

                        if (rc.Threshold.Count > 1) { Ycord += 2; }

                        prefix = "And ";
                        index++;
                    }
                    catch (Exception e)
                    {
                        MessageBox.Show("\nError: Failed to modify rule condition parameters. \n" +
                                        "Problem: \t Unable to modify rule condition parameters.\n\n" +
                                        "Solution: \t Please see error details for recovery information.\n\n" +
                                        "Details: \t " + e.Message + "\n\n",
                                        "Error Modifying Rule Condition Parameters", MessageBoxButtons.OK, MessageBoxIcon.Error);

                    }

                }
            }
        }

        private void DisplayCondition(string nodeHeader, ClientRuleCondition ruleCon)
        {
            if (!this.ruleConditionsTreeView.Nodes.ContainsKey(nodeHeader))
            {
                this.ruleConditionsTreeView.Nodes.Add(nodeHeader, nodeHeader);
            }
            try
            {

                TreeNode newNode = new TreeNode();
                newNode.Tag = ruleCon;
                newNode.Text = ruleCon.ListText;
                newNode.ImageKey = ruleCon.ListText;
                newNode.Name = ruleCon.QueryProperty.Trim();

                newNode.Checked = this.newRule.IsConditionEnabled(ruleCon);
                this.ruleConditionsTreeView.Nodes[nodeHeader].Nodes.Add(newNode);
                if (newNode.Checked)
                    newNode.Parent.Expand();
            }
            catch (Exception) { }
        }

        //This function is for displaying alerts in the
        //Rule Summary area.
        private void DisplayAlerts(ClientRule qr, ref int Xcord, ref int Ycord, ref int index)
        {
            //this.listView1.Visible = true;

            Xcord = 4;
            string displayHeader = "Then:";
            Label lbl = new Label();
            lbl.Text = displayHeader;
            lbl.AutoSize = true;
            this.ruleText.Controls.Add(lbl);
            lbl.SetBounds(Xcord, Ycord, lbl.Width, lbl.Height);
            Ycord += lbl.Height;
            Xcord = 12;

            string prefix = "";

            foreach (AlertType alert in Enum.GetValues(typeof(AlertType)))
            {
                if (qr.Alerts.isAlertActive(alert))
                {
                    LinkLabel ll = new LinkLabel();
                    ll.AutoSize = true;

                    Alert currentAlert = AlertDict[alert];
                    string linkText = ConDict[currentAlert.LinkFieldType];

                    //If this alert has all required fields completed....
                    if (qr.Alerts.AlertTypeIsComplete(alert))
                        linkText = qr.Alerts.getValue(alert);

                    string alertText = currentAlert.AlertText;

                    if (prefix != "")
                        alertText = prefix + alertText.Substring(0, 1).ToLower() + alertText.Substring(1);

                    //If the alert has an input field
                    if (alertText.Contains("{0}"))
                    {
                        ll.Text = alertText.Replace("{0}", linkText);
                        ll.LinkArea = new LinkArea(alertText.LastIndexOf("{"), linkText.Length);
                    }
                    else
                    {
                        ll.Text = alertText;
                        ll.LinkArea = new LinkArea();
                    }

                    ll.Click += new EventHandler(labelAction_Click);

                    ll.SetBounds(Xcord, Ycord, ll.Width, ll.Height);
                    ll.Tag = alert;
                    this.ruleText.Controls.Add(ll);
                    Ycord += ll.Height;
                    Xcord = 12;
                    prefix = "And ";
                    index++;
                }
            }
        }

        private String GetFormattedLinkText(ClientRuleCondition ruleCond, int idx)
        {
            String theValue = ruleCond.Threshold[idx].ToString();

            switch (ruleCond.LinkFieldType)
            {
                case LinkType.DURATION:
                    try
                    {
                        TimeSpan theTimeSpan = TimeSpan.FromMilliseconds(Double.Parse(theValue));
                        theValue = WMSUtils.getFormattedElapsedTime(theTimeSpan);
                    }
                    catch (Exception)
                    {
                    }
                    break;

                case LinkType.FLOAT_VALUE:
                    try
                    {
                        Double dVal = Double.Parse(theValue);
                        String theFormat = TriageHelper.getLocaleNumberFormat(3, true);
                        theValue = String.Format(theFormat, dVal);
                    }
                    catch (Exception)
                    {
                    }
                    break;

                case LinkType.VALUE:
                    try
                    {
                        Int64 intValue = Int64.Parse(theValue);
                        String theFormat = TriageHelper.getNumberFormatForCurrentLocale(0);
                        theValue = String.Format(theFormat, intValue);

                    }
                    catch (Exception)
                    {
                    }
                    break;

                default: break;
            }
            return theValue;
        }

        private void labelCondition_Click(object sender, EventArgs e)
        {
            LinkLabel ll = (LinkLabel)sender;
            ClientRuleCondition rc = ll.Tag as ClientRuleCondition;

            /*if(this.applyToAllWS && rc.q)
                rc.isComplete = rc.updateValue(rc.Threshold, NCCQueryRuler.Instance.WorkspaceHash);
            else*/
            rc.isComplete = rc.UpdateValue(rc.Threshold, activeSystem);
            DisplayRule(this.newRule);
        }

        private void labelAction_Click(object sender, EventArgs e)
        {
            LinkLabel ll = (LinkLabel)sender;
            AlertType at = (AlertType)ll.Tag;
            this.NewRule.Alerts.updateValue(at);
            DisplayRule(this.newRule);
        }

        private void btnSaveRule_Click(object sender, EventArgs e)
        {
            String ruleSavePath = "";
            TextWriter ruleWriter = null;

            try
            {
                //Add more error handlin'

                if (!this.ValidateRule(true))
                {
                    return;
                }

                bool alertValidated = ValidateAlert();
                if (!alertValidated) 
                {
                    return;
                }

                this.FinalizeRule();

                newRule.AppliedSystems.Clear();
                foreach (TreeNode tn in this.treeView_Systems.Nodes)
                {
                    if (tn.Checked)
                    {
                        newRule.AppliedSystems.Add(((ConnectionDefinition)tn.Tag).Name);
                    }
                }

                ClientRule ruleToSave = this.newRule;

                SaveFileDialog saveFileDialog1 = new SaveFileDialog();
                saveFileDialog1.Title = "Save Rule";
                saveFileDialog1.FileName = ruleToSave.RuleName;
                string folderName = ClientRuleOptions.GetOptions().ClientRuleFolderName;
                try
                {
                    if (!Directory.Exists(folderName)) Directory.CreateDirectory(folderName);
                }
                catch (Exception)
                {
                    MessageBox.Show("\nError: Failed to create client rule folder: " + folderName);
                }

                saveFileDialog1.InitialDirectory = folderName;
                saveFileDialog1.DefaultExt = "xml";
                saveFileDialog1.Filter = "Extensible Markup Language (*.xml)|*.xml|All files (*.*)|*.*";

                DialogResult result = saveFileDialog1.ShowDialog();
                if (result == DialogResult.OK)
                {
                    ruleSavePath = saveFileDialog1.FileName;
                    ruleWriter = new StreamWriter(ruleSavePath);
                    if (ruleToSave.Alerts.saveAlertDictionary()) 
                    {
                        //ruleSerializer = new XmlSerializer(typeof(ClientRule));
                        ruleSerializer.Serialize(ruleWriter, ruleToSave);
                    }
                        

                    ruleWriter.Close();
                    ruleWriter = null;
                }
            }
            catch (Exception ex)
            {
                String errDetails = ex.Message;
                if (null != ex.InnerException)
                    errDetails += "\n       \t " + ex.InnerException.Message;

                MessageBox.Show("\nError: Failed to save a rule to file : \n" +
                                "                \t\"" + ruleSavePath + "\"\n\n" +
                                "Problem: \t Unable to save rule details. \n\n" +
                                "Solution: \t Please see error details for recovery information. \n\n" +
                                "Details: \t " + errDetails + " \n\n",
                                "Error Saving Rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                if (null != ruleWriter)
                    try { ruleWriter.Close(); }
                    catch (Exception) { }
            }

        }

        private bool ValidateRule(bool validateSystem)
        {
            //Validate Result
            bool systemSelected = false;
            bool ruleNameEntered = false;

            if (!this.textBox_QueryName.Text.Equals(""))
            {
                ruleNameEntered = true;
                foreach (TreeNode tn in this.treeView_Systems.Nodes)
                {
                    if (tn.Checked)
                        systemSelected = true;
                }
            }
            if (!ruleNameEntered || (!systemSelected && !newRule.ApplyToAllSystems && validateSystem))
            {
                string errorString = "";
                if (!ruleNameEntered)
                    errorString += "Please specify a name for this rule.";
                else if (!systemSelected)
                    errorString += "Please select at least one system";
                MessageBox.Show("\nError: Rule validation failures. \n" +
                                        "Problem: \t Unable to create or modify the rule.\n\n" +
                                        "Solution: \t " + errorString + "\n\n",
                                        "Rule Validation Errors", MessageBoxButtons.OK, MessageBoxIcon.Error);

                return false;
            }
            return true;
        }

        private bool ValidateAlert() 
        {
            bool checkPassed = true;
            string errorString="";
            //check alert writing to log is checked, but filepath is not provided;
            bool IsLog = newRule.Alerts.ActiveAlertDict[AlertType.LOG];
            if(IsLog)
            {
                if (!VerifyFilePath(newRule.Alerts.FileToLog)) 
                {
                    errorString += "Please speficy a valid log file path.\n";
                    checkPassed = false;
                }
                    
            }

            if(!string.IsNullOrEmpty(errorString))
                MessageBox.Show("\nError: Rule validation failures. \n" +
                                        "Problem: \t " + errorString + "\n\n",
                                        "Rule Validation Errors", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return checkPassed;
        }

        private bool VerifyFilePath(string filePath) 
        {
            MatchCollection matches = Regex.Matches(filePath, @"^(([a-zA-Z]\:)|(\\))(\\{1}|((\\{1})[^\\]([^/:*?<>""|]*))+)$");
            if (matches!=null&&matches.Count>0)
                return true;
            else
                return false;
        }


        private void FinalizeRule()
        {
            newRule.DetailsComments = this.textAdditionalComments.Text;
            newRule.DetailsCreatorEmail = this.textEmail.Text;
            newRule.DetailsCreatorName = this.textName.Text;
            newRule.DetailsVersion = this.textPlatform.Text;
            newRule.ApplyToAllSystems = this.checkBox_allSystem.Checked;
            newRule.Enabled = this.checkBox_RuleEnabled.Checked;
            if (this.advancedMode)
                newRule.ExpressionOverride = this.textBoxExpression.Text;
            newRule.RuleName = this.textBox_QueryName.Text;
        }

        private void buttonAdvancedMode_Click(object sender, EventArgs e)
        {
            if (this.advancedMode)
                StartSimpleMode();
            else
                StartAdvancedMode();

            this.DisplayRule(this.newRule);
        }

        private void buttonComments_Click(object sender, EventArgs e)
        {
            if (this.CommentsFieldOpen)
                CloseCommentsField();
            else
                OpenCommentsField();
        }


        private void checkBox_allSystem_Click(object sender, EventArgs e)
        {
            CheckToggleAllSystems((this.checkBox_allSystem.Checked));    
            this.treeView_Systems.Enabled = !checkBox_allSystem.Checked;            
            
        }

        private void checkBox_allConnect_Click(object sender, EventArgs e)
        {
            if (checkBox_allConnect.Checked == true)
            {
                checkBox_allSystem.Checked = false;
                this.treeView_Systems.Enabled = true;
                CheckToggleAllConnectedSystems();
            }
            else 
            {
                checkBox_allSystem.Checked = false;
                checkBox_allSystem_Click(sender,e);
            }            
        }

        private void treeView_Systems_AfterCheck(object sender, TreeViewEventArgs e)
        {
            checkBox_allSystem.Checked = IsAllTreeViewNodesChecked();
            checkBox_allConnect.Checked = IsAllConnectedNodesChecked();

            RebuildMasters();
            this.ListConditions();
        }

        private bool IsAllTreeViewNodesChecked() 
        {
            foreach (TreeNode tn in this.treeView_Systems.Nodes)
            {
                if (!tn.Checked)
                    return false;
            }
            return true;    
        }

        private bool IsAllConnectedNodesChecked() 
        {
            int connectedCount=0, connectedCheckedCount=0;
            foreach (TreeNode tn in this.treeView_Systems.Nodes)
            {
                ConnectionDefinition theConn = (ConnectionDefinition)tn.Tag;
                if (theConn.TheState == ConnectionDefinition.State.TestSucceeded) 
                {
                    connectedCount++;
                    if (tn.Checked)
                        connectedCheckedCount++;
                }
            }
            if ((connectedCount == connectedCheckedCount) && connectedCount > 0)
                return true;
            else
                return false;
        }


        private void CheckToggleAllSystems(bool check)
        {
            foreach (TreeNode tn in this.treeView_Systems.Nodes)
            {
                tn.Checked = check;
            }
        }

        private void CheckToggleAllConnectedSystems()
        {
            foreach (TreeNode tn in this.treeView_Systems.Nodes)
            {
                ConnectionDefinition theConn = (ConnectionDefinition)tn.Tag;
                if (theConn.TheState == ConnectionDefinition.State.TestSucceeded)
                    tn.Checked = true;
                else
                    tn.Checked = false;
            }
        }

        private void listView2_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            listView2.SelectedItems.Clear();
            listView2.Items[e.Index].Selected = true;
            listView2.Items[e.Index].Focused = true;

            bool checkState = false;
            if (e.NewValue == CheckState.Checked)
                checkState = true;

            AlertType at = (AlertType)listView2.Items[e.Index].Tag;
            newRule.Alerts.SetAlert(at, checkState);
            DisplayRule(newRule);
        }
        
        private void wizardFinish_Click(object sender, EventArgs e)
        {
            //Validate result
            if (!ValidateRule(false))
                return;

            bool alertValidated = ValidateAlert();
            if (!alertValidated)
            {
                return;
            }

            FinalizeRule();

            //load the workspaces
            newRule.AppliedSystems.Clear();
            foreach (TreeNode tn in this.treeView_Systems.Nodes)
            {
                if (tn.Checked)
                {
                    newRule.AppliedSystems.Add(((ConnectionDefinition)tn.Tag).Name);
                }
            }

            this.DialogResult = DialogResult.OK;      

            this.Close();
        }

        private void wizardCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        public void AddDTCols_To_Hash(ConnectionDefinition inSystem, DataTable in_DT)
        {
            ArrayList ColumnEntry = new ArrayList();

            //Build conditions list from DataTable
            foreach (DataColumn dc in in_DT.Columns)
            {
                Hashtable newColHash = new Hashtable();
                String ColumnName = dc.ColumnName;
                Type conType = dc.DataType;
                newColHash.Add("Name", ColumnName);
                newColHash.Add("Type", conType);
                ColumnEntry.Add(newColHash);
            }

            if (SystemTables.Contains(inSystem.Name))
                this.SystemTables[inSystem.Name] = ColumnEntry;
            else
                this.SystemTables.Add(inSystem.Name, ColumnEntry);
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ClientThresholdRuleWizard);
        }
       
    }
}
