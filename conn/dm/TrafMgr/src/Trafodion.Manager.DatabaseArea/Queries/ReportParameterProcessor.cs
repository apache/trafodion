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

using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Queries
{
    public class ReportParameterProcessor : Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase
    {
        //[Note] The following constants have been moved to Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.
        //public const int MAX_PARAMS_TO_STORE = 10;
        //public const string REPORT_PARAMS = "REPORT_PARAMS";

        //public const string TIME_ZONE_OFFSET = "TIMEZONE_OFFSET";
        //public const string SEARCH_DSN = "SEARCH_DSN";
        //public const string RUN_TIME = "RUN_TIME";
        //public const string RECORD_COUNT = "RECORD_COUNT";
        //public const string CLIENT_ID = "CLIENT_ID";
        //public const string FROM_TIME = "__FROM_TIME";
        //public const string TO_TIME = "__TO_TIME";
        //public const string CONNECTED_DSN = "__CONNECTED_DSN";
        //public const string LOGGEDON_USER_ID = "__LOGGEDON_USER_ID";

        //public const string PARAMETER_NAME_TOKEN = "PARAMETER_NAME";
        //public const string DISPLAY_NAME_TOKEN = "DISPLAY_NAME";
        //public const string TOOLTIP_TOKEN = "TOOLTIP";
        //public const string LEGAL_VALUES_TOKEN = "LEGAL_VALUES";
        //public const string EXAMPLE_TOKEN = "EXAMPLE";
        //public const string CATALOG_NAME = "CATALOG_NAME";
        //public const string SCHEMA_NAME = "SCHEMA_NAME";
        //public const string SESSION_NAME = "__SESSION_NAME";
        //public const string REPORT_NAME = "__REPORT_NAME";
        //public const string ROW_COUNT = "__ROW_COUNT";
        //public const string TIME_RANGE_KEY = "TIME_RANGE_KEY";
        //public const string SYSTEM_CATALOG_NAME = "SYSTEM_CATALOG_NAME";
        //public const string SYSTEM_CATALOG_VERSION = "SYSTEM_CATALOG_VERSION";

        //private static string commentStart = "/*QueryDocumentationStart";
        //private static string commentEnd = "QueryDocumentationEnd*/";

        private static ReportParameterProcessor instance;

        private static Dictionary<string, ReportParameter> _theGlobalParameters = new Dictionary<string, ReportParameter>();

        public static ReportParameterProcessor Instance
        {
            get { return instance; }
        }

        static ReportParameterProcessor()
        {
            instance = new ReportParameterProcessor();
            LoadPersistence();
            Persistence.PersistenceHandlers += new Persistence.PersistenceHandler(PersistencePersistenceHandlers);
        }

        static void LoadPersistence()
        {
            _theGlobalParameters = Persistence.Get(thePersistenceKey) as Dictionary<string, ReportParameter>;
            if (_theGlobalParameters == null)
            {
                _theGlobalParameters = new Dictionary<string, ReportParameter>();
            }
        }

        private ReportParameterProcessor() {}

        private static readonly string thePersistenceKey = "QueryParametersPersistence";

        private static void PersistencePersistenceHandlers(Dictionary<string, object> aDictionary, Persistence.PersistenceOperation aPersistenceOperation)
        {
            switch (aPersistenceOperation)
            {
                case Persistence.PersistenceOperation.Load:
                    {
                        LoadPersistence();
                        break;
                    }
                case Persistence.PersistenceOperation.Save:
                    {
                        aDictionary[thePersistenceKey] = _theGlobalParameters;
                        break;
                    }
            }
        }

        //Given the aReportDefinition meta-data, finds out all the parameters needed by the query
        //to execute, pre-populates it based on any pre-serialized values and returns
        //it.
        public List<ReportParameter> getReportParams(ReportDefinition reportDefinition)
        {
            List<ReportParameter> parameters = extractParams(reportDefinition);
            prepopulateParams(parameters);
            decorateParams(parameters, reportDefinition);
            return parameters;
        }//getReportParams

        //method display the params to the user and get it populated.  A dialog will be
        //displayed that will allow the user to provide the values of the params
        public bool populateParamsFromUser(List<ReportParameter> parameters, Hashtable predefinedParameters)
        {
            bool needDialog = populateDefinedParameters(parameters, predefinedParameters);
            if (needDialog)
            {
                ReportParamDialog paramDialog = new ReportParamDialog(parameters, true);
                return (paramDialog.ShowDialog() == DialogResult.OK);
            }
            else
            {
                return true;
            }
        }//populatedParamsFromUser

        public bool populateDefinedParameters(List<ReportParameter> parameters, Hashtable predefinedParameters)
        {
            bool needDialog = false;

            foreach (ReportParameter parameter in parameters)
            {
                if (predefinedParameters.ContainsKey(parameter.ParamName))
                {
                    parameter.Value = predefinedParameters[parameter.ParamName];
                }
                else
                {
                    needDialog = true;
                }
            }

            return needDialog;
        }

        //Given the populated aReportDefinition parameters, persists them in the local store
        public void persistReportParams(List<ReportParameter> parameters)
        {
            if (parameters != null)
            {
                foreach (ReportParameter parameter in parameters)
                {
                    persistReportParam(parameter);
                }
            }
        }

        public void persistReportParam(ReportParameter parameter)
        {
            ReportParameter storedParam = null;
            if ((parameter.Value != null) && (parameter.Value.ToString().Trim().Length > 0))
            {
                if (_theGlobalParameters.ContainsKey(parameter.ParamName))
                {
                    storedParam = _theGlobalParameters[parameter.ParamName];
                    storedParam.Value = parameter.Value;
                    if (storedParam.PossibleValues != null)
                    {
                        int idx = storedParam.PossibleValues.IndexOf(parameter.Value);
                        if (idx < 0)
                        {
                            storedParam.PossibleValues.Insert(0, parameter.Value);
                        }
                        else
                        //the parameter was used earlier, hence bring it to the top
                        {
                            storedParam.PossibleValues.RemoveAt(idx);
                            storedParam.PossibleValues.Insert(0, parameter.Value);
                        }
                        //trim what ever is beyond the max number of parameters used
                        if (storedParam.PossibleValues.Count > MAX_PARAMS_TO_STORE)
                        {
                            storedParam.PossibleValues.RemoveRange(MAX_PARAMS_TO_STORE - 1,
                                (storedParam.PossibleValues.Count - MAX_PARAMS_TO_STORE));
                        }
                    }
                    else
                    {
                        storedParam.PossibleValues = new ArrayList();
                        storedParam.PossibleValues.Insert(0, parameter.Value);
                    }
                }
                else
                {
                    parameter.PossibleValues = new ArrayList();
                    parameter.PossibleValues.Insert(0, parameter.Value);
                    _theGlobalParameters.Add(parameter.ParamName, parameter);
                }
            }
        }


        //This method takes in a list of unpopulated parameters and returns a list of
        //populated parameters
        private void prepopulateParams(List<ReportParameter> parameters)
        {
            foreach (ReportParameter parameter in parameters)
            {
                prepopulateParam(parameter);
            }
        }//prepopulateParams

        public void prepopulateParam(ReportParameter parameter)
        {
            ReportParameter storedParam = null;
            if (_theGlobalParameters.ContainsKey(parameter.ParamName))
            {
                storedParam = _theGlobalParameters[parameter.ParamName];
                if ((storedParam != null) &&
                (storedParam.PossibleValues != null) &&
                (storedParam.PossibleValues.Count > 0))
                {
                    if (parameter.PossibleValues == null)
                    {
                        parameter.PossibleValues = new ArrayList();
                    }
                    parameter.Value = storedParam.Value;
                    foreach (object tempValue in storedParam.PossibleValues)
                    {
                        parameter.PossibleValues.Add(tempValue);
                    }
                }
            }
        }

        private void decorateParams(List<ReportParameter> parameters, ReportDefinition reportDefinition)
        {

            if (parameters != null)
            {
                foreach (ReportParameter parameter in parameters)
                {
                    if (parameter.ParamName != null)
                    {
                        if (parameter.ParamName.Equals(TIME_ZONE_OFFSET))
                        {
                            parameter.DisplayName = "Timezone Offset";
                            parameter.Description = "The offset in hours between Local Time and UTC (also known as GMT)";
                        }
                        else if (parameter.ParamName.Equals(TO_TIME))
                        {
                            parameter.DisplayName = "To Time";
                            parameter.Description = "The end time of the range to be returned by this query";
                            parameter.SqlDataType = "java.sql.Timestamp";
                        }
                        if (parameter.ParamName.Equals(FROM_TIME))
                        {
                            parameter.DisplayName = "From Time";
                            parameter.Description = "The start time of the range to be returned by this query";
                            parameter.SqlDataType = "java.sql.Timestamp";
                        }
                        if (parameter.ParamName.Equals(SEARCH_DSN))
                        {
                            parameter.DisplayName = "Search DSN";
                            parameter.Description = "Search DSN";
                        }
                        if (parameter.ParamName.Equals(RUN_TIME))
                        {
                            parameter.DisplayName = "Run Time";
                            parameter.Description = "The minimum elapsed query time in minutes to be used for results in this query";
                        }
                        if (parameter.ParamName.Equals(RECORD_COUNT))
                        {
                            parameter.DisplayName = "Record Count";
                            parameter.Description = "The maximum number of rows to be returned by this query";
                        }
                        if (parameter.ParamName.Equals(CLIENT_ID))
                        {
                            parameter.DisplayName = "Client Id";
                            parameter.Description = "The Client Id";
                        }
                    }
                }
            }

            if (reportDefinition != null)
            {
                //get the aReportDefinition type
                int reportType = ReportDefinition.TEXT_REPORT;
                if (reportDefinition.GetProperty(ReportDefinition.TYPE) != null)
                {
                    reportType = (int)(reportDefinition.GetProperty(ReportDefinition.TYPE));
                }
                //extract parameters based on the aReportDefinition type
                switch (reportType)
                {
                    case ReportDefinition.TEXT_REPORT:
                        {
                            if (parameters != null)
                            {
                                ParameterDocumentationReader docReader = new ParameterDocumentationReader();
                                docReader.populateParamsFromComment(reportDefinition, parameters);
                            }
                        }
                        break;
                    case ReportDefinition.CRYSTAL_REPORT:
                        {
                        }
                        break;
                }
            }

        }


        //obtains the DEFINITION property of the ReportDefinition and figures out the
        //parameters based on the aReportDefinition type
        private List<ReportParameter> extractParams(ReportDefinition reportDefinition)
        {
            bool hasFrom = false;
            bool hasTo = false;

            List<ReportParameter> parameters = new List<ReportParameter>();
            if (reportDefinition != null)
            {
                //get the aReportDefinition type
                int reportType = ReportDefinition.TEXT_REPORT;
                if (reportDefinition.GetProperty(ReportDefinition.TYPE) != null)
                {
                    reportType = (int)(reportDefinition.GetProperty(ReportDefinition.TYPE));
                }
                //extract parameters based on the aReportDefinition type
                switch (reportType)
                {
                    case ReportDefinition.TEXT_REPORT:
                        {
                            string query = (string)reportDefinition.GetProperty(ReportDefinition.DEFINITION);
                            if (query != null)
                            {

                                // Split the query text on the parameter delimiters so that we get alternations of text and paramter name fragments
                                string[] theParts = Regex.Split(query, "\\$\\$");

                                // Holds unique parameter names in this query
                                Hashtable theCurrentParameters = new Hashtable();

                                // Get the unique parameter names
                                for (int theIndex = 0, theCount = theParts.Length; theIndex < theCount; theIndex++)
                                {
                                    // The odd numbered fragments are parameter names
                                    if ((theIndex & 1) != 0)
                                    {
                                        // Insert name into hashmap ... no-op if already there
                                        if (!theCurrentParameters.ContainsKey(theParts[theIndex]))
                                        {
                                            theCurrentParameters.Add(theParts[theIndex], new string(new char[] { }));
                                        }
                                    }
                                }

                                ReportParameter parameter = null;
                                foreach (string paramName in theCurrentParameters.Keys)// Iterator i = theCurrentParameters.keySet().iterator(); i.hasNext();)
                                {
                                    hasFrom = (hasFrom || paramName.Equals(ReportParameterProcessor.FROM_TIME)) ? true : false;
                                    hasTo = (hasTo || paramName.Equals(ReportParameterProcessor.TO_TIME)) ? true : false;

                                    parameter = new ReportParameter();
                                    parameter.ParamName = paramName;
                                    //The following have to be cleansed in some way
                                    parameter.DisplayName = paramName;
                                    parameter.Description = paramName;
                                    parameters.Add(parameter);
                                }
                                if (hasFrom && hasTo)
                                {
                                    parameter = new ReportParameter();
                                    parameter.ParamName = ReportParameterProcessor.TIME_RANGE_KEY;
                                    parameter.IsInternal = true;
                                    //The following have to be cleansed in some way
                                    parameter.DisplayName = ReportParameterProcessor.TIME_RANGE_KEY;
                                    parameter.Description = ReportParameterProcessor.TIME_RANGE_KEY;
                                    parameters.Add(parameter);
                                }

                            }
                        }
                        break;
                }
            }

            parameters.Sort();

            return parameters;
        }//extractParams

        //method to obtain the raw query
        public string getRawQuery(ReportDefinition reportDefinition)
        {
            string query = null;
            if (reportDefinition != null)
            {
                //get the aReportDefinition type
                int reportType = ReportDefinition.TEXT_REPORT;
                if (reportDefinition.GetProperty(ReportDefinition.TYPE) != null)
                {
                    reportType = (int)(reportDefinition.GetProperty(ReportDefinition.TYPE));
                }
                //extract parameters based on the aReportDefinition type
                switch (reportType)
                {
                    case ReportDefinition.TEXT_REPORT:
                        {
                            query = (string)reportDefinition.GetProperty(ReportDefinition.DEFINITION);
                        }
                        break;
                }
            }
            return query;
        }//getRawQuery

        //method to obtain the processed SQL that has been updated with the parameter
        public string getQueryForExecution(string rawQuery, List<ReportParameter> parameters)
        {
            string theExecutableQuery = rawQuery;
            //Remove all lines that start with -- till the end of the line
            theExecutableQuery = Regex.Replace(theExecutableQuery, "[ ]*-{2}.*[\\r\\n]", "");

            //remove the block comment
            theExecutableQuery = removeBlockComment(theExecutableQuery);

            //Populate query with parameters for execution
            theExecutableQuery = populateQueryWithParams(theExecutableQuery, parameters, false);

            return theExecutableQuery;
        }//getQueryForExecution

        //Method to remove the block comment from the definition
        // We let the sql parser complian about badly formatted block comments by not removing them here
        private string removeBlockComment(string rawQuery)
        {
            // TODO - string formatBars = "....V....1";
            int theStartIndex = rawQuery.ToUpper().IndexOf(commentStart.ToUpper());
            if (theStartIndex < 0)
            {
                return rawQuery;
            }

            int theEndIndex = rawQuery.ToUpper().IndexOf(commentEnd.ToUpper());
            if (theEndIndex < 0)
            {
                return rawQuery;
            }

            if (theStartIndex > theEndIndex)
            {
                return "";
            }

            StringBuilder theResult = new StringBuilder(rawQuery.Length);
            theResult.Append(rawQuery.Substring(0, theStartIndex));
            int theEndEnd = theEndIndex + commentEnd.Length;
            theResult.Append(rawQuery.Substring(theEndEnd, rawQuery.Length - theEndEnd));
            return theResult.ToString();
        }

        /// <summary>
        /// Obtain the block comment (the text between "/*QueryDocumentationStart" 
        /// and "QueryDocumentationEnd*/") from a statement.
        /// </summary>
        /// <param name="rawQuery">the statement</param>
        /// <returns>the block comment</returns>
        public string getBlockComment(string rawQuery)
        {
            int theStartIndex = rawQuery.ToUpper().IndexOf(commentStart.ToUpper());
            if (theStartIndex < 0)
            {
                return "";
            }

            int theEndIndex = rawQuery.ToUpper().IndexOf(commentEnd.ToUpper());
            if (theEndIndex < 0)
            {
                return "";
            }

            if (theStartIndex > theEndIndex)
            {
                return "";
            }

            int theStartEnd = theStartIndex + commentStart.Length;
            return rawQuery.Substring(theStartEnd, theEndIndex - theStartEnd);

        }

        //Method to obtain the query text populated with parameters
        private string populateQueryWithParams(string query, List<ReportParameter> parameters, bool forDisp)
        {
            string theName = null;
            string theValue = null;
            string queryCopy = query;

            foreach (ReportParameter parameter in parameters)
            {
                theName = parameter.ParamName;
                theValue = (parameter.Value != null) ? parameter.Value.ToString() : "";
                // Substitute this paramter with its value

                // Backslash and dollar must be escaped in the replacement string.  They are both common in Guardian names which are
                // common in data in the repository tables
                string theEscapedValue;
                {
                    int theCount = theValue.Length;
                    StringWriter sw = new StringWriter();
                    char[] chars = theValue.ToCharArray();
                    for (int theIndex = 0; theIndex < theCount; theIndex++)
                    {
                        char theChar = chars[theIndex];
                        sw.Write(theChar);
                    }
                    theEscapedValue = sw.ToString(); //The StringWriters ToString() escapes special characters
                }
                string replaceString = @"$$" + theName + "$$";
                if (forDisp)
                {
                    queryCopy = Regex.Replace(queryCopy, replaceString, "<strong bgcolor=yellow>" + theEscapedValue + "</strong>");
                }
                else
                {
                    //                    queryCopy = Regex.Replace(queryCopy, replaceString, theEscapedValue);
                    queryCopy = queryCopy.Replace(replaceString, theEscapedValue);
                }
            }
            return queryCopy;
        }//populateQueryWithParams


    }
}
