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

//using System;
//using System.Collections.Generic;
//using System.Text;
//using Trafodion.Manager.Framework.Controls;
//using System.Windows.Forms;

//namespace Trafodion.Manager.DatabaseArea.Queries.Controls
//{
//    public class QueryTabControl : TrafodionTabControl
//    {
//        private QueryTextTabPage _theQueryTextTabPage;
//        private QueryInputControl _theQueryInputControl;

//#if PARAMS_TAB

//        private QueryParametersTabPage _theQueryParametersTabPage;

//#endif

//        public QueryTabControl()
//        {

//            _theQueryTextTabPage = new QueryTextTabPage();
//            TabPages.Add(_theQueryTextTabPage);

//            _theQueryInputControl = new QueryInputControl();
//            _theQueryTextTabPage.TheQueryInputControl = _theQueryInputControl;

//#if PARAMS_TAB

//            _theQueryParametersTabPage = new QueryParametersTabPage();
//            TabPages.Add(_theQueryParametersTabPage);

//            SelectedIndexChanged += new EventHandler(QueryTabControlSelectedIndexChanged);
//#endif

//        }

//#if PARAMS_TAB

//        void QueryTabControlSelectedIndexChanged(object sender, EventArgs e)
//        {
//            if (SelectedTab == _theQueryParametersTabPage)
//            {
//                TheQueryParametersTabPage.LoadParameters(TheQueryInputControl.ReportParameters, true);
//            }
//        }

//#endif

//        public QueryTextTabPage TheQueryTextTabPage
//        {
//            get { return _theQueryTextTabPage; }
//        }

//        public QueryInputControl TheQueryInputControl
//        {
//            get { return _theQueryInputControl; }
//        }

//        public TextBox TheQueryTextBox
//        {
//            get { return TheQueryInputControl.TheQueryTextBox; }
//        }

//#if PARAMS_TAB

//        public QueryParametersTabPage TheQueryParametersTabPage
//        {
//            get { return _theQueryParametersTabPage; }
//        }
//#endif

//    }
//}
