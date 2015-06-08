// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.Data.Odbc;
using System.Drawing;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.DatabaseArea.Queries.Controls;

namespace Trafodion.Manager.WorkloadArea.Model
{
    [Serializable]
    public class MonitorWorkloadOptions
    {
        #region Members

        public static readonly string MonitorWorkloadPersistenceKey = "MonitorWorkloadOptionsPersistence";
        private Color m_executingColor = Color.Green;
        private Color m_waitingColor = Color.DimGray;
        private Color m_holdingColor = Color.DimGray;
        private Color m_suspendedColor = Color.DimGray;
        private Color m_rejectedColor = Color.Red;
        private Color m_completedColor = Color.Blue;
        private Color m_warnHighColor = Color.WhiteSmoke;
        private Color m_warnMediumColor = Color.WhiteSmoke;
        private Color m_warnLowColor = Color.WhiteSmoke;
        private Color m_noWarnColor = Color.White;
        private bool m_sqlPreview = false;
        private Color m_sqlPreviewColor = Color.DimGray;
        private bool m_highLightChanges = true;
        private Color m_highLightChangesColor = Color.Blue;
        private List<string> m_queryStates = null;
        private List<string> m_warnLevels = null;
        private string[] m_states = { "EXECUTING", "WAITING", "HOLDING", "SUSPENDED", "REJECTED", "COMPLETED" };
        private string[] m_levels = { "HIGH", "MEDIUM", "LOW", "NOWARN" };
        private bool m_showManageabilityQueries = true;
        int _maxGraphPoints = 120;

        public const int DefaultMaxGraphPoints = 120;

        #endregion Members

        #region Properties

        public Color ExecutingColor
        {
            get { return m_executingColor; }
            set { m_executingColor = value; }
        }

        public Color WaitingColor
        {
            get { return m_waitingColor; }
            set { m_waitingColor = value; }
        }

        public Color HoldingColor
        {
            get { return m_holdingColor; }
            set { m_holdingColor = value; }
        }

        public Color SuspendedColor
        {
            get { return m_suspendedColor; }
            set { m_suspendedColor = value; }
        }

        public Color RejectedColor
        {
            get { return m_rejectedColor; }
            set { m_rejectedColor = value; }
        }

        public Color CompletedColor
        {
            get { return m_completedColor; }
            set { m_completedColor = value; }
        }

        public Color WarnHighColor 
        {
            get { return m_warnHighColor; }
            set { m_warnHighColor = value; }
        }

        public Color WarnMediumColor
        {
            get { return m_warnMediumColor; }
            set { m_warnMediumColor = value; }
        }

        public Color WarnLowColor
        {
            get { return m_warnLowColor; }
            set { m_warnLowColor = value; }
        }

        public Color NoWarnColor
        {
            get { return m_noWarnColor; }
            set { m_noWarnColor = value; }
        }

        public List<string> QueryStates
        {
            get { return m_queryStates; }
        }

        public List<string> WarnLevels
        {
            get { return m_warnLevels; }
        }
        
        public bool SQLPreview 
        {
            get { return m_sqlPreview; }
            set { m_sqlPreview = value; }
        }

        public Color SQLPreviewColor 
        {
            get { return m_sqlPreviewColor; }
            set { m_sqlPreviewColor = value; }
        }

        public bool HighLightChanges 
        {
            get { return m_highLightChanges; }
            set { m_highLightChanges = value; }
        }

        public Color HighLightChangesColor
        {
            get { return m_highLightChangesColor; }
            set { m_highLightChangesColor = value; }
        }
		
        public bool ShowManageabilityQueries
        {
	        get { return m_showManageabilityQueries; }
	        set { m_showManageabilityQueries = value; }
        }

        public int MaxGraphPoints
        {
            get { return _maxGraphPoints; }
            set { _maxGraphPoints = value; }
        }

        #endregion Properties

        private MonitorWorkloadOptions()
        {
            m_sqlPreview = false;
            m_highLightChanges = true;
            m_queryStates = new List<string>();
            m_queryStates.AddRange(m_states);
            m_warnLevels = new List<string>();
            m_warnLevels.AddRange(m_levels);
            m_showManageabilityQueries = true;
            _maxGraphPoints = DefaultMaxGraphPoints;
        }

        public void SaveIntoPersistence()
        {
            try
            {
                Trafodion.Manager.Framework.Persistence.Put(MonitorWorkloadOptions.MonitorWorkloadPersistenceKey, this);
            }
            catch (Exception)
            {

            }
        }
        public static MonitorWorkloadOptions GetOptions()
        {
            MonitorWorkloadOptions monitorWorkloadOptions = null;

            try
            {
                monitorWorkloadOptions = Trafodion.Manager.Framework.Persistence.Get(MonitorWorkloadPersistenceKey) as MonitorWorkloadOptions;

                if (monitorWorkloadOptions == null)
                {
                    monitorWorkloadOptions = new MonitorWorkloadOptions();
                }
            }
            catch (Exception ex)
            {
                monitorWorkloadOptions = new MonitorWorkloadOptions();
            }

            return monitorWorkloadOptions;
        }
    }
}
