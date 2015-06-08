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

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// 
	/// </summary>
	public class TrafodionCatalogRegistration : IHasTrafodionCatalog
	{
		public TrafodionCatalogRegistration(TrafodionCatalog aTrafodionCatalog, string aSegmentName, string aVolumeName, string aRule)
		{
			theTrafodionCatalog = aTrafodionCatalog;
			theSegmentName = aSegmentName;
			theVolumeName = aVolumeName;
			theRule = aRule;
		}

		public TrafodionCatalog TheTrafodionCatalog
		{
			get
			{
				return theTrafodionCatalog;
			}
		}

		public string TheSegmentName
		{
			get
			{
				return theSegmentName;
			}
		}

		public string TheVolumeName
		{
			get
			{
				return theVolumeName;
			}
		}

		public string TheRule
		{
			get
			{
				return theRule;
			}
		}

		private TrafodionCatalog theTrafodionCatalog;
		private string theSegmentName;
		private string theVolumeName;
		private string theRule;

	}
}
