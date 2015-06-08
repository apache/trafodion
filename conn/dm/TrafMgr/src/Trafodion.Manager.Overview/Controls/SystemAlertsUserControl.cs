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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Data.Odbc;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework.Queries;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// The Alerts Widget. Displays alerts details
    /// </summary>
    public class SystemAlertsUserControl
    {
        public static readonly string ALERT_CREATE_TS_LCT_COL_NAME = "CREATE_TS_LCT";
        public static readonly string ALERT_CREATOR_PROCESS_ID_COL_NAME = "CREATOR_PROCESS_ID";
        public static readonly string ALERT_CREATOR_HOST_ID_COL_NAME = "CREATOR_HOST_ID";
        public static readonly string ALERT_LAST_UPDATE_TS_LCT_COL_NAME = "LAST_UPDATE_TS_LCT";
        public static readonly string ALERT_CLOSE_TS_LCT_COL_NAME = "CLOSE_TS_LCT";
        public static readonly string ALERT_COMPONENT_NAME_COL_NAME = "PROBLEM_COMPONENT_NAME";
        public static readonly string ALERT_PROCESS_NAME_COL_NAME = "PROBLEM_PROCESS_NAME";
        public static readonly string ALERT_PROCESS_ID_COL_NAME = "PROBLEM_PROCESS_ID";
        public static readonly string ALERT_IP_ADDRESS_ID_COL_NAME = "PROBLEM_IP_ADDRESS_ID";
        public static readonly string ALERT_RESOURCE_COL_NAME = "RESOURCE_NAME";
        public static readonly string ALERT_RESOURCE_TYPE_COL_NAME = "RESOURCE_TYPE";
        public static readonly string ALERT_SEVERITY_COL_NAME = "SEVERITY";
        public static readonly string ALERT_SEVERITY_NAME_COL_NAME = "SEVERITY_NAME";
        public static readonly string ALERT_DESCRIPTION_COL_NAME = "DESCRIPTION";
        public static readonly string ALERT_STATUS_COL_NAME = "STATUS";
        public static readonly string ALERT_CREATE_TS_UTC_COL_NAME = "CREATE_TS_UTC";
        public static readonly string ALERT_NOTES_COL_NAME = "NOTES";
        public static readonly string ALERT_TYPE_COL_NAME = "TYPE";
        public static readonly string ALERT_TYPE_DESCRIPTION_COL_NAME = "TYPE_DESCRIPTION";
        public static readonly string ALERT_STATUS_OPEN = "OPEN";
        public static readonly string ALERT_STATUS_USER_CLOSED = "USERCLOSED";
        public static readonly string ALERT_STATUS_OP_CLOSED = "OPCLOSED";
        public static readonly string ALERT_STATUS_AUTO_CLOSED = "AUTOCLOSED";
        public static readonly string ALERT_STATUS_ACK = "ACKNOWLEDGED";
        public static readonly string ALERT_STATUS_UNKNOWN = "UNKNOWN";
    }

    /// <summary>
    /// Structure of the Alarms Primary key
    /// </summary>
    public class AlertsPrimaryKey
    {
        DateTime _createTime;
        long _createHostId;
        int _createProcessId;
        string _originalStatus;
        string _originalNotes;

        public string OriginalStatus
        {
            get { return _originalStatus; }
            set { _originalStatus = value; }
        }

        public string OriginalNotes
        {
            get { return _originalNotes; }
            set { _originalNotes = value; }
        }

        public DateTime CreateTime
        {
            get { return _createTime; }
        }

        public long CreateHostId
        {
            get { return _createHostId; }
        }

        public int CreateProcessId
        {
            get { return _createProcessId; }
        }

        public AlertsPrimaryKey(DateTime createTime, long hostId, int processId)
        {
            _createTime = createTime;
            _createHostId = hostId;
            _createProcessId = processId;
        }
    }

}
