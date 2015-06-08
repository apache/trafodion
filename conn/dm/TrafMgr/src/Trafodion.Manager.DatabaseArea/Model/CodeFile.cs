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
using Trafodion.Manager.DatabaseArea.Controls.Tree;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// The details of the files that are present on the server at a given location
    /// The connection definition has the user name and that will help us find the 
    /// logical location.
    /// </summary>
    public class CodeFile
    {
        #region Private member variables

        String _theName;
        String _theFullyQualifiedPath;
        long _theSize;
        long _theLastModifiedTime;
        List<string> _classNames = null;
        PCFModel.AccessLevel _accessLevel;

        #endregion Private member variables

        #region Public properties

        /// <summary>
        /// Name of the code file
        /// </summary>
        public String Name
        {
            get { return _theName; }
            set { _theName = value; }
        }

        /// <summary>
        /// Fully qualified path of the code file
        /// </summary>
        public String FullyQualifiedPath
        {
            get { return _theFullyQualifiedPath; }
            set { _theFullyQualifiedPath = value; }
        }
        /// <summary>
        /// Size of the file in bytes
        /// </summary>
        public long Size
        {
            get { return _theSize; }
            set { _theSize = value; }
        }

        /// <summary>
        /// Last modified timestamp of the file
        /// </summary>
        public long LastModifiedTime
        {
            get { return _theLastModifiedTime; }
            set { _theLastModifiedTime = value; }
        }

        /// <summary>
        /// Last modified timestamp formatted for display
        /// </summary>
        public DateTime FormattedModifiedTime
        {
            get
            {
                return GetFormattedTime(LastModifiedTime);
            }
        }
        /// <summary>
        /// Indicates if the code file is a class file
        /// </summary>
        public bool IsClass
        {
            get
            {
                if (_theName != null)
                {
                    return _theName.ToLower().EndsWith(".class");
                }
                return false;
            }
        }

        /// <summary>
        /// Indicates if the code file is a jar file
        /// </summary>
        public bool IsJar
        {
            get
            {
                if (_theName != null)
                {
                    return _theName.ToLower().EndsWith(".jar");
                }
                return false;
            }
        }

        /// <summary>
        /// List of class names contained in the code file
        /// </summary>
        public List<string> ClassNames
        {
            get { return _classNames; }
            set { _classNames = value; }
        }

        public PCFModel.AccessLevel AccessLevel
        {
            get { return _accessLevel; }
        }

        #endregion Public properties


        /// <summary>
        /// Default constructor
        /// </summary>
        public CodeFile()
        { 
        }

        /// <summary>
        /// Constructs the code file object
        /// </summary>
        /// <param name="aName">Name of the code file</param>
        /// <param name="aSize">Size of the code file</param>
        /// <param name="aLastModifiedTime">Last modified timestamp of the file</param>
        public CodeFile(string aName, long aSize, long aLastModifiedTime, PCFModel.AccessLevel accessLevel)
        {
            this._theName = aName;
            this._theSize = aSize;
            this._theLastModifiedTime = aLastModifiedTime;
            this._accessLevel = accessLevel;
        }

        /// <summary>
        /// Formats the datetime to a human readable form
        /// </summary>
        /// <param name="aTime"></param>
        /// <returns></returns>
        public static DateTime GetFormattedTime(long aTime)
        {
            return new DateTime((aTime * 10000000) + 621355968000000000, DateTimeKind.Utc).ToLocalTime();
        }
    }
}
