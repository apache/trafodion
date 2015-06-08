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
using System.Collections;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Controls;

namespace Trafodion.Manager.WorkloadArea.Model
{
    public class ClientRuleCondition : ICloneable
    {
        private ClientRule cannedRule = null;
        public ClientRule PredefinedRule 
        {
            get { return cannedRule; }
            set { cannedRule = value; }
        }

        private bool isACannedRule = false;
        public bool IsAPredefinedRule
        {
            get { return isACannedRule; }
            set { isACannedRule = value; }
        }

        private string sText;
        public string Text
        {
            get { return sText; }
            set { sText = value; }
        }

        private string listText;
        public string ListText
        {
            get { return listText; }
            set { listText = value; }
        }

        private int iPriority;
        public int Priority
        {
            get { return iPriority; }
            set { iPriority = value; }
        }

        private string sQueryProperty;
        public string QueryProperty
        {
            get { return sQueryProperty; }
            set { sQueryProperty = value; }
        }

        private string comparator;
        public string Comparator
        {
            get { return comparator; }
            set { comparator = value; }
        }

        private ArrayList sThreshold;
        public ArrayList Threshold
        {
            get { return sThreshold; }
            set { sThreshold = value; }
        }

        private LinkType lLinkFieldType;
        public LinkType LinkFieldType
        {
            get { return lLinkFieldType; }
            set { lLinkFieldType = value; }
        }

        private bool bComplete = false;
        public bool isComplete
        {
            get { return bComplete; }
            set { bComplete = value; }
        }

        private bool bEnabled = false;
        public bool isEnabled
        {
            get { return bEnabled; }
            set { bEnabled = value; }
        }

        public ClientRuleCondition() 
        {
            
        }
        
        public ClientRuleCondition(ClientRule clientRule) 
        {
            this.sThreshold = new ArrayList();
            this.cannedRule = (ClientRule)clientRule.Clone();
            this.isACannedRule = true;
            this.sText = clientRule.RuleName;
            this.listText = clientRule.RuleName;
            this.iPriority = 0;
            this.lLinkFieldType = LinkType.ERROR_CODE;
            this.sQueryProperty = "";
            this.comparator = "";
        }

        public ClientRuleCondition(string sText, string listText, int iPriority, LinkType linkType) 
        {
            this.sThreshold = new ArrayList();
            this.listText = listText;
            this.sText = sText;
            this.iPriority = iPriority;
            this.lLinkFieldType = linkType;
        }

        public ClientRuleCondition(string sText, string listText, int iPriority, LinkType linkType, string queryProperty, string comparator, string threshold) 
        {
            this.sThreshold = new ArrayList();
            this.sText = sText;
            this.listText = listText;
            this.iPriority = iPriority;
            this.lLinkFieldType = linkType;
            this.sQueryProperty = queryProperty;
            this.comparator = comparator;
            this.sThreshold.Add(threshold);
        }

        public object Clone()
        {
            ClientRuleCondition RC = new ClientRuleCondition();
            RC.Comparator = this.Comparator;
            RC.isComplete = this.isComplete;
            RC.LinkFieldType = this.LinkFieldType;
            RC.Priority = this.Priority;
            RC.QueryProperty = this.QueryProperty;
            RC.Text = this.Text;
            RC.Threshold = (ArrayList)this.Threshold.Clone();
            RC.isEnabled = this.isEnabled;
            RC.ListText = this.ListText;

            return RC;
        }

        public bool UpdateValue(ArrayList existingArray, Hashtable systemHash) 
        {
            bool isString = false;

            //inconsequential for now...
            if(this.LinkFieldType!=LinkType.FLOAT_VALUE&&this.LinkFieldType!=LinkType.VALUE&&this.LinkFieldType!=LinkType.AMOUNT)
            {
                isString = true;
            }
            ClientRuleValue rv = new ClientRuleValue(this.lLinkFieldType, existingArray);//isString, isColor);
            rv.isStringValue = isString;
            rv.ShowDialog();
            if (rv.DialogResult == DialogResult.OK)
            {
                if (rv.Threshold != null && rv.Threshold.Count > 0)
                    this.Threshold = (ArrayList)rv.Threshold.Clone();
                return true;
            }
            else if (existingArray.Count > 0 && this.isComplete)
            {
                return true;
            }

            return false;
        }

        public bool UpdateValue(ArrayList existingArray, ConnectionDefinition system)
        {
            bool isString = false;

            //inconsequetial for now....
            if (this.LinkFieldType != LinkType.FLOAT_VALUE
                && this.LinkFieldType != LinkType.VALUE
                && this.LinkFieldType != LinkType.AMOUNT 
                && this.lLinkFieldType != LinkType.BOOLEAN 
                && this.lLinkFieldType != LinkType.ERROR_CODE) 
            {
                isString = true;
            }

            ClientRuleValue rv = new ClientRuleValue(this.lLinkFieldType, (ArrayList)existingArray.Clone());//isString, isColor);
            rv.isStringValue = isString;
            rv.ShowDialog();
            if (rv.DialogResult == DialogResult.OK)
            {
                if (rv.Threshold != null && rv.Threshold.Count > 0)
                    this.Threshold = (ArrayList)rv.Threshold;
                return true;
            }
            else if (existingArray.Count > 0 && this.isComplete)
            {
                return true;
            }           

            return false;
        }
    }
}
