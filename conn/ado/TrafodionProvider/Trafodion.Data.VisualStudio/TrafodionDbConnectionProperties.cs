/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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
********************************************************************/

using System;
using Microsoft.VisualStudio.Data.AdoDotNet;

namespace Trafodion.Data.VisualStudio
{
	public class TrafodionDbConnectionProperties : AdoDotNetConnectionProperties
	{
        public TrafodionDbConnectionProperties(): 
            base("Trafodion.Data")
        {
        }

		public override bool IsComplete
		{
			get
			{
                return !(String.IsNullOrEmpty(this["Server"].ToString()) ||
                    String.IsNullOrEmpty(this["User"].ToString()) ||
                    String.IsNullOrEmpty(this["Password"].ToString()));
			}
		}
	}
}
