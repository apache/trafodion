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
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.Data.AdoDotNet;
using System.Windows.Forms;
using Microsoft.VisualStudio.Data;
using System.Data.Common;

namespace Trafodion.Data.VisualStudio
{
    internal class TrafodionDbConnectionSupport: AdoDotNetConnectionSupport
	{
        public TrafodionDbConnectionSupport(): 
            base("Trafodion.Data")
        {
        }

        protected override object GetServiceImpl(Type serviceType)
        {
            if (serviceType == typeof(DataViewSupport))
            {
                return new TrafodionDbDataViewSupport();
            }
            if (serviceType == typeof(DataObjectSupport))
            {
                return new TrafodionDbDataObjectSupport();
            }
            if (serviceType == typeof(DataSourceInformation))
            {
                return new TrafodionDbDataSourceInformation(Site as DataConnection);
            }
         
            return base.GetServiceImpl(serviceType);
        }
    }
}
