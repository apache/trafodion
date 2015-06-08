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
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using System.Xml.Serialization;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public enum LinkType
    {
        FLOAT_VALUE, VALUE, AMOUNT, APPLICATION, USER_ID, QUERY_ID, EXCEED, COLOR, ALERT, PROPERTY,
        TOGGLE, CLIENT_ID, USER_NAME, SESSION_ID, SQL_TEXT, ERROR_CODE, DATASOURCE, STRING, DATE_TIME,
        STATE, FILE_NAME, DURATION, BOOLEAN
    }

    public partial class ClientRuleManager : TrafodionForm
    {
        public ClientRuleWizard ruleWizard;
        private ArrayList m_aRulesList;
        public ArrayList RulesList
        {
            get { return m_aRulesList; }
            set { m_aRulesList = value; }
        }

        private string keyOfRuleIGridSelectRow="";
        public string KeyOfRuleIGridSelectRow
        {
            get { return keyOfRuleIGridSelectRow; }
            set { keyOfRuleIGridSelectRow = value; }
        }

        private XmlSerializer ruleSerializer;
        private int selectedIndex = 0;
        private iGCellStyle m_RuleStyle; 

        

        public ClientRuleManager()
        {
            InitializeComponent();
            SetupRuleManager();
        }

        private void SetupRuleManager()
        {
            //Visual Studio Designer will remove the two lines of codes automatically
            //in ClientRuleManager.Designer.cs, so add them here.
            this.clientRuleManagerIGrid.ReadOnly = false;
            this.clientRuleManagerIGrid.RowMode = false;
            
            ruleSerializer = new XmlSerializer(typeof(ClientRule));
            ruleWizard = new ClientRuleWizard(this);
            m_aRulesList = new ArrayList();

            if (clientRuleManagerIGrid.SelectedCells.Count > 0)
                enableDetails();
            else
                disableDetails();

            //Define default Rule Style
            m_RuleStyle = new iGCellStyle();
            m_RuleStyle.Font = new Font(this.clientRuleManagerIGrid.Font.FontFamily, this.clientRuleManagerIGrid.Font.Size, FontStyle.Regular);
            m_RuleStyle.ForeColor = Color.Black;
            
            //Remove Default Double Click Event of iGrid, now double click a cell will popup edit rule window
            if (this.clientRuleManagerIGrid.DoubleClickHandler != null)          
                this.clientRuleManagerIGrid.DoubleClickHandler = null;
            CenterToParent();
        }


        public bool LoadOpenSystems()
        {
            if (this.clientRuleManagerIGrid.Rows.Count > 0)
            {
                this.clientRuleManagerIGrid.Rows.Clear();
            }

            //Create a pattern for workspaces
            iGRowPattern myRowPattern = new iGRowPattern();

            //Set properties common to all the rows. 
            myRowPattern.Height = 20;
            myRowPattern.Type = iGRowType.ManualGroupRow;
            myRowPattern.Level = 0;
            myRowPattern.TreeButton = iGTreeButtonState.Visible;

            iGRow row = null;

            //For each system, create a row entry
            foreach (DictionaryEntry de in ClientQueryRuler.Instance.SystemHash)
            {
                ConnectionDefinition tempSystem = (ConnectionDefinition)de.Value;
                row = this.clientRuleManagerIGrid.Rows.Insert(0, myRowPattern);
                row.RowTextCell.Value = tempSystem.Name;
                row.Key = tempSystem.Name;
                row.Tag = de.Value;
                row.ReadOnly = iGBool.True;
            }

            // ** Create tree-node roots for "All Systems" options
            row = this.clientRuleManagerIGrid.Rows.Insert(0,myRowPattern);
            row.RowTextCell.Value = "Global Rules";
            row.Key = "All";
            row.ReadOnly = iGBool.True;
            /*/For rules not applied to any Workspaces
                row = this.iGridRulerManager.Rows.Add(myRowPattern);
                row.RowTextCell.Value = "Not Enabled";
                row.Key = "None";
            */

            //Add rules
            //foreach (ClientRule qr in m_aRulesList)
            for(int i =0;i<m_aRulesList.Count;i++)
            {
                ClientRule qr = (ClientRule)m_aRulesList[i];
                if (qr.ApplyToAllSystems)
                {
                    int index = this.clientRuleManagerIGrid.Rows["All"].Index;
                    iGRow tempRow = this.clientRuleManagerIGrid.Rows.Insert(index + 1);
                    tempRow.Level = 1;
                    tempRow.Cells["Enabled"].Value = qr.Enabled;
                    tempRow.Cells["Popup"].Value = qr.Popup;
                    tempRow.Cells["Rule"].Value = qr.RuleName;
                    tempRow.Cells["Cell"].Value = "Text";
                    tempRow.Cells["Row"].Value = "Text";
                    tempRow.Tag = qr;
                    StyleRow(tempRow, qr.Alerts);

                } else {
                
                //foreach (NCCWorkspace ws in qr.AppliedWorkspaces)
                for(int j=0;j<qr.AppliedSystems.Count;j++)
                {                    
                    ConnectionDefinition ws = ConnectionDefinition.Find((string)qr.AppliedSystems[j]);
                    //Check to make sure that the workspace is still open before you assume an index
                    int index = -1;
                    try
                    {
                        index = this.clientRuleManagerIGrid.Rows[ws.Name].Index;
                    }
                    catch (Exception) {
                        index = -1;
                        //The user closed a workspace that had rules associated w/ it
                    }

                    //If the workspace has been closed, remove the association.
                    if (index >= 0)
                    {
                        iGRow tempRow = this.clientRuleManagerIGrid.Rows.Insert(index + 1);
                        tempRow.Level = 1;
                        tempRow.Cells["Enabled"].Value = qr.Enabled;
                        tempRow.Cells["Popup"].Value = qr.Popup;
                        tempRow.Cells["Rule"].Value = qr.RuleName;
                        tempRow.Cells["Cell"].Value = "Text";
                        tempRow.Cells["Row"].Value = "Text";
                        tempRow.Tag = qr;
                        StyleRow(tempRow, qr.Alerts);
                    }
                    else {
                        //Remove from appliedWorkspaces
                        qr.AppliedSystems.Remove(ws);
                    }
                }

                //If this rule no longer applies to any *OPEN* workspaces, delete the rule.
                if (qr.AppliedSystems.Count <= 0)
                {
                    //Remove the rule from the list
                    m_aRulesList.Remove(qr);
                }
                }
            }

            
            if (this.clientRuleManagerIGrid.Rows.Count > 0)
            {
                if (this.clientRuleManagerIGrid.Rows.Count <= selectedIndex)
                    selectedIndex = this.clientRuleManagerIGrid.Rows.Count - 1;

                //this.clientRuleManagerIGrid.Rows[selectedIndex].Cells[0].Selected = true;
                this.clientRuleManagerIGrid.Rows[selectedIndex].Selected = true;
                this.clientRuleManagerIGrid.CurRow = this.clientRuleManagerIGrid.Rows[selectedIndex];
                if (this.clientRuleManagerIGrid.CurRow.Key == null)
                {
                    this.selectRuleInGrid((ClientRule)this.clientRuleManagerIGrid.CurRow.Tag);
                }
            }


            return true;
        }

        private void StyleRow(iGRow tempRow, ClientRuleAlert ra)
        {
            tempRow.Level = 1;
            // ****** Style 'n Format the new rule **********           
            foreach (iGCell gc in tempRow.Cells)
            {
                gc.ForeColor = Color.Black;
                gc.Font = new Font(this.clientRuleManagerIGrid.Font.FontFamily, this.clientRuleManagerIGrid.Font.Size, FontStyle.Regular);
            }

            tempRow.Cells["Row"].Style.ForeColor = Color.Blue;
            tempRow.Cells["Cell"].Style.ForeColor = Color.Blue;

            if (ra.ActiveAlertDict[AlertType.BACK_COL] && ra.BackgroundColor != null)
            {
                tempRow.Cells["Row"].BackColor = Color.FromArgb(int.Parse(ra.BackgroundColor));
                tempRow.Cells["Cell"].BackColor = Color.FromArgb(int.Parse(ra.BackgroundColor));
            }

            if (ra.ActiveAlertDict[AlertType.CELL_COL] && ra.ViolatorColor != null)
                tempRow.Cells["Cell"].BackColor = Color.FromArgb(int.Parse(ra.ViolatorColor));

            if (ra.ActiveAlertDict[AlertType.FORE_COL] && ra.ForegroundColor != null)
            {
                tempRow.Cells["Row"].ForeColor = Color.FromArgb(int.Parse(ra.ForegroundColor));
                tempRow.Cells["Cell"].ForeColor = Color.FromArgb(int.Parse(ra.ForegroundColor));
            }
            //****** End rule stylin' ********** 
        }

        public bool LoadExistingRules(ArrayList in_RulesList)
        {
            m_aRulesList = in_RulesList;
            LoadOpenSystems();
            return true;
        }

        public void LoadRulesFromFolder()
        {
            try
            {
                string homeDirectory = "";
                try
                {
                    homeDirectory = ClientRuleOptions.GetOptions().ClientRuleFolderName;
                }
                catch (Exception)
                {
                    homeDirectory = Persistence.HomeDirectory;
                }

                string[] filePaths = Directory.GetFiles(homeDirectory,
                    "*." + Trafodion.Manager.Properties.Resources.DefaultClientRuleFileExtention, SearchOption.AllDirectories);

                string errorLoadMsg = "";

                for (int i = 0; i < filePaths.Length; i++)
                {
                    string rulePathToLoad = filePaths[i];
                    ClientRule ruleToLoad = new ClientRule();
                    TextReader r = new StreamReader(rulePathToLoad);

                    try
                    {
                        ruleToLoad = (ClientRule)ruleSerializer.Deserialize(r);
                        ruleToLoad.BuildAlertDictionary();
                        //Generate the alert dictionary
                        if (ruleToLoad.Alerts.LoadAlertDictionary() && !CheckClientRuleInRuleList(this.RulesList, ruleToLoad))
                            this.RulesList.Add(ruleToLoad);
                    }
                    catch (Exception loadException)
                    {
                        Logger.OutputErrorLog("Loading client rule " + rulePathToLoad + " failed : " + loadException.Message);
                        errorLoadMsg += rulePathToLoad+"\n";
                    }
                    finally 
                    {
                        r.Close();
                    }
                }
                if (!errorLoadMsg.Equals(""))
                {
                    MessageBox.Show(" The following client rules are not loaded: " + errorLoadMsg + "\n Please check the TrafodionManager log file for details.", "Error loading all client rules", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            catch (Exception ex)
            {
                //Error loading canned rules. no directory exists
            }
        }

        private bool CheckClientRuleInRuleList(ArrayList argRuleList, ClientRule argClientRule) 
        {
            if (argRuleList == null || argRuleList.Count == 0)
                return false;

            for (int i = 0; i < argRuleList.Count; i++) 
            {
                ClientRule CR = (ClientRule)argRuleList[i];
                if (CR.RuleName.Equals(argClientRule.RuleName))
                    return true;
            }
            return false;
        }

        private void openRuleWizard()
        {
            string SystemSelectedName = "";
            if (this.clientRuleManagerIGrid.SelectedCells.Count != 0 && this.clientRuleManagerIGrid.SelectedCells[0].Row.Key != null) 
            {
                SystemSelectedName = this.clientRuleManagerIGrid.SelectedCells[0].Row.Key;
            }            
            //ruleWizard.ApplyToAllSystem = true;
            this.KeyOfRuleIGridSelectRow = SystemSelectedName;                
            
            ruleWizard.Reset();

            //To Rebuild Conditions List
            ruleWizard.SetListSystemValue();
            ruleWizard.RebuildMasters();

            ruleWizard.ShowDialog();
            ClientRule newRule = null;
            if (ruleWizard.DialogResult == DialogResult.OK)
            {
                
                newRule = (ClientRule)ruleWizard.NewRule.Clone();
                m_aRulesList.Add(newRule);
            }
            else if (ruleWizard.DialogResult == DialogResult.Cancel)
            {
                return;
                //Cancel rule creation
            }
            LoadOpenSystems();
        }



        private void editRuleInWizard(ClientRule qr)
        {

            ruleWizard.Reset();
            ruleWizard.editRule(qr.Clone());

            //To Rebuild Conditions List
            ruleWizard.RebuildMasters();
            ruleWizard.ListConditions();
            ruleWizard.ShowDialog();
            if (ruleWizard.DialogResult == DialogResult.OK)
            {
                m_aRulesList.Remove(qr);
                qr = (ClientRule)ruleWizard.NewRule.Clone();
                m_aRulesList.Add(qr);

            }

            LoadOpenSystems();

            this.selectRuleInGrid(qr);
            foreach (iGRow gr in clientRuleManagerIGrid.Rows)
            {
                if (gr.Tag == qr)
                {
                    gr.Cells[0].Selected = true;
                    return;
                }
            }
        }

        private void clientRuleManagerNewRuleButton_Click(object sender, EventArgs e) {
            openRuleWizard();
        }


        //Edit Rule
        private void clientRuleManagerEditRuleButton_Click(object sender, EventArgs e) {
            editCurrentlySelectedRule();
        }


        private void editCurrentlySelectedRule() {
            try {
                selectedIndex = this.clientRuleManagerIGrid.SelectedCells[0].Row.Index;
                //Make sure something selected...

                ClientRule tempRule = (ClientRule)this.clientRuleManagerIGrid.SelectedCells[0].Row.Tag;
                this.editRuleInWizard(tempRule);

            } catch (Exception) {
                MessageBox.Show("\nWarning: No rules were selected.\n" +
                                "Problem: \t No rules were selected for editing.\n\n" +
                                "Solution: \t Please select atleast one rule.\n\n",
                                "No Rules Selected", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            
        }

        private void clientRuleManagerDeleteRuleButton_Click(object sender, EventArgs e)
        {
            try
            {
                selectedIndex = this.clientRuleManagerIGrid.SelectedCells[0].Row.Index;
                if (selectedIndex > this.clientRuleManagerIGrid.Rows.Count)
                    selectedIndex -= 1;

                if (selectedIndex < 0)
                    selectedIndex = 0;
                //Make sure something selected...
                if (this.clientRuleManagerIGrid.SelectedCells.Count > 0)
                {
                    ClientRule ruletoRemove=(ClientRule)this.clientRuleManagerIGrid.SelectedCells[0].Row.Tag;
                    this.m_aRulesList.Remove(ruletoRemove);

                    //Remove the XML file in default folder, so that it would not be automatically loaded next time
                    string homeDirectory = "";
                    try
                    {
                        homeDirectory = ClientRuleOptions.GetOptions().ClientRuleFolderName;
                    }
                    catch (Exception)
                    {
                        homeDirectory = Environment.ExpandEnvironmentVariables("%HOMEDRIVE%%HOMEPATH%");
                    }
                    try 
                    {
                        if (checkRuleXMLFileInDefaultFolder(ruletoRemove, homeDirectory))
                        {
                            File.Delete(homeDirectory + "\\" + ruletoRemove.RuleName + "." + Trafodion.Manager.Properties.Resources.DefaultClientRuleFileExtention);
                        }
                    }
                    catch (Exception ex) 
                    {
                        MessageBox.Show("\nError: Failed to delete a rule file : \n" +
                               "                \t\"" + ruletoRemove.RuleName + "\"\n\n" +
                               "Solution: \t Please see error details for recovery information. \n\n" +
                               "Details: \t " + ex.Message + " \n\n",
                               "Error Delete Rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }                    

                    LoadOpenSystems();
                }
            }
            catch (Exception) { }
        }

        //This method is public, because it would be used in wizard class
        public bool checkRuleXMLFileInDefaultFolder(ClientRule argRule, string folder) 
        {
            bool ruleExist = false;
            string ruleName = argRule.RuleName;

            string rulePath = folder + "\\" + ruleName + "." + Trafodion.Manager.Properties.Resources.DefaultClientRuleFileExtention;

            if (File.Exists(rulePath)) 
                return ruleExist = true;

            return ruleExist;
        }

        private void disableDetails()
        {
                this.clientRuleManagerEditRuleButton.Enabled = false;
                this.clientRuleManagerDeleteRuleButton.Enabled = false;
                this.clientRuleManagerSaveRuleButton.Enabled = false;


                //Gotta add a 'workspace selector' 
                //this.button1.Enabled = true;
                this.clientRuleManagerLoadRulesButton.Enabled = true;

                this.clientRuleManagerAdditionalCommentsTextBox.Text = "";
                this.clientRuleManagerCreatorEmailTextBox.Text = "";
                this.clientRuleManagerCreatorNameTextBox.Text = "";
                this.clientRuleManagerPlatformVersionTextBox.Text = "";   

                this.clientRuleManagerAdditionalCommentsTextBox.Enabled = false;
                this.clientRuleManagerCreatorEmailTextBox.Enabled = false;
                this.clientRuleManagerCreatorNameTextBox.Enabled = false;
                this.clientRuleManagerPlatformVersionTextBox.Enabled = false;

                this.clientRuleManagerAdditionalCommentsTextBox.BackColor = SystemColors.Control;
                this.clientRuleManagerCreatorEmailTextBox.BackColor = SystemColors.Control;
                this.clientRuleManagerCreatorNameTextBox.BackColor = SystemColors.Control;
                this.clientRuleManagerPlatformVersionTextBox.BackColor = SystemColors.Control;   
        }

        private void enableDetails()
        {           
            if (clientRuleManagerIGrid.SelectedCells.Count > 0)
            {
                try
                {
                    this.clientRuleManagerEditRuleButton.Enabled = true;
                    this.clientRuleManagerDeleteRuleButton.Enabled = true;
                    this.clientRuleManagerSaveRuleButton.Enabled = true;

                    //Gotta add a 'workspace selector' 
                    //this.button1.Enabled = false;
                    this.clientRuleManagerLoadRulesButton.Enabled = false;

                    this.clientRuleManagerAdditionalCommentsTextBox.BackColor = SystemColors.ControlLightLight;
                    this.clientRuleManagerCreatorEmailTextBox.BackColor = SystemColors.ControlLightLight;
                    this.clientRuleManagerCreatorNameTextBox.BackColor = SystemColors.ControlLightLight;
                    this.clientRuleManagerPlatformVersionTextBox.BackColor = SystemColors.ControlLightLight;

                    this.clientRuleManagerAdditionalCommentsTextBox.Enabled = true;
                    this.clientRuleManagerCreatorEmailTextBox.Enabled = true;
                    this.clientRuleManagerCreatorNameTextBox.Enabled = true;
                    this.clientRuleManagerPlatformVersionTextBox.Enabled = true;

                    ClientRule tempRule = (ClientRule)this.clientRuleManagerIGrid.SelectedCells[0].Row.Tag;
                    //ClientRule tempRule = ((ClientRule)this.igrid.SelectedRows[0].DataBoundItem);
                    this.clientRuleManagerAdditionalCommentsTextBox.Text = tempRule.DetailsComments;
                    this.clientRuleManagerCreatorEmailTextBox.Text = tempRule.DetailsCreatorEmail;
                    this.clientRuleManagerCreatorNameTextBox.Text = tempRule.DetailsCreatorName;
                    this.clientRuleManagerPlatformVersionTextBox.Text = tempRule.DetailsVersion;

                    if (this.clientRuleManagerAdditionalCommentsTextBox.Text == "")
                        this.clientRuleManagerAdditionalCommentsTextBox.Text = "[No data available]";

                    if (this.clientRuleManagerCreatorEmailTextBox.Text == "")
                        this.clientRuleManagerCreatorEmailTextBox.Text = "[No data available]";

                    if (this.clientRuleManagerCreatorNameTextBox.Text == "")
                        this.clientRuleManagerCreatorNameTextBox.Text = "[No data available]";

                    if (this.clientRuleManagerPlatformVersionTextBox.Text == "")
                        this.clientRuleManagerPlatformVersionTextBox.Text = "[No data available]";
                }
                catch (Exception) {
                    this.clientRuleManagerAdditionalCommentsTextBox.Text = "[No data available]";
                    this.clientRuleManagerCreatorEmailTextBox.Text = "[No data available]";
                    this.clientRuleManagerCreatorNameTextBox.Text = "[No data available]";
                    this.clientRuleManagerPlatformVersionTextBox.Text = "[No data available]";
                }
            }

        }



        private void selectRuleInGrid(ClientRule inQR)
        {
            foreach (iGRow gr in clientRuleManagerIGrid.Rows)
            {
                if (gr.Tag == inQR)
                {
                    //loop through and select all cells in the row except cell and row
                    foreach (iGCell gc in gr.Cells)
                    {
                        if (gc.ColKey != "Cell" && gc.ColKey != "Row")
                        {
                            gc.BackColor = Color.SteelBlue;// = true;
                            gc.ForeColor = Color.White;
                        }
                    }
                }
            }
        }

        private void unselectAll()
        {

            //Reset the row that used to be selected back to the way it should look.
            foreach (iGRow gr in clientRuleManagerIGrid.Rows)
            {
                if (gr.Key != null)
                {
                    gr.Cells[-1].BackColor = Color.White;// = true;
                }
                foreach (iGCell gc in gr.Cells)//selectedRow].Cells)
                {
                    if (gc.ColKey != "Cell" && gc.ColKey != "Row")
                    {
                        gc.BackColor = Color.White;// = true;
                        gc.ForeColor = Color.Black;
                    }
                }
            }
        }

        void iGridRulerManager_CurRowChanged(object sender, System.EventArgs e)
        {
            try
            {

                this.unselectAll();                

                //selected a rule
                if ((this.clientRuleManagerIGrid.SelectedCells.Count > 0) &&
                   (this.clientRuleManagerIGrid.SelectedCells[0].Row.Pattern.Level == 1))
                {
                    //this.iGridRulerManager.BackColor = Color.White;
                    selectRuleInGrid((ClientRule)this.clientRuleManagerIGrid.SelectedCells[0].Row.Tag);
                    
                    this.clientRuleManagerIGrid.SelectionMode = iGSelectionMode.One;
                    enableDetails();
                }
                else
                {
                    //selected a system
                    this.clientRuleManagerIGrid.SelectedCells[0].BackColor = Color.SteelBlue;// = true;                      
                    disableDetails();
                }

            }
            catch (Exception) { }
        }

        private void clientRuleManagerSaveRuleButton_Click(object sender, EventArgs e) {
            String ruleSavePath = "";
            TextWriter ruleWriter = null;

            try {
                selectedIndex = this.clientRuleManagerIGrid.SelectedCells[0].Row.Index;

                //Add more error handlin'
                ClientRule ruleToSave = (ClientRule)this.clientRuleManagerIGrid.SelectedCells[0].Row.Tag;

                SaveFileDialog saveFileDialog1 = new SaveFileDialog();
                saveFileDialog1.Title = "Save Rule";
                saveFileDialog1.FileName = ruleToSave.RuleName;
                saveFileDialog1.DefaultExt = "xml";
                saveFileDialog1.Filter = "Extensible Markup Language (*.xml)|*.xml|All files (*.*)|*.*";

                DialogResult result = saveFileDialog1.ShowDialog();
                if (result == DialogResult.OK) {
                    ruleSavePath = saveFileDialog1.FileName;
                    ruleWriter = new StreamWriter(ruleSavePath);

                    if (ruleToSave.Alerts.saveAlertDictionary())
                        ruleSerializer.Serialize(ruleWriter, ruleToSave);

                    ruleWriter.Close();
                    ruleWriter = null;
                }

            } catch (Exception ex) {
                String errDetails = ex.Message;
                if (null != ex.InnerException)
                    errDetails += "\n       \t " + ex.InnerException.Message;

                MessageBox.Show("\nError: Failed to save a rule to file : \n" +
                                "                \t\"" + ruleSavePath + "\"\n\n" +
                                "Problem: \t Unable to save rule details.\n\n" +
                                "Solution: \t Please see error details for recovery information. \n\n" +
                                "Details: \t " + errDetails + " \n\n",
                                "Error Saving Rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
            } finally {
                if (null != ruleWriter)
                    try { ruleWriter.Close(); } catch (Exception) { }
            }

        }

        private void fixupRuleConditions(ClientRule aRule) {
            try {
                foreach(ClientRuleCondition rc in aRule.Conditions) {
                    if (LinkType.STATE == rc.LinkFieldType) {
                        for (int i = 0; i < rc.Threshold.Count; i++) {
                            if ("'*'".Equals(rc.Threshold[i].ToString() ) )
                                rc.Threshold[i] = "'" + States.Any.ToString() + "'";

                        }

                        continue;
                    }

                    if ((LinkType.VALUE != rc.LinkFieldType)  &&  (LinkType.FLOAT_VALUE != rc.LinkFieldType) )
                        continue;

                    if (rc.Comparator.Equals("LIKE") )
                        rc.Comparator = ">";

                }

            } catch (Exception) {
            }
        }

        private void clientRuleManagerLoadRulesButton_Click(object sender, EventArgs e)
        {
            String rulePathToLoad = "";

            try {

                ruleWizard.Reset();

                OpenFileDialog openFileDialog1 = new OpenFileDialog();
                openFileDialog1.Title = "Load Rule";
                openFileDialog1.FileName = "";
                openFileDialog1.Multiselect = true;
                openFileDialog1.Filter = "Extensible Markup Language (*.xml)|*.xml|All files (*.*)|*.*";

                // Available file extensions
                DialogResult result = openFileDialog1.ShowDialog();

                if (result == DialogResult.OK) {
                    foreach (string strPath in openFileDialog1.FileNames) {

                        rulePathToLoad = strPath;

                        ClientRule ruleToLoad = new ClientRule();
                        TextReader r = new StreamReader(rulePathToLoad);
                        ruleToLoad = (ClientRule)ruleSerializer.Deserialize(r);
                        fixupRuleConditions(ruleToLoad);
                        ruleToLoad.BuildAlertDictionary();
                        r.Close();

                        string WorkspaceSelectedName = this.clientRuleManagerIGrid.SelectedCells[0].Row.Key;
                        selectedIndex = this.clientRuleManagerIGrid.SelectedCells[0].Row.Index;

                        if (WorkspaceSelectedName == "All") {
                            ruleToLoad.ApplyToAllSystems = true;
                            ruleToLoad.AppliedSystems.Clear();
                        }
                        else {
                            ruleToLoad.ApplyToAllSystems = false; 
                            
                            //ToDo: Need to add check, overwrite or add, or do nothing;
                           
                            //ruleToLoad.AppliedSystems.Add(((ConnectionDefinition)this.clientRuleManagerIGrid.SelectedRows[0].Tag).Name);//.AssociatedWorkspace = (NCCWorkspace)this.iGridRulerManager.SelectedCells[0].Row.Tag;
                        }

                        //Generate the alert dictionary
                        if (ruleToLoad.Alerts.LoadAlertDictionary() ) {
                            m_aRulesList.Add(ruleToLoad);
                            LoadOpenSystems();
                        }
                    }
                }

            }
            catch (Exception ex) {
                MessageBox.Show("\nError: Failed to load rule from file :\n" +
                                "                \t\"" + rulePathToLoad + "\"\n\n" +
                                "Problem: \t Unable to load rule.\n\n" +
                                "Solution: \t Please see error details for recovery information.\n\n" +
                                "Details: \t " + ex.Message + "\n\n",
                                "Error Loading Rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

        }

        void iGridRulerManager_RequestEdit(object sender, TenTec.Windows.iGridLib.iGRequestEditEventArgs e)
        {
            if(this.clientRuleManagerIGrid.Cols[e.ColIndex].Key == "Popup")
            {
                selectRuleInGrid((ClientRule)this.clientRuleManagerIGrid.SelectedCells[0].Row.Tag);

                //Handle popup change for e.RowIndex
                ((ClientRule)this.clientRuleManagerIGrid.Rows[e.RowIndex].Tag).Alerts.saveAlertDictionary();                

                ((ClientRule)this.clientRuleManagerIGrid.Rows[e.RowIndex].Tag).Alerts.ShowAlertBox = !((bool)this.clientRuleManagerIGrid.Rows[e.RowIndex].Cells["Popup"].Value);

                foreach (iGRow oneiGRow in clientRuleManagerIGrid.Rows)
                {
                    if (oneiGRow.Tag == this.clientRuleManagerIGrid.Rows[e.RowIndex].Tag && oneiGRow.Index != e.RowIndex)
                        oneiGRow.Cells["Popup"].Value = !((bool)this.clientRuleManagerIGrid.Rows[e.RowIndex].Cells["Popup"].Value);
                }

                ((ClientRule)this.clientRuleManagerIGrid.Rows[e.RowIndex].Tag).Alerts.LoadAlertDictionary();
            } 
            else if(this.clientRuleManagerIGrid.Cols[e.ColIndex].Key == "Enabled")
            {
                //Handle enable/disable for e.RowIndex
                ((ClientRule)this.clientRuleManagerIGrid.Rows[e.RowIndex].Tag).Enabled = !((bool)this.clientRuleManagerIGrid.Rows[e.RowIndex].Cells["Enabled"].Value);

                foreach (iGRow oneiGRow in clientRuleManagerIGrid.Rows)
                {
                    if (oneiGRow.Tag == this.clientRuleManagerIGrid.Rows[e.RowIndex].Tag && oneiGRow.Index != e.RowIndex)
                        oneiGRow.Cells["Enabled"].Value = !((bool)this.clientRuleManagerIGrid.Rows[e.RowIndex].Cells["Enabled"].Value);
                }
            }
            e.DoDefault = true;
        }



        void clientRuleManager_FormClosing(object sender, System.Windows.Forms.FormClosingEventArgs e)
        {
            this.Hide();
            if (e.CloseReason.ToString().Equals("ApplicationExitCall"))
                e.Cancel = false;
            else
                e.Cancel = true;

            //e.Cancel = true; // this cancels the close event.              
        }

        private void clientRuleManagerIGrid_CellDoubleClick(object sender, iGCellDoubleClickEventArgs e) {
            if ((0 >= this.clientRuleManagerIGrid.SelectedCells.Count)  ||
                (iGRowType.Normal != this.clientRuleManagerIGrid.SelectedCells[0].Row.Type))
                return;

            editCurrentlySelectedRule();
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ClientThresholdsRuleManager);

        }
                
    }
}
