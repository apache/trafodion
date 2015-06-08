//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using Trafodion.Manager.Framework.Connections;
using System.Data;
using System.Data.Odbc;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// Security Audit Logging Configuration system model
    /// </summary>
    public class AuditLogsConfigModel
    {
        private static Dictionary<ConnectionDefinition, AuditLogsConfigModel> _activeAuditLogsConfigModels = 
            new Dictionary<ConnectionDefinition, AuditLogsConfigModel>(new MyConnectionDefinitionComparer());
        
        private ConnectionDefinition _theConnectionDefinition = null;
        private Connection _theCurrentConnection = null;


        public ConnectionDefinition ConnectionDefinition
        {
            get
            {
                return _theConnectionDefinition;
            }
            set
            {
                _theConnectionDefinition = value;
            }
        }

        public Connection CurrentConnection
        {
            get
            {
                return _theCurrentConnection;
            }
            set
            {
                _theCurrentConnection = value;
            }
        }

        private AuditLogsConfigModel(ConnectionDefinition aConnectionDefinition)
        {
            ConnectionDefinition = aConnectionDefinition;
            //Add this instance to the static dictionary, so in future if need a reference to this system, 
            //we can look up using the connection definition
            if (!_activeAuditLogsConfigModels.ContainsKey(aConnectionDefinition))
                _activeAuditLogsConfigModels.Add(aConnectionDefinition, this);

            //Subscribe to connection definition's events, so that you can maintain the static dictionary
            ConnectionDefinition.Changed += new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);

        }

        /// <summary>
        /// If the connection definition has changed/removed, the static dictionary is updated accordingly
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason != ConnectionDefinition.Reason.Tested)
            {
                _activeAuditLogsConfigModels.Remove(aConnectionDefinition);
            }
        }

        public static AuditLogsConfigModel FindAuditLogsConfigModel(ConnectionDefinition connectionDefinition)
        {
            AuditLogsConfigModel auditLogsConfigModel = null;
            _activeAuditLogsConfigModels.TryGetValue(connectionDefinition, out auditLogsConfigModel);
            if (auditLogsConfigModel == null)
            {
                auditLogsConfigModel = new AuditLogsConfigModel(connectionDefinition);
            }
            return auditLogsConfigModel;
        }



        /// <summary>
        /// Display security audit configuration by calling SPJ.
        /// e.g. CALL TRAFODION.TRAFODION_SP.DISPLAY_SECURITY_AUDIT_CONFIGURATION();
        /// </summary>
        public DataTable DisplaySecurityAuditConfiguration()
        {
            OdbcDataReader dsReader = null;
            AuditLogsConfigDataModel auditLogsConfiguration = new AuditLogsConfigDataModel();
            DataTable auditLogsConfigTbl = new DataTable();
            try
            {

                if (!GetConnection())
                {
                    throw new Exception("Unable to get connection.");
                }
                dsReader = Queries.DisplayAuditLogsConfig(CurrentConnection);
                auditLogsConfigTbl.Load(dsReader); 
            }
            finally
            {
                CloseConnection();
            }
            return auditLogsConfigTbl;
        }

        /// <summary>
        /// Restore security audit configuration by calling SPJ.
        /// e.g. CALL TRAFODION.TRAFODION_SP.RESTORE_SECURITY_AUDIT_CONFIGURATION();
        /// values will be reset as:    
        ///     REFRESH_CONFIG_TIME = 100 
        ///     TABLE_AGING = 15
        ///     SPACE_USAGE_NOTIFY_THRESHOLD = 70
        ///     SPACE_USAGE_WARN_THRESHOLD = 80
        ///     SPACE_USAGE_DIALOUT_THRESHOLD = 90
        /// </summary>
        public void RestoreSecurityAuditConfiguration(string logType)
        {
            if (!GetConnection())
            {
                return;
            }

            try
            {
                Queries.RestoreAuditLogsConfig(CurrentConnection, logType);
            }
            finally
            {
                CloseConnection();
            }
        }

        /// <summary>
        /// Reset security audit configuration by calling SPJ.
        /// e.g. CALL TRAFODION.TRAFODION_SP.RESET_SECURITY_AUDIT_CONFIGURATION();
        /// values will be reset as:    
        ///     REFRESH_CONFIG_TIME = 100 
        ///     TABLE_AGING = 15
        ///     SPACE_USAGE_NOTIFY_THRESHOLD = 70
        ///     SPACE_USAGE_WARN_THRESHOLD = 80
        ///     SPACE_USAGE_DIALOUT_THRESHOLD = 90
        /// </summary>
        public void ResetSecurityAuditConfiguration(string logType)
        {
            if (!GetConnection())
            {
                return;
            }

            try
            {
                Queries.ResetAuditLogsConfig(CurrentConnection, logType);
            }
            finally
            {
                CloseConnection();
            }
        }

        /// <summary>
        /// Updating Audit Logging Configuration to server.
        /// </summary>
        /// <param name="alterAuditLogsConfigCommand"></param>
        public void AlterSecurityAuditConfiguration(string alterAuditLogsConfigCommand)
        {  
            if (!GetConnection())
            {
                return;
            }

            try
            {
                Queries.AlterAuditLogsConfig(CurrentConnection, alterAuditLogsConfigCommand);
            }
            finally
            {
                CloseConnection();
            }
        }

   
        public bool GetConnection()
        {
            if (this._theCurrentConnection == null && this._theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                _theCurrentConnection = new Connection(ConnectionDefinition);
                return true;
            }
            else
            {
                return true;
            }
        }

        public void CloseConnection()
        {
            if (this._theCurrentConnection != null)
            {
                _theCurrentConnection.Close();
                _theCurrentConnection = null;
            }
        }

     
    }

    /// <summary>
    /// Security Audit Logging Configuration Data model
    /// </summary>
    public class AuditLogsConfigDataModel
    {
        public const int AUDIT_LOGGING_ENABLE_NUMBER = 1;
        public const int REFRESH_CONFIG_TIME_NUMBER = 2;
        public const int AUDIT_CONFIGURATION_CHANGES_NUMBER = 3;
        public const int AUDIT_AUTHENTICATION_SUCCESS_NUMBER = 4;
        public const int AUDIT_AUTHENTICATION_FAILURE_NUMBER = 5;
        public const int AUDIT_OBJECT_CHANGES_NUMBER = 6;
        public const int AUDIT_PRIVILEGE_CHANGES_NUMBER = 7;
        public const int AUDIT_SECURITY_VIOLATIONS_NUMBER = 8;
        public const int AUDIT_USER_MGMT_CHANGES_NUMBER = 9;
        public const int TABLE_AGING_NUMBER = 10;
        public const int SPACE_USAGE_NOTIFY_THRESHOLD_NUMBER = 11;
        public const int SPACE_USAGE_WARN_THRESHOLD_NUMBER = 12;
        public const int SPACE_USAGE_DIALOUT_THRESHOLD_NUMBER = 13;
        public const int AUDIT_LOG_TARGET = 14;
        public const int AUDIT_LOG_ALTERNATE = 15;

        public const string LOG_TYPE_SAR_DESC = "SAR";
        public const string LOG_TYPE_SYSLOG_DESC = "SYSLOG";
        public const string LOG_TYPE_SAR_CODE = "1";
        public const string LOG_TYPE_SYSLOG_CODE = "2";
        public const string LOG_FAIL_ACTION_OPT_DESC1 = "Ignore and continue";
        public const string LOG_FAIL_ACTION_OPT_DESC2 = "Force request to fail";
        public const string LOG_FAIL_ACTION_OPT_DESC3 = "Log to alternate location";
        public const string LOG_FAIL_ACTION_OPT_CODE1 = "N";
        public const string LOG_FAIL_ACTION_OPT_CODE2 = "Y";
        public const string LOG_FAIL_ACTION_OPT_CODE3 = "L";

        //LogType
        public string LogType
        {
            get;
            set;
        }
               

        //ENABLE
        public bool AuditLoggingEnable
        {
            get;
            set;
        }
                

        //REFRESH_CONFIG_TIME
        public Int32 RefreshConfigTime
        {
            get;
            set;
        }

        //AUDIT_CONFIGURATION_CHANGES
        public bool AuditConfigurationChanges
        {
            get;
            set;
        }

        //AUDIT_CONFIGURATION_CHANGES Fail Action
        public string AuditConfigurationChangesFailAction
        {
            get;
            set;
        }

        //AUDIT_AUTHENTICATION_SUCCESS
        public bool AuditAuthenticationSuccess
        {
            get;
            set;
        }

        //AUDIT_AUTHENTICATION_SUCCESS  Fail Action
        public string AuditAuthenticationSuccessFailAction
        {
            get;
            set;
        }

        //AUDIT_AUTHENTICATION_FAILURE
        public bool AuditAuthenticationFailure
        {
            get;
            set;
        }

        //AUDIT_AUTHENTICATION_FAILURE Fail Action
        public string AuditAuthenticationFailureFailAction
        {
            get;
            set;
        }

        //AUDIT_OBJECT_CHANGES
        public bool AuditObjectChanges
        {
            get;
            set;
        }

        //AUDIT_OBJECT_CHANGES Fail Action
        public string AuditObjectChangesFailAction
        {
            get;
            set;
        }

        //AUDIT_PRIVILEGE_CHANGES
        public bool AuditPrivilegeChanges
        {
            get;
            set;
        }

        //AUDIT_PRIVILEGE_CHANGES Fail Action
        public string AuditPrivilegeChangesFailAction
        {
            get;
            set;
        }

        //AUDIT_SECURITY_VIOLATIONS
        public bool AuditSecurityViolations
        {
            get;
            set;
        }

        //AUDIT_SECURITY_VIOLATIONS Fail Action
        public string AuditSecurityViolationsFailAction
        {
            get;
            set;
        }

        //AUDIT_USER_MGMT_CHANGES
        public bool AuditUserMgmtChanges
        {
            get;
            set;
        }

        //AUDIT_USER_MGMT_CHANGES  Fail Action
        public string AuditUserMgmtChangesFailAction
        {
            get;
            set;
        }

        //TABLE_AGING
        public int TableAging
        {
            get;
            set;
        }

        //SPACE_USAGE_NOTIFY_THRESHOLD
        public int SpaceUsageNotifyThreshold
        {
            get;
            set;
        }

        //SPACE_USAGE_WARN_THRESHOLD
        public int SpaceUsageWarnThreshold
        {
            get;
            set;
        }

        //SPACE_USAGE_DIALOUT_THRESHOLD
        public int SpaceUsageDialoutThreshold
        {
            get;
            set;
        }
        //AUDIT_LOG_TARGET
        public int AuditLogTarget
        {
            get;
            set;
        }

        //AUDIT_LOG_ALTERNATE
        public int AuditLogAlternate
        {
            get;
            set;
        }

        public StringBuilder AssemblyAlterAuditConfigCommand(AuditLogsConfigDataModel anAuditLogsConfiguration)
        {
            List<string> lstColNum = new List<string>();
            List<string> lstValue = new List<string>();
            StringBuilder sbReturn=new StringBuilder();

            if (this.AuditLoggingEnable != anAuditLogsConfiguration.AuditLoggingEnable)
            {
                lstColNum.Add(AuditLogsConfigDataModel.AUDIT_LOGGING_ENABLE_NUMBER.ToString());
                lstValue.Add(anAuditLogsConfiguration.AuditLoggingEnable ? "Y" : "N");
            }

            if (this.RefreshConfigTime != anAuditLogsConfiguration.RefreshConfigTime)
            {
                lstColNum.Add(AuditLogsConfigDataModel.REFRESH_CONFIG_TIME_NUMBER.ToString());
                lstValue.Add(anAuditLogsConfiguration.RefreshConfigTime.ToString());
            }

            if (this.AuditConfigurationChanges != anAuditLogsConfiguration.AuditConfigurationChanges||
                (!this.AuditConfigurationChangesFailAction.Equals(anAuditLogsConfiguration.AuditConfigurationChangesFailAction) &&
                !anAuditLogsConfiguration.AuditConfigurationChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1)))
            {                
                lstColNum.Add(AuditLogsConfigDataModel.AUDIT_CONFIGURATION_CHANGES_NUMBER.ToString());
                if (!this.AuditConfigurationChangesFailAction.Equals(anAuditLogsConfiguration.AuditConfigurationChangesFailAction) &&
                !anAuditLogsConfiguration.AuditConfigurationChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1))
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditConfigurationChanges ? "Y" + anAuditLogsConfiguration.AuditConfigurationChangesFailAction : "N");
                }
                else
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditConfigurationChanges ? "Y" : "N");
                }
            }

            if (this.AuditAuthenticationSuccess != anAuditLogsConfiguration.AuditAuthenticationSuccess||
                (!this.AuditAuthenticationSuccessFailAction.Equals(anAuditLogsConfiguration.AuditAuthenticationSuccessFailAction)&&
                !anAuditLogsConfiguration.AuditAuthenticationSuccessFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1)))
            {
                lstColNum.Add(AuditLogsConfigDataModel.AUDIT_AUTHENTICATION_SUCCESS_NUMBER.ToString());
                if (!this.AuditAuthenticationSuccessFailAction.Equals(anAuditLogsConfiguration.AuditAuthenticationSuccessFailAction) &&
                !anAuditLogsConfiguration.AuditAuthenticationSuccessFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1))
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditAuthenticationSuccess ? "Y" + anAuditLogsConfiguration.AuditAuthenticationSuccessFailAction : "N");
                }
                else
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditAuthenticationSuccess ? "Y" : "N");
                }
            }

            if (this.AuditAuthenticationFailure != anAuditLogsConfiguration.AuditAuthenticationFailure||
                (!this.AuditAuthenticationFailureFailAction.Equals(anAuditLogsConfiguration.AuditAuthenticationFailureFailAction)&&
                !anAuditLogsConfiguration.AuditAuthenticationFailureFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1)))
            {
                lstColNum.Add(AuditLogsConfigDataModel.AUDIT_AUTHENTICATION_FAILURE_NUMBER.ToString());
                if (!this.AuditAuthenticationFailureFailAction.Equals(anAuditLogsConfiguration.AuditAuthenticationFailureFailAction) &&
                !anAuditLogsConfiguration.AuditAuthenticationFailureFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1))
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditAuthenticationFailure ? "Y" + anAuditLogsConfiguration.AuditAuthenticationFailureFailAction : "N");
                }
                else
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditAuthenticationFailure ? "Y" : "N");
                }
            }

            if (this.AuditObjectChanges != anAuditLogsConfiguration.AuditObjectChanges||
                (!this.AuditObjectChangesFailAction.Equals(anAuditLogsConfiguration.AuditObjectChangesFailAction)&&
                !anAuditLogsConfiguration.AuditObjectChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1)))
            {
                lstColNum.Add(AuditLogsConfigDataModel.AUDIT_OBJECT_CHANGES_NUMBER.ToString());
                if (!this.AuditObjectChangesFailAction.Equals(anAuditLogsConfiguration.AuditObjectChangesFailAction) &&
                !anAuditLogsConfiguration.AuditObjectChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1))
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditObjectChanges ? "Y" + anAuditLogsConfiguration.AuditObjectChangesFailAction : "N");
                }
                else
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditObjectChanges ? "Y" : "N");
                }
            }


            if (this.AuditPrivilegeChanges != anAuditLogsConfiguration.AuditPrivilegeChanges||
                (!this.AuditPrivilegeChangesFailAction.Equals(anAuditLogsConfiguration.AuditPrivilegeChangesFailAction)&&
                !anAuditLogsConfiguration.AuditPrivilegeChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1)))
            {
                lstColNum.Add(AuditLogsConfigDataModel.AUDIT_PRIVILEGE_CHANGES_NUMBER.ToString());
                if (!this.AuditPrivilegeChangesFailAction.Equals(anAuditLogsConfiguration.AuditPrivilegeChangesFailAction) &&
                !anAuditLogsConfiguration.AuditPrivilegeChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1))
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditPrivilegeChanges ? "Y" + anAuditLogsConfiguration.AuditPrivilegeChangesFailAction : "N");
                }
                else
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditPrivilegeChanges ? "Y" : "N");
                }
            }

            if (this.AuditSecurityViolations != anAuditLogsConfiguration.AuditSecurityViolations||
                (!this.AuditSecurityViolationsFailAction.Equals(anAuditLogsConfiguration.AuditSecurityViolationsFailAction)&&
                !anAuditLogsConfiguration.AuditSecurityViolationsFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1)))
            {
                lstColNum.Add(AuditLogsConfigDataModel.AUDIT_SECURITY_VIOLATIONS_NUMBER.ToString());
                if (!this.AuditSecurityViolationsFailAction.Equals(anAuditLogsConfiguration.AuditSecurityViolationsFailAction) &&
                !anAuditLogsConfiguration.AuditSecurityViolationsFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1))
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditSecurityViolations ? "Y" + anAuditLogsConfiguration.AuditSecurityViolationsFailAction : "N");
                }
                else
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditSecurityViolations ? "Y" : "N");
                }
            }

            if (this.AuditUserMgmtChanges != anAuditLogsConfiguration.AuditUserMgmtChanges||
                (!this.AuditUserMgmtChangesFailAction.Equals(anAuditLogsConfiguration.AuditUserMgmtChangesFailAction) &&
                !anAuditLogsConfiguration.AuditUserMgmtChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1)))
            {
                lstColNum.Add(AuditLogsConfigDataModel.AUDIT_USER_MGMT_CHANGES_NUMBER.ToString());
                if (!this.AuditUserMgmtChangesFailAction.Equals(anAuditLogsConfiguration.AuditUserMgmtChangesFailAction) &&
                !anAuditLogsConfiguration.AuditUserMgmtChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1))
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditUserMgmtChanges ? "Y" + anAuditLogsConfiguration.AuditUserMgmtChangesFailAction : "N");
                }
                else
                {
                    lstValue.Add(anAuditLogsConfiguration.AuditUserMgmtChanges ? "Y" : "N");
                }
            }

            if (this.TableAging != anAuditLogsConfiguration.TableAging)
            {
                lstColNum.Add(AuditLogsConfigDataModel.TABLE_AGING_NUMBER.ToString());
                lstValue.Add(anAuditLogsConfiguration.TableAging.ToString());
            }

            if (this.SpaceUsageNotifyThreshold != anAuditLogsConfiguration.SpaceUsageNotifyThreshold)
            {
                lstColNum.Add(AuditLogsConfigDataModel.SPACE_USAGE_NOTIFY_THRESHOLD_NUMBER.ToString());
                lstValue.Add(anAuditLogsConfiguration.SpaceUsageNotifyThreshold.ToString());
            }

            if (this.SpaceUsageWarnThreshold != anAuditLogsConfiguration.SpaceUsageWarnThreshold)
            {
                lstColNum.Add(AuditLogsConfigDataModel.SPACE_USAGE_WARN_THRESHOLD_NUMBER.ToString());
                lstValue.Add(anAuditLogsConfiguration.SpaceUsageWarnThreshold.ToString());
            }

            if (this.SpaceUsageDialoutThreshold != anAuditLogsConfiguration.SpaceUsageDialoutThreshold)
            {
                lstColNum.Add(AuditLogsConfigDataModel.SPACE_USAGE_DIALOUT_THRESHOLD_NUMBER.ToString());
                lstValue.Add(anAuditLogsConfiguration.SpaceUsageDialoutThreshold.ToString());
            }
            
            if (!this.AuditLogTarget.Equals(anAuditLogsConfiguration.AuditLogTarget))
            {
                lstColNum.Add(AuditLogsConfigDataModel.AUDIT_LOG_TARGET.ToString());
                lstValue.Add(anAuditLogsConfiguration.AuditLogTarget.ToString());
            }

            if (!this.AuditLogAlternate.Equals(anAuditLogsConfiguration.AuditLogAlternate))
            {
                lstColNum.Add(AuditLogsConfigDataModel.AUDIT_LOG_ALTERNATE.ToString());
                lstValue.Add(anAuditLogsConfiguration.AuditLogAlternate.ToString());

            }

            if (lstColNum.Count > 0 && lstValue.Count > 0)
            {
                sbReturn.Append("ALTER AUDIT CONFIG LOG ");
                sbReturn.Append("'");
                sbReturn.Append(anAuditLogsConfiguration.LogType);
                sbReturn.Append("'");
                sbReturn.Append(" '");
                sbReturn.Append(string.Join(", ", lstColNum.ToArray()));
                sbReturn.Append("' '");
                sbReturn.Append(string.Join(", ", lstValue.ToArray()));
                sbReturn.Append("'");
            }

            return sbReturn;
        }

        public override bool Equals(object obj)
        {
            AuditLogsConfigDataModel configDataModel = obj as AuditLogsConfigDataModel;
            if (obj == null)
            {
                return false;
            }
            return configDataModel.AuditLoggingEnable == this.AuditLoggingEnable &&
                    configDataModel.RefreshConfigTime == this.RefreshConfigTime &&
                    configDataModel.AuditConfigurationChanges == this.AuditConfigurationChanges &&
                    (configDataModel.AuditConfigurationChangesFailAction == this.AuditConfigurationChangesFailAction||
                    (this.AuditConfigurationChangesFailAction.Equals(string.Empty) && 
                    configDataModel.AuditConfigurationChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1))) &&
                    configDataModel.AuditAuthenticationSuccess == this.AuditAuthenticationSuccess &&
                    (configDataModel.AuditAuthenticationSuccessFailAction == this.AuditAuthenticationSuccessFailAction||
                    (this.AuditAuthenticationFailureFailAction.Equals(string.Empty) && 
                    configDataModel.AuditAuthenticationSuccessFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1)))&&
                    configDataModel.AuditAuthenticationFailure == this.AuditAuthenticationFailure &&
                    (configDataModel.AuditConfigurationChangesFailAction == this.AuditConfigurationChangesFailAction ||
                    (this.AuditConfigurationChangesFailAction.Equals(string.Empty) && 
                    configDataModel.AuditConfigurationChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1)))&&
                    configDataModel.AuditObjectChanges == this.AuditObjectChanges &&
                    (configDataModel.AuditObjectChangesFailAction == this.AuditObjectChangesFailAction||
                    (this.AuditObjectChangesFailAction.Equals(string.Empty) &&
                    configDataModel.AuditObjectChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1)))&&
                    configDataModel.AuditPrivilegeChanges == this.AuditPrivilegeChanges &&
                    (configDataModel.AuditPrivilegeChangesFailAction == this.AuditPrivilegeChangesFailAction ||
                    (this.AuditPrivilegeChangesFailAction.Equals(string.Empty) &&
                    configDataModel.AuditPrivilegeChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1))) &&
                    configDataModel.AuditSecurityViolations == this.AuditSecurityViolations &&
                    (configDataModel.AuditSecurityViolationsFailAction == this.AuditSecurityViolationsFailAction||
                    (this.AuditSecurityViolationsFailAction.Equals(string.Empty) &&
                    configDataModel.AuditSecurityViolationsFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1))) &&
                    configDataModel.AuditUserMgmtChanges == this.AuditUserMgmtChanges &&
                    (configDataModel.AuditUserMgmtChangesFailAction == this.AuditUserMgmtChangesFailAction||
                    (this.AuditUserMgmtChangesFailAction.Equals(string.Empty) &&
                    configDataModel.AuditUserMgmtChangesFailAction.Equals(LOG_FAIL_ACTION_OPT_CODE1))) &&
                    configDataModel.TableAging == this.TableAging &&
                    configDataModel.SpaceUsageNotifyThreshold == this.SpaceUsageNotifyThreshold &&
                    configDataModel.SpaceUsageWarnThreshold == this.SpaceUsageWarnThreshold &&
                    configDataModel.SpaceUsageDialoutThreshold == this.SpaceUsageDialoutThreshold &&
                    configDataModel.AuditLogTarget == this.AuditLogTarget &&
                    configDataModel.AuditLogAlternate == this.AuditLogAlternate;

        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }
    }

    public class DropdownItemObject
    {
        public string Desc { get; set; }
        public string Code { get; set; }
        public DropdownItemObject(string desc, string code)
        {
            Desc = desc;
            Code = code;
        }
    }
}
