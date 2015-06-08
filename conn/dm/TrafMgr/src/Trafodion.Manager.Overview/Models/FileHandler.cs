// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Trafodion.Manager.Framework.Connections;
using System.IO;
using System.Data.Odbc;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.OverviewArea.Models
{
    abstract public class FileHandler
    {
        private const String _GetTempFileProcedure = "CALL MANAGEABILITY.NCI.GETTEMPORARYFILE('{0}', {1}, ?, ?);";
        protected ConnectionDefinition theConnectionDefinition;

        protected Connection GetConnection()
        {
            return new Connection(theConnectionDefinition);
        }

        /// <summary>
        /// Reads content from temporary file
        /// </summary>
        /// <param name="aTemporaryFile">temp file name</param>
        /// <returns>temp file content</returns>
        protected string getTemporaryFileContent(string aTemporaryFile)
        {
            Connection theTempConnection = GetConnection();
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
                    string cmdQuery = String.Format(_GetTempFileProcedure,
                                        new object[] { filename, offset });
                    OdbcCommand theQuery = new OdbcCommand(cmdQuery, theTempConnection.OpenOdbcConnection);
                    theQuery.CommandType = System.Data.CommandType.StoredProcedure;
                    //since command cobject is preserved, clear the parameters every time
                    theQuery.Parameters.Clear();

                    OdbcParameter param1 = theQuery.Parameters.Add("@FILEDATA", OdbcType.Text, 10240);
                    param1.Direction = System.Data.ParameterDirection.Output;
                    param1.DbType = System.Data.DbType.String;

                    OdbcParameter param2 = theQuery.Parameters.Add("@DATALENGTH", OdbcType.Int);
                    param2.Direction = System.Data.ParameterDirection.Output;
                    param2.Value = cnt;

                    //Execute the SPJ
                    Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, "FileHandler", true);

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
                if (theTempConnection != null)
                {
                    theTempConnection.Close();
                }
            }

            return sw.ToString();
        }
    }
}
