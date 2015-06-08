//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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
using System.Data;
using System.Linq;
using System.Text;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// object for LDAP config section
    /// </summary>
    public class LDAPConfigSection : ICloneable
    {
        public const string Key_Section = "Section";
        public const string Key_LDAPHostname = "LDAPHostName";
        public const string Key_LDAPPort = "LDAPPort";
        public const string Key_LDAPSearchDN = "LDAPSearchDN";
        public const string Key_LDAPSearchPWD = "LDAPSearchPWD";
        //remove DirectoryBase
        //public const string Key_DirectoryBase = "DirectoryBase";
        public const string Key_LDAPVersion = "LDAPVersion";
        public const string Key_UniqueIdentifier = "UniqueIdentifier";
        public const string Key_LDAPSSL = "LDAPSSL";
        public const string Key_LDAPNetworkTimeout = "LDAPNetworkTimeout";
        public const string Key_LDAPTimelimit = "LDAPTimelimit";
        public const string Key_LDAPTimeout = "LDAPTimeout";
        public const string Key_RetryCount = "RetryCount";
        public const string Key_RetryDelay = "RetryDelay";
        public const string Key_PreserveConnection = "PreserveConnection";

        const string Key_Section_low = "section";
        const string Key_LDAPHostname_low = "ldaphostname";
        const string Key_LDAPPort_low = "ldapport";
        const string Key_LDAPSearchDN_low = "ldapsearchdn";
        const string Key_LDAPSearchPWD_low = "ldapsearchpwd";
        const string Key_DirectoryBase_low = "directorybase";
        const string Key_LDAPVersion_low = "ldapversion";
        const string Key_UniqueIdentifier_low = "uniqueidentifier";
        const string Key_LDAPSSL_low = "ldapssl";
        const string Key_LDAPNetworkTimeout_low = "ldapnetworktimeout";
        const string Key_LDAPTimelimit_low = "ldaptimelimit";
        const string Key_LDAPTimeout_low = "ldaptimeout";
        const string Key_RetryCount_low = "retrycount";
        const string Key_RetryDelay_low = "retrydelay";
        const string Key_PreserveConnection_low = "preserveconnection";


        public const string FIELD_SEPERATOR = ":";
        public const string VALUE_SEPERATOR = ",";
        public const string LINE_FEED_WINDOWS = "\r\n";
        public const string LINE_FEED_UNIX = "\n";
        private String _section;

        public String Section
        {
            get
            {
                if (string.IsNullOrEmpty(_section)) return string.Empty;

                if (_section.ToUpper().Trim().Equals("REMOTE") || _section.ToUpper().Trim().Equals("CLUSTER"))
                    return "Cluster";
                else
                {
                    return "Enterprise";
                }
            }
            set { _section = value; }
        }

        public string LdapPort;
        public int LdapVersion = 3;
        public String LDAPSearchDN = string.Empty;
        public String LDAPSearchPWD = string.Empty;
        public int LDAPSSL;
        public List<String> LdapHostname = new List<string>();
        //public List<String> DirectoryBase = new List<string>();
        public List<String> UniqueIdentifier = new List<string>();
        public int LDAPNetworkTimeout;
        public int LDAPTimelimit;
        public int LDAPTimeout;
        public int RetryCount;
        public int RetryDelay;
        public bool PreserveConnection;

        public Dictionary<string, string> errorMap = new Dictionary<string, string>();
        public Boolean isPreM10Version = false;
        public List<String> TheOtherUndefinedEntries = new List<string>();

        /// <summary>
        /// grid table datasource
        /// </summary>
        public DataTable attributes
        {
            get
            {
                DataTable dt= new DataTable();
                dt.Columns.Add("Name");
                dt.Columns.Add("Value");                

                //foreach (String value in LdapHostname)
                //    dt.Rows.Add(Key_LDAPHostname, value);
                //foreach (String value in DirectoryBase)
                //    dt.Rows.Add(Key_DirectoryBase, value);
                foreach (String value in UniqueIdentifier)
                    dt.Rows.Add(Key_UniqueIdentifier, value);

                return dt;
            }
        }

        /// <summary>
        /// fill object by string
        /// </summary>
        /// <param name="configString">file content</param>
        public void FillObject(String configString)
        {
            string[] pairs = configString.Split(new string[] { LINE_FEED_WINDOWS }, StringSplitOptions.RemoveEmptyEntries);
            string key, value;
            List<String> keyStore = new List<String>();
            TheOtherUndefinedEntries.Clear();

            foreach (string item in pairs)
            {
                if (item.Contains(FIELD_SEPERATOR))
                {
                    int index = item.IndexOf(FIELD_SEPERATOR);
                    key = item.Substring(0,index).Trim().ToLower();
                    value = item.Substring(index+1).Trim();

                    if (key != String.Empty)
                    {
                        keyStore.Add(key);
                        switch (key)
                        {
                            case Key_Section_low:
                                this.Section = value;
                                break;
                            case Key_LDAPHostname_low:
                                this.LdapHostname.Add(value);
                                break;
                            case Key_LDAPPort_low:
                                try
                                {
                                    int port = Int32.Parse(value);
                                    if (port < 1 || port > 65535)
                                    {
                                        this.errorMap.Add("LDAP Port", value);
                                        value = "0";
                                    }
                                }
                                catch (Exception e)
                                {
                                    this.errorMap.Add("LDAP Port", value);
                                    value = "0";
                                }
                                this.LdapPort = value;
                                break;
                            case Key_LDAPSearchDN_low:
                                LDAPSearchDN = value;
                                break;
                            case Key_LDAPSearchPWD_low:
                                this.LDAPSearchPWD = value;
                                break;
                            case Key_DirectoryBase_low:
                                ////this.DirectoryBase.AddRange(SplitAttributeValue(value));
                                //if (!String.IsNullOrEmpty(value))
                                //    this.DirectoryBase.Add(value);
                                break;
                            case Key_LDAPVersion_low:
                                //this.LdapVersion = Int32.Parse(value);
                                break;
                            case Key_UniqueIdentifier_low:
                                //this.UniqueIdentifier.AddRange(SplitAttributeValue(value));
                                if (!String.IsNullOrEmpty(value))
                                    this.UniqueIdentifier.Add(value);
                                break;
                            case Key_LDAPSSL_low:
                                try
                                {
                                    int ssl = Int32.Parse(value);
                                    if (ssl != 0 && ssl != 1 && ssl != 2)
                                    {
                                        this.errorMap.Add("Encryption Method", value);
                                        value = "0";
                                    }
                                }
                                catch (Exception e)
                                {
                                    this.errorMap.Add("Encryption Method", value);
                                    value = "0";
                                }
                                this.LDAPSSL = Int32.Parse(value);
                                break;
                            case Key_LDAPNetworkTimeout_low:
                                try
                                {
                                    int digit = Int32.Parse(value);
                                    if (digit == 0 || digit < -1 || digit > 3600)
                                    {
                                        this.errorMap.Add("Network Timeout", value);
                                        value = "30";
                                    }
                                }
                                catch (Exception e)
                                {
                                    this.errorMap.Add("Network Timeout", value);
                                    value = "30";
                                }
                                this.LDAPNetworkTimeout = Int32.Parse(value);
                                break;
                            case Key_LDAPTimelimit_low:
                                try
                                {
                                    int digit = Int32.Parse(value);
                                    if (digit < 1 || digit > 3600)
                                    {
                                        this.errorMap.Add("Time Limit", value);
                                        value = "30";
                                    }
                                }
                                catch (Exception e)
                                {
                                    this.errorMap.Add("Time Limit", value);
                                    value = "30";
                                }
                                this.LDAPTimelimit = Int32.Parse(value);
                                break;
                            case Key_LDAPTimeout_low:
                                try
                                {
                                    int digit = Int32.Parse(value);
                                    if (digit == 0 || digit < -1 || digit > 3600)
                                    {
                                        this.errorMap.Add("LDAP Timeout", value);
                                        value = "30";
                                    }
                                }
                                catch (Exception e)
                                {
                                    this.errorMap.Add("LDAP Timeout", value);
                                    value = "30";
                                }
                                this.LDAPTimeout = Int32.Parse(value);
                                break;
                            case Key_RetryCount_low:
                                try
                                {
                                    int digit = Int32.Parse(value);
                                    if (digit < 0 || digit > 10)
                                    {
                                        this.errorMap.Add("Retry Count", value);
                                        value = "5";
                                    }
                                }
                                catch (Exception e)
                                {
                                    this.errorMap.Add("Retry Count", value);
                                    value = "5";
                                }
                                this.RetryCount = Int32.Parse(value);
                                break;
                            case Key_RetryDelay_low:
                                try
                                {
                                    int digit = Int32.Parse(value);
                                    if (digit < 0 || digit > 300)
                                    {
                                        this.errorMap.Add("Retry Delay", value);
                                        value = "2";
                                    }
                                }
                                catch (Exception e)
                                {
                                    this.errorMap.Add("Retry Delay", value);
                                    value = "2";
                                }
                                this.RetryDelay = Int32.Parse(value);
                                break;
                            case Key_PreserveConnection_low:
                                {
                                    if (value.ToUpper().Equals("YES"))
                                        this.PreserveConnection = true;
                                    else if (value.ToUpper().Equals("NO"))
                                        this.PreserveConnection = false;
                                    break;
                                }
                            default:
                                TheOtherUndefinedEntries.Add(item);
                                break;
                        } 
                    }
                }
            }

            if (LdapHostname.Count == 0)
            {
                this.errorMap.Add("LDAP Host Name", String.Empty);
            }
            if (!keyStore.Contains(Key_LDAPPort_low))
            {
                this.errorMap.Add("LDAP Port", String.Empty);
                this.LdapPort = "0";
            }
            if (!keyStore.Contains(Key_LDAPSSL_low))
            {
                this.errorMap.Add("Encryption Method", String.Empty);
                this.LDAPSSL = 0;
            }
            if (UniqueIdentifier.Count == 0)
            {
                this.errorMap.Add("UniqueIdentifier", String.Empty);
            }
            //set to default value if key doesn't exist
            if (!keyStore.Contains(Key_LDAPNetworkTimeout_low))
            {
                isPreM10Version = true;
                this.LDAPNetworkTimeout = 30;
            }
            if (!keyStore.Contains(Key_LDAPTimelimit_low))
            {
                isPreM10Version = true;
                this.LDAPTimelimit = 30;
            }
            if (!keyStore.Contains(Key_LDAPTimeout_low))
            {
                isPreM10Version = true;
                this.LDAPTimeout = 30;
            }
            if (!keyStore.Contains(Key_RetryCount_low))
            {
                isPreM10Version = true;
                this.RetryCount = 5;
            }
            if (!keyStore.Contains(Key_RetryDelay_low))
            {
                isPreM10Version = true;
                this.RetryDelay = 2;
            }
        }

        private string[] SplitAttributeValue(string value)
        {
            string[] valueArray = value.Split(new string[] { VALUE_SEPERATOR }, StringSplitOptions.RemoveEmptyEntries);
            List<string> filteredValue = new List<string>();
            for (int i = 0; i < valueArray.Length; i++)
            {
                valueArray[i] = valueArray[i].Trim();
                if (valueArray[i].Contains("="))
                {
                    int index = valueArray[i].IndexOf("=");
                    string A = valueArray[i].Substring(0, valueArray[i].IndexOf("=")).Trim();
                    string B = valueArray[i].Substring(index + 1).Trim();
                    valueArray[i] = A + "=" + B;
                }
                if (valueArray[i] != string.Empty)
                {
                    filteredValue.Add(valueArray[i]);
                }
            }

            return filteredValue.ToArray();
        }

        /// <summary>
        /// change object to string
        /// </summary>
        /// <returns></returns>
        public override String ToString()
        {
            StringBuilder configString = new StringBuilder();
            StringBuilder directoryBaseValue = new StringBuilder();
            StringBuilder uniqueIdentifierValue = new StringBuilder();
            int count=0;

            configString.Append(Key_Section);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(Section);
            configString.Append(LINE_FEED_UNIX);

            foreach (String value in LdapHostname)
            {
                configString.Append(Key_LDAPHostname);
                configString.Append(FIELD_SEPERATOR);
                configString.Append(value);
                configString.Append(LINE_FEED_UNIX);
            }

            configString.Append(Key_LDAPPort);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(LdapPort);
            configString.Append(LINE_FEED_UNIX);

            configString.Append(Key_LDAPVersion);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(LdapVersion);
            configString.Append(LINE_FEED_UNIX);

            //foreach (String value in DirectoryBase)
            //{
            //    count++;
            //    if (count == DirectoryBase.Count)
            //        directoryBaseValue.Append(value);
            //    else
            //    {
            //        directoryBaseValue.Append(value);
            //        directoryBaseValue.Append(VALUE_SEPERATOR);
            //    }
            //}
            //configString.Append(Key_DirectoryBase);
            //configString.Append(FIELD_SEPERATOR);
            //configString.Append(directoryBaseValue);
            //configString.Append(LINE_FEED_UNIX);

            foreach (String value in UniqueIdentifier)
            {
                configString.Append(Key_UniqueIdentifier);
                configString.Append(FIELD_SEPERATOR);
                configString.Append(value);
                configString.Append(LINE_FEED_UNIX);
            }

            configString.Append(Key_LDAPSearchDN);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(LDAPSearchDN);
            configString.Append(LINE_FEED_UNIX);

            configString.Append(Key_LDAPSearchPWD);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(LDAPSearchPWD);
            configString.Append(LINE_FEED_UNIX);

            configString.Append(Key_LDAPSSL);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(LDAPSSL);
            configString.Append(LINE_FEED_UNIX);

            configString.Append(Key_LDAPNetworkTimeout);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(LDAPNetworkTimeout);
            configString.Append(LINE_FEED_UNIX);

            configString.Append(Key_LDAPTimelimit);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(LDAPTimelimit);
            configString.Append(LINE_FEED_UNIX);

            configString.Append(Key_LDAPTimeout);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(LDAPTimeout);
            configString.Append(LINE_FEED_UNIX);

            configString.Append(Key_RetryCount);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(RetryCount);
            configString.Append(LINE_FEED_UNIX);

            configString.Append(Key_RetryDelay);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(RetryDelay);
            configString.Append(LINE_FEED_UNIX);

            configString.Append(Key_PreserveConnection);
            configString.Append(FIELD_SEPERATOR);
            configString.Append(PreserveConnection ? "YES" : "NO");
            configString.Append(LINE_FEED_UNIX);

            if (TheOtherUndefinedEntries != null && TheOtherUndefinedEntries.Count > 0)
            {
                foreach (string str in TheOtherUndefinedEntries)
                {
                    configString.Append(str);
                    configString.Append(LINE_FEED_UNIX);
                }
            }
            return configString.ToString();
        }

        public object Clone()
        {
            return this.MemberwiseClone();
        }

        public override bool Equals(Object obj)
        {

            LDAPConfigSection configObj = obj as LDAPConfigSection;
            if (configObj == null)
                return false;
            else
            {
                return this.Section.Equals(configObj.Section) &&
                    this.LdapPort.Equals(configObj.LdapPort) &&
                    this.LdapVersion.Equals(configObj.LdapVersion) &&
                    this.LDAPSearchDN.Equals(configObj.LDAPSearchDN) &&
                    this.LDAPSearchPWD.Equals(configObj.LDAPSearchPWD) &&
                    this.LDAPSSL.Equals(configObj.LDAPSSL) &&
                    Enumerable.SequenceEqual(this.LdapHostname, configObj.LdapHostname) &&
                    //Enumerable.SequenceEqual(this.DirectoryBase, configObj.DirectoryBase) &&
                    Enumerable.SequenceEqual(this.UniqueIdentifier, configObj.UniqueIdentifier) &&
                    this.LDAPNetworkTimeout.Equals(configObj.LDAPNetworkTimeout) &&
                    this.LDAPTimelimit.Equals(configObj.LDAPTimelimit) &&
                    this.LDAPTimeout.Equals(configObj.LDAPTimeout) &&
                    this.RetryCount.Equals(configObj.RetryCount) &&
                    this.RetryDelay.Equals(configObj.RetryDelay) &&
                    this.PreserveConnection.Equals(configObj.PreserveConnection);

            }

        }
    }
}
