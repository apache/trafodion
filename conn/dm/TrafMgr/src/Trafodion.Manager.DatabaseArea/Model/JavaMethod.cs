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
using System.Collections;

namespace Trafodion.Manager.DatabaseArea.Model
{

    /// <summary>
    /// A Java method with the name and signature
    /// </summary>
    public class JavaMethod
    {
        #region Private Member variables
        
        string _theMethodName;
        string _theMethodSignature;
        string _theRawMethodSignature;
        List<JavaParameter> _parameters = null;
        Hashtable parameterNamesHashTable;

        #endregion Private Member variables

        #region Public Properties
        public string MethodName
        {
            get { return _theMethodName; }
            set { _theMethodName = value; }
        }

        public string MethodSignature
        {
            get { return _theMethodSignature; }
            set 
            { 
                _theMethodSignature = value;
                populateParameters();
            }
        }

        public string RawMethodSignature
        {
            get { return _theRawMethodSignature; }
            set { _theRawMethodSignature = value; }
        }

        public List<JavaParameter> Parameters
        {
            get 
            {
                if (_parameters == null)
                    populateParameters();
                return _parameters; 
            }
        }

        #endregion Public Properties

        #region Private Methods

        private void populateParameters()
        {
            _parameters = new List<JavaParameter>();
            parameterNamesHashTable = new Hashtable();

            if (!String.IsNullOrEmpty(_theMethodSignature))
            {
                string[] signatureStringArray = _theMethodSignature.Split(new char[] { ',' });
                foreach (string currString in signatureStringArray)
                {
                    _parameters.Add(new JavaParameter(getName(currString), currString.Trim()));
                }
            }

            parameterNamesHashTable.Clear();
        }


        //Given a class name, returns the prefix that can be used as 
        //a param name. The following rules as used
        // 1. The package is stripped
        // 2. The first character is converted to lower case
        // 3. The brackets [] for the array are stripped.
        // e.g. lava.lang.String[] will return string
        private string getParamNamePrefix(string paramType)
        {
            string[] paramParts = paramType.Split( new char[] {'.'});
            string prefix = paramParts[paramParts.Length - 1];
            
            // check if it's a primitive type
            string theName = DataTypeHelper.PrimitivePrefixes[prefix] as string;
            if (theName != null)
            {
                prefix = theName;
            }
            else
            {
                if (prefix.IndexOf("[]") >= 0)
                {
                    prefix = prefix.Substring(0 , prefix.IndexOf("[")) + "_ARRAY";
                }
                if (prefix.Length > 1)
                {
                        prefix = prefix.Substring(0, 1) +  prefix.Substring(1);
                }
                else
                {
                    prefix = prefix.Substring(0, 1);
                }
            }
            return prefix.ToUpper();
        }
    

        private string getName(string type)
        {
            // Trim whitespace
            string theName = "";
            if (type.Length < 1)
            {
                return theName;
            }

            theName = getParamNamePrefix(type.Trim());

            if (parameterNamesHashTable.ContainsKey(theName))
            {
                int typeCount = (int) parameterNamesHashTable[theName];
                parameterNamesHashTable[theName] = typeCount + 1;
                return theName + ((int)parameterNamesHashTable[theName]).ToString();
            }
            else
            {
                parameterNamesHashTable.Add(theName, 1);
                return theName + "1";
            }
            
        }

        #endregion Private Methods

        public JavaMethod(string name, string signature)
        {
            MethodName = name;
            MethodSignature = signature;
        }
        public JavaMethod(string name, string rawSignature, string signature)
        {
            MethodName = name;
            MethodSignature = signature;
            RawMethodSignature = rawSignature;
        }
    }
}
