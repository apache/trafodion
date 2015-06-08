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

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Model for a TrafodionProcedure column
    /// </summary>
    public class TrafodionProcedureColumn : TrafodionColumn
    {
        #region Public Variables
        /// <summary>
        /// Literal string that identifies that the Column is an input column
        /// </summary>
        public static readonly string DirectionIn = "IN";
        /// <summary>
        /// Literal string that identifies that the Column is an output column
        /// </summary>
        public static readonly string DirectionOut = "OUT";
        /// <summary>
        /// Literal string that identifies that the Column is an in/out column
        /// </summary>
        public static readonly string DirectionInOut = "INOUT";

        #endregion Public Variables

        #region Private member variables

        private string _javaDataType = null;

        #endregion Private member variables

        #region Public Properties

        /// <summary>
        /// Java datype of the procedure column
        /// </summary>
        public string JavaDataType
        {
            get { return _javaDataType; }
            set { _javaDataType = value; }
        }

        /// <summary>
        /// Identifies if the java type associated with this column is an array type
        /// </summary>
        public bool IsArray
        {
            get { return _javaDataType.IndexOf("[]") > 0; }
        }

        /// <summary>
        /// Identifies if the java type associated with this column is a native type
        /// </summary>
        public bool IsPrimitive
        {
            get { return DataTypeHelper.IsPrimitive(_javaDataType); }
        }

        /// <summary>
        /// Returns the procedure model that contains this column
        /// </summary>
        public TrafodionProcedure TrafodionProcedure
        {
            get { return TrafodionSchemaObject as TrafodionProcedure; }
        }

        #endregion Public Properties

        /// <summary>
        /// Constructs the model
        /// </summary>
        public TrafodionProcedureColumn()
        { 
        }
    }
}
