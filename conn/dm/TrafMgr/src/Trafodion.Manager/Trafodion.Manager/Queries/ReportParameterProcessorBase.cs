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
using System.Collections;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
//using Trafodion.Manager.Framework.Queries.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.Framework.Queries
{
    [Serializable]
    public class ReportParameterProcessorBase
    {
        public const int MAX_PARAMS_TO_STORE = 10;
        public const string REPORT_PARAMS = "REPORT_PARAMS";

        public const string TIME_ZONE_OFFSET = "TIMEZONE_OFFSET";
        public const string SEARCH_DSN = "SEARCH_DSN";
        public const string RUN_TIME = "RUN_TIME";
        public const string RECORD_COUNT = "RECORD_COUNT";
        public const string CLIENT_ID = "CLIENT_ID";
        public const string FROM_TIME = "__FROM_TIME";
        public const string TO_TIME = "__TO_TIME";
        public const string CONNECTED_DSN = "__CONNECTED_DSN";
        public const string LOGGEDON_USER_ID = "__LOGGEDON_USER_ID";

        public const string PARAMETER_NAME_TOKEN = "PARAMETER_NAME";
        public const string DISPLAY_NAME_TOKEN = "DISPLAY_NAME";
        public const string TOOLTIP_TOKEN = "TOOLTIP";
        public const string LEGAL_VALUES_TOKEN = "LEGAL_VALUES";
        public const string EXAMPLE_TOKEN = "EXAMPLE";
        public const string CATALOG_NAME = "CATALOG_NAME";
        public const string SCHEMA_NAME = "SCHEMA_NAME";
        public const string SESSION_NAME = "__SESSION_NAME";
        public const string REPORT_NAME = "__REPORT_NAME";
        public const string ROW_COUNT = "__ROW_COUNT";
        public const string TIME_RANGE_KEY = "TIME_RANGE_KEY";
        public const string SYSTEM_CATALOG_NAME = "SYSTEM_CATALOG_NAME";
        public const string SYSTEM_CATALOG_VERSION = "SYSTEM_CATALOG_VERSION";

        public static string commentStart = "/*QueryDocumentationStart";
        public static string commentEnd = "QueryDocumentationEnd*/";

    }
}
