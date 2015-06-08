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
using Trafodion.Manager.Framework.Controls;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.UniversalWidget.Controls;
namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// This class shall be used to display the data in the TabularDisplayControl
    /// The methods in this class can be extended to provide cutomized look to the 
    /// tabular display control
    /// </summary>
    public class TabularDataDisplayHandler : IDataDisplayHandler
    {
        public virtual void DoPopulate(UniversalWidgetConfig aConfig, DataTable aDataTable, TrafodionIGrid aDataGrid)
        {
            try
            {
                aDataGrid.BeginUpdate();

                //Populate the grid with the data table passed
                TrafodionIGridUtils.PopulateGrid(aConfig, aDataTable, aDataGrid);

                //apply the width, sort order, etc
                TabularDataDisplayControl.ApplyColumnAttributes(aDataGrid, aConfig.DataProviderConfig);
            }
            finally
            {
                aDataGrid.EndUpdate();
            }
        }
    }
}
