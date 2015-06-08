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

namespace Trafodion.Manager.WorkloadArea.Model
{
    class WMSHistoryModel
    {
        #region Members
        private int m_number = 0;
        private string m_type = null;
        private string m_command = null;
        private DateTime m_executedTime = DateTime.Now;
        #endregion

        #region Properties
        public int Number
        {
            get { return m_number; }
        }

        public string SQLType
        {
            get { return m_type; }
        }

        public string SQLCommand
        {
            get { return m_command; }
        }

        public DateTime ExecutedTime
        {
            get { return m_executedTime; }
        }
        #endregion

        public WMSHistoryModel(int number, string command, string type, DateTime executedTime)
        {
            m_number = number;
            m_command = command;
            m_type = type;
            m_executedTime = executedTime;
        }
    }
}
