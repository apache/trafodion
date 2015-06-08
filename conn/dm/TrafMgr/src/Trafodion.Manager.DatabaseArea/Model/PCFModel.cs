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
using System.Data;
using System.Data.Odbc;
using System.IO;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// The model that performs PCF operations on behalf of the PCF tool
    /// </summary>
    public class PCFModel
    {
        #region Member Variables

        public enum AccessLevel : short { Role = 0, User = 1 };

        private DataSet _dataSet = new DataSet();
        private ConnectionDefinition _theConnectionDefinition = null;
        private Connection _theCurrentConnection = null;
        private OdbcCommand theQuery = new OdbcCommand();
        private const string TRACE_SUB_AREA_NAME = "PCF Model";

        #endregion

        #region Public Properties

        /// <summary>
        /// The ODBC command object used by the model
        /// </summary>
        public OdbcCommand PCFCommandObject
        {
            get { return theQuery; }
        }

        public string SPJVersion
        {
            get
            {
                TrafodionSystem sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_theConnectionDefinition);
                return sqlMxSystem.SPJVersion;
            }
        }

        #endregion Public Properties

        #region Constructors

        /// <summary>
        /// Constructs a PCF model for the given connection definition
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public PCFModel(ConnectionDefinition aConnectionDefinition)
        {
            _theConnectionDefinition = aConnectionDefinition;

            GetConnection();

            //Reads the XML schema that will be used the parse the PCF xml string received from the backend
            try
            {
                _dataSet.ReadXmlSchema(Path.Combine(Application.StartupPath, "TrafodionSPJ.XSD"));
            }
            catch (Exception ex)
            {
                throw new Exception("Error loading TrafodionSPJ.XSD from " + Application.StartupPath + " Error :" + ex.Message);
            }
        }

        #endregion

        #region Public methods

        /// <summary>
        /// Helper method to see if SPJ management infrastructure is available on the target system
        /// If the call fails an odbc exception will be thrown which is an indication that the 
        /// SPJ management infrastructure does not exist
        /// </summary>
        /// <returns></returns>
        public string ping()
        {
            try
            {
                if (!GetConnection())
                    return null;

                theQuery.CommandText = "CALL MANAGEABILITY.NCI.PING(?);";
                theQuery.CommandType = System.Data.CommandType.StoredProcedure;

                theQuery.Parameters.Clear(); //since command cobject is preserved, clear the parameters every time
                OdbcParameter param1 = theQuery.Parameters.Add("@PINGRESULT", OdbcType.Text, 10240);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;

                theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

                return (string)theQuery.Parameters[0].Value;
            }
            finally
            {
                close();
            }

        }

        /// <summary>
        /// Renames a code file
        /// </summary>
        /// <param name="oldName">Current file name</param>
        /// <param name="newName">New file Name</param>
        public void rename(PCFModel.AccessLevel accessLevel, String oldName, String newName)
        {
            if (!GetConnection())
                return;

            try
            {
                theQuery.CommandText = String.Format(
                    "CALL MANAGEABILITY.NCI.MV('{0}', '{1}');",
                    new object[] { oldName, newName });

                theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

            }
            finally
            {
                close();
            }
        }

        /// <summary>
        /// Writes the content to the file specified on the server
        /// </summary>
        /// <param name="file"></param>
        /// <param name="content"></param>
        /// <param name="count"></param>
        /// <param name="mode"></param>
        public void write(PCFModel.AccessLevel accessLevel, String file, char[] content, Int32 count, Int32 mode)
        {

            if (!GetConnection())
                return;

            try
            {
                theQuery.CommandText = String.Format(
                    "CALL MANAGEABILITY.NCI.PUT(?, '{0}', {1});",
                    new object[] { file, mode }
                );

                theQuery.Parameters.Clear(); //since command cobject is preserved, clear the parameters every time
                OdbcParameter param = theQuery.Parameters.Add("@FILEDATA", OdbcType.VarChar, 10240);
                param.Direction = System.Data.ParameterDirection.Input;
                param.Value = new string(content);

                theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

            }
            finally
            {
                close();
            }
        }

        /// <summary>
        /// Gets the content from the file specified from the server
        /// </summary>
        /// <param name="file"></param>
        /// <param name="offset"></param>
        /// <param name="content"></param>
        /// <param name="count"></param>
        public void get(PCFModel.AccessLevel accessLevel, string file, int offset, out string content, out Int32 count)
        {
            if (!GetConnection())
            {
                count = 0;
                content = "";
                return;
            }

            string text = "";
            Int32 cnt = 0;

            try
            {
                string filename = file.Trim();
                theQuery.CommandText = String.Format(
                    "CALL MANAGEABILITY.NCI.GETFILE('{0}', {1}, ?, ?);",
                    new object[] { filename, offset });

                theQuery.CommandType = System.Data.CommandType.StoredProcedure;

                theQuery.Parameters.Clear(); //since command cobject is preserved, clear the parameters every time
                OdbcParameter param1 = theQuery.Parameters.Add("@FILEDATA", OdbcType.Text, 10240);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;
 
                OdbcParameter param2 = theQuery.Parameters.Add("@DATALENGTH", OdbcType.Int);
                param2.Direction = System.Data.ParameterDirection.Output;
                param2.Value = cnt;

                theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

                count = (Int32)theQuery.Parameters[1].Value;
                content = (count > 0) ? (string)theQuery.Parameters[0].Value : "";
            }
            catch (Exception e)
            {
                content = text;
                count = cnt;
                throw e;
            }
            finally
            {
                close();
            }
        }

        /// <summary>
        /// Closes the open ODBC connection
        /// </summary>
        public void close()
        {

            // Always close the connection if there is one
            if (_theCurrentConnection != null)
            {
                try
                {
                    _theCurrentConnection.Close();
                }
                finally
                {
                    //Temporary workaround to gurantee that a new connection is opened after a communication failure
                    //long term fix is to handle it in the base Connection wrapper in the framework
                    _theCurrentConnection = null; 
                }
            }
        }

        /// <summary>
        /// Deletes the file specified
        /// </summary>
        /// <param name="file"></param>
        public void delete(PCFModel.AccessLevel accessLevel, String file)
        {
            if (!GetConnection())
                return;

            try
            {
                theQuery.CommandText = String.Format(
                "CALL MANAGEABILITY.NCI.RM('{0}');",
                new object[] { file });

                theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);
            }
            finally
            {
                close();
            }
        }

        /// <summary>
        /// Obtains a datatable that has the file information 
        /// </summary>
        /// <returns></returns>
        public DataTable GetFiles(PCFModel.AccessLevel accessLevel)
        {
            return GetFiles(accessLevel, null);
        }

        /// <summary>
        /// Obtains a datatable that has the file information 
        /// </summary>
        /// <returns></returns>
        public DataTable GetFiles(PCFModel.AccessLevel accessLevel, String aFileName)
        {
            DataTable dataTable = new DataTable();

            if (_dataSet.Tables["File"] != null)
            {
                _dataSet.Tables["File"].Clear();
                String fileNames = list(accessLevel, aFileName);
                StringReader sr = new StringReader(fileNames);
                _dataSet.ReadXml(sr);
                dataTable = _dataSet.Tables["File"].Copy();
            }
            return dataTable;
        }

        /// <summary>
        /// Get the class names in a given code file
        /// </summary>
        /// <param name="aJarFile"></param>
        /// <returns></returns>
        public List<string> getClasses(PCFModel.AccessLevel accessLevel, String aJarFile)
        {
            List<string> classesInJar = new List<string>();

            if (!GetConnection())
                return classesInJar;

            try
            {
                theQuery.CommandText = String.Format("CALL MANAGEABILITY.NCI.ClassesInJar('{0}', ?, ?);", aJarFile);
                theQuery.Parameters.Clear(); //since command cobject is preserved, clear the parameters every time

                OdbcParameter param0 = theQuery.Parameters.Add("@CLASSNAMES", OdbcType.Text, 10240);
                param0.Direction = System.Data.ParameterDirection.Output;
                param0.DbType = System.Data.DbType.String;
                //param0.OdbcType = OdbcType.VarChar;

                OdbcParameter param1 = theQuery.Parameters.Add("@TEMPFILENAME", OdbcType.Text, 256);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;
                //param1.OdbcType = OdbcType.VarChar;

                theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

                string classNames = theQuery.Parameters["@CLASSNAMES"].Value.ToString();
                if ((classNames != null) && (classNames.Trim().Length > 0))
                {
                    //we got the class names
                }
                else
                {
                    string temporaryFileName = theQuery.Parameters["@TEMPFILENAME"].Value.ToString();
                    if ((temporaryFileName != null) && (temporaryFileName.Trim().Length > 0))
                    {
                        //read the temp file and get the XML
                        classNames = getTemporaryFileContent(temporaryFileName);
                    }
                }

                _dataSet.Tables["Class"].Clear();
                StringReader sr = new StringReader(classNames);
                _dataSet.ReadXml(sr);

                DataTable table = _dataSet.Tables["Class"].Copy();
                DataView sortedView = table.DefaultView;
                sortedView.Sort = table.Columns[0].ColumnName + " asc";
                table = sortedView.ToTable();                 

                for (int i = 0; i < table.Rows.Count; i++)
                {
                    classesInJar.Add(((string)table.Rows[i][0]).Trim());
                }
            }
            finally
            {
                close();
            } 
            return classesInJar;
        }

        /// <summary>
        /// Gets the method details for the given class 
        /// </summary>
        /// <param name="aJarFile">Code File Name</param>
        /// <param name="aClassName">Class Name</param>
        /// <returns></returns>
        public DataTable getMethods(PCFModel.AccessLevel accessLevel, String aJarFile, string aClassName)
        {

            DataTable table = null;
            if (!GetConnection())
                return table;

            try
            {
                theQuery.CommandText = String.Format("CALL MANAGEABILITY.NCI.MethodsInClass('{0}','{1}', ?, ?);", aJarFile, aClassName);
                theQuery.Parameters.Clear(); //since command cobject is preserved, clear the parameters every time

                OdbcParameter param0 = theQuery.Parameters.Add("@METHODS", OdbcType.Text, 10240);
                param0.Direction = System.Data.ParameterDirection.Output;
                param0.DbType = System.Data.DbType.String;

                OdbcParameter param1 = theQuery.Parameters.Add("@TEMPFILENAME", OdbcType.Text, 256);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;

                theQuery.Connection = _theCurrentConnection.OpenOdbcConnection; ;
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

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

                _dataSet.Tables["Method"].Clear();
                StringReader sr = new StringReader(methods);
                _dataSet.ReadXml(sr);
                table = _dataSet.Tables["Method"].Copy();
                DataView sortedView = table.DefaultView;
                sortedView.Sort = table.Columns[0].ColumnName + " asc";
                table = sortedView.ToTable();                 
            }
            finally
            {
                close();
            } 
            return table;
        }

        #endregion
        
        #region Private Methods

        /// <summary>
        /// Get list of all code files or a specific code file
        /// </summary>
        /// <param name="aFileName">CodeFile</param>
        /// <returns></returns>
        private string list(PCFModel.AccessLevel accessLevel, String aFileName)
        {
            if (!GetConnection())
                return "";

            try
            {
                if (aFileName == null)
                {
                    theQuery.CommandText = "CALL MANAGEABILITY.NCI.LS(?, ?);";
                }
                else
                {
                    theQuery.CommandText = String.Format("CALL MANAGEABILITY.NCI.LSFILE('{0}', ?, ?);", aFileName);
                }
                theQuery.Parameters.Clear(); //since command cobject is preserved, clear the parameters every time

                OdbcParameter param0 = theQuery.Parameters.Add("@FILENAMES", OdbcType.Text, 10240);
                param0.Direction = System.Data.ParameterDirection.Output;
                param0.DbType = System.Data.DbType.String;

                OdbcParameter param1 = theQuery.Parameters.Add("@TEMPFILENAME", OdbcType.Text, 256);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;

                theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

                string fileNames = "";
                if (theQuery.Parameters["@FILENAMES"].Value != null)
                {
                    fileNames = theQuery.Parameters["@FILENAMES"].Value.ToString();
                    if ((fileNames != null) && (fileNames.Trim().Length > 0))
                    {
                        return fileNames;
                    }

                }
                if (theQuery.Parameters["@TEMPFILENAME"].Value != null)
                {
                    string temporaryFileName = theQuery.Parameters["@TEMPFILENAME"].Value.ToString();
                    if ((temporaryFileName != null) && (temporaryFileName.Trim().Length > 0))
                    {
                        //read the temp file and get the XML
                        fileNames = getTemporaryFileContent(temporaryFileName);
                        return fileNames;
                    }
                }
            }
            catch (OdbcException ex)
            {
                throw new Exception(ex.Message);
            }
            finally
            {
                close();
            }

            return "";
        }

        /// <summary>
        /// Gets a new connection object
        /// </summary>
        /// <returns></returns>
        private bool GetConnection()
        {
            if (this._theCurrentConnection == null)
            {
                _theCurrentConnection = new Connection(_theConnectionDefinition);
                return true;
            }
            else
            {
                return true;
            }
        }

        /// <summary>
        /// Reads content from temporary file
        /// </summary>
        /// <param name="aTemporaryFile"></param>
        /// <returns></returns>
        private string getTemporaryFileContent(string aTemporaryFile)
        {
            if (!GetConnection())
            {
                return null;
            }

            int offset = 0;
            Int32 cnt = 0;
            byte[] data = new byte[10240];
            StringWriter sw = new StringWriter();           
            string filename = aTemporaryFile;
            try
            {
                int count = 0;
                do
                {
                    //The count is the actual count rather than the length of the content
                    theQuery.CommandText = String.Format(
                                        "CALL MANAGEABILITY.NCI.GETTEMPORARYFILE('{0}', {1}, ?, ?);",
                                        new object[] { filename, offset });

                    theQuery.CommandType = System.Data.CommandType.StoredProcedure;
                    theQuery.Parameters.Clear(); //since command cobject is preserved, clear the parameters every time

                    OdbcParameter param1 = theQuery.Parameters.Add("@FILEDATA", OdbcType.Text, 10240);
                    param1.Direction = System.Data.ParameterDirection.Output;
                    param1.DbType = System.Data.DbType.String;


                    OdbcParameter param2 = theQuery.Parameters.Add("@DATALENGTH", OdbcType.Int);
                    param2.Direction = System.Data.ParameterDirection.Output;
                    param2.Value = cnt;

                    //Execute the SPJ
                    theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                    Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

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
                close();
            }
            return sw.ToString();
        }

        /// <summary>
        /// Cleanup and close current connection
        /// </summary>
        private void Dispose()
        {
            try
            {
                if (this._theCurrentConnection != null)
                {
                    this._theCurrentConnection.Close();
                }
            }
            catch (Exception)
            {
            }
        }

        #endregion Private Methods
    }
}
