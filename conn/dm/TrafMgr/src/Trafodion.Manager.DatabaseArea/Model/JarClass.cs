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
using System.Data;
using System.Data.Odbc;
using System.IO;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// JarClass model for class node in a library.
    /// </summary>
    public class JarClass : TrafodionObject,IHasTrafodionCatalog,IHasTrafodionSchema
    {
        #region private fields
        private string _ClassName;
        private TrafodionLibrary _sqlMxLibrary=null;
        private DataTable _methodList = null;
        #endregion

        #region Public Properties

        public TrafodionLibrary TheTrafodionLibrary
        {
            get { return _sqlMxLibrary; }
        }

        public string ClassName
        {
            get { return _ClassName; }
            set { _ClassName = value; }
        }
        
        public DataTable MethodList
        {
            get
            {
                return _methodList;
            }
        }
        public override ConnectionDefinition ConnectionDefinition
        {
            get { return _sqlMxLibrary.ConnectionDefinition; }
        }

        public TrafodionCatalog TheTrafodionCatalog
        {
            get
            {
                return _sqlMxLibrary.TheTrafodionCatalog;
            }
        }

        public TrafodionSchema TheTrafodionSchema
        {
            get
            {
                return _sqlMxLibrary.TheTrafodionSchema;
            }
        }

        #endregion

        #region Public Methods

        public override Connection GetConnection()
        {
            return _sqlMxLibrary.GetConnection();
        }

        /// <summary>
        /// Gets the method details for the given class 
        /// </summary>
        /// <param name="aClassName">Class Name</param>
        /// <returns></returns>
        public DataTable getMethods(string aClassName, DataSet datasetXSD)
        {
            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();
                string cmdGetMethods = String.Format("CALL MANAGEABILITY.NCI.MethodsInClass('{0}','{1}', ?, ?);", this._sqlMxLibrary.CodeFileName, aClassName);
                OdbcCommand theQuery = new OdbcCommand(cmdGetMethods, theConnection.OpenOdbcConnection);
                theQuery.Parameters.Clear(); //since command cobject is preserved, clear the parameters every time

                OdbcParameter param0 = theQuery.Parameters.Add("@METHODS", OdbcType.Text, 10240);
                param0.Direction = System.Data.ParameterDirection.Output;
                param0.DbType = System.Data.DbType.String;

                OdbcParameter param1 = theQuery.Parameters.Add("@TEMPFILENAME", OdbcType.Text, 256);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;

                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TrafodionLibrary.TRACE_SUB_AREA_NAME, true);

                string methods = theQuery.Parameters["@METHODS"].Value.ToString();
                if ((methods != null) && (methods.Trim().Length > 0))
                {
                    //we got the class names
                }
                else
                {
                    string temporaryFileName = theQuery.Parameters["@TEMPFILENAME"].Value.ToString();
                    if ((temporaryFileName != null) && (temporaryFileName.Trim().Length > 0))
                    {
                        //read the temp file and get the XML
                        methods = getTemporaryFileContent(temporaryFileName);
                    }
                }
                datasetXSD.Tables["Method"].Clear();                
                StringReader sr = new StringReader(methods);
                datasetXSD.ReadXml(sr);
                _methodList = new DataTable();
                _methodList = datasetXSD.Tables["Method"].Copy();
                DataView sortedView = _methodList.DefaultView;
                sortedView.Sort = _methodList.Columns[0].ColumnName + " asc";
                _methodList = sortedView.ToTable();
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            return _methodList;
        }
        #endregion

        #region Private methods
        /// <summary>
        /// Reads content from temporary file
        /// </summary>
        /// <param name="aTemporaryFile"></param>
        /// <returns></returns>
        private string getTemporaryFileContent(string aTemporaryFile)
        {
            Connection theConnection = null;

            int offset = 0;
            Int32 cnt = 0;
            byte[] data = new byte[10240];
            StringWriter sw = new StringWriter();
            string filename = aTemporaryFile;
            try
            {
                theConnection = GetConnection();
                int count = 0;

                do
                {

                    //The count is the actual count rather than the length of the content
                    string cmdQuery = String.Format(
                                        "CALL MANAGEABILITY.NCI.GETTEMPORARYFILE('{0}', {1}, ?, ?);",
                                        new object[] { filename, offset });
                    OdbcCommand theQuery = new OdbcCommand(cmdQuery, theConnection.OpenOdbcConnection);
                    theQuery.CommandType = System.Data.CommandType.StoredProcedure;
                    theQuery.Parameters.Clear(); //since command cobject is preserved, clear the parameters every time

                    OdbcParameter param1 = theQuery.Parameters.Add("@FILEDATA", OdbcType.Text, 10240);
                    param1.Direction = System.Data.ParameterDirection.Output;
                    param1.DbType = System.Data.DbType.String;


                    OdbcParameter param2 = theQuery.Parameters.Add("@DATALENGTH", OdbcType.Int);
                    param2.Direction = System.Data.ParameterDirection.Output;
                    param2.Value = cnt;

                    //Execute the SPJ
                    Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TrafodionLibrary.TRACE_SUB_AREA_NAME, true);

                    //Get the data
                    count = (Int32)theQuery.Parameters[1].Value;

                    string content = (count > 0) ? (string)theQuery.Parameters[0].Value : "";
                    offset += count;

                    //Add it to the stream if something is read
                    if (count > 0)
                    {
                        sw.Write(content);
                    }

                } while (count > 0);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            return sw.ToString();
        }

        #endregion

        #region Constructors

        //TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
        public JarClass(string className, TrafodionLibrary aTrafodionLibrary)
            : base(className, aTrafodionLibrary.UID)
        {
            _ClassName = className;
            _sqlMxLibrary = aTrafodionLibrary;
        }

        #endregion Constructors


    }
}
