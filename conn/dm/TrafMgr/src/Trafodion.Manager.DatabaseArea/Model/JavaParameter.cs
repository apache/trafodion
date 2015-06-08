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
    public class JavaParameter
    {
        private string _name;
        private string _direction;
        private string _sqlDataType;
        private string _javaDataType;

        public string Name
        {
            get { return _name; }
        }
        public string Direction
        {
            get 
            {
                if ((_javaDataType == null) || (_javaDataType.Length < 1))
                    return "";
                return _javaDataType.Contains("[]") ? "O" : "I"; 
            }
        }
        public bool isArray
        {
            get { return _javaDataType.Contains("[]") ? true : false; }
        }

        public string SqlDataType
        {
            get 
            {
                return DataTypeHelper.MapJavaTypeToSqlType(_javaDataType);
            }
        }

        public int DefaultSize
        {
            get
            {
                DataTypeHelper.TypeIDEnum dataType = DataTypeHelper.MapJavaTypeToTypeId(_javaDataType);
                switch (dataType)
                {
                    case DataTypeHelper.TypeIDEnum.Character:
                    case DataTypeHelper.TypeIDEnum.Varchar:
                    case DataTypeHelper.TypeIDEnum.NChar:
                    case DataTypeHelper.TypeIDEnum.Varnchar:
                        {
                            return 1;
                        }
                    case DataTypeHelper.TypeIDEnum.Time:
                        {
                            return 0;
                        }
                    case DataTypeHelper.TypeIDEnum.TimeStamp:
                        {
                            return 6;
                        }
                    case DataTypeHelper.TypeIDEnum.NumericSigned:
                    case DataTypeHelper.TypeIDEnum.NumericUnsigned:
                    case DataTypeHelper.TypeIDEnum.DecimalSigned:
                    case DataTypeHelper.TypeIDEnum.DecimalUnsigned:
                        {
                            if (DataTypeHelper.GetBaseJavaType(JavaDataType).Equals(DataTypeHelper.JAVA_BIGINTEGER))
                            {
                                return 18;
                            }
                            else
                            {
                                return 9;
                            }
                        }
                    default:
                        {
                            return 0;
                        }
                }
            }
        }
        public string JavaDataType
        {
            get { return _javaDataType; }
        }

        public JavaParameter(string name, string javaDataType)
        {
            _name = name;
            _javaDataType = javaDataType;
        }
    }
}
