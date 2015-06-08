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
using System.Threading;
using System.Data.Odbc;

namespace Trafodion.Manager.WorkloadArea.Model
{
    public class WMSConnectInfo
    {
        #region Members
        private OdbcConnection m_connection = null;
        private OdbcCommand m_command = null;
        private string m_connectString = null;
        private string m_driver = null;
        private string m_server = null;
        private string m_serverDSN = null;
        private string m_uid = null;
        private string m_pwd = null;
        #endregion

        #region Properties
        public OdbcConnection Connection
        {
            get { return m_connection; }
            set { m_connection = value; }
        }

        public OdbcCommand Command
        {
            get { return m_command; }
            set { m_command = value; }
        }

        public string ConnectString
        {
            get { return m_connectString; }
            set { m_connectString = value; }
        }

        public string Driver
        {
            get { return m_driver; }
            set { m_driver = value; }
        }

        public string Server
        {
            get { return m_server; }
            set { m_server = value; }
        }

        public string ServerDSN
        {
            get { return m_serverDSN; }
            set { m_serverDSN = value; }
        }
        
        public string UID
        {
            get { return m_uid; }
            set { m_uid = value; }
        }

        public string PWD
        {
            get { return m_pwd; }
            set { m_pwd = value; }
        }
        #endregion

        #region Constructors
        public WMSConnectInfo(OdbcConnection connection, OdbcCommand command, string connectString)
        {
            m_connection = connection;
            m_command = command;
            m_connectString = connectString;
            string tString = connectString;
            char[] delim = { ';' };
            string[] aString = tString.Split(delim);
            for (int i = 0; i < aString.Length; i++)
            {
                string tString2 = aString[i];
                char[] delim2 = { '=' };
                string[] aString2 = tString2.Split(delim2);
                if (aString2.Length == 2)
                {
                    if (aString2[0].ToUpper().Equals("DRIVER"))
                        m_driver = aString2[1];
                    else if (aString2[0].ToUpper().Equals("SERVER"))
                        m_server = aString2[1];
                    else if (aString2[0].ToUpper().Equals("SERVERDSN"))
                        m_serverDSN = aString2[1];
                    else if (aString2[0].ToUpper().Equals("UID"))
                        m_uid = aString2[1];
                    else if (aString2[0].ToUpper().Equals("PWD"))
                        m_pwd = aString2[1];
                }
            }

        }
        #endregion

    }
}
