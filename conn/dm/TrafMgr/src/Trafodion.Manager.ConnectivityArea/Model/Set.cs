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
using System.Linq;
using System.Text;

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// Represents a set statement in a Datasource
    /// </summary>
    public class Set
    {
        #region Member variables
        private String _theName = "";
        private String _theValue = "";
        public const String Command = "SET ";
        #endregion Member variables

        #region Properties
        /// <summary>
        /// The value of the Set statement
        /// </summary>
        public String Value
        {
            get { return _theValue; }
            set { _theValue = value; }
        }

        /// <summary>
        /// The name of the set statement
        /// </summary>
        public String Name
        {
            get { return _theName; }
            set { _theName = value; }
        }
        #endregion Properties
        
        #region Constructors
        /// <summary>
        /// Default constructor.
        /// </summary>
        public Set()
        {
        }

        /// <summary>
        /// Creates a new Set and copies another Set's attributes into the new one.
        /// param set The set whose attributes will be copied.
        /// </summary>
        /// <param name="set"></param>
        public Set(Set set)
        {
            Copy(set);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="set"></param>
        public Set(String set)
        {
            // Strip out the text to the left of the "=" and split the string into
            // the define's name and attribute.
            String temp = set.Trim().Substring(4);
            String[] setDef = temp.Trim().Split(new char[]{' '}, 2);

            // Clean up any unnecessary white space.
            for (int i = 0; i < setDef.Length; i++)
            {
                setDef[i].Trim();
            }

            Name = setDef[0];
            Value = setDef[1];
        }
        #endregion Constructors

        #region Public methods
        /// <summary>
        /// Copies the attributes of another set.
        /// </summary>
        /// <param name="set"></param>
        public void Copy(Set set)
        {
            Name = set.Name;
            Value = set.Value;
        }
        /// <summary>
        /// Overrides the default Equals method
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public override bool Equals(object obj)
        {
            Set set = obj as Set;
            if (set != null)
            {
                if (NDCSObject.AreStringsEqual(this.Value, set.Value)
                    && NDCSObject.AreStringsEqual(this.Name, set.Name))
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        ///Generates a string representation of this object.
        /// </summary>
        /// <returns></returns>
        public override String ToString()
        {
            return String.Format("{0} {1} {2}", Command, Name, Value);
        }

        /// <summary>
        /// Creates a SET statement for NDCS
        /// </summary>
        /// <param name="name"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        static public String GenerateSetStmt(String name, String value)
        {
            // The value must be within single quotes.
            string setCommand = String.Format("{0} {1} {2}", Command, name, value) ;
            return NDCSName.EscapeSingleQuotes(setCommand);
        }

        /// <summary>
        /// Creates a SET statement for NDCS
        /// </summary>
        /// <param name="set"></param>
        /// <returns></returns>
        static public String GenerateSetStmt(Set set)
        {
            return GenerateSetStmt(set.Name, set.Value);
        }

        /// <summary>
        /// Returns an list of SET statements for NDCS
        /// </summary>
        /// <param name="sets"></param>
        /// <returns></returns>
        static public String[] GenerateSetStmts(List<Set> sets)
        {
            String[] stmts = new String[sets.Count];
            int i = 0;
            foreach (Set set in sets)
            {
                stmts[i++] = GenerateSetStmt(set);
            }

            return stmts;
        }
        #endregion Public methods
    }
}
