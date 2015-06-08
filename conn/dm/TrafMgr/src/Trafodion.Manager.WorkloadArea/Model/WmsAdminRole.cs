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
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.WorkloadArea.Model
{
    /// <summary>
    /// Model for a WMS Admin Role
    /// </summary>
    public class WmsAdminRole : WmsObject
    {
        #region Private member variables

        private WmsSystem _wmsSystem;
        private string _name;

        #endregion Private Fields

        #region Public properties

        /// <summary>
        /// The Create wms command to create this wms service
        /// </summary>
        public override string CreateCommandString
        {
            get
            {
                return String.Format("ADD ADMIN {0};", FormattedName);
            }
        }

        /// <summary>
        /// The Alter wms command to alter this wms service
        /// </summary>
        public override string AlterCommandString
        {
            get
            {
                return "";
            }
        }

        public override string DDLText
        {
            get { return CreateCommandString; }
        }
        /// <summary>
        /// System in which the service is defined
        /// </summary>
        public WmsSystem WmsSystem
        {
            get { return _wmsSystem; }
        }

        /// <summary>
        /// Name of the service
        /// </summary>
        override public string Name
        {
            get { return _name; }
            set { _name = value; }
        }  
        
        /// <summary>
        /// Formatted name of the service
        /// </summary>
        public string FormattedName
        {
            //get { return NCCWMSCommand.escapeServiceName(Name); }            
            get { return Name; }
        }

        /// <summary>
        /// Identifies if this admin role is system created
        /// </summary>
        public bool isSystemAdminRole
        {
            get
            {
                if (Name.Equals("ROLE.DBA") || Name.Equals("SUPER.SERVICES") || Name.Equals("SUPER.SUPER"))
                    return true;
                else
                    return false;
            }

        }       
        #endregion Public properties

        /// <summary>
        /// Constructs a new WmsAdminRole instance
        /// </summary>
        /// <param name="wmsSystem"></param>
        public WmsAdminRole(WmsSystem wmsSystem)
            :base(wmsSystem.ConnectionDefinition)
        {
            _wmsSystem = wmsSystem;
        }

        /// <summary>
        /// Read and Set admin role attributes from the OdbcDataReader
        /// </summary>
        /// <param name="dataReader"></param>
        internal void SetAttributes(DataRow dataRow)
        {
            _name = dataRow[WmsCommand.COL_ADMIN_ROLE_NAME] as string;
        }
      
        /// <summary>
        /// Refreshes the admin role attributes
        /// </summary>
        override public void Refresh()
        {
            //try
            //{
            //    DataTable dataTable = WmsCommand.executeCommand("STATUS ADMIN " + FormattedName);

            //    //Set the attributes
            //    if (dataTable.Rows.Count > 0)
            //    {
            //        SetAttributes(dataTable.Rows[0]);
            //        this.OnWmsModelEvent(WmsCommand.WMS_ACTION.STATUS_ADMIN, this);
            //    }
            //    else
            //    {
            //        //No records returned. Maybe the admin role has been removed
            //        WmsSystem.WmsAdminRoles.Remove(this);
            //        this.OnWmsModelEvent(WmsCommand.WMS_ACTION.DELETE_ADMIN, this);
            //    }

            //    WmsSystem.WmsAdminRoles.Sort();
            //}
            //catch (OdbcException)
            //{
            //    //Failed to fetch. Maybe the admin role has been removed
            //    WmsSystem.WmsAdminRoles.Remove(this);
            //    this.OnWmsModelEvent(WmsCommand.WMS_ACTION.DELETE_ADMIN, this);
            //}
        }

        /// <summary>
        /// Adds the admin role to the WMS system configuration
        /// </summary>
        public void Add()
        {
            //WmsCommand.executeNonQuery(CreateCommandString);

            ////If add is successful, add the admin role to the WmsSystem's admin role list.
            //_wmsSystem.AddAdminRoleToList(this);
        }

        /// <summary>
        /// Deletes the admin role
        /// </summary>
        public void Delete()
        {
            //WmsCommand.executeNonQuery("DELETE ADMIN " + FormattedName);
            //_wmsSystem.WmsAdminRoles.Remove(this);
            //OnWmsModelEvent(WmsCommand.WMS_ACTION.DELETE_ADMIN, this);
        }
        /// <summary>
        /// Returns the admin role name
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
 	         return Name;
        }
    }
}
