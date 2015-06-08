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
using System.Data;
using System.Data.Odbc;
using System.IO;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// Model for a sql Library
	/// </summary>
	public class TrafodionLibrary : TrafodionSchemaObject
    {
        #region private fields
        private string _fileName = null;
        private string _clientName = null;
        private string _clientFileName = null;
        private List<TrafodionRoutine> _usedByRoutine = null;
        private List<JarClass> _jarClass = null;
        #endregion

        #region Public variables
        /// <summary>
        /// Constant that defines the Library object name space
        /// </summary>
        public const string ObjectNameSpace = "LB";
        public const string ObjectType = "LB";
        public const string TRACE_SUB_AREA_NAME = "Library";

        #endregion Public variables

        #region Public Properties

        public List<JarClass> TheJarClasses
        {
            get
            {
                return _jarClass;
            }
        }

        /// <summary>
        /// Displayable Object type for the Library
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return Properties.Resources.Library;
            }
        }


        /// <summary>
        /// Object name space for the Library
        /// </summary>
        override public string SchemaObjectNameSpace
        {
            get
            {
                return ObjectNameSpace;
            }
        }


        /// <summary>
        /// Object type for the Library
        /// </summary>
        override public string SchemaObjectType
        {
            get
            {
                return ObjectType;
            }
        }


        /// <summary>
        /// The JAR file name of the Library
        /// </summary>
        public string FileName
        {
            get { return _fileName; }
            set { _fileName = value; }
        }

        public string CodeFileName
        {
            get
            {
                return _fileName.Contains("/") ? _fileName.Substring(_fileName.LastIndexOf("/") + 1) : _fileName;
            }
        }
        /// <summary>
        /// The client name of the Library
        /// </summary>
        public string ClientName
        {
            get { return _clientName; }
            set { _clientName = value; }
        }

        /// <summary>
        /// The client file name of the Library
        /// </summary>
        public string ClientFileName
        {
            get { return _clientFileName; }
            set { _clientFileName = value; }
        }

        public string ClientCodeFileName
        {
            get
            {
                string clientFileName = _clientFileName.Contains(Path.DirectorySeparatorChar.ToString()) ? _clientFileName.Substring(_clientFileName.LastIndexOf(Path.DirectorySeparatorChar.ToString()) + 1) : _clientFileName;
                clientFileName = clientFileName.Contains(Path.AltDirectorySeparatorChar.ToString()) ? clientFileName.Substring(clientFileName.LastIndexOf(Path.AltDirectorySeparatorChar.ToString()) + 1) : clientFileName;
                return clientFileName;
            }
        }
        /// <summary>
        /// List of Routines of this Lib uses
        /// </summary>
        public List<TrafodionRoutine> TheUsedByTrafodionRoutines
        {
            get
            {
                if (_usedByRoutine == null)
                {
                    _usedByRoutine = new LibraryUsageLoader().Load(this);
                }
                return _usedByRoutine;
            }
        }
        #endregion Public Properties

        #region Contructors
        /// <summary>
        /// Constructs a model for the Library
        /// </summary>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="anInternalName"></param>
        /// <param name="aUID"></param>
        /// <param name="aCreateTime"></param>
        /// <param name="aRedefTime"></param>
        /// <param name="aSecurityClass"></param>
        /// <param name="anOwner">The owner of the object.</param>
        public TrafodionLibrary(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, 
            long aRedefTime, string aSecurityClass, string anOwner, 
            string fileName, string clientName, string clientFileName)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
		{
            _fileName = fileName;
            _clientName = clientName;
            _clientFileName = clientFileName;
        }

        #endregion
        
        #region Public Methods
        /// <summary>
        /// Resets the Library model
        /// </summary>
        override public void Refresh()
        {
            //Create a temp model
            TrafodionLibrary sqlMxLibrary = this.TheTrafodionSchema.LoadLibraryByName(this.InternalName);

            //IF temp model is null, the object has been removed by other client
            //So cleanup and notify the UI
            if (sqlMxLibrary == null)
            {
                this.TheTrafodionSchema.TrafodionLibraries.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(sqlMxLibrary) != 0)
            {
                //If sql object has been recreated, attach the new sql model to the parent.
                this.TheTrafodionSchema.TrafodionLibraries.Remove(this);
                this.TheTrafodionSchema.TrafodionLibraries.Add(sqlMxLibrary);
                //Notify the UI
                this.OnModelReplacedEvent(sqlMxLibrary);
            }
            else
            {
                base.Refresh();
            }
        }

        /// <summary>
        /// Compares this model to another model object
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        private int CompareTo(TrafodionLibrary aTrafodionLibrary)
        {
            long resultUID = this.UID - ((TrafodionLibrary)aTrafodionLibrary).UID;
            long resultFileName = string.Compare(this.FileName, aTrafodionLibrary.FileName, true);
            return (int)(Math.Abs(resultUID)+Math.Abs(resultFileName));
        }

        public void CreateLibrary(string libName,string fileName,string clientName,string clientFileName)
        {
            Connection theConnection = null;

            try
            {

                theConnection = GetConnection();

                //ADDLIB(FILENAME,LIBNAME,HOSTNAME,LOCALFILE)
                string cmdText = String.Format("CALL MANAGEABILITY.NCI.ADDLIB('{0}','{1}','{2}','{3}');", libName, fileName, clientName, clientFileName);

                OdbcCommand theQuery = new OdbcCommand(cmdText, theConnection.OpenOdbcConnection);
                theQuery.Parameters.Clear();

                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

            }
            catch(Exception ex)
            {
                throw ex;
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        public void UpdateLibrary(string fileName, string clientName, string clientFileName)
        {
            Connection theConnection = null;

            try
            {

                theConnection = GetConnection();

                //ALTERLIB(LIBNAME,FILENAME,HOSTNAME,LOCALFILE)
                string cmdText = String.Format("CALL MANAGEABILITY.NCI.ALTERLIB('{0}','{1}','{2}','{3}');",this.RealAnsiName,fileName, clientName, clientFileName);

                OdbcCommand theQuery = new OdbcCommand(cmdText, theConnection.OpenOdbcConnection);
                theQuery.Parameters.Clear();

                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }


        public void Drop(bool isCascade)
        {
            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                //DROPLIB(LIBNAME,MODETYPE<RESTRICT or CASCADE>)
                string cmdText = String.Format("CALL MANAGEABILITY.NCI.DROPLIB('{0}','{1}');", this.RealAnsiName, isCascade ? "CASCADE" : "RESTRICT");

                OdbcCommand theQuery = new OdbcCommand(cmdText, theConnection.OpenOdbcConnection);
                theQuery.Parameters.Clear();

                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

                //Since the drop succeeded add this Library model to the parent schema and send an event
                TheTrafodionSchema.DropLibrary(this);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        /// <summary>
        /// Writes the content to the file specified on the server
        /// </summary>
        /// <param name="file"></param>
        /// <param name="content"></param>
        /// <param name="count"></param>
        /// <param name="mode"></param>
        public void write(char[] content, Int32 mode,ref string fileName)
        {
            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                string cmdText = String.Format("CALL MANAGEABILITY.NCI.PUT_M7(?, {0},?);",mode);
                OdbcCommand theQuery = new OdbcCommand(cmdText, theConnection.OpenOdbcConnection);
                theQuery.Parameters.Clear(); 
                OdbcParameter param = theQuery.Parameters.Add("@FILEDATA", OdbcType.VarChar, 10240);
                param.Direction = System.Data.ParameterDirection.Input;
                param.Value = new string(content);

                OdbcParameter param2 = theQuery.Parameters.Add("@FILENAME",OdbcType.VarChar, 256);
                param2.Direction = System.Data.ParameterDirection.InputOutput;
                param2.Value = fileName;

                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);
                fileName = theQuery.Parameters[1].Value.ToString();
                
            }
            finally
            {

                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        /// <summary>
        /// Gets the content from the file specified from the server
        /// </summary>
        /// <param name="fileName"></param>
        /// <param name="offset"></param>
        /// <param name="content"></param>
        /// <param name="count"></param>
        ///  <param name="theQuery">use to cancelling the download.</param>
        public void getJarFile(string fileName, int offset, out string content, out Int32 count, ref OdbcCommand theQuery)
        {
            Connection theConnection = null;

            string text = "";
            Int32 cnt = 0;

            try
            {
                theConnection = GetConnection();
                if (theConnection == null)
                {
                    count = 0;
                    content = "";
                    return;
                }

                string filename = fileName.Trim();

                string cmdText = String.Format("CALL MANAGEABILITY.NCI.GETFILE('{0}', {1}, ?, ?);", new object[] { filename, offset });
                theQuery = new OdbcCommand(cmdText, theConnection.OpenOdbcConnection);
                theQuery.Parameters.Clear(); 
                theQuery.CommandType = System.Data.CommandType.StoredProcedure;
                
                OdbcParameter param1 = theQuery.Parameters.Add("@FILEDATA", OdbcType.Text, 10240);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;

                OdbcParameter param2 = theQuery.Parameters.Add("@DATALENGTH", OdbcType.Int);
                param2.Direction = System.Data.ParameterDirection.Output;
                param2.Value = cnt;
  
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
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        /// <summary>
        /// Get the class names in a given code file
        /// </summary>
        /// <param name="aJarFile"></param>
        /// <returns></returns>
        public List<JarClass> GetClasses(DataSet datasetXSD)
        {            
            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                string cmdGetClasses = string.Format("CALL MANAGEABILITY.NCI.ClassesInJar('{0}', ?, ?);", CodeFileName);
                OdbcCommand theQuery = new OdbcCommand(cmdGetClasses, theConnection.OpenOdbcConnection);
                theQuery.Parameters.Clear(); 

                OdbcParameter param0 = theQuery.Parameters.Add("@CLASSNAMES", OdbcType.Text, 10240);
                param0.Direction = System.Data.ParameterDirection.Output;
                param0.DbType = System.Data.DbType.String;

                OdbcParameter param1 = theQuery.Parameters.Add("@TEMPFILENAME", OdbcType.Text, 256);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;

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

                datasetXSD.Tables["Class"].Clear();
                StringReader sr = new StringReader(classNames);
                datasetXSD.ReadXml(sr);
                DataTable table = datasetXSD.Tables["Class"].Copy();
                DataView sortedView = table.DefaultView;
                sortedView.Sort = table.Columns[0].ColumnName + " asc";
                table = sortedView.ToTable();
                _jarClass = new List<JarClass>();

                for (int i = 0; i < table.Rows.Count; i++)
                {
                    _jarClass.Add(new JarClass(((string)table.Rows[i][0]).Trim(),this));
                }
              
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            return _jarClass;
        }

        override public bool IsMetadataObject
        {
            get
            {
                return TheSecurityClass.Trim().Equals("ST");
            }
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
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            return sw.ToString();
        }
        #endregion
    }

    #region Loader
    class LibraryUsageLoader : TrafodionObjectsLoader<TrafodionLibrary, TrafodionRoutine>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionLibrary aTrafodionLibrary)
        {
            return Queries.ExecuteSelectRoutinesUsedByLibrary(aConnection, aTrafodionLibrary);
        }

        /// <summary>
        /// Retrieve all routine objects from library.
        /// </summary>
        /// <param name="aList"></param>
        /// <param name="aTrafodionLibrary"></param>
        /// <param name="aReader"></param>
        override protected void LoadOne(List<TrafodionRoutine> aList, TrafodionLibrary aTrafodionLibrary, OdbcDataReader aReader)
        {
            string routineName = aReader.GetString(0).Trim();
            string schemaName = aReader.GetString(1);
            //long schemaUID = aReader.GetInt64(1);
            string udrType = aReader.GetString(2).Trim();

            TrafodionSchema schema = aTrafodionLibrary.TheTrafodionCatalog.FindSchema(schemaName);
            if (schema == null)
            {
                return;
            }
            TrafodionRoutine sqlMxRoutine = null;
            if ("F".Equals(udrType))
            {
                sqlMxRoutine = schema.FindFunction(routineName);
            }
            else if ("AC".Equals(udrType))
            {
                sqlMxRoutine = schema.FindFunctionAction(routineName);
            }
            else if ("P".Equals(udrType))
            {
                sqlMxRoutine = schema.LoadProcedureByName(routineName);
            }

            if (sqlMxRoutine != null)
            {
                aList.Add(sqlMxRoutine);
            }

        }
    }
#endregion
      

}


