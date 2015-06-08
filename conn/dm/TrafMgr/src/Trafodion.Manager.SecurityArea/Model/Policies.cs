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
using System.Data;
using System.IO;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.SecurityArea.Model
{
    /// <summary>
    /// Model object for security policies
    /// </summary>
    public class Policies : SecurityObject
    {
        #region Fields

        public const string TABLE_LOGGING = "Logging";
        public const string TABLE_ROLEPASSWORD = "RolePassword";
        public const string TABLE_PASSWORDQUALITY = "PasswordQuality";
        public const string TABLE_PASSWORDCONTROL = "PasswordControl";
        public const string TABLE_COMPATIBILITY = "Compatibility";

        public const string ROLE_PWD_FOR_PLATFORM_ALL = "A";
        public const string ROLE_PWD_FOR_PLATFORM_POWER = "P";
        public const string ROLE_PWD_FOR_PLATFORM_NONE = "N";

        public enum UPDATE_VECTOR_ALTERPWDSECPOLICY
        {
            SYS_AUTO_DOWNLOAD = 0,
            SYS_ENFORCE_CERT_EXPIRY,
            SYS_PASSWORD_ENCRYPTION // This is always ON and can not be changed.
        }

        private UInt16 _updateVectorSysCertificate = 0;

        public enum UPDATE_VECTOR_SECCONFIGPWRESETREQUIRESCURRENTPW
        {
            PWD_REQUIRED_FOR_POWER_ROLE_RESET = 0,
            PWD_REQUIRED_FOR_SUPER_RESET
        }

        private UInt16 _updateVectorCurPwdReqdForResetPwd = 0;

        public enum UPDATE_VECTOR_SECCONFIGDELAYAFTERLOGONFAILURE
        {
            PWD_MAX_LOGON_ATTEMPTS = 0,
            PWD_LOGON_DELAY
        }

        private UInt16 _updateVectorDelayForLogonFailure = 0;

        public enum UPDATE_VECTOR_SECCONFIGSETSYSOPTIONS
        {
            PWD_MIN_LENGTH = 0,
            PWD_GRACE_PERIOD,
            PWD_HISTORY,
            PWD_CAN_CHANGE_WITHIN,
            PWD_DEFAULT_EXPIRY_DATE,
            PWD_DEFAULT_EXPIRY_DAYS
        }

        private UInt16 _updateVectorSafeguardSysOptions = 0;

        /// <summary>
        /// Possible logging attr updated flag
        /// </summary>
        public enum UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS
        {
            LOG_USERMANAGEMENT = 0,
            LOG_USERMANAGEMENT_REQUIRED,
            LOG_CHANGE_PASSWORD,
            LOG_CHANGE_PASSWORD_REQUIRED,
            LOG_PLATFORM_LOGON_FAILURE,
            LOG_DATABASE_LOGON_FAILURE,
            LOG_PLATFORM_LOGON_OK,
            LOG_PLATFORM_LOGON_OK_REQUIRED,
            LOG_DATABASE_LOGON_OK,
            LOG_DATABASE_LOGON_OK_REQUIRED,
            LOG_FILE_AGES_IN_DAYS
        };

        private UInt16 _updateVectorLoggingOptions = 0;

        public enum UPDATE_VECTOR_SECCONFIGPASSWORDCOMPLEXITY
        {
            PWD_NUMBER_REQUIRED = 0,
            PWD_UPPER_REQUIRED,
            PWD_LOWER_REQUIRED,
            PWD_SPECIAL_REQUIRED,
            PWD_NUMBER_OF_CRITERIA, 
            PWD_NO_USER_NAME,
            PWD_NO_REPEAT_CHARS,
        }

        private UInt16 _updateVectorPasswordComplexity = 0;

        public enum UPDATE_VECTOR_MISCOPTIONS
        {
            PWD_ROLE_PWD_FOR_PLATFORM = 0,
            PWD_ROLE_PWD_FOR_DATABASE,
            COM_ALLOW_PRER25_DRIVERS,
            COM_OPT_FOR_LOCAL_ACCESS
        }

        private UInt16 _updateVectorMiscOptions = 0;

        /// <summary>
        /// Password options
        /// </summary>
        public enum ROLE_PWD_FOR_PLATOFRM
        {
            N = 0,
            A,
            P
        };

        /// <summary>
        /// Logging options
        /// Note: These names and order have to match with the elment names of the XML returned by the SPJs.
        ///       Check it out from the TrafodionSecSchema.XSD file. 
        /// </summary>
        public enum LOGGING_POLICY
        {
            LogUserMgmt = 0,
            IsUserMgmtLoggingRequired,
            LogPwdChange,
            IsPwdChangeLoggingRequired,
            LogPFLogonFailure,
            LogDBLogonFailure,
            LogPFLogonSuccess,
            LogDBLogonSuccess,
            IsPFLogonLoggingRequired,
            IsDBLogonLoggingRequired,
            LogFileAgingInDays
        }

        /// <summary>
        /// Role password policy options
        /// Note: These names and order have to match with the elment names of the XML returned by the SPJs.
        ///       Check it out from the TrafodionSecSchema.XSD file. 
        /// </summary>
        public enum ROLE_PASSWORD_POLICY
        {
            RolePwdForPlatform = 0,
            RolePwdForDB
        }

        /// <summary>
        /// Password quality options
        /// Note: These names and order have to match with the elment names of the XML returned by the SPJs.
        ///       Check it out from the TrafodionSecSchema.XSD file. 
        /// </summary>
        public enum PASSWORD_QUALITY_POLICY
        {
            PwdQualMinLen = 0,
            PwdQualReqCriteria,
            PwdQualReqSpecChar,
            PwdQualReqUpper,
            PwdQualReqLower,
            PwdQualReqDigit,
            PwdReqForPowerRoleReset,
            PwdReqForSuperReset,
            PwdQualNoUserName,
            PwdQualNoRepeatChars
        }

        /// <summary>
        /// Password control options
        /// Note: These names and order have to match with the elment names of the XML returned by the SPJs.
        ///       Check it out from the TrafodionSecSchema.XSD file. 
        /// </summary>
        public enum PASSWORD_CONTROL_POLICY
        {
            PwdCtrlGracePeriod = 0,
            PwdCtrlDefaultExprDate,
            PwdCtrlDefaultExprDays,
            PwdHistory,
            PwdCanChangeWithin,
            PwdAuthFailsBeforeDelay,
            PwdAuthFailDelayInSecs
        }

        /// Compatibility
        /// Note: These names and order have to match with the elment names of the XML returned by the SPJs.
        ///       Check it out from the TrafodionSecSchema.XSD file. 
        /// </summary>
        public enum COMPATIBILITY_POLICY
        {
            R2_93_CompatibilityMode = 0,
            AllowPreR25Drivers,
            OptimizeForLocalAccess
        }

        /// <summary>
        /// Possible policy setting
        /// </summary>
        public enum POLICY_SETTING
        {
            SETTING_FACTORY_DEFAULT = 1,
            SETTING_MOSE_SECURE,
            SETTING_CUSTOMER
        }

        // Denote that this is a factory default policies.
        private POLICY_SETTING _thePolicySetting = POLICY_SETTING.SETTING_CUSTOMER;
        private ConnectionDefinition _theConnectionDefinition = null;
        private DataSet _dataSet = new DataSet();

        // System policies
        private bool _autoDownload = false;
        private bool _enforceCertificateExpiry = true;
        private bool _passwordEncryptionRequired = true;
        private bool _allowPreR25Drivers = false;
        private bool _optimizeForLocalAccess = true;

        // Role password policies
        private ROLE_PWD_FOR_PLATOFRM _rolePasswordForPlatform = ROLE_PWD_FOR_PLATOFRM.N;
        private bool _rolePasswordForDatabase = false;

        // Logging Policies
        private bool _logUserManagement = true;
        private bool _logUserManagementRequired = true;
        private bool _logChangePassword = false;
        private bool _logChangePasswordRequired = false;
        private bool _logDatabaseLogonOK = false;
        private bool _logDatabaseLogonOKRequired = false;
        private bool _logPlatformLogonOK = false;
        private bool _logPlatformLogonOKRequired = false;
        private bool _logDatabaseLogonFailure = true;
        private bool _logPlatformLogonFailure = true;
        private int _logFileAgesInDays = 365; 

        // Password Policies
        private int _pwdQualMinLength = 8;
        private int _pwdQualMaxLength = 64;
        private int _pwdQualReqCriteria = 0;
        private bool _pwdQualReqSpecChar = false;
        private bool _pwdQualReqUpper = false;
        private bool _pwdQualReqLower = false;
        private bool _pwdQualReqNumber = false;
        private bool _pwdRequiredForPowerRoleReset = false;
        private bool _pwdRequiredForSuperReset = false;
        private bool _pwdQualNoUserName = false;
        private bool _pwdQualNoRepeatChars = false;

        // Password Controls
        private int _pwdHistory = 10;
        private int _pwdGracePeriod = 7;
        private int _pwdCanChangeWithin = 0;
        private int _pwdMaxLogonAttempts = 6;
        private int _pwdLogonDelay = 30;

        private bool _defaultPwdExpiry = true;
        private int _defaultPwdExpiryDays = 0;
        private string _defaultPwdExpiryDate = (DateTime.Now.AddDays(5)).ToShortDateString();

        private bool _xmlSchemaLoaded = false;
        private bool _reconnectRequired = false;
        private bool _rolePasswordRequirementChanged = false;

        #endregion Fields

        #region Properties

        /// <summary>
        /// PolicySetting: policy setting of this model object
        /// </summary>
        public POLICY_SETTING PolicySetting
        {
            get { return _thePolicySetting; }
        }

        /// <summary>
        /// ConnectionDefn: the connection defintiion
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        //==== System Policies =====================================

        /// <summary>
        /// AutoDownloadCertificate: enable/disable automatic downloading certificate to client machine.
        /// </summary>
        public bool AutoDownloadCertificate
        {
            get { return _autoDownload; }
            set 
            {
                if (_autoDownload != value)
                {
                    _autoDownload = value;
                    SetUpdateVectorSysCertificate(UPDATE_VECTOR_ALTERPWDSECPOLICY.SYS_AUTO_DOWNLOAD);
                }
            }
        }

        /// <summary>
        /// EnforceCertificateExpiry: enforce the certificate expiry.
        /// </summary>
        public bool EnforceCertificateExpiry
        {
            get { return _enforceCertificateExpiry; }
            set 
            {
                if (_enforceCertificateExpiry != value)
                {
                    _enforceCertificateExpiry = value;
                    SetUpdateVectorSysCertificate(UPDATE_VECTOR_ALTERPWDSECPOLICY.SYS_ENFORCE_CERT_EXPIRY);
                }
            }
        }

        /// <summary>
        /// PasswordEncryptionRequired: enable/disable password encryption when storing passwords on server.
        /// </summary>
        public bool PasswordEncryptionRequired
        {
            get { return _passwordEncryptionRequired; }
            set
            {
                if (_passwordEncryptionRequired != value)
                {
                    _passwordEncryptionRequired = value;
                    SetUpdateVectorSysCertificate(UPDATE_VECTOR_ALTERPWDSECPOLICY.SYS_PASSWORD_ENCRYPTION);
                }
            }
        }

        /// <summary>
        /// AllowDownRevDrivers: allow down-rev drivers to connect
        /// </summary>
        public bool AllowDownRevDrivers
        {
            get { return _allowPreR25Drivers; }
            set 
            {
                if (_allowPreR25Drivers != value)
                {
                    _allowPreR25Drivers = value;
                    SetUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS.COM_ALLOW_PRER25_DRIVERS);
                }
            }
        }

        public bool OptimizeForLocalAccess
        {
            get { return _optimizeForLocalAccess; }
            set 
            {
                if (_optimizeForLocalAccess != value)
                {
                    _optimizeForLocalAccess = value;
                    SetUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS.COM_OPT_FOR_LOCAL_ACCESS);
                }
            }
        }

        /// <summary>
        /// RolePasswordRequired: required role passwords when user logon. 
        /// </summary>
        public bool RolePasswordRequired
        {
            get 
            {
                if (_rolePasswordForDatabase || _rolePasswordForPlatform != ROLE_PWD_FOR_PLATOFRM.N)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        /// <summary>
        /// RolePasswordForPlatform: rquire role password for platform users?
        /// </summary>
        public ROLE_PWD_FOR_PLATOFRM RolePasswordForPlatform
        {
            get { return _rolePasswordForPlatform; }
            set 
            { 
                if (_rolePasswordForPlatform != value)
                {
                    _rolePasswordForPlatform = value;
                    SetUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS.PWD_ROLE_PWD_FOR_PLATFORM);
                }
            }
        }

        /// <summary>
        /// RolePasswordForDatabase: require role password for database user?
        /// </summary>
        public bool RolePasswordForDatabase
        {
            get { return _rolePasswordForDatabase; }
            set 
            {
                if (_rolePasswordForDatabase != value)
                {
                    _rolePasswordForDatabase = value;
                    SetUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS.PWD_ROLE_PWD_FOR_DATABASE);
                }
            }
        }



        //==== Logging Policies =====================================

        /// <summary>
        /// LogUserManagement: log all user management operations.
        /// </summary>
        public bool LogUserManagement
        {
            get { return _logUserManagement; }
            set 
            {
                if (_logUserManagement != value)
                {
                    _logUserManagement = value;
                    SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_USERMANAGEMENT);
                }
            }
        }

        /// <summary>
        /// LogUserManagementRequired: logging is required for all user management activities.
        /// Note: this will disallow user managemenet operations if the logging is not successful.
        /// </summary>
        public bool LogUserManagementRequired
        {
            get { return _logUserManagementRequired; }
            set 
            {
                if (_logUserManagementRequired != value)
                {
                    _logUserManagementRequired = value;
                    SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_USERMANAGEMENT_REQUIRED);
                }
            }
        }

        /// <summary>
        /// LogChangePassword: log all change passwords. 
        /// </summary>
        public bool LogChangePassword
        {
            get { return _logChangePassword; }
            set 
            {
                if (_logChangePassword != value)
                {
                    _logChangePassword = value;
                    SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_CHANGE_PASSWORD);
                }
            }
        }

        /// <summary>
        /// LogChangePasswordRequired: logging is required for all change passwords.
        /// Note: this will disallow change passwords if the logging is not successful.
        /// </summary>
        public bool LogChangePasswordRequired
        {
            get { return _logChangePasswordRequired; }
            set 
            {
                if (_logChangePasswordRequired != value)
                {
                    _logChangePasswordRequired = value;
                    SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_CHANGE_PASSWORD_REQUIRED);
                }
            }
        }

        /// <summary>
        /// LogDatabaseLogonOK: Log all database logon OK. 
        /// </summary>
        public bool LogDatabaseLogonOK
        {
            get { return _logDatabaseLogonOK; }
            set 
            {
                if (_logDatabaseLogonOK != value)
                {
                    _logDatabaseLogonOK = value;
                    SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_DATABASE_LOGON_OK);
                }
            }
        }

        /// <summary>
        /// LogDatabaseLogonOKRequired: logging is required for all database logon OK.
        /// Note: this will disallow logon if the logging is not successful.
        /// </summary>
        public bool LogDatabaseLogonOKRequired
        {
            get { return _logDatabaseLogonOKRequired; }
            set 
            {
                if (_logDatabaseLogonOKRequired != value)
                {
                    _logDatabaseLogonOKRequired = value;
                    SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_DATABASE_LOGON_OK_REQUIRED);
                }
            }
        }

        /// <summary>
        /// LogPlatformLogonOK: log all platform logon OK. 
        /// </summary>
        public bool LogPlatformLogonOK
        {
            get { return _logPlatformLogonOK; }
            set 
            {
                if (_logPlatformLogonOK != value)
                {
                    _logPlatformLogonOK = value;
                    SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_PLATFORM_LOGON_OK);
                }
            }
        }

        /// <summary>
        /// LogPlatformLogonOKRequired: logging is required for all platform logon OK.
        /// Note: this will disallow logon if the logging is not successful.
        /// </summary>
        //public bool LogPlatformLogonOKRequired
        //{
        //    get { return _logPlatformLogonOKRequired; }
        //    set 
        //    {
        //        if (_logPlatformLogonOKRequired != value)
        //        {
        //            _logPlatformLogonOKRequired = value;
        //            SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_PLATFORM_LOGON_OK_REQUIRED);
        //        }
        //    }
        //}

        /// <summary>
        /// LogDatabaseLogonFailure: log all database logon failures
        /// </summary>
        public bool LogDatabaseLogonFailure
        {
            get { return _logDatabaseLogonFailure; }
            set 
            {
                if (_logDatabaseLogonFailure != value)
                {
                    _logDatabaseLogonFailure = value;
                    SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_DATABASE_LOGON_FAILURE);
                }
            }
        }

        /// <summary>
        /// LogPlatformLogonFailure: log all platform logon failures
        /// </summary>
        public bool LogPlatformLogonFailure
        {
            get { return _logPlatformLogonFailure; }
            set 
            {
                if (_logPlatformLogonFailure != value)
                {
                    _logPlatformLogonFailure = value;
                    SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_PLATFORM_LOGON_FAILURE);
                }
            }
        }

        /// <summary>
        /// LogFileAgesInDays: Log file will be aged in number of days
        /// </summary>
        public int LogFileAgesInDays
        {
            get { return (int)_logFileAgesInDays; }
            set 
            {
                if (_logFileAgesInDays != value)
                {
                    _logFileAgesInDays = value;
                    SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_FILE_AGES_IN_DAYS);
                }
            }
        }

        //==== Password Policies =====================================

        /// <summary>
        /// MinLength: the min. length for a password. 
        /// </summary>
        public int PwdQualMinLength
        {
            get { return (int)_pwdQualMinLength; }
            set 
            {
                if (_pwdQualMinLength != value)
                {
                    _pwdQualMinLength = value;
                    SetUpdateVectorSafeguardSysOptions(UPDATE_VECTOR_SECCONFIGSETSYSOPTIONS.PWD_MIN_LENGTH);
                }
            }
        }

        //public int MaxLength
        //{
        //    get { return (int)_pwdQualMaxLength; }
        //    set { _pwdQualMaxLength = value; }
        //}

        /// <summary>
        /// History: the number of password history entries kept.  
        /// </summary>
        public int PwdHistory
        {
            get { return (int)_pwdHistory; }
            set 
            {
                if (_pwdHistory != value)
                {
                    _pwdHistory = value;
                    SetUpdateVectorSafeguardSysOptions(UPDATE_VECTOR_SECCONFIGSETSYSOPTIONS.PWD_HISTORY);
                }
            }
        }

        /// <summary>
        /// GracePeriod: grace period after a password has expired.
        /// </summary>
        public int PwdCtrlGracePeriod
        {
            get { return (int)_pwdGracePeriod; }
            set 
            {
                if (_pwdGracePeriod != value)
                {
                    _pwdGracePeriod = value;
                    SetUpdateVectorSafeguardSysOptions(UPDATE_VECTOR_SECCONFIGSETSYSOPTIONS.PWD_GRACE_PERIOD);
                }
            }
        }

        /// <summary>
        /// CanChangeWithin: user can change passwords within the number of days before the expiry. 
        /// </summary>
        public int PwdCanChangeWithin
        {
            get { return (int)_pwdCanChangeWithin; }
            set 
            {
                if (_pwdCanChangeWithin != value)
                {
                    _pwdCanChangeWithin = value;
                    SetUpdateVectorSafeguardSysOptions(UPDATE_VECTOR_SECCONFIGSETSYSOPTIONS.PWD_CAN_CHANGE_WITHIN);
                }
            }
        }

        /// <summary>
        /// PwdAuthFailsBeforeDelay: the max. logon failures allowed for a user to retry. 
        /// </summary>
        public int PwdAuthFailsBeforeDelay
        {
            get { return (int)_pwdMaxLogonAttempts; }
            set 
            {
                if (_pwdMaxLogonAttempts != value)
                {
                    _pwdMaxLogonAttempts = value;
                    SetUpdateVectorDelayForLogonFailure(UPDATE_VECTOR_SECCONFIGDELAYAFTERLOGONFAILURE.PWD_MAX_LOGON_ATTEMPTS);
                }
            }
        }

        /// <summary>
        /// PwdAuthFailDelayInSecs: the delay in seconds if user logons have reached the max logon attempts
        /// </summary>
        public int PwdAuthFailDelayInSecs
        {
            get { return (int)_pwdLogonDelay; }
            set 
            {
                if (_pwdLogonDelay != value)
                {
                    _pwdLogonDelay = value;
                    SetUpdateVectorDelayForLogonFailure(UPDATE_VECTOR_SECCONFIGDELAYAFTERLOGONFAILURE.PWD_LOGON_DELAY);
                }
            }
        }

        /// <summary>
        /// PwdQualReqCriteria: requires password to meet number of quality criteria
        /// </summary>
        public int PwdQualReqCriteria
        {
            get { return _pwdQualReqCriteria; }
            set 
            {
                if (_pwdQualReqCriteria != value)
                {
                    _pwdQualReqCriteria = value;
                    SetUpdateVectorPasswordComplexity(UPDATE_VECTOR_SECCONFIGPASSWORDCOMPLEXITY.PWD_NUMBER_OF_CRITERIA);
                }
            }
        }

        /// <summary>
        /// PwdQualReqUpper: to specify whether passwords require uppercase characters.
        /// </summary>
        public bool PwdQualReqUpper
        {
            get { return _pwdQualReqUpper; }
            set 
            {
                if (_pwdQualReqUpper != value)
                {
                    _pwdQualReqUpper = value;
                    SetUpdateVectorPasswordComplexity(UPDATE_VECTOR_SECCONFIGPASSWORDCOMPLEXITY.PWD_UPPER_REQUIRED);
                }
            }
        }

        /// <summary>
        /// PwdQualReqLower: to specify whether passwords require lowercase characters.
        /// </summary>
        public bool PwdQualReqLower
        {
            get { return _pwdQualReqLower; }
            set 
            {
                if (_pwdQualReqLower != value)
                {
                    _pwdQualReqLower = value;
                    SetUpdateVectorPasswordComplexity(UPDATE_VECTOR_SECCONFIGPASSWORDCOMPLEXITY.PWD_LOWER_REQUIRED);
                }
            }
        }

        /// <summary>
        /// PwdQualReqNumber: to specify whether passwords require numeric characters.
        /// </summary>
        public bool PwdQualReqNumber
        {
            get { return _pwdQualReqNumber; }
            set 
            {
                if (_pwdQualReqNumber != value)
                {
                    _pwdQualReqNumber = value;
                    SetUpdateVectorPasswordComplexity(UPDATE_VECTOR_SECCONFIGPASSWORDCOMPLEXITY.PWD_NUMBER_REQUIRED);
                }
            }
        }

        /// <summary>
        /// PwdQualReqSpecChar: to specify whether passwords require special characters.
        /// </summary>
        public bool PwdQualReqSpecChar
        {
            get { return _pwdQualReqSpecChar; }
            set 
            {
                if (_pwdQualReqSpecChar != value)
                {
                    _pwdQualReqSpecChar = value;
                    SetUpdateVectorPasswordComplexity(UPDATE_VECTOR_SECCONFIGPASSWORDCOMPLEXITY.PWD_SPECIAL_REQUIRED);
                }
            }
        }

        /// <summary>
        /// PwdQualNoRepeatChars: to specify whether repeated chars are allowed.
        /// </summary>
        public bool PwdQualNoRepeatChars
        {
            get { return _pwdQualNoRepeatChars; }
            set 
            {
                if (_pwdQualNoRepeatChars != value)
                {
                    _pwdQualNoRepeatChars = value;
                    SetUpdateVectorPasswordComplexity(UPDATE_VECTOR_SECCONFIGPASSWORDCOMPLEXITY.PWD_NO_REPEAT_CHARS);
                }
            }
        }

        /// <summary>
        /// PwdQualNoUserName: to disallow user name as the password
        /// </summary>
        public bool PwdQualNoUserName
        {
            get { return _pwdQualNoUserName; }
            set 
            {
                if (_pwdQualNoUserName != value)
                {
                    _pwdQualNoUserName = value;
                    SetUpdateVectorPasswordComplexity(UPDATE_VECTOR_SECCONFIGPASSWORDCOMPLEXITY.PWD_NO_USER_NAME);
                }
            }
        }

        /// <summary>
        /// DefaultPasswordExpiry: the default value for password expiry
        /// </summary>
        public bool DefaultPasswordExpiry
        {
            get 
            {
                if (PwdDefaultExprDays > 0 || !String.IsNullOrEmpty(PwdDefaultExprDate))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        /// <summary>
        /// PwdDefaultExprDays: the default value for password to expire in days
        /// </summary>
        public int PwdDefaultExprDays
        {
            get { return _defaultPwdExpiryDays; }
            set 
            {
                if (_defaultPwdExpiryDays != value)
                {
                    _defaultPwdExpiryDays = value;
                    SetUpdateVectorSafeguardSysOptions(UPDATE_VECTOR_SECCONFIGSETSYSOPTIONS.PWD_DEFAULT_EXPIRY_DAYS);
                }
            }
        }

        /// <summary>
        /// PwdDefaultExprDate: the default password expiry date. 
        /// </summary>
        public string PwdDefaultExprDate
        {
            get { return _defaultPwdExpiryDate; }
            set 
            {
                if (String.Compare(_defaultPwdExpiryDate, value, true) != 0)
                {
                    _defaultPwdExpiryDate = value;
                    SetUpdateVectorSafeguardSysOptions(UPDATE_VECTOR_SECCONFIGSETSYSOPTIONS.PWD_DEFAULT_EXPIRY_DATE);
                }
            }
        }

        /// <summary>
        /// PwdReqForPowerRoleReset: required the current password to reset power roles' passwords.
        /// </summary>
        public bool PwdReqForPowerRoleReset
        {
            get { return _pwdRequiredForPowerRoleReset; }
            set
            {
                if (_pwdRequiredForPowerRoleReset != value)
                {
                    _pwdRequiredForPowerRoleReset = value;
                    SetUpdateVectorCurPwdReqdForResetPwd(UPDATE_VECTOR_SECCONFIGPWRESETREQUIRESCURRENTPW.PWD_REQUIRED_FOR_POWER_ROLE_RESET);
                }
            }
        }

        /// <summary>
        /// PwdReqForSuperReset: required the current password to reset super user's password.
        /// </summary>
        public bool PwdReqForSuperReset
        {
            get { return _pwdRequiredForSuperReset; }
            set
            {
                if (_pwdRequiredForSuperReset != value)
                {
                    _pwdRequiredForSuperReset = value;
                    SetUpdateVectorCurPwdReqdForResetPwd(UPDATE_VECTOR_SECCONFIGPWRESETREQUIRESCURRENTPW.PWD_REQUIRED_FOR_SUPER_RESET);
                }
            }
        }

        /// <summary>
        /// IsSysCertificateChanged: Return true if the system policy has been changed.
        /// </summary>
        /// <returns></returns>
        public bool IsSysCertificateChanged
        {
            get { return (_updateVectorSysCertificate > 0); }
        }

        /// <summary>
        /// IsLoggingPolicyChanged: Has the logging policy been changed?
        /// </summary>
        /// <returns></returns>
        public bool IsLoggingOptionsChanged
        {
            get { return (_updateVectorLoggingOptions > 0); }
        }

        /// <summary>
        /// IsPasswordPolicyChanged: Return true if password policy has been updated.
        /// </summary>
        /// <returns></returns>
        public bool IsPasswordComplexityChanged
        {
            get { return (_updateVectorPasswordComplexity > 0); }
        }

        /// <summary>
        /// IsCurPwdReqdForResetPwdChanged: return true if power user reset password policy has been updated.
        /// </summary>
        public bool IsCurPwdReqdForResetPwdChanged
        {
            get { return (_updateVectorCurPwdReqdForResetPwd > 0); }
        }

        /// <summary>
        /// IsDelayForLogonFailureChanged: return true if the logon failure policy has been updated.
        /// </summary>
        public bool IsDelayForLogonFailureChanged
        {
            get { return (_updateVectorDelayForLogonFailure > 0); } 
        }

        /// <summary>
        /// IsSafeguardSysOptionsChanged: return true if the Safeguard related options have been updated.
        /// </summary>
        public bool IsSafeguardSysOptionsChanged
        {
            get { return (_updateVectorSafeguardSysOptions > 0); }
        }

        /// <summary>
        /// IsMiscOptionsChanged: return true if any of the misc options have been updated.
        /// </summary>
        public bool IsMiscOptionsChanged
        {
            get { return (_updateVectorMiscOptions > 0); }
        }

        /// <summary>
        /// IsChangePending: return true if any change is pending. 
        /// </summary>
        public bool IsChangePending
        {
            get
            {
                return (IsCurPwdReqdForResetPwdChanged ||
                        IsDelayForLogonFailureChanged ||
                        IsLoggingOptionsChanged ||
                        IsPasswordComplexityChanged ||
                        IsSysCertificateChanged ||
                        IsSafeguardSysOptionsChanged ||
                        IsMiscOptionsChanged);
            }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor for factory default policies
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aFactoryDefaultFlag"></param>
        public Policies(ConnectionDefinition aConnectionDefinition, POLICY_SETTING aPolicySetting)
            : base(aConnectionDefinition)
        {
            _thePolicySetting = aPolicySetting;
            _theConnectionDefinition = aConnectionDefinition;
            //LoadPolicies(); // Since the loading of policies takes time, we'll let the caller have the control of 
            //showing progress dialog.
        }

        #endregion Construcutors

        #region Public methods

        /// <summary>
        /// Refresh the system policies
        /// </summary>
        private void RefreshSystemPolicies()
        {
            //If read only access then do not read system policies.
            if (SystemSecurity.FindSystemModel(_theConnectionDefinition).IsViewOnly)
            {
                return;
            }

            if (!GetConnection())
                return;

            bool autoDownload;
            bool enforceCertificateExpiry;
            bool passwordEncryption;

            try
            {
                Queries.ExecuteInfoSystemPolicies(CurrentConnection, ConnectionDefn.MasterSegmentName,
                    out autoDownload, out enforceCertificateExpiry, out passwordEncryption);
            }
            finally
            {
                CloseConnection();
            }

            AutoDownloadCertificate = autoDownload;
            EnforceCertificateExpiry = enforceCertificateExpiry;
            PasswordEncryptionRequired = passwordEncryption;
            ResetUpdateVectorSysCertificate();
        }

        /// <summary>
        /// Refresh the entire policies. 
        /// </summary>
        public void Refresh()
        {
            if (!_xmlSchemaLoaded)
            {
                LoadXMLSchema();
            }
            _rolePasswordRequirementChanged = false;
            _reconnectRequired = false;
            RefreshSystemPolicies();
            GetAttributes();
        }

        /// <summary>
        /// Reset all attributes to the factory default setting.
        /// </summary>
        public void ResetAllToDefault()
        {
            if (!GetConnection())
                return;

            try
            {
                Queries.ExecuteResetAllToDefaultPolicies(CurrentConnection);
                _reconnectRequired = true;
            }
            finally
            {
                CloseConnection();
            }
        }

        /// <summary>
        /// Reset all attributes to the most secure setting.
        /// </summary>
        public void ResetAllToMostSecure()
        {
            if (!GetConnection())
                return;

            try
            {
                Queries.ExecuteResetAllToMostSecurePolicies(CurrentConnection);
                _reconnectRequired = true;
            }
            finally
            {
                CloseConnection();
            }
        }

        /// <summary>
        /// Update the entire policies.
        /// </summary>
        public void UpdateAll()
        {
            UpdatePolicies();
        }


        /// <summary>
        /// Update the system policies.  But, call SPJs only if there is a change in that particular setting.
        /// All exceptions will be caught first and a cancatenated message will be thrown at the end.
        /// </summary>
        public void UpdatePolicies()
        {
            string exceptionMessage = null;

            if ((!IsChangePending) || (!GetConnection()))
            {
                return;
            }

            _reconnectRequired = false;

            try
            {
                if (IsSysCertificateChanged)
                {
                    try
                    {
                        Queries.ExecuteAlterSystemPolicies(CurrentConnection, ConnectionDefn.MasterSegmentName, this);
                    }
                    catch (Exception ex)
                    {
                        exceptionMessage += "AlterPwdSecPolicy failed: " + ex.Message;
                    }
                }

                if (IsPasswordComplexityChanged)
                {
                    try
                    {
                        Queries.ExecuteAlterPwdComplexityPolicies(CurrentConnection, this);
                    }
                    catch (Exception ex)
                    {
                        exceptionMessage += "\n" + "SecConfigPasswordComplexity failed: " + ex.Message;
                    }
                }

                if (IsCurPwdReqdForResetPwdChanged)
                {
                    try
                    {
                        Queries.ExecuteAlterResetPwdPolicies(CurrentConnection, this);
                    }
                    catch (Exception ex)
                    {
                        exceptionMessage += "\n" + "SecConfigPWResetRequiresCurrentPW failed: " + ex.Message;
                    }
                }

                if (IsDelayForLogonFailureChanged)
                {
                    try
                    {
                        Queries.ExecuteAlterDelayAfterLogonFailurePolicies(CurrentConnection, this);
                    }
                    catch (Exception ex)
                    {
                        exceptionMessage += "\n" + "SecConfigDelayAfterLogonFailure failed: " + ex.Message;
                    }
                }

                if (IsSafeguardSysOptionsChanged)
                {
                    try
                    {
                        Queries.ExecuteAlterSafeGuardSysOptions(CurrentConnection, this);
                    }
                    catch (Exception ex)
                    {
                         exceptionMessage += "\n" + "SecConfigSetSysOptions failed: " + ex.Message;
                    }

                }

                if (IsLoggingOptionsChanged)
                {
                    try
                    {
                        Queries.ExecuteAlterLoggingPolicies(CurrentConnection, this);
                    }
                    catch (Exception ex)
                    {
                        exceptionMessage += "\n" + "SecConfigSetLoggingOptions failed: " + ex.Message;
                    }
                }

                if (IsMiscOptionsChanged)
                {
                    if (CheckUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS.COM_ALLOW_PRER25_DRIVERS))
                    {
                        try
                        {
                            Queries.ExecuteAlterOneOption(CurrentConnection,
                                COMPATIBILITY_POLICY.AllowPreR25Drivers.ToString(),
                                (this.AllowDownRevDrivers ? "Y" : "N"));
                            _reconnectRequired = true;
                        }
                        catch (Exception ex)
                        {
                            exceptionMessage += "\n" + "SecConfigSetOneOption(AllowPreR25Drivers) failed: " + ex.Message;
                        }
                    }

                    if (CheckUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS.COM_OPT_FOR_LOCAL_ACCESS))
                    {
                        try
                        {
                            Queries.ExecuteAlterOneOption(CurrentConnection,
                                                          COMPATIBILITY_POLICY.OptimizeForLocalAccess.ToString(),
                                                          (this.OptimizeForLocalAccess ? "Y" : "N"));
                        }
                        catch (Exception ex)
                        {
                            exceptionMessage += "\n" + "SecConfigSetOneOption(OptimizeForLocalAccess) failed: " + ex.Message;
                        }
                    }

                    if (CheckUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS.PWD_ROLE_PWD_FOR_DATABASE))
                    {
                        try
                        {
                            Queries.ExecuteAlterOneOption(CurrentConnection,
                                                          ROLE_PASSWORD_POLICY.RolePwdForDB.ToString(),
                                                          (this.RolePasswordForDatabase ? "Y" : "N"));
                            _reconnectRequired = true;
                            _rolePasswordRequirementChanged = true;
                        }
                        catch (Exception ex)
                        {
                            exceptionMessage += "\n" + "SecConfigSetOneOption(RolePasswordForDatabase) failed: " + ex.Message;
                        }
                    }

                    if (CheckUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS.PWD_ROLE_PWD_FOR_PLATFORM))
                    {
                        try
                        {
                            Queries.ExecuteAlterOneOption(CurrentConnection,
                                                          ROLE_PASSWORD_POLICY.RolePwdForPlatform.ToString(),
                                                          this.RolePasswordForPlatform.ToString());
                            _reconnectRequired = true;
                            _rolePasswordRequirementChanged = true;
                        }
                        catch (Exception ex)
                        {
                            exceptionMessage += "\n" + "SecConfigSetOneOption(RolePasswordForPlatform) failed: " + ex.Message;
                        }
                    }
                }
            }
            finally
            {
                CloseConnection();
            }

            if (!string.IsNullOrEmpty(exceptionMessage))
            {
                throw new Exception(exceptionMessage);
            }
        }

        //====== Vector operations for System certificate options 

        /// <summary>
        /// Turn on a certifcate and connection setting.
        /// </summary>
        /// <param name="vector"></param>
        public void SetUpdateVectorSysCertificate(UPDATE_VECTOR_ALTERPWDSECPOLICY vector)
        {
            _updateVectorSysCertificate |= (UInt16)(1 << (int)vector);
        }

        /// <summary>
        /// Check if a system certificate and connection setting is changed. 
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        public bool CheckUpdateVectorSysCertificate(UPDATE_VECTOR_ALTERPWDSECPOLICY vector)
        {
            return ((_updateVectorSysCertificate & (UInt16)(1 << (int)vector)) > 0);
        } 

        /// <summary>
        /// Clear all change flag for certificate and conneciton.
        /// </summary>
        public void ResetUpdateVectorSysCertificate()
        {
            _updateVectorSysCertificate = 0;
        }


        //====== Vector operations for Current password required for Reset passwords 

        /// <summary>
        /// Turn on a flag for reset password policy
        /// </summary>
        /// <param name="vector"></param>
        public void SetUpdateVectorCurPwdReqdForResetPwd(UPDATE_VECTOR_SECCONFIGPWRESETREQUIRESCURRENTPW vector)
        {
            _updateVectorCurPwdReqdForResetPwd |= (UInt16)(1 << (int)vector);
        }

        /// <summary>
        /// Check if a reset password policy is changed. 
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        public bool CheckUpdateVectorCurPwdReqdForResetPwd(UPDATE_VECTOR_SECCONFIGPWRESETREQUIRESCURRENTPW vector)
        {
            return ((_updateVectorCurPwdReqdForResetPwd & (UInt16)(1 << (int)vector)) > 0);
        } 

        /// <summary>
        /// Clear all reset password policy change flag.
        /// </summary>
        public void ResetUpdateVectorCurPwdReqdForResetPwd()
        {
            _updateVectorCurPwdReqdForResetPwd = 0;
        }

        //====== Vector operations for Logon failure delay options 

        /// <summary>
        /// Turn on a change flag for logon failure policy. 
        /// </summary>
        /// <param name="vector"></param>
        public void SetUpdateVectorDelayForLogonFailure(UPDATE_VECTOR_SECCONFIGDELAYAFTERLOGONFAILURE vector)
        {
            _updateVectorDelayForLogonFailure |= (UInt16)(1 << (int)vector);
        }

        /// <summary>
        /// Check if a change flag for logon failure policy has been turned on. 
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        public bool CheckUpdateVectorDelayForLogonFailure(UPDATE_VECTOR_SECCONFIGDELAYAFTERLOGONFAILURE vector)
        {
            return ((_updateVectorDelayForLogonFailure & (UInt16)(1 << (int)vector)) > 0);
        } 

        /// <summary>
        /// Clear all change flag for logon failure policy.
        /// </summary>
        public void ResetUpdateVectorDelayForLogonFailure()
        {
            _updateVectorDelayForLogonFailure = 0;
        }

        //====== Vector operations for Safeguard System Options 

        /// <summary>
        /// Turn on a change flag for safeguard options policy. 
        /// </summary>
        /// <param name="vector"></param>
        public void SetUpdateVectorSafeguardSysOptions(UPDATE_VECTOR_SECCONFIGSETSYSOPTIONS vector)
        {
            _updateVectorSafeguardSysOptions |= (UInt16)(1 << (int)vector);
        }

        /// <summary>
        /// Check if a change flag for safeguard options policy has been turned on. 
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        public bool CheckUpdateVectorSafeguardSysOptions(UPDATE_VECTOR_SECCONFIGSETSYSOPTIONS vector)
        {
            return ((_updateVectorSafeguardSysOptions & (UInt16)(1 << (int)vector)) > 0);
        }

        /// <summary>
        /// Clear all change flag for safeguard options policy.
        /// </summary>
        public void ResetUpdateVectorSafeguardSysOptions()
        {
            _updateVectorSafeguardSysOptions = 0;
        }


        //====== Vector operations for Logging options
        /// <summary>
        /// Turn on a change flag for logging options policy. 
        /// </summary>
        /// <param name="vector"></param>
        public void SetUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS vector)
        {
            _updateVectorLoggingOptions |= (UInt16)(1 << (int)vector);
        }

        /// <summary>
        /// Check if a change flag for logging options policy has been turned on. 
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        public bool CheckUpdateVectorLoggingOptions(UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS vector)
        {
            return ((_updateVectorLoggingOptions & (UInt16)(1 << (int)vector)) > 0);
        }

        /// <summary>
        /// Clear all change flag for logging options policy.
        /// </summary>
        public void ResetUpdateVectorLoggingOptions()
        {
            _updateVectorLoggingOptions = 0;
        }

        //====== Vector operations for Password Complexity 
        /// <summary>
        /// Turn on a change flag for password complexity policy. 
        /// </summary>
        /// <param name="vector"></param>
        public void SetUpdateVectorPasswordComplexity(UPDATE_VECTOR_SECCONFIGPASSWORDCOMPLEXITY vector)
        {
            _updateVectorPasswordComplexity |= (UInt16)(1 << (int)vector);
        }

        /// <summary>
        /// Check if a change flag for password complexity policy has been turned on. 
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        public bool CheckUpdateVectorPasswordComplexity(UPDATE_VECTOR_SECCONFIGPASSWORDCOMPLEXITY vector)
        {
            return ((_updateVectorPasswordComplexity & (UInt16)(1 << (int)vector)) > 0);
        }

        /// <summary>
        /// Clear all change flag for password complexity policy.
        /// </summary>
        public void ResetUpdateVectorPasswordComplexity()
        {
            _updateVectorPasswordComplexity = 0;
        }

        //====== Vector operations for Misc Options 
        /// <summary>
        /// Turn on a change flag for misc options policy. 
        /// </summary>
        /// <param name="vector"></param>
        public void SetUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS vector)
        {
            _updateVectorMiscOptions |= (UInt16)(1 << (int)vector);
        }

        /// <summary>
        /// Check if a change flag for misc options policy has been turned on. 
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        public bool CheckUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS vector)
        {
            return ((_updateVectorMiscOptions & (UInt16)(1 << (int)vector)) > 0);
        }

        /// <summary>
        /// Clear all change flag for misc options policy.
        /// </summary>
        public void ResetUpdateVectorMiscOptions()
        {
            _updateVectorMiscOptions = 0;
        }

        /// <summary>
        /// Indicates if the policy change would require a NDCS restart
        /// </summary>
        public bool DoesPolicyUpdateRequireNDCSRestart
        {
            get
            {
                if (((_updateVectorLoggingOptions & 1 << (int)UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_DATABASE_LOGON_OK) != 0) ||
                    ((_updateVectorLoggingOptions & 1 << (int)UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_PLATFORM_LOGON_FAILURE) != 0) ||
                    ((_updateVectorLoggingOptions & 1 << (int)UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_DATABASE_LOGON_OK_REQUIRED) != 0) ||
                    ((_updateVectorLoggingOptions & 1 << (int)UPDATE_VECTOR_SECCONFIGSETLOGGINGOPTIONS.LOG_PLATFORM_LOGON_OK_REQUIRED) != 0)
                   )
                {
                    return true;
                }
                if (IsMiscOptionsChanged)
                {
                    if (CheckUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS.COM_ALLOW_PRER25_DRIVERS))
                    {
                        return true;
                    }
                }
                if (CheckUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS.PWD_ROLE_PWD_FOR_DATABASE))
                {
                    return true;
                }
                if (CheckUpdateVectorMiscOptions(UPDATE_VECTOR_MISCOPTIONS.PWD_ROLE_PWD_FOR_PLATFORM))
                {
                    return true;
                }

                return false;
            }
        }

        public bool ReconnectRequired
        {
            get { return _reconnectRequired; }
        }

        public bool RolePasswordRequirementChanged
        {
            get
            {
                return _rolePasswordRequirementChanged;
            }
        }


        /// <summary>
        /// Load all policy settings
        /// </summary>
        public void LoadPolicies()
        {
            Refresh();
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Load the security XML schema.
        /// </summary>
        private void LoadXMLSchema()
        {
            _dataSet.ReadXmlSchema(Path.Combine(Application.StartupPath, "TrafodionSecSchema.XSD"));
            _xmlSchemaLoaded = true;
        }

        /// <summary>
        /// Get all attributes from server. 
        /// </summary>
        public void GetAttributes()
        {
            string rawXMLString = null;

            if (!_xmlSchemaLoaded)
            {
                LoadXMLSchema();
            }

            // First, cleanup what we have in the cache. 
            _dataSet.Tables[TABLE_LOGGING].Clear();
            _dataSet.Tables[TABLE_ROLEPASSWORD].Clear();
            _dataSet.Tables[TABLE_PASSWORDQUALITY].Clear();
            _dataSet.Tables[TABLE_PASSWORDCONTROL].Clear();
            _dataSet.Tables[TABLE_COMPATIBILITY].Clear();

            if (!GetConnection())
                return;

            try
            {
                switch (PolicySetting)
                {
                    case POLICY_SETTING.SETTING_FACTORY_DEFAULT:
                        rawXMLString = Queries.ExecuteInfoFactoryDefaultPolicies(CurrentConnection).Trim();
                        break;

                    case POLICY_SETTING.SETTING_MOSE_SECURE:
                        rawXMLString = Queries.ExecuteInfoMostStrongPolicies(CurrentConnection).Trim();
                        break;

                    default:
                        rawXMLString = Queries.ExecuteInfoPolicies(CurrentConnection).Trim();
                        break;
                }

                if (rawXMLString != null)
                {
                    StringReader sr = new StringReader(rawXMLString);
                    try
                    {
                        _dataSet.ReadXml(sr, XmlReadMode.Auto);
                    }
                    finally
                    {
                        sr.Close();
                        sr.Dispose();
                    }
                }
            }
            finally
            {
                CloseConnection();
            }

            // Now, we can parse the attributes

            DataTable loggingTable = _dataSet.Tables[TABLE_LOGGING];
            DataTable roleTable = _dataSet.Tables[TABLE_ROLEPASSWORD];
            DataTable pwdQualityTable = _dataSet.Tables[TABLE_PASSWORDQUALITY];
            DataTable pwdControlTable = _dataSet.Tables[TABLE_PASSWORDCONTROL];
            DataTable compatibilityTable = _dataSet.Tables[TABLE_COMPATIBILITY];

            // Retrieve logging policies
            LogUserManagement = (bool)loggingTable.Rows[0][LOGGING_POLICY.LogUserMgmt.ToString()];
            LogUserManagementRequired = (bool)loggingTable.Rows[0][LOGGING_POLICY.IsUserMgmtLoggingRequired.ToString()];
            LogChangePassword = (bool)loggingTable.Rows[0][LOGGING_POLICY.LogPwdChange.ToString()];
            LogChangePasswordRequired = (bool)loggingTable.Rows[0][LOGGING_POLICY.IsPwdChangeLoggingRequired.ToString()];
            LogPlatformLogonFailure = (bool)loggingTable.Rows[0][LOGGING_POLICY.LogPFLogonFailure.ToString()];
            LogDatabaseLogonFailure = (bool)loggingTable.Rows[0][LOGGING_POLICY.LogDBLogonFailure.ToString()];
            LogPlatformLogonOK = (bool)loggingTable.Rows[0][LOGGING_POLICY.LogPFLogonSuccess.ToString()];
            //LogPlatformLogonOKRequired = (bool)loggingTable.Rows[0][LOGGING_POLICY.IsPFLogonLoggingRequired.ToString()];
            LogDatabaseLogonOK = (bool)loggingTable.Rows[0][LOGGING_POLICY.LogDBLogonSuccess.ToString()];
            LogDatabaseLogonOKRequired = (bool)loggingTable.Rows[0][LOGGING_POLICY.IsDBLogonLoggingRequired.ToString()];
            LogFileAgesInDays = (int)loggingTable.Rows[0][LOGGING_POLICY.LogFileAgingInDays.ToString()];
            ResetUpdateVectorLoggingOptions();

            // Retrieve Role password policies
            RolePasswordForDatabase = (bool)roleTable.Rows[0][ROLE_PASSWORD_POLICY.RolePwdForDB.ToString()];
            RolePasswordForPlatform = (ROLE_PWD_FOR_PLATOFRM)Enum.Parse(typeof(ROLE_PWD_FOR_PLATOFRM), roleTable.Rows[0][ROLE_PASSWORD_POLICY.RolePwdForPlatform.ToString()] as string);
            
            // Reset Misc Options is at the end...

            // Password Quality policies
            PwdQualMinLength = (int)pwdQualityTable.Rows[0][PASSWORD_QUALITY_POLICY.PwdQualMinLen.ToString()];
            PwdQualReqCriteria = (int)pwdQualityTable.Rows[0][PASSWORD_QUALITY_POLICY.PwdQualReqCriteria.ToString()];
            PwdQualReqSpecChar = (bool)pwdQualityTable.Rows[0][PASSWORD_QUALITY_POLICY.PwdQualReqSpecChar.ToString()];
            PwdQualReqUpper = (bool)pwdQualityTable.Rows[0][PASSWORD_QUALITY_POLICY.PwdQualReqUpper.ToString()];
            PwdQualReqLower = (bool)pwdQualityTable.Rows[0][PASSWORD_QUALITY_POLICY.PwdQualReqLower.ToString()];
            PwdQualReqNumber = (bool)pwdQualityTable.Rows[0][PASSWORD_QUALITY_POLICY.PwdQualReqDigit.ToString()];
            PwdReqForPowerRoleReset = (bool)pwdQualityTable.Rows[0][PASSWORD_QUALITY_POLICY.PwdReqForPowerRoleReset.ToString()];
            PwdReqForSuperReset = (bool)pwdQualityTable.Rows[0][PASSWORD_QUALITY_POLICY.PwdReqForSuperReset.ToString()];
            PwdQualNoUserName = (bool)pwdQualityTable.Rows[0][PASSWORD_QUALITY_POLICY.PwdQualNoUserName.ToString()];
            PwdQualNoRepeatChars = (bool)pwdQualityTable.Rows[0][PASSWORD_QUALITY_POLICY.PwdQualNoRepeatChars.ToString()]; 

            // Password control policies
            PwdCtrlGracePeriod = (int)pwdControlTable.Rows[0][PASSWORD_CONTROL_POLICY.PwdCtrlGracePeriod.ToString()];
            PwdDefaultExprDate = pwdControlTable.Rows[0][PASSWORD_CONTROL_POLICY.PwdCtrlDefaultExprDate.ToString()] as string;
            PwdDefaultExprDays = (int)pwdControlTable.Rows[0][PASSWORD_CONTROL_POLICY.PwdCtrlDefaultExprDays.ToString()];
            PwdHistory = (int)pwdControlTable.Rows[0][PASSWORD_CONTROL_POLICY.PwdHistory.ToString()];
            PwdCanChangeWithin = (int)pwdControlTable.Rows[0][PASSWORD_CONTROL_POLICY.PwdCanChangeWithin.ToString()];
            PwdAuthFailsBeforeDelay = (int)pwdControlTable.Rows[0][PASSWORD_CONTROL_POLICY.PwdAuthFailsBeforeDelay.ToString()];
            PwdAuthFailDelayInSecs = (int)pwdControlTable.Rows[0][PASSWORD_CONTROL_POLICY.PwdAuthFailDelayInSecs.ToString()];
            ResetUpdateVectorSafeguardSysOptions();
            ResetUpdateVectorPasswordComplexity();
            ResetUpdateVectorDelayForLogonFailure();
            ResetUpdateVectorCurPwdReqdForResetPwd();

            // Compatibility policies
            AllowDownRevDrivers = (bool)compatibilityTable.Rows[0][COMPATIBILITY_POLICY.AllowPreR25Drivers.ToString()];
            OptimizeForLocalAccess = (bool)compatibilityTable.Rows[0][COMPATIBILITY_POLICY.OptimizeForLocalAccess.ToString()];
            ResetUpdateVectorMiscOptions();
        }

        #endregion Private methods
    }
}
