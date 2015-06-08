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

using System.Collections.Generic;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// This class is used to assist with the function of loading multiple objects when
    /// issuing a query. The parent object (of type ParentTypes) represents the object
    /// that is loading objects of type SubType. A TrafodionObjectsLoader has a one-to-one
    /// relationship with a query.
    /// </summary>
    /// <typeparam name="ParentType"></typeparam>
    /// <typeparam name="SubType"></typeparam>
    abstract class TrafodionObjectsLoader<ParentType, SubType>
        where ParentType : TrafodionObject
        where SubType : TrafodionObject
    {
        /// <summary>
        /// Loads a list of SubType objects into a List.
        /// </summary>
        /// <param name="aParentObject">The object whose subobjects are being loaded.</param>
        /// <returns>A List of SubType objects.</returns>
        public List<SubType> Load(ParentType aParentObject)
        {
            Connection connection = null;

            List<SubType> subTypeList = new List<SubType>();

            try
            {
                connection = aParentObject.GetConnection();
                OdbcDataReader dataReader = GetQueryReader(connection, aParentObject);
                while (dataReader.Read())
                {
                    LoadOne(subTypeList, aParentObject, dataReader);
                }

            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }
            return subTypeList;
        }

        /// <summary>
        /// Loads a object given its name
        /// </summary>
        /// <param name="aParentObject">Parent object type</param>
        /// <param name="sqlObjectName">child object name</param>
        /// <returns></returns>
        public SubType LoadObjectByName(ParentType aParentObject, string sqlObjectName)
        {
            Connection connection = null;

            SubType sqlObject = null;

            try
            {
                connection = aParentObject.GetConnection();
                OdbcDataReader dataReader = GetReaderForSqlObject(connection, aParentObject, sqlObjectName);
                if (dataReader != null)
                {
                    while (dataReader.Read())
                    {
                        sqlObject = ReadSqlObject(aParentObject, dataReader);
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
            return sqlObject;
        }
        /// <summary>
        /// Returns an OdbcDataReader that already has had a query executed.
        /// </summary>
        /// <param name="aConnection">The connection that will be used to execute the query.</param>
        /// <param name="aParentObject">The parenting object for whom the queries will be executed.</param>
        /// <returns>An OdbcDataReader that has had a query executed, but no data read.</returns>
        abstract protected OdbcDataReader GetQueryReader(Connection aConnection, ParentType aParentObject);
        virtual protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, ParentType aParentObject, string sqlObjectName)
        {
            return null;
        }

        /// <summary>
        /// Reads the data from an OdbcDataReader to create SubType objects that will be added to a list.
        /// The reader will already have had its Read() function already called.
        /// </summary>
        /// <param name="aList">The List that will be populated.</param>
        /// <param name="aParentObject">The parenting object for whom the SubType objects belong.</param>
        /// <param name="aReader">
        /// The read that already had a query executed and data read via the Read() function.
        /// </param>
        abstract protected void LoadOne(List<SubType> aList, ParentType aParentObject, OdbcDataReader aReader);

        /// <summary>
        /// Reads data from the OdbcDataReader to create the SubType object
        /// </summary>
        /// <param name="aParentObject"></param>
        /// <param name="aReader"></param>
        /// <returns></returns>
        virtual protected SubType ReadSqlObject(ParentType aParentObject, OdbcDataReader aReader)
        {
            return null;
        }
    }
}
