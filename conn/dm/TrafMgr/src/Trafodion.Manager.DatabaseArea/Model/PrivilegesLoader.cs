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
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// This class is used to consolidate all of the means of obtaining privileges.
    /// Privileges are broken into three seperate catagories:
    /// 1) Schema privileges: These are privileges that pertain to the schema
    ///    as well as all of the objects under the schema.
    /// 2) Schema Object privileges: These are a subset of schema priviliges
    ///    that only pertain to an individual object under a schema.
    /// 3) Column Privileges: These are privileges that pertain to a schema
    ///    object that actually contains colums such as atable or view.
    /// </summary>
    public class PrivilegesLoader
    {

        /// <summary>
        /// Loads the privileges for a schema.
        /// Combines the schema privileges for any user in the following manner:
        ///   + All privileges that share the same user, with grantable option, and
        ///     grantor values will be grouped together with the privileges
        ///     showing up as a comma seperated list.
        /// </summary>
        /// <param name="schema">The schema whose privileges are to be loaded.</param>
        /// <returns>The list of privileges for the schema.</returns>
        static public List<SchemaPrivilege> LoadSchemaPrivileges(TrafodionSchema schema)
        {
            Connection connection = null;

            List<SchemaPrivilege> privilegeList = new List<SchemaPrivilege>();

            try
            {
                connection = schema.GetConnection();
                OdbcDataReader dataReader = Queries.ExecuteSelectSchemaPrivileges(connection, schema);

                SchemaPrivilege priv = null;

                // The data will be sorted by grantee, grantable, and then grantor. Create a new privilege
                // whenever any of these values changes. Otherwise, modify the previous privilege.
                int previous_grantee = 0;
                bool previous_grantable = false;
                int previous_grantor = 0;

                while (dataReader.Read())
                {
                    int grantor = dataReader.GetInt32(0);
                    string grantorName = dataReader.GetString(1).Trim().Replace("\0", "");
                    string grantorType = dataReader.GetString(2).Trim();
                    int grantee = dataReader.GetInt32(3);
                    string granteeName = dataReader.GetString(4).Trim().Replace("\0", "");
                    string granteeType = dataReader.GetString(5).Trim();
                    string privType = dataReader.GetString(6).Trim();
                    bool grantable = (dataReader.GetString(7).Trim().Equals("Y", StringComparison.OrdinalIgnoreCase));

                    // Do we lump this into a single privilege?
                    if (priv != null && previous_grantee == grantee && previous_grantable == grantable && previous_grantor == grantor)
                    {
                        // Yes, reuse the previously created schema privilege.
                        priv.AddPrivilegesByTypeString(privType);
                    }
                    else
                    {
                        // No, create a new schema privilege.
                        priv = new SchemaPrivilege(privType, grantable, grantor, grantorName, grantorType, grantee, granteeName, granteeType);
                        privilegeList.Add(priv);

                        // Remember this privilege's settings.
                        previous_grantor = grantor;
                        previous_grantable = grantable;
                        previous_grantee = grantee;
                    }
                }
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }

            return privilegeList;
        }

        /// <summary>
        /// Loads the privileges for a schema object.
        /// Combines the schema privileges for any user in the following manner:
        ///   + All privileges that share the same user, with grantable option, and
        ///     grantor values will be grouped together with the privileges
        ///     showing up as a comma seperated list.
        /// </summary>
        /// <param name="schemaObject">The schema object whose privileges are to be loaded.</param>
        /// <returns>The list of privileges for the schema object.</returns>
        static public List<SchemaObjectPrivilege> LoadSchemaObjectPrivileges(TrafodionSchemaObject schemaObject)
        {
            Connection connection = null;

            List<SchemaObjectPrivilege> privilegeList = new List<SchemaObjectPrivilege>();

            try
            {
                connection = schemaObject.GetConnection();
                OdbcDataReader dataReader = Queries.ExecuteSelectSchemaObjectPrivileges(
                    connection, schemaObject.TheTrafodionSchema, schemaObject);

                SchemaObjectPrivilege priv = null;

                // The data will be sorted by grantee, grantable, and then grantor. Create a new privilege
                // whenever any of these values changes. Otherwise, modify the previous privilege.
                int previous_grantee = 0;
                bool previous_grantable = false;
                int previous_grantor = 0;

                while (dataReader.Read())
                {
                    int grantor = dataReader.GetInt32(0);
                    string grantorName = dataReader.GetString(1).Trim().Replace("\0", "");
                    string grantorType = dataReader.GetString(2).Trim();
                    int grantee = dataReader.GetInt32(3);
                    string granteeName = dataReader.GetString(4).Trim().Replace("\0", "");
                    string granteeType = dataReader.GetString(5).Trim();
                    string privType = dataReader.GetString(6).Trim();
                    bool grantable = (dataReader.GetString(7).Trim().Equals("Y", StringComparison.OrdinalIgnoreCase));

                    // Do we lump this into a single privilege?
                    if (priv != null && previous_grantee == grantee && previous_grantable == grantable && previous_grantor == grantor)
                    {
                        // Yes, reuse the previously created schema privilege.
                        priv.AddPrivilegesByTypeString(privType);
                    }
                    else
                    {
                        // No, create a new object privilege.
                        priv = new SchemaObjectPrivilege(privType, grantable, grantor, grantorName, grantorType, grantee, granteeName, granteeType);
                        privilegeList.Add(priv);

                        // Remember this privilege's settings.
                        previous_grantor = grantor;
                        previous_grantable = grantable;
                        previous_grantee = grantee;
                    }
                }
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }

            return privilegeList;
        }

        /// <summary>
        /// Loads the privileges for a table's columns.
        /// </summary>
        /// <param name="schemaObject">The schema object whose column's privileges are to be loaded.</param>
        /// <returns>The list of privileges for the table's columns</returns>
        static public List<ColumnPrivilege> LoadColumnPrivileges(TrafodionSchemaObject schemaObject)
        {
            Connection connection = null;

            List<ColumnPrivilege> privilegeList = new List<ColumnPrivilege>();

            try
            {
                connection = schemaObject.GetConnection();
                OdbcDataReader dataReader = Queries.ExecuteSelectColumnPrivileges(
                    connection, schemaObject.TheTrafodionSchema, schemaObject);

                ColumnPrivilege priv = null;

                // The data will be sorted by grantee, grantable, grantor and then column name. Create a new privilege
                // whenever any of these values changes. Otherwise, modify the previous privilege.
                int previous_grantee = 0;
                bool previous_grantable = false;
                int previous_grantor = 0;
                string previous_columnName = "";
                int previous_columnNumber = -1;

                while (dataReader.Read())
                {
                    string columnName = dataReader.GetString(0).TrimEnd();
                    TrafodionColumn.ColumnClass columnClass = TrafodionColumn.TranslateColumnClassString(dataReader.GetString(1).Trim());
                    int columnNumber = dataReader.GetInt32(2);
                    int grantor = dataReader.GetInt32(3);
                    string grantorName = dataReader.GetString(4).Trim().Replace("\0", "");
                    string grantorType = dataReader.GetString(5).Trim();
                    int grantee = dataReader.GetInt32(6);
                    string granteeName = dataReader.GetString(7).Trim().Replace("\0", "");
                    string granteeType = dataReader.GetString(8).Trim();
                    string privType = dataReader.GetString(9).Trim();
                    string withgrant = dataReader.GetString(10).Trim();
                    bool grantable = (withgrant.Equals("Y", StringComparison.OrdinalIgnoreCase));

                    // Do we lump this into a single privilege?
                    if (priv != null && previous_grantee == grantee && previous_grantable == grantable && previous_grantor == grantor && previous_columnName == columnName && previous_columnNumber == columnNumber)
                    {
                        // Yes, reuse the previously created schema privilege.
                        priv.AddPrivilegesByTypeString(privType);
                    }
                    else
                    {
                        // No, create a new object privilege.
                        priv = new ColumnPrivilege(columnName, columnClass, columnNumber, privType, grantable, grantor, grantorName, grantorType, grantee, granteeName, granteeType);
                        privilegeList.Add(priv);

                        // Remember this privilege's settings.
                        previous_grantor = grantor;
                        previous_grantable = grantable;
                        previous_grantee = grantee;
                        previous_columnName = columnName;
                        previous_columnNumber = columnNumber;
                    }
                }
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }

            return privilegeList;
        }

        /// <summary>
        /// Loads the components.
        /// </summary>
        /// <returns>The list of components</returns>
        static public List<ComponentModel> LoadComponents(TrafodionSystem sqlMxSystem)
        {
            Connection connection = null;

            List<ComponentModel> components = new List<ComponentModel>();

            try
            {
                connection = sqlMxSystem.GetConnection();
                OdbcDataReader dataReader = Queries.ExecuteSelectComponents(connection);

                while (dataReader.Read())
                {
                    long componentId = dataReader.GetInt64(0);
                    string componentName = dataReader.GetString(1).Trim();

                    ComponentModel compPrivilege = new ComponentModel(componentId, componentName);
                    components.Add(compPrivilege);
                }
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }

            return components;
        }

        /// <summary>
        /// Loads the component level privilege usages.
        /// </summary>
        /// <returns>The list of component level privilege usages</returns>
        static public List<ComponentPrivilegeUsage> LoadComponentPrivilegeUsages(TrafodionSystem sqlMxSystem)
        {
            Connection connection = null;

            List<ComponentPrivilegeUsage> privilegeUsages = new List<ComponentPrivilegeUsage>();

            try
            {
                connection = sqlMxSystem.GetConnection();
                OdbcDataReader dataReader = Queries.ExecuteSelectComponentPrivilegeUsages(connection);

                while (dataReader.Read())
                {
                    long componentId = dataReader.GetInt64(0);
                    string privType = dataReader.GetString(1).Trim();
                    string privName = dataReader.GetString(2).Trim();
                    string privDescription = dataReader.GetString(3).Trim();

                    ComponentPrivilegeUsage compPrivilege = new ComponentPrivilegeUsage(componentId, privType, privName, privDescription);
                    privilegeUsages.Add(compPrivilege);
                }
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }

            return privilegeUsages;
        }


        /// <summary>
        /// Loads the component level privileges.
        /// </summary>
        /// <returns>The list of component level privileges</returns>
        static public List<ComponentPrivilege> LoadComponentPrivileges(TrafodionSystem sqlMxSystem)
        {
            Connection connection = null;

            List<ComponentPrivilege> privileges = new List<ComponentPrivilege>();

            try
            {
                connection = sqlMxSystem.GetConnection();
                OdbcDataReader dataReader = Queries.ExecuteSelectComponentPrivileges(connection);

                while (dataReader.Read())
                {
                    long componentId = dataReader.GetInt64(0);
                    int grantor = dataReader.GetInt32(1);
                    string grantorName = dataReader.GetString(2).Trim().Replace("\0","");
                    int grantee = dataReader.GetInt32(3);
                    string granteeName = dataReader.GetString(4).Trim().Replace("\0","");
                    string privType = dataReader.GetString(5).Trim();
                    string withgrant = dataReader.GetString(6).Trim();
                    string granteeType = dataReader.GetString(7).Trim();
                    string grantorType = dataReader.GetString(8).Trim();
                    bool grantable = (withgrant.Equals("Y", StringComparison.OrdinalIgnoreCase));

                    ComponentPrivilege compPrivilege = new ComponentPrivilege(componentId, privType, grantable, grantor, grantorName, grantorType, grantee, granteeName, granteeType);
                    privileges.Add(compPrivilege);
                }
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }

            return privileges;
        }
    }
}
