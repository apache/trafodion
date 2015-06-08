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
using System.Collections;
using System.Reflection;
using Trafodion.Manager.WorkloadArea.Model;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class ClientRuleValue : TrafodionForm
    {
        public ArrayList Threshold;
        public bool isStringValue;
        public bool isColorValue;
        public bool showingTB;
        private LinkType linkFieldType;


        public ClientRuleValue()
        {
            InitializeComponent();

            this.Threshold = new ArrayList();
            showingTB = true;
            this.valueEntryPanel.Visible = true;
            isStringValue = false;
            isColorValue = false;
            this.linkFieldType = LinkType.STRING;
        }

        public ClientRuleValue(LinkType linkFieldType, ArrayList existingArray)
        {
            InitializeComponent();

            this.Threshold = new ArrayList();
            this.Name = linkFieldType.ToString();
            this.Text = linkFieldType.ToString();
            this.linkFieldType = linkFieldType;

            bool isColor = false;
            bool isString = false;

            this.clientRuleValueAddButton.Enabled = false;
            this.dataGridView_Thresholds.Visible = true;
            this.comboSelectComboBox.Items.Clear();
            FieldInfo[] theFields = States.Any.GetType().GetFields();

            foreach (FieldInfo aField in theFields)
            {
                if (FieldAttributes.SpecialName == (aField.Attributes & FieldAttributes.SpecialName)) { continue; }
                string itemName = aField.Name;
                try
                {
                    Object[] customAttrArray = aField.GetCustomAttributes(typeof(DescriptionAttribute), false);
                    if (0 < customAttrArray.Length)
                    {
                        DescriptionAttribute da = (DescriptionAttribute)customAttrArray[customAttrArray.Length - 1];
                        itemName = da.Description;
                    }
                }
                catch (Exception)
                {
                    //Do nothing
                }
                this.comboSelectComboBox.Items.Add(itemName);
            }

            //If the link type is "Duration" then
            if (linkFieldType == LinkType.DURATION)
            {
                showingTB = false;
                isString = false;
                this.comboSelectPanel.Visible = false;
                this.valueEntryPanel.Visible = false;
                this.dataGridView_Thresholds.Visible = false;
                this.durationEntryPanel.Visible = true;
                this.ActiveControl = this.durationEntryHoursTextBox;
                this.durationEntryHoursTextBox.Focus();

                try
                {
                    String previousValue = "0";
                    if (0 < existingArray.Count)
                    {
                        previousValue = existingArray[0].ToString();
                    }
                    TimeSpan theTimeSpan = TimeSpan.FromMilliseconds(Double.Parse(previousValue));
                    this.durationEntryHoursTextBox.Text = theTimeSpan.Hours.ToString();
                    this.durationEntryMinutesTextBox.Text = theTimeSpan.Minutes.ToString();
                    this.durationEntrySecondsTextBox.Text = theTimeSpan.Seconds.ToString();
                    this.durationEntryMillisecondsTextBox.Text = theTimeSpan.Milliseconds.ToString();
                }
                catch (Exception ex) { }
                return;
            }
            //If the link type is "State" then load the drop-down menu
            //with the appropriate values.
            else if (linkFieldType == LinkType.STATE)
            {
                showingTB = false;
                this.durationEntryPanel.Visible = false;
                this.comboSelectPanel.Visible = true;
                this.valueEntryPanel.Visible = false;
                this.ActiveControl = this.comboSelectComboBox;
                this.comboSelectComboBox.Focus();

                foreach (string str in existingArray)
                {
                    try
                    {
                        if (false == string.IsNullOrEmpty(str))
                        {
                            this.dataGridView_Thresholds.Rows.Add(str);
                        }
                    }
                    catch (Exception) { }
                }

                try
                {
                    string previousSelection = "'*'";
                    if (0 < existingArray.Count)
                        previousSelection = existingArray[0].ToString();
                    if ("'*'".Equals(previousSelection) || string.IsNullOrEmpty(previousSelection))
                        previousSelection = "'" + States.Any.ToString() + "'";

                    previousSelection = previousSelection.Substring(1, previousSelection.Length - 2);
                    int selectionIndex = this.comboSelectComboBox.Items.IndexOf(previousSelection);
                    if (-1 != selectionIndex)
                        this.comboSelectComboBox.SelectedIndex = selectionIndex;
                }
                catch (Exception) { }
                return;
            }

            if (linkFieldType == LinkType.BOOLEAN) 
            {
                showingTB = false;                
                this.isStringValue = false;                
                this.durationEntryPanel.Visible = false;
                this.comboSelectPanel.Visible = true;
                this.valueEntryPanel.Visible = false;
                this.ActiveControl = this.comboSelectComboBox;

                this.comboSelectComboBox.Items.Clear();
                this.comboSelectComboBox.Items.Add("TRUE");
                this.comboSelectComboBox.Items.Add("FALSE");

                this.comboSelectComboBox.Focus();

                foreach (string str in existingArray)
                {
                    try
                    {
                        if (false == string.IsNullOrEmpty(str))
                        {
                            this.dataGridView_Thresholds.Rows.Add(str);
                        }
                    }
                    catch (Exception) { }
                }

                try
                {
                    int previousSelection = 0;
                    if (0 < existingArray.Count && !string.IsNullOrEmpty(existingArray[0].ToString())) 
                    {
                        previousSelection = Convert.ToInt32(existingArray[0]);
                    }
                        
                    string previousSelectionText;
                    if (previousSelection==1)
                        previousSelectionText = "TRUE";
                    else
                        previousSelectionText = "FALSE";

                    int selectionIndex = this.comboSelectComboBox.Items.IndexOf(previousSelectionText);
                    if (-1 != selectionIndex)
                        this.comboSelectComboBox.SelectedIndex = selectionIndex;
                }
                catch (Exception) { }
                return;            
            }


            if (linkFieldType != LinkType.FLOAT_VALUE && linkFieldType != LinkType.VALUE && linkFieldType != LinkType.AMOUNT)
            {
                isString = true;
            }
            showingTB = true;
            this.durationEntryPanel.Visible = false;
            this.comboSelectPanel.Visible = false;
            this.valueEntryPanel.Visible = true;
            try
            {
                foreach (string str in existingArray)
                {
                    if (str != null && str != "")
                        this.dataGridView_Thresholds.Rows.Add(str);
                }
            }
            catch
            {
            }

            this.valueEntryTextBox.Text = "";
            this.ActiveControl = this.valueEntryTextBox;
            this.valueEntryTextBox.Focus();

            isStringValue = isString;
            isColorValue = isColor;            
        }

        public ClientRuleValue(bool isString, bool isColor)
        {
            InitializeComponent();
            isStringValue = isString;
            isColorValue = isColor;
        }

        //private void BindCombBox(string fieldName)
        //{
        //    this.comboSelectComboBox.Items.Clear();

        //    if (fieldName.Equals( "STATE")) 
        //    {
        //        FieldInfo[] theFields = States.Any.GetType().GetFields();

        //        foreach (FieldInfo aField in theFields)
        //        {
        //            if (FieldAttributes.SpecialName == (aField.Attributes & FieldAttributes.SpecialName)) { continue; }
        //            string itemName = aField.Name;
        //            try
        //            {
        //                Object[] customAttrArray = aField.GetCustomAttributes(typeof(DescriptionAttribute), false);
        //                if (0 < customAttrArray.Length)
        //                {
        //                    DescriptionAttribute da = (DescriptionAttribute)customAttrArray[customAttrArray.Length - 1];
        //                    itemName = da.Description;
        //                }
        //            }
        //            catch (Exception)
        //            {
        //                //Do nothing
        //            }
        //            this.comboSelectComboBox.Items.Add(itemName);
        //        }
        //    }

        //    if(fieldName.Equals("QUERY_SUBTYPE"))
        //    {
        //        this.comboSelectComboBox.Items.Add("SQL_STMT_CTAS");
        //        this.comboSelectComboBox.Items.Add("SQL_STMT_NA");
        //    }

        //    if (fieldName.Equals("EXEC_STATE")) 
        //    {
                
                
        //       // this.comboSelectComboBox.Items.Add("INITIAL");
        //        //this.comboSelectComboBox.Items.Add("SQL_STMT_NA");
        //    }



        //}


        //textBox1_TextChanged
        private void valueEntryTextBox_TextChanged(object sender, EventArgs e)
        {
            if (valueEntryTextBox.Text.Equals(""))
            {
                this.clientRuleValueAddButton.Enabled = false;
                return;
            }
            if (!isStringValue)
            {
                float result = 0f;
                if (!float.TryParse(valueEntryTextBox.Text, out result))
                {
                    this.clientRuleValueAddButton.Enabled = false;
                    return;
                }
                Int64 intVal = 0;
                if ((LinkType.VALUE == this.linkFieldType) && !Int64.TryParse(valueEntryTextBox.Text, out intVal))
                {
                    this.clientRuleValueAddButton.Enabled = false;
                    return;
                }                
            }
            else 
            {
                DateTime temp;
                if (this.linkFieldType == LinkType.DATE_TIME && !DateTime.TryParse(valueEntryTextBox.Text, out temp))
                {
                    this.clientRuleValueAddButton.Enabled = false;
                    return;
                }
            }
            this.clientRuleValueAddButton.Enabled = true;
        }

        //button1_Click
        private void clientRuleValueFinishButton_Click(object sender, EventArgs e)
        {
            if (durationEntryPanel.Visible) 
            {
                this.Threshold.Clear();
                TimeSpan theTimeSpan = TimeSpan.FromSeconds(0.0);
                try
                {
                    //TimeSpan(int days, int hours, int minutes, int seconds, int milliseconds);
                    theTimeSpan = new TimeSpan(0, Int32.Parse(durationEntryHoursTextBox.Text), Int32.Parse(durationEntryMinutesTextBox.Text), Int32.Parse(durationEntrySecondsTextBox.Text), Int32.Parse(durationEntryMillisecondsTextBox.Text));
                }
                catch (Exception) { }
                Threshold.Add(theTimeSpan.TotalMilliseconds);
                this.DialogResult = DialogResult.OK;
                this.Close();
                return;
            }
            if (this.dataGridView_Thresholds.Rows.Count <= 0) 
            {
                this.DialogResult = DialogResult.Cancel;
                this.Close();
                return;
            }

            this.Threshold.Clear();
            foreach (DataGridViewRow row in this.dataGridView_Thresholds.Rows) 
            {
                Threshold.Add(row.Cells[0].Value.ToString());
            }
            this.DialogResult = DialogResult.OK;
            this.Close();
        }
        //button2_Click
        private void clientRuleValueCancelButton_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void comboSelectComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            this.clientRuleValueAddButton.Enabled = true;
        }

        //button_Add_Click
        private void clientRuleValueAddButton_Click(object sender, EventArgs e)
        {
            
            if (showingTB)
            {
                if (this.isStringValue)
                {
                    this.dataGridView_Thresholds.Rows.Add("'" + this.valueEntryTextBox.Text + "'");
                }
                else
                {
                    this.dataGridView_Thresholds.Rows.Add(this.valueEntryTextBox.Text);
                }
            }
            else 
            {                
                if (this.isStringValue) 
                {
                    string selectedVal = this.comboSelectComboBox.SelectedItem.ToString();
                    if (this.comboSelectComboBox.SelectedItem.Equals(States.Any))                 
                        selectedVal = "*";
                    this.dataGridView_Thresholds.Rows.Add("'" + selectedVal + "'");
                }
                else
                {
                    int selectedVal=0;
                    if(this.comboSelectComboBox.SelectedItem.Equals("TRUE"))
                        selectedVal=1;
                    else
                        selectedVal=0;
                    this.dataGridView_Thresholds.Rows.Add(selectedVal.ToString());
                }
                
            }
            this.valueEntryTextBox.Text = "";
            this.valueEntryTextBox.Focus();
        }

        

        private void dataGridView_Thresholds_CellClick(object sender, DataGridViewCellEventArgs e)
        {
            if (e.ColumnIndex == 1)
                this.dataGridView_Thresholds.Rows.RemoveAt(e.RowIndex);
            else
                valueEntryTextBox.Text = this.dataGridView_Thresholds.Rows[e.RowIndex].Cells["ThresholdValue"].Value.ToString().TrimEnd('\'').TrimStart('\'');
        }

        private void clientRuleValue_GotFocus(object sender, System.EventArgs e)
        {
            if (showingTB)
                this.comboSelectComboBox.Focus();
            else
                this.valueEntryTextBox.Focus();
        }
    }
}
