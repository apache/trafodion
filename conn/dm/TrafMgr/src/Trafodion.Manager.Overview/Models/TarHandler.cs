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
using System.Data.Odbc;
using System.IO;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// class for call procedure of download OSIM data files
    /// </summary>
    class TarHandler : FileHandler
    {
        private const String  _ListTarProcedure = "CALL MANAGEABILITY.NCI.LISTTARFILES(?, ?);";
        private const String _GetTarProcedure = "CALL MANAGEABILITY.NCI.GETTARFILE('{0}', {1}, ?, ?);";

        public enum TarOperation {List, Get, Cancel};

        public TarHandler(ConnectionDefinition aConnectionDefinition)
        {
            this.theConnectionDefinition = aConnectionDefinition;
        }

        #region Public methods
        /// <summary>
        /// get tar info from server
        /// </summary>
        /// <returns>file info</returns>
        public DataTable getFileInfo()
        {
            Connection theConnection = GetConnection();
            DataSet _datasetXSD = new DataSet();
            DataTable _fileList;
            _datasetXSD.Clear();
            try
            {
                string cmdGetListTarFiles = String.Format(_ListTarProcedure);
                OdbcCommand theQuery = new OdbcCommand(cmdGetListTarFiles, theConnection.OpenOdbcConnection);
                theQuery.Parameters.Clear(); //since command cobject is preserved, clear the parameters every time

                OdbcParameter param0 = theQuery.Parameters.Add("@FILENAMES", OdbcType.Text, 10240);
                param0.Direction = System.Data.ParameterDirection.Output;
                param0.DbType = System.Data.DbType.String;

                OdbcParameter param1 = theQuery.Parameters.Add("@TEMPFILENAME", OdbcType.Text, 256);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;

                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, "Tar", true);

                string fileinfo = theQuery.Parameters["@FILENAMES"].Value.ToString();
                if ((fileinfo == null) || (fileinfo.Trim().Length == 0))
                {
                    string temporaryFileName = theQuery.Parameters["@TEMPFILENAME"].Value.ToString();
                    if ((temporaryFileName != null) && (temporaryFileName.Trim().Length > 0))
                    {
                        //read the temp file and get the XML
                        fileinfo = getTemporaryFileContent(temporaryFileName);
                    }
                }

                StringReader sr = new StringReader(fileinfo);
                _datasetXSD.ReadXml(sr);
                _fileList = new DataTable();
                if (_datasetXSD.Tables["File"] == null)
                    return null;

                _fileList = _datasetXSD.Tables["File"].Copy();
                DataView sortedView = _fileList.DefaultView;
                sortedView.Sort = _fileList.Columns[0].ColumnName + " asc";
                _fileList = sortedView.ToTable();

                return _fileList;
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
        /// Gets the content from the tar file specified from the server
        /// </summary>
        /// <param name="fileName"></param>
        /// <param name="offset"></param>
        /// <param name="content"></param>
        /// <param name="count"></param>
        ///  <param name="theQuery">use to cancelling the download.</param>
        public void getTarFile(string fileName, int offset, out string content, out Int32 count, ref OdbcCommand theQuery)
        {
            Connection theConnection = GetConnection();
            //OdbcCommand theQuery;
            string text = "";
            Int32 cnt = 0;
            
            try
            {
                if (theConnection == null)
                {
                    count = 0;
                    content = "";
                    return;
                }

                string filename = fileName.Trim();
                string cmdText = String.Format(_GetTarProcedure, new object[] { filename, offset });
                theQuery = new OdbcCommand(cmdText, theConnection.OpenOdbcConnection);
                theQuery.Parameters.Clear();
                theQuery.CommandType = System.Data.CommandType.StoredProcedure;

                OdbcParameter param1 = theQuery.Parameters.Add("@FILEDATA", OdbcType.Text, 10240);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;

                OdbcParameter param2 = theQuery.Parameters.Add("@DATALENGTH", OdbcType.Int);
                param2.Direction = System.Data.ParameterDirection.Output;
                param2.Value = cnt;

                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, "TAR", true);

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

        #endregion
    }
}
