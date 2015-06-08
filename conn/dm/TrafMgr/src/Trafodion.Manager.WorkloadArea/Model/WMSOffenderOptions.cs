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
	public class WMSOffenderOptions
	{
		public enum STATUS_OFFENDER
		{
			CPU = 0,
			MEMORY = 1
		};

		#region Members
		private int m_sampleInterval;
		private int m_sampleCPUs;
		private int m_sampleCache;
		private bool m_sqlProcess;
		private STATUS_OFFENDER m_statusCommand;
		#endregion

		#region Properties
		public int SampleInterval
		{
			get { return m_sampleInterval; }
			set { m_sampleInterval = value; }
		}

		public int SampleCPUs
		{
			get { return m_sampleCPUs; }
			set { m_sampleCPUs = value; }
		}

		public int SampleCache
		{
			get { return m_sampleCache; }
			set { m_sampleCache = value; }
		}

		public bool SQLProcess
		{
			get { return m_sqlProcess; }
			set { m_sqlProcess = value; }
		}

		public STATUS_OFFENDER StatusOffender
		{
			get { return m_statusCommand; }
			set { m_statusCommand = value; }
		}
		#endregion

		public WMSOffenderOptions()
		{
			m_sampleInterval = 0;
			m_sampleCPUs = 0;
			m_sampleCache = 0;
			m_sqlProcess = false;
			m_statusCommand = STATUS_OFFENDER.CPU;
		}
	}
}
