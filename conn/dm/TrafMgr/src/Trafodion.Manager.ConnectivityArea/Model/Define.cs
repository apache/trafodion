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
    /// A define for a Datasource
    /// </summary>
    public class Define
    {
        #region Member Variables
        private String _theName = "";
        private String _theAttribute = "";
        public const String Command = "DEFINE ";
        #endregion Member Variables

        #region Properties
        public String Attribute
        {
            get { return _theAttribute; }
            set { _theAttribute = value; }
        }
        public String Name
        {
            get { return _theName; }
            set { _theName = value; }
        }
        #endregion Properties


        #region Constructors
        /// <summary>
        /// Default constructor
        /// </summary>
        public Define()
        {
        }

        /// <summary>
        /// Creates a new Define and copies another Define's attributes into the new one.
        /// </summary>
        /// <param name="define"></param>
        public Define(Define define)
        {
            Copy(define);
        }
        
        /// <summary>
        ///Creates a new Define and populates the name and attribute fields. The 
        ///define needs to be in the format "ADD DEFINE ={name}, {value}".
        /// </summary>
        /// <param name="aDefineString">The define string to parse.</param>
        public Define(String aDefineString)
        {
            // Strip out the text to the left of the "=" and split the string into
            // the define's name and attribute.
            String temp = aDefineString.Substring(aDefineString.IndexOf("=") + 1);
            String[] defineDef = temp.Trim().Split(new Char[]{','}, 2);

            // Clean up any unnecessary white space.
            for (int i = 0; i < defineDef.Length; i++)
            {
                defineDef[i].Trim();
            }

            _theName = defineDef[0];
            _theAttribute = defineDef[1];
        }
        #endregion Constructors

        #region Public Methods
        /// <summary>
        /// Copies the attributes of another define.
        /// </summary>
        /// <param name="define">The define whose attributes will be copied</param>
        public void Copy(Define define)
        {
            _theName = define.Name;
            _theAttribute = define.Attribute;
        }
        /// <summary>
        /// Overrides the default Equals method
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public override bool Equals(object obj)
        {
            Define define = obj as Define;
            if (define != null)
            {
                if (NDCSObject.AreStringsEqual(this.Attribute, define.Attribute)
                    && NDCSObject.AreStringsEqual(this.Name, define.Name))
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
            return String.Format("ADD DEFINE ={0}, {1}", _theName, _theAttribute);
        }

        /// <summary>
        /// Creates a Define statement for NDCS
        /// </summary>
        /// <param name="name"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        static public String GenerateDefineStmt(String name, String attribute)
        {
            // The value must be within single quotes.
            return String.Format("ADD DEFINE ={0}, {1}", name, attribute); 
        }

        /// <summary>
        /// Creates a Define statement for NDCS
        /// </summary>
        /// <param name="define"></param>
        /// <returns></returns>
        static public String GenerateDefineStmt(Define define)
        {
            return GenerateDefineStmt(define.Name, define.Attribute);
        }

        /// <summary>
        /// Returns an list of Defines for NDCS
        /// </summary>
        /// <param name="defines"></param>
        /// <returns></returns>
        static public String[] GenerateDefineStmts(List<Define> defines)
        {
            String[] stmts = new String[defines.Count];
            int i = 0;
            foreach (Define define in defines)
            {
                stmts[i++] = GenerateDefineStmt(define);
            }

            return stmts;
        }
        #endregion Public Methods
    }
}
