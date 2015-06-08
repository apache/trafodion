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
using System.Data.Odbc;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.DatabaseArea.Queries.Controls;

namespace Trafodion.Manager.WorkloadArea.Model
{
    [Serializable]
    public class WMSOffenderStatusOptions
    {
        //e.g. STATUS CPU 0 SEGMENT 1 PROCESS SQL
        #region Members
        private bool m_statusCpu;
        private bool m_useCpu;
        private int m_cpuNumber;
        private bool m_useSegment;
        private int m_segmentNumber;
        private bool m_sqlProcess;
        private string m_statusCommand;
        #endregion 

        #region Properties
        public bool StatusCpu
        {
            get { return m_statusCpu; }
            set { m_statusCpu = value; }
        }

        public bool UseCpu
        {
            get { return m_useCpu; }
            set { m_useCpu = value; }
        }

        public int CpuNumber
        {
            get { return m_cpuNumber; }
            set { m_cpuNumber = value; }
        }

        public bool UseSegment
        {
            get { return m_useSegment; }
            set { m_useSegment = value; }
        }

        public int SegmentNumber
        {
            get { return m_segmentNumber; }
            set { m_segmentNumber = value; }
        }

        public bool SQLProcess
        {
            get { return m_sqlProcess; }
            set { m_sqlProcess = value; }
        }

        public string StatusCommand
        {
            get { return m_statusCommand; }
            set { m_statusCommand = value; }
        }
        #endregion

        public WMSOffenderStatusOptions()
        {
            m_statusCpu = true;
            m_useCpu = false;
            m_cpuNumber = 0;
            m_useSegment = false;
            m_segmentNumber = 1;
            m_sqlProcess = false;
            m_statusCommand = "STATUS CPU PROCESS ALL";
        }
    }
}
