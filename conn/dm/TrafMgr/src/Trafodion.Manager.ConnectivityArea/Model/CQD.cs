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
using System.Text.RegularExpressions;

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// Represents a CQD object
    /// </summary>
    public class CQD
    {
        #region member variables
        public  const String Command = "CONTROL QUERY DEFAULT";
        private const String CONTROL = "CONTROL";
        private const String QUERY = "QUERY";
        private const String DEFAULT = "DEFAULT";
        private const String RESET = "RESET";

        private static Dictionary<string, ReservedCQD> RESERVED_CQD_NAMES = new Dictionary<string, ReservedCQD>();
        
        private String _theAttribute = "";
        private String _theValue = "";
        private bool _theSystemDefault = true;
        #endregion

        #region properties
        /// <summary>
        /// Indicates if the CQD uses System Default
        /// </summary>
        public bool SystemDefault
        {
          get { return _theSystemDefault; }
          set 
          { 
              _theSystemDefault = value;
              if (_theSystemDefault)
              {
                  _theValue = "";
              }
          }
        }

        /// <summary>
        /// The value of the CQD
        /// </summary>
        public String Value
        {
          get { return _theValue; }
          set 
          { 
              _theValue = value;
              if ((value != null) && (! value.Trim().Equals("")))
              {
                  _theSystemDefault = false;
              }
          }
        }

        /// <summary>
        /// The CQD name
        /// </summary>
        public String Attribute
        {
          get { return _theAttribute; }
          set { _theAttribute = value; }
        }
        #endregion Properties

        #region Constructor
        /// <summary>
        /// The default constructor
        /// </summary>
        public CQD()
        {
        }        

        /// <summary>
        /// Creates a new control query default by parsing the string passed.        
        /// </summary>
        /// <param name="aControl"></param>
        public CQD(String aControl)
        {
            this.parseControlStatement(aControl);
        }        
        #endregion

        #region Public methods
        /// <summary>
        ///    Adds CQD attribute names to a list of reserved CQD names.
        /// </summary>
        /// <param name="names">The names of the CQD which will be considered as reserved
        /// names. If null or an empty array, the function will do nothing. In
        /// addition, any null or empty string will be ignored.</param>
        /// <param name="info">A string that will associate information with this grouping
        /// of CQDs</param>        
        static public void addReservedCQDNames(List<string> names, Object info)
        {
            // Ignoring null and empty arrays.
            if (names == null || names.Count == 0)
                return;
            
            foreach (string name in names)
            {
                RESERVED_CQD_NAMES.Add(GetUppercaseName(name), new ReservedCQD(GetUppercaseName(name), info));
            }
        }

        /// <summary>
        /// Returns the list of reserved CQD names
        /// </summary>
        /// <returns></returns>
        static public List<string> getReservedCQDNames()
        {
            List<string> reservedCQDNames = new List<string>();
            if (RESERVED_CQD_NAMES != null)
            {
                foreach (KeyValuePair<string, ReservedCQD> kvp in RESERVED_CQD_NAMES)
                {
                    reservedCQDNames.Add(kvp.Key);
                }
            }
            return reservedCQDNames;
        }

        /// <summary>
        /// Looks the name parameter up in a list of reserved CQD names and indicates
        /// if the CQD is reserved.
        /// </summary>
        /// <param name="name">The name to look up.</param>
        /// <returns>the name is reserved, false if not reserved.</returns>
        static public bool isReservedCQDName(String name)
        {
            // Use the US local since CQD names are based upon US spelling standard.
            return RESERVED_CQD_NAMES.Keys.Contains(GetUppercaseName(name));
        }

        /// <summary>
        /// Given a string determines if it starts with control query default
        /// </summary>
        /// <param name="aControl"></param>
        /// <returns></returns>
        static public bool IsCQD(string aControl)
        {
            Regex r = new Regex("\\s*CONTROL\\s*QUERY\\s*DEFAULT\\s*.*");
            if ((aControl != null) && (r.IsMatch(aControl)))
            {
                return true;
            }
            return false;
        }

        /// <summary>
        /// Creates a CQD statement for NDCS
        /// </summary>
        /// <param name="attribute"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        static public String GenerateControlStmt(String attribute, String value)
        {
            // The value must be within single quotes.
            return String.Format("{0} {1} {2}",Command ,attribute.Trim(), DelimitedValue(value.Trim()));
        }

        /// <summary>
        /// Delimit the value as needed so the values are enclosed in doubled single quotes
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        static public string DelimitedValue(string value)
        {
            //value is already enclosed within doubled singe quotes,
            //it is already delimited return as is
            if (value.StartsWith("''") && value.EndsWith("''"))
                return value;

            //If value is enclosed within single quotes
            //escape with additional single quotes
            if (value.StartsWith("'") && value.EndsWith("'"))
                return "'" + value + "'";

            //value does not have any quotes. enclose with doubled single quotes
            return "''" + value + "''";
        }

        /// <summary>
        /// Creates a CQD statement for NDCS
        /// </summary>
        /// <param name="cqd"></param>
        /// <returns></returns>
        static public String GenerateControlStmt(CQD cqd)
        {
            return GenerateControlStmt(cqd.Attribute, cqd.Value);
        }

        /// <summary>
        /// Returns an array of CQD statements for NDCS
        /// </summary>
        /// <param name="cqds"></param>
        /// <returns></returns>
        static public String[] GenerateControlStmts(CQD[] cqds)
        {
            String[] controls = new String[cqds.Length];
            for (int i = 0; i < cqds.Length; i++)
            {
                controls[i] = GenerateControlStmt(cqds[i]);
            }

            return controls;
        }

        /// <summary>
        /// Copies the valuse from the passed CQD to the current object
        /// </summary>
        /// <param name="cqd"></param>
        public void copy(CQD cqd)
        {
            Value = cqd.Value;
            SystemDefault = cqd.SystemDefault;
        }

        /// <summary>
        /// Overrides the default Equals method
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public override bool Equals(object obj)
        {
            CQD cqd = obj as CQD;
            if (cqd != null)
            {
                if (NDCSObject.AreStringsEqual(this.Attribute, cqd.Attribute)
                    && (this.SystemDefault == cqd.SystemDefault)
                    && NDCSObject.AreStringsEqual(this.Value, cqd.Value))
                {
                    return true;
                }
            }
            return false;
        }

        #endregion public methods

        #region private methods
        /// <summary>
        /// 
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        private static string GetUppercaseName(string name)
        {
            return name.ToUpper();
        }
        /// <summary>
        ///       
        /// Expected Statement Syntax:
        ///      CONTROL QUERY DEFAULT attribute 'attr-value'
        /// Tokens: 0      1      2        3        ?
        /// 
        /// ?: This can be token 4, however if the value is whitespace, then
        ///    this token will not exist.
        ///         
        /// </summary>
        /// <param name="aControl"></param>
        private void parseControlStatement(String aControl)
        {
            if (!IsCQD(aControl))
            {
                return;
            }

            string[] tokens = aControl.Trim().Split(new string[] { Command }, StringSplitOptions.RemoveEmptyEntries);

            if (tokens.Length > 0)
            {
                String[] cqdAttributeValue = tokens[0].Trim().Split(new char[] { ' ' }, 2);

                _theAttribute = cqdAttributeValue[0].Trim();

                if (cqdAttributeValue.Length > 1)
                {
                    Value = cqdAttributeValue[1];
                    int idx = Value.IndexOf("'");
                    if (idx >= 0)
                    {
                        int lastIdx = Value.LastIndexOf("'");
                        if (lastIdx > 0)
                        {
                            int startIdx = idx + 1;
                            Value = Value.Substring(startIdx, lastIdx - startIdx);
                        }
                    }
                    _theSystemDefault = false;
                }
            }
        }
        #endregion private methods
    }

    /// <summary>
    /// Represents a class for Reserved CQDs
    /// </summary>
    class ReservedCQD
    {
        public String _theName;
        public Object _theInfo;
        
        public ReservedCQD(String name, Object info)
        {
            _theName = name;
            _theInfo = info;
        }
    }

}
