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
using Trafodion.Manager.Framework;
using System.ComponentModel;
using Trafodion.Manager.DatabaseArea.Queries;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea
{

    /// <summary>
    /// This class is a singleton that manages the list of report definitions.  It handles persistence
    /// events and fires its own events as query strings are added, updated, and removed.
    /// </summary>
    static public class QueryStringsPersistence
    {
        /// <summary>
        /// Strings are either added or removed
        /// </summary>
        public enum Operation
        {
            /// <summary>
            /// The query string has been added
            /// </summary>
            Added,

            /// <summary>
            /// The query string has been removed
            /// </summary>
            Removed
        }

        /// <summary>
        /// Signature of a handler to find out when a query string has been added or removed
        /// </summary>
        /// <param name="aReportDefinition">the string</param>
        /// <param name="aReportDefinitionsOperation">Added or Removed</param>
        public delegate void AddedRemovedHandler(ReportDefinition aReportDefinition, Operation aReportDefinitionsOperation);


        static private List<ReportDefinition> _theReportDefinitions = null;

        private static readonly string _theQueryStringsPersistenceKey = "QueryStringsPersistence";
        private static readonly string _theReportDefinitionsPersistenceKey = "ReportDefinitionsPersistence";

        static private EventHandlerList _theEventHandlers = new EventHandlerList();

        private static readonly string _theAddRemoveEventKey = "AddRemove";

        /// <summary>
        /// The list of all query strings
        /// </summary>
        public static List<ReportDefinition> TheReportDefinitions
        {
            get
            {
                if (_theReportDefinitions == null)
                {
                    _theReportDefinitions = new List<ReportDefinition>();

                    LoadPersistence();
                }
                return _theReportDefinitions;
            }
            set
            {
                _theReportDefinitions = value;
            }
        }

        /// <summary>
        /// Add a ReportDefinition to the list
        /// </summary>
        /// <param name="aReportDefinition">the ReportDefinition</param>
        static public void Add(ReportDefinition aReportDefinition)
        {
            if (!TheReportDefinitions.Contains(aReportDefinition))
            {
                ((SimpleReportDefinition)aReportDefinition).Group = Properties.Resources.PersistenceFile;
                TheReportDefinitions.Add(aReportDefinition);
                FireAddedRemoved(aReportDefinition, Operation.Added);
            }
        }

        static public bool ReportDefinitionExists(string name)
        {
            ReportDefinition theReportDefinition = TheReportDefinitions.Find(delegate(ReportDefinition reportDefinition)
            {
                return reportDefinition.Name == name;
            });
            return (theReportDefinition != null);
        }

        /// <summary>
        /// Remove a ReportDefinition from the list
        /// </summary>
        /// <param name="aReportDefinition">the query string</param>
        static public void Remove(ReportDefinition aReportDefinition)
        {
            if (TheReportDefinitions.Contains(aReportDefinition))
            {
                TheReportDefinitions.Remove(aReportDefinition);
                FireAddedRemoved(aReportDefinition, Operation.Removed);
            }
        }
        /// <summary>
        /// Get a report definition given the report name
        /// </summary>
        /// <param name="reportName"></param>
        /// <returns></returns>
        public static ReportDefinition GetReportDefinition(string reportName)
        {
            ReportDefinition theReportDefinition = TheReportDefinitions.Find(delegate(ReportDefinition reportDefinition)
            {
                return reportDefinition.Name == reportName;
            });
            return theReportDefinition;
        }

        /// <summary>
        /// The list of handlers for getting notified when strings are added and deleted
        /// </summary>
        static public event AddedRemovedHandler AddedRemoved
        {
            add { _theEventHandlers.AddHandler(_theAddRemoveEventKey, value); }
            remove { _theEventHandlers.RemoveHandler(_theAddRemoveEventKey, value); }
        }

        static public void LoadPersistence()
        {

            // Remove existing ones
            while (_theReportDefinitions.Count > 0)
            {
                Remove(_theReportDefinitions[0]);
            }

            List<ReportDefinition> thePersistedReportDefinitions = Persistence.Get(_theReportDefinitionsPersistenceKey) as List<ReportDefinition>;
            if (thePersistedReportDefinitions != null)
            {
                foreach (ReportDefinition theReportDefinition in thePersistedReportDefinitions)
                {
                    // Reset properties; we want to have a brand new reports loaded from persistence 
                    theReportDefinition.ResetProperty();
                    //theReportDefinition.SetProperty(ReportDefinition.START_TIME, null);
                    //theReportDefinition.SetProperty(ReportDefinition.ACTUAL_QUERY, null);
                    if (String.IsNullOrEmpty(theReportDefinition.Name))
                    {
                        // Fillin a new name if a blank name is saved by an older version.
                        theReportDefinition.Name = SimpleReportDefinition.CreateNewName("");
                    }
                    Add(theReportDefinition);
                }
                return;
            }

            // We'll need at least an empty list from here on
            TheReportDefinitions = new List<ReportDefinition>();

            // TODO - Check to see if we are upgrading the old style of persistence.  If so, they will end up saved in
            // the new style so this code can go away fairly soon.
            List<string> thePersistedQueryStrings = Persistence.Get(_theQueryStringsPersistenceKey) as List<string>;
            if (thePersistedQueryStrings != null)
            {

                foreach (string theString in thePersistedQueryStrings)
                {

                    // Make an unnamed simple report definition
                    SimpleReportDefinition theSimpleReportDefinition = new SimpleReportDefinition("");

                    // Set the string
                    theSimpleReportDefinition.QueryText = theString;

                    // Fire event as we add each
                    Add(theSimpleReportDefinition);

                }
            }
        }

        static public void SavePersistence(Dictionary<string, object> aDictionary)
        {
            aDictionary[_theReportDefinitionsPersistenceKey] = _theReportDefinitions;
        }

        static private void FireAddedRemoved(ReportDefinition aReportDefinition, Operation aReportDefinitionsOperation)
        {
            AddedRemovedHandler theChangedHandlers = (AddedRemovedHandler)_theEventHandlers[_theAddRemoveEventKey];

            if (theChangedHandlers != null)
            {
                theChangedHandlers(aReportDefinition, aReportDefinitionsOperation);
            }
        }

    }
}
