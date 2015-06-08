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
using System.Collections;
using System.Collections.Generic;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.SecurityArea.Model
{
    public class SystemSecurity : SecurityObject
    {
        private static Dictionary<ConnectionDefinition, SystemSecurity> _activeSystemSecurity = new Dictionary<ConnectionDefinition, SystemSecurity>(new MyConnectionDefinitionComparer());
        private bool _isSecPrivilegesLoaded = false;
        private bool _isPasswordChangeableRolesLoaded = false;
        private Policies _theFactoryDefaultPolicies = null;
        private Policies _theMostSecurePolicies = null;
        private Policies _thePolicies = null;

        public bool IsViewOnly
        {
            get
            {
                return currentConfigPrivileges.Contains(VIEW_CONFIG);
            }
        }

        public bool RolePasswordChangeOnly
        {
            get
            {
                return currentConfigPrivileges.Contains(CHANGE_ROLE_PASSWORD);
            }
        }

        public bool IsPasswordChangeableRolesLoaded
        {
            get { return _isPasswordChangeableRolesLoaded; }
        }

        public bool IsSecPrivilegesLoaded
        {
            get { return _isSecPrivilegesLoaded; }
        }

        public Policies FactoryDefaultPolicies
        {
            get { return _theFactoryDefaultPolicies; }
            set { _theFactoryDefaultPolicies = value; }
        }

        public Policies MostSecurePolicies
        {
            get { return _theMostSecurePolicies; }
            set { _theMostSecurePolicies = value; }
        }

        public Policies Policies
        {
            get 
            {
                if (null == _thePolicies)
                {
                    _thePolicies = new Policies(ConnectionDefinition, Policies.POLICY_SETTING.SETTING_CUSTOMER);
                }
                return _thePolicies; 
            }
            set { _thePolicies = value; }
        }

        private ArrayList currentConfigPrivileges = new ArrayList();
        private ArrayList passwordChangeableRoles = new ArrayList();

        public const string CERTIFICATE_MGMT = "CERTIFICATE_MGMT" ;
        public const string POLICY_MGMT = "POLICY_MGMT" ;
        public const string ROLE_MGMT = "ROLE_MGMT" ;
        public const string DATABASE_USER_MGMT = "DATABASE_USER_MGMT" ;
        public const string PLATFORM_USER_MGMT = "PLATFORM_USER_MGMT" ;
        public const string DIRECTORY_SERVER_MGMT = "DIRECTORY_SERVER_MGMT";
        public const string CHANGE_ROLE_PASSWORD = "CHANGE_ROLE_PASSWORD";
        public const string VIEW_CONFIG = "VIEW_CONFIG";

        public event EventHandler PrivilegesLoadedEvent;

        private SystemSecurity(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
            //Add this instance to the static dictionary, so in future if need a reference to this system, 
            //we can look up using the connection definition
            if (!_activeSystemSecurity.ContainsKey(aConnectionDefinition))
                _activeSystemSecurity.Add(aConnectionDefinition, this);

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
                _activeSystemSecurity.Remove(aConnectionDefinition);
            }
        }

        public bool IsConfigFunctionAllowed(string functionName)
        {
            if (!string.IsNullOrEmpty(functionName))
            {
                functionName = functionName.Trim();
            }
            return currentConfigPrivileges.Contains(functionName);
        }

        public bool IsChangePasswordAllowed(string roleName)
        {
            if (!string.IsNullOrEmpty(roleName))
            {
                roleName = roleName.Trim();
            }
            return passwordChangeableRoles.Contains(roleName);
        }

        public static SystemSecurity FindSystemModel(ConnectionDefinition connectionDefinition)
        {
            SystemSecurity systemSecurity = null;
            _activeSystemSecurity.TryGetValue(connectionDefinition, out systemSecurity);
            if (systemSecurity == null)
            {
                systemSecurity = new SystemSecurity(connectionDefinition);
            }
            return systemSecurity;
        }

        /// <summary>
        /// Raise the model removed event
        /// </summary>
        protected virtual void OnPrivilegesLoadedEvent()
        {
            EventHandler handler = PrivilegesLoadedEvent;
            if (handler != null)
            {
                handler(this, new EventArgs());
            }
        }

        public void LoadSecConfigPrivileges()
        {
            if (!GetConnection())
                return;

            currentConfigPrivileges.Clear();
            try
            {
                OdbcDataReader dataReader = Queries.ExecuteSecConfigFunctionsForRole(CurrentConnection);
                while (dataReader.Read())
                {
                    currentConfigPrivileges.Add(dataReader.GetString(0).Trim());
                }
            }
            finally
            {
                CloseConnection();
            }
            _isSecPrivilegesLoaded = true;
        }

        public void FirePrivilegesLoadedEvent()
        {
            OnPrivilegesLoadedEvent();
        }

        public List<string> LoadRoleNames()
        {
            List<string> roleNames = new List<string>();

            if (!GetConnection())
                return roleNames;

            try
            {
                OdbcDataReader dataReader = Queries.ExecuteSelectRoleNames(CurrentConnection);
                while (dataReader.Read())
                {
                    roleNames.Add(dataReader.GetString(0).Trim());
                }
            }
            finally
            {
                CloseConnection();
            }
            return roleNames;
        }

        public void LoadPasswordChangeableRoles()
        {
            if (!GetConnection())
                return;

            passwordChangeableRoles.Clear();
            try
            {
                OdbcDataReader dataReader = Queries.ExecuteSecCanChangeTheseRolesPW(CurrentConnection);
                while (dataReader.Read())
                {
                    passwordChangeableRoles.Add(dataReader.GetString(0).Trim());
                }
            }
            finally
            {
                CloseConnection();
            }
            _isPasswordChangeableRolesLoaded = true;
        }

        public void SetDefaultPrivileges()
        {
             currentConfigPrivileges.Clear();
             _isSecPrivilegesLoaded = true;
             currentConfigPrivileges.Add(CERTIFICATE_MGMT);
             currentConfigPrivileges.Add(DIRECTORY_SERVER_MGMT);
             currentConfigPrivileges.Add(POLICY_MGMT);
             currentConfigPrivileges.Add(ROLE_MGMT);
             currentConfigPrivileges.Add(DATABASE_USER_MGMT);
             currentConfigPrivileges.Add(PLATFORM_USER_MGMT);
             OnPrivilegesLoadedEvent();
        }

    }
}
