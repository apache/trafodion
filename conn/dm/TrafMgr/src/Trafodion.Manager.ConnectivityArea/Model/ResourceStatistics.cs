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

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// The resource statictics for a data Source
    /// </summary>
    public class ResourceStatistics
    {
        #region Member varaiables
        // Resource Statistics Flags.
        public static int CONNECT_INFO_FLAG = 1;
        public static int SESSION_SUMMARY_FLAG = 2;
        public static int SQL_STMT_FLAG = 256;
        public static int SQL_PREPARE_FLAG = 512;
        public static int SQL_EXEC_FLAG = 1024;
        public static int SQL_EXECDIR_FLAG = 2048;
        public static int SQL_FETCH_FLAG = 4096;
        public static int ALL_STATS_FLAG = (
                                      CONNECT_INFO_FLAG
                                    + SESSION_SUMMARY_FLAG
                                    + SQL_STMT_FLAG
                                    + SQL_PREPARE_FLAG
                                    + SQL_EXEC_FLAG
                                    + SQL_EXECDIR_FLAG
                                    + SQL_FETCH_FLAG
                                    );
    
        // Default value
        public static int EMPTY_FLAG = 0;
    
        public bool connectInfo = false, sessionSummary = false,
        sqlStatement = false, sqlPrepare = false, sqlExecute = false,
        sqlExecuteDirect = false, sqlFetch = false;
        #endregion Member varaiables

        #region Properties
        /// <summary>
        /// Instructs the DS to record ConnectInfo related stats
        /// </summary>
        public bool ConnectInfo
        {
            get { return connectInfo; }
            set { connectInfo = value; }
        }

        /// <summary>
        /// Instructs the DS to record Session Summary related stats
        /// </summary>
        public bool SessionSummary
        {
            get { return sessionSummary; }
            set { sessionSummary = value; }
        }

        /// <summary>
        /// Instructs the DS to record SQL Statement related stats
        /// </summary>
        public bool SqlStatement
        {
            get { return sqlStatement; }
            set { sqlStatement = value; }
        }

        /// <summary>
        /// Instructs the DS to record SQL Prepare statement related stats
        /// </summary>
        public bool SqlPrepare
        {
            get { return sqlPrepare; }
            set { sqlPrepare = value; }
        }

        /// <summary>
        /// Instructs the DS to record SQL execute related stats
        /// </summary>
        public bool SqlExecute
        {
            get { return sqlExecute; }
            set { sqlExecute = value; }
        }
        /// <summary>
        /// Instructs the DS to record SQL execute direct related stats
        /// </summary>
        public bool SqlExecuteDirect
        {
            get { return sqlExecuteDirect; }
            set { sqlExecuteDirect = value; }
        }

        /// <summary>
        /// Instructs the DS to record SQL fetch related stats
        /// </summary>
        public bool SqlFetch
        {
            get { return sqlFetch; }
            set { sqlFetch = value; }
        }
        #endregion Properties

        #region Constructors
        public ResourceStatistics()
        {
        }
        
        public ResourceStatistics(int value)
        {
            connectInfo = (value & CONNECT_INFO_FLAG) == CONNECT_INFO_FLAG;
            sessionSummary = (value & SESSION_SUMMARY_FLAG) == SESSION_SUMMARY_FLAG;
            sqlStatement = (value & SQL_STMT_FLAG) == SQL_STMT_FLAG;
            sqlPrepare = (value & SQL_PREPARE_FLAG) == SQL_PREPARE_FLAG;
            sqlExecute = (value & SQL_EXEC_FLAG) == SQL_EXEC_FLAG;
            sqlExecuteDirect = (value & SQL_EXECDIR_FLAG) == SQL_EXECDIR_FLAG;
            sqlFetch = (value & SQL_FETCH_FLAG) == SQL_FETCH_FLAG;
        }
        #endregion Constructors

        #region Public Methods
        /// <summary>
        /// Returns the int value for the Resource statistics
        /// </summary>
        /// <returns></returns>
        public int GetIntValue()
        {
            int value = EMPTY_FLAG;
            if (connectInfo) value += CONNECT_INFO_FLAG;
            if (sessionSummary) value += SESSION_SUMMARY_FLAG;
            if (sqlStatement) value += SQL_STMT_FLAG;
            if (sqlPrepare) value += SQL_PREPARE_FLAG;
            if (sqlExecute) value += SQL_EXEC_FLAG;
            if (sqlExecuteDirect) value += SQL_EXECDIR_FLAG;
            if (sqlFetch) value += SQL_FETCH_FLAG;
            
            return value;
        }

        /// <summary>
        /// Copies the attributes of another ResourceStatistics.
        /// </summary>
        /// <param name="stats">The ResourceStatistics object whose attributes will be copied</param>
        public void Copy(ResourceStatistics stats)
        {
            connectInfo = stats.connectInfo;
            sessionSummary = stats.sessionSummary;
            sqlStatement = stats.sqlStatement;
            sqlPrepare = stats.sqlPrepare;
            sqlExecute = stats.sqlExecute;
            sqlExecuteDirect = stats.sqlExecuteDirect;
            sqlFetch = stats.sqlFetch;
        }

        /// <summary>
        /// Overrides the default Equals method
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public override bool Equals(object obj)
        {
            ResourceStatistics stats = obj as ResourceStatistics;
            if (stats != null)
            {
                if (this.GetIntValue() == stats.GetIntValue())
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Generates a string representation of this object.
        /// </summary>
        /// <returns></returns>
        public override String ToString()
        {
            return GetFlagStatus("SQLPREPARESTAT", sqlPrepare)
                + " ," + GetFlagStatus("SQLEXECUTESTAT", sqlExecute)
            +" ," + GetFlagStatus("SQLEXECDIRECTSTAT", sqlExecuteDirect)
            +" ," + GetFlagStatus("SQLFETCHSTAT", sqlFetch)
            + " ," + GetFlagStatus("SQLSTMTSTAT", sqlStatement)
            + " ," + GetFlagStatus("CONNINFOSTAT", connectInfo)
            + " ," + GetFlagStatus("SESSIONINFOSTAT", sessionSummary);
        }
        #endregion Public Methods

        #region private methods
        private string GetFlagStatus(string aName, bool value)
        {
            string ret = "{0} {1}";
            ret = String.Format(ret, aName, (value) ? "ON" : "OFF");
            return ret;
        }
        #endregion private methods
    }
}
