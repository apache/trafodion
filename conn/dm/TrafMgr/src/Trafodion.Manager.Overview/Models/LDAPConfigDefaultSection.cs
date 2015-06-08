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
using System.Linq;
using System.Text;

namespace Trafodion.Manager.OverviewArea.Models
{
    public class LDAPConfigDefaultSection : ICloneable
    {
        const string SECTION = "SECTION: Defaults";
        const string KEY_DEFAULT_SECTION_NAME = "DefaultSectionName";
        const string KEY_REFRESH_TIME = "RefreshTime";
        const string KEY_TLS_CACERT_FILE_NAME = "TLS_CACERTFilename";

        const string KEY_DEFAULT_SECTION_NAME_LOW = "defaultsectionname";
        const string KEY_REFRESH_TIME_LOW = "refreshtime";
        const string KEY_TLS_CACERT_FILE_NAME_LOW = "tls_cacertfilename";
        const string KEY_SECTION_LOW = "section";
        
        ////default is ENTERPRISE
        //public String defaultSectionName = LDAPConfigObject.ENTERPRISE;
        public String defaultSectionName = string.Empty;
        public int refreshTime = 30;
        public String cerLocation = string.Empty;
        public String cerContent = string.Empty;
        public Dictionary<string, string> errorMap = new Dictionary<string, string>();
        public List<String> TheOtherUndefinedEntries = new List<string>();

        /// fill object by string
        /// </summary>
        /// <param name="defaultSectionString">string of default section</param>
        /// <param name="cerContent">certificate file content</param>
        public void FillObject(String defaultSectionString, String cerContent)
        {
            //if Section:default doesn't exist
            if (defaultSectionString == null || defaultSectionString.Trim() == String.Empty)
            {
                //this.defaultSectionName = LDAPConfigObject.ENTERPRISE;
                this.refreshTime = 30;
            }
            else
            {
                string[] pairs = defaultSectionString.Split(new string[] { LDAPConfigSection.LINE_FEED_WINDOWS }, StringSplitOptions.RemoveEmptyEntries);
                string key, value;

                this.cerContent = cerContent;
                ////if DefaultSectionName doesn't exist, default is ENTERPRISE
                //this.defaultSectionName = LDAPConfigObject.ENTERPRISE;
                TheOtherUndefinedEntries.Clear();
                foreach (string item in pairs)
                {
                    if (item.Contains(LDAPConfigSection.FIELD_SEPERATOR))
                    {
                        int index = item.IndexOf(LDAPConfigSection.FIELD_SEPERATOR);
                        key = item.Substring(0, index).Trim().ToLower();
                        value = item.Substring(index + 1).Trim();
                        if (key != String.Empty)
                        {
                            try
                            {
                                switch (key)
                                {
                                    case KEY_DEFAULT_SECTION_NAME_LOW:
                                        
                                        if (value.ToLower().Equals("local"))
                                            value = LDAPConfigObject.ENTERPRISE;
                                        else if (value.ToLower().Equals("remote"))
                                            value = LDAPConfigObject.CLUSTER;
                                        else if (value.ToLower().Equals("enterprise"))
                                            value = LDAPConfigObject.ENTERPRISE;
                                        else if (value.ToLower().Equals("cluster"))
                                            value = LDAPConfigObject.CLUSTER;
                                        else
                                        {
                                            this.errorMap.Add("Default Configuration", value);
                                            //default is Enterprise
                                            value = LDAPConfigObject.ENTERPRISE;
                                        }
                                        this.defaultSectionName = value;
                                        break;
                                    case KEY_REFRESH_TIME_LOW:
                                        {
                                            try
                                            {
                                                int _refreshTime = Int32.Parse(value);
                                                if (_refreshTime < 0 || _refreshTime > 86400)
                                                {
                                                    this.errorMap.Add("Refresh Time", value);
                                                    value = "1800";
                                                }
                                            }
                                            catch (Exception e)
                                            {
                                                this.errorMap.Add("Refresh Time", value);
                                                value = "1800";
                                            }

                                            int refreshTime = Int32.Parse(value);
                                            this.refreshTime = refreshTime / 60 + (refreshTime % 60 == 0 ? 0 : 1);
                                            break;
                                        }
                                    case KEY_TLS_CACERT_FILE_NAME_LOW:
                                        {
                                            //only display the file name
                                            this.cerLocation = value.Substring(value.LastIndexOf("/") + 1);
                                        }
                                        break;
                                    case KEY_SECTION_LOW:
                                        break;
                                    default:
                                        TheOtherUndefinedEntries.Add(item);
                                        break;
                                }
                            }
                            catch (InvalidCastException e)
                            {
                                throw new Exception("There is invalid value in the config file. " + e.Message);
                            }

                        }
                    }
                }
            }

            if (cerContent != null && cerContent.Length > 0)
                this.cerContent = cerContent;
        }


        /// <summary>
        /// change object to string
        /// </summary>
        /// <returns></returns>
        public override String ToString()
        {
            StringBuilder defaultSection = new StringBuilder();

            defaultSection.Append(SECTION);
            defaultSection.Append(LDAPConfigSection.LINE_FEED_UNIX);

            defaultSection.Append(KEY_DEFAULT_SECTION_NAME);
            defaultSection.Append(LDAPConfigSection.FIELD_SEPERATOR);
            defaultSection.Append(this.defaultSectionName);
            defaultSection.Append(LDAPConfigSection.LINE_FEED_UNIX);

            defaultSection.Append(KEY_REFRESH_TIME);
            defaultSection.Append(LDAPConfigSection.FIELD_SEPERATOR);
            defaultSection.Append(this.refreshTime * 60);
            defaultSection.Append(LDAPConfigSection.LINE_FEED_UNIX);

            defaultSection.Append(KEY_TLS_CACERT_FILE_NAME);
            defaultSection.Append(LDAPConfigSection.FIELD_SEPERATOR);
            defaultSection.Append(this.cerLocation);
            defaultSection.Append(LDAPConfigSection.LINE_FEED_UNIX);
            if (TheOtherUndefinedEntries != null && TheOtherUndefinedEntries.Count > 0)
            {
                foreach (string str in TheOtherUndefinedEntries)
                {
                    defaultSection.Append(str);
                    defaultSection.Append(LDAPConfigSection.LINE_FEED_UNIX);
                }
            }
                

            return defaultSection.ToString();
        }

        public object Clone()
        {
            return this.MemberwiseClone();
        }

        public override bool Equals(Object obj)
        {

            LDAPConfigDefaultSection defaultObj = obj as LDAPConfigDefaultSection;
            if (defaultObj == null)
                return false;
            else
            {
                return this.defaultSectionName.Equals(defaultObj.defaultSectionName) &&
                    this.refreshTime.Equals(defaultObj.refreshTime) &&
                    this.cerLocation.Equals(defaultObj.cerLocation) &&
                    this.cerContent.Equals(defaultObj.cerContent);
            }

        }
    }
}
