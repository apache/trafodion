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
using System.Text;
using System.Data;
using System.IO;
using System.Diagnostics;
namespace Trafodion.Manager.DatabaseArea.Model
{
    class JavaLauncher
    {
        // These are the Win32 error code for file not found or access denied.
        const int ERROR_FILE_NOT_FOUND = 2;
        const int ERROR_ACCESS_DENIED = 5;
        String fileName = "C:\\TrafodionTrafodionManager2\\introspect.cmd";

        static string readData;
        /// <summary>
        /// Code to get Classes
        /// </summary>
        /// 
        public List<string> GetClasses(String aJarFile)
        {
            List<string> classesInJar = new List<string>();
            try
            {
                StringBuilder strBuilder = new StringBuilder();
                string type = "0";
                string startTag = "<Jar";
                string tableName = "Classes";

                strBuilder.AppendFormat(" {0} {1} {2} ", type, aJarFile, "");

                readData = "";
                LaunchProcess(ref fileName, strBuilder.ToString(), "SPJ");
                string ret = readData.Substring(readData.IndexOf(startTag));
                DataSet dataSet = new DataSet();
                StringReader reader = new StringReader(ret);
                dataSet.ReadXml(reader);
                DataTable table = dataSet.Tables["ClassName"];
                //Kludge for now. Needed for 1 row
                if (table == null)
                {
                    table = dataSet.Tables[tableName];
                }

                for (int i = 0; i < table.Rows.Count; i++)
                {
                    classesInJar.Add((string)table.Rows[i][0]);
                }
            }
            catch (Exception ex)
            {
                throw ex;
            }
            return classesInJar;
        }

        public DataTable GetMethods(string aJarFile, string aClass)
        {
            DataTable table = null;
            //List<Trafodion.Manager.DatabaseArea.Controls.JavaMethod> methodsInClass = new List<Trafodion.Manager.DatabaseArea.Controls.JavaMethod>();
            try
            {
                StringBuilder strBuilder = new StringBuilder();
                string type = "1";
                string startTag = "<Class";
                string tableName = "Method";

                strBuilder.AppendFormat(" {0} {1} {2} ", type, aJarFile, aClass);

                readData = "";
                LaunchProcess(ref fileName, strBuilder.ToString(), "SPJ");
                string ret = readData.Substring(readData.IndexOf(startTag));
                DataSet dataSet = new DataSet();
                StringReader reader = new StringReader(ret);
                dataSet.ReadXml(reader);
                table = dataSet.Tables[tableName];

                //for (int i = 0; i < table.Rows.Count; i++)
                //{
                //    Trafodion.Manager.DatabaseArea.Controls.JavaMethod method = new Trafodion.Manager.DatabaseArea.Controls.JavaMethod();
                //    method.MethodName((string)table.Rows[i][0]);
                //    method.RawMethodSignature((string)table.Rows[i][1]);
                //    method.MethodSignature((string)table.Rows[i][2]);
                //    methodsInClass.Add(method);
                //}
            }
            catch (Exception ex)
            {
                throw ex;
            }
            return table;
        }
        //public void LaunchJava()
        //{
        //    try
        //    {

        //        String fileName = "C:\\TrafodionTrafodionManager2\\introspect.cmd";
        //        StringBuilder strBuilder = new StringBuilder();
        //        string type = "0";
        //        string startTag = "<Jar";
        //        string tableName = "ClassName";
        //        if (! this._theFetchClassnames.Checked)
        //        {
        //            type = "1";
        //            startTag = "<Class";
        //            tableName = "Method";
        //        }

        //        strBuilder.AppendFormat(" {0} {1} {2} ", type, this._theJarFile.Text, this._theClassName.Text);

        //        readData = "";
        //        LaunchProcess(ref fileName, strBuilder.ToString(), "SPJ");
        //        this._theReturnValue.Text = readData.Substring(readData.IndexOf(startTag));
        //        DataSet dataSet = new DataSet();
        //        StringReader reader = new StringReader(this._theReturnValue.Text);
        //        dataSet.ReadXml(reader);
        //        DataTable table = dataSet.Tables[tableName];
        //        _theData.DataSource = table;
        //    }
        //    catch (Exception ex)
        //    {
        //        this._theReturnValue.Text = ex.Message;
        //        this._theReturnValue.Text += ex.StackTrace;
        //    }
        //}


        private bool LaunchProcess(ref String fileName, String args, String programName)
        {
            Process myProcess = new Process();
            myProcess.StartInfo.FileName = fileName;
            if (!String.IsNullOrEmpty(args))
            {
                myProcess.StartInfo.Arguments = args;
            }
            myProcess.StartInfo.UseShellExecute = false;
            myProcess.StartInfo.RedirectStandardOutput = true;
            myProcess.StartInfo.CreateNoWindow = true;

            bool isStarted = false;
            try
            {
                myProcess.OutputDataReceived += new DataReceivedEventHandler(JavaOutputHandler);
                myProcess.Start();
                myProcess.BeginOutputReadLine();
                myProcess.WaitForExit();
                myProcess.Close();
            }
            catch (System.ComponentModel.Win32Exception ex)
            {
                isStarted = false;
            }
            return isStarted;             
        }

        private static void JavaOutputHandler(object sendingProcess,
                    DataReceivedEventArgs outLine)
        {
            // Collect the sort command output.
            if (!String.IsNullOrEmpty(outLine.Data))
            {
                readData += outLine.Data;
            }
        }

    }
}
