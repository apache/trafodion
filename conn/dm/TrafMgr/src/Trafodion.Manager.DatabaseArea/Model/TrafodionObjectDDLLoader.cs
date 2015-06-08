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

using System.Data.Odbc;
using System.Text;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// This Class has methods to load DDL text for a sql object
    /// </summary>
    public class TrafodionObjectDDLLoader
    {
        /// <summary>
        /// Given a Sql object instance, this method loads the DDL text for it by invoking
        /// the SHOWDDL sql command
        /// </summary>
        /// <param name="aTrafodionObject">The sql object whose ddl needs to be loaded</param>
        /// <returns>The DDL text as string</returns>
        public static string LoadDDL(TrafodionObject aTrafodionObject)
        {
            Connection connection = null;
            string ddlText = "";

            try
            {
                connection = aTrafodionObject.GetConnection();
                OdbcDataReader reader = Queries.ExecuteShowDDL(connection, aTrafodionObject);
                StringBuilder ddlBuilder = new StringBuilder();

                //Read each line from the SHOWDDL output and append to a string builder
                while (reader.Read())
                {
                    ddlBuilder.AppendLine(reader.GetString(0));
                }
                ddlText = ddlBuilder.ToString();
            }
            finally
            {
                if (connection != null)
                    connection.Close();
            }

            return ddlText;
        }

        /// <summary>
        /// Extracts the DDL for the child object from the Parent's DDL text.
        /// It uses a pattern to match the childs DDL
        /// </summary>
        /// <param name="parentDDLText">Parent object's DDL</param>
        /// <param name="childName">Name of the child object</param>
        /// <param name="patterns">Patterns used to match the child DDL</param>
        /// <returns></returns>
        public static string FindChildDDL(string parentDDLText, string childName, string[] patterns)
        {
            foreach (string pattern in patterns)
            {
                int startPosition = 0;
                bool done = false;

                while (!done)
                {
                    //Find all the occurrances of the patterns
                    startPosition = parentDDLText.IndexOf(pattern, startPosition);
                    if (startPosition > 0)
                    {
                        int endClausePosition = FindClauseEndPosition(parentDDLText, startPosition);
                        if (endClausePosition == parentDDLText.Length)
                        {
                            //We've reached the end of the parent DDL text.
                            done = true;
                        }

                        //There could be comments before the CREATE so go to the beginning of the line 
                        string tempstr = parentDDLText.Substring(0, startPosition);
                        int lineBegin = tempstr.LastIndexOf("\n");

                        if (lineBegin >= 0)
                        {
                            startPosition = startPosition < lineBegin ? startPosition : lineBegin;
                        }

                        string fragment = parentDDLText.Substring(startPosition, endClausePosition - startPosition);
                        
                        if (IsChildDDLInFragment(fragment, pattern, childName))
                        {
                            //Found it, now prepare to return it.
                            if (fragment.Trim().Length > 0)
                            {
                                //Remove these comments, there are for the parents.
                                fragment = fragment.Replace("--  Table has an IUD log.", "");
                                fragment = fragment.Replace("--  Materialized view has an IUD log.", "");
                                return fragment;
                            }
                        }

                        startPosition = endClausePosition;
                    }
                    else
                    {
                        done = true;
                    }
                }
            }
            return "";
        }

        #region Private Methods

        /// <summary>
        /// Finds the end position of a Create DDL fragment.  The end position could be either the beginning of the next
        /// CREATE, ALTER, or GRANT verb or just the end of the given CREATE fragment.
        /// </summary>
        /// <param name="aDDLFragment">The parent DDL fragment</param>
        /// <param name="start">The start position of the current Create fragment</param>
        /// <returns></returns>
        static private int FindClauseEndPosition(string aDDLFragment, int start)
        {
            //Search if there is another CREATE clause in the  fragment
            int nextCreateStartPosition = aDDLFragment.IndexOf("\nCREATE", start + 5);
            if (nextCreateStartPosition < 0)
                nextCreateStartPosition = aDDLFragment.Length;

            //Search if there is another ALTER clause in the parent DDL
            int nextAlterStartPosition = aDDLFragment.IndexOf("\nALTER", start + 5);
            if (nextAlterStartPosition < 0)
                nextAlterStartPosition = aDDLFragment.Length;

            //Eliminate GRANT statements from the child ddl
            int nextGrantStartPosition = aDDLFragment.IndexOf("\nGRANT", start + 5);
            if (nextGrantStartPosition < 0)
                nextGrantStartPosition = aDDLFragment.Length;

            //Determine the end position based on the next alter or create which ever comes first
            int endPosition = nextAlterStartPosition <= nextCreateStartPosition ? nextAlterStartPosition : nextCreateStartPosition;
            endPosition = endPosition <= nextGrantStartPosition ? endPosition : nextGrantStartPosition;

            return endPosition;
        }

        /// <summary>
        /// Checks to see if a Create pattern is in the given fragment.  Since spaces are allowed in many places in the DDL text,
        /// looking up names would have to remove all spaces first.  For example, a DDL could be like,
        ///     CREATE  TRIGGER 
        ///     TRAFODION
        ///     .
        ///         MYSCHEMA
        ///     . MYTRIGGER
        ///     
        ///     BEFORE ...
        ///     
        /// So, to search for the pattern of "CREATE TRIGGER TRAFODION.MYSCHEMA.MYTRIGGER" would need to consolidate all 
        /// spaces on both the DDL fragment and the pattern itself. 
        /// </summary>
        /// <param name="aFragment">The given DDL fragment</param>
        /// <param name="pattern">A specific Create pattern</param>
        /// <param name="childName">A given child name</param>
        /// <returns></returns>
        static private bool IsChildDDLInFragment(string aFragment, string pattern, string childName)
        {
            string tempStr = aFragment.Replace(" ", "").Replace("\n","");
            string ptr = pattern.Replace(" ", "") + childName.Replace(" ", "");
            if (tempStr.IndexOf(ptr) >= 0)
            {
                return true;
            }

            return false;
        }

        #endregion Private Methods

    }
}
