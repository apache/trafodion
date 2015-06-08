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
using System.Collections;
using System.Text;

namespace Trafodion.Manager.DatabaseArea.Model
{
    class DataTypeHelper
    {
        #region SQL Data Types
        public const String SQL_CHARACTER = "CHARACTER";
        public const String SQL_VARCHAR = "VARCHAR";
        public const String SQL_SIGNED_NUMERIC = "SIGNED NUMERIC";
        public const String SQL_UNSIGNED_NUMERIC = "UNSIGNED NUMERIC";
        public const String SQL_SIGNED_SMALLINT = "SIGNED SMALLINT";
        public const String SQL_UNSIGNED_SMALLINT = "UNSIGNED SMALLINT";
        public const String SQL_SIGNED_INTEGER = "SIGNED INTEGER";
        public const String SQL_UNSIGNED_INTEGER = "UNSIGNED INTEGER";
        public const String SQL_SIGNED_LARGEINT = "SIGNED LARGEINT";
        public const String SQL_FLOAT = "FLOAT";
        public const String SQL_REAL = "REAL";
        public const String SQL_DOUBLE = "DOUBLE";
        public const String SQL_SIGNED_DECIMAL = "SIGNED DECIMAL";
        public const String SQL_UNSIGNED_DECIMAL = "UNSIGNED DECIMAL";
        public const String SQL_TIMESTAMP = "TIMESTAMP";
        public const String SQL_DATE = "DATE";
        public const String SQL_TIME = "TIME";
        public const String SQL_INTERVAL = "INTERVAL"; 
        public const String SQL_NCHAR = "NCHAR";
        public const String SQL_UNKNOWN_TYPE = "UNKNOWN";
        public const String SQL_NO_TYPE = "N/A";
        #endregion SQL Data Types

        #region Java Data Types

        public const String JAVA_STRING = "java.lang.String";
        public const String JAVA_INTEGER = "java.lang.Integer";
        public const String JAVA_LONG = "java.lang.Long";
        public const String JAVA_FLOAT = "java.lang.Float";
        public const String JAVA_DOUBLE = "java.lang.Double";
        public const String JAVA_BIGDECIMAL = "java.math.BigDecimal";
        public const String JAVA_BIGINTEGER = "java.math.BigInteger";
        public const String JAVA_DATE = "java.sql.Date";
        public const String JAVA_TIME = "java.sql.Time";
        public const String JAVA_TIMESTAMP = "java.sql.Timestamp";
        public const String JAVA_PRIMITIVE_INT = "int";
        public const String JAVA_PRIMITIVE_LONG = "long";
        public const String JAVA_PRIMITIVE_FLOAT = "float";
        public const String JAVA_PRIMITIVE_DOUBLE = "double";
        public const String JAVA_PRIMITIVE_SHORT = "short";
        public const String JAVA_PRIMITIVE_INT_ARRAY = "int[]";
        public const String JAVA_PRIMITIVE_LONG_ARRAY = "long[]";
        public const String JAVA_PRIMITIVE_FLOAT_ARRAY = "float[]";
        public const String JAVA_PRIMITIVE_DOUBLE_ARRAY = "double[]";
        public const String JAVA_PRIMITIVE_SHORT_ARRAY = "short[]";
        public const String JAVA_RESULTSET = "java.sql.ResultSet";
        public const String JAVA_RESULTSET_ARRAY = "java.sql.ResultSet[]";

        #endregion Java Data Types

        public enum TypeIDEnum
        {
            UnknownType = 0, Character = 1, Varchar = 2,
            NumericSigned = 3, NumericUnsigned = 4,
            SmallIntSigned = 5, SmallIntUnsigned = 6,
            IntegerSigned = 7, IntegerUnsigned = 8, LargeIntSigned = 9,
            Float = 10, Real = 11, Double = 12, DecimalSigned = 13, DecimalUnsigned = 14,
            TimeStamp = 15, Date = 16, Time = 17, Interval = 18, DateTime = 19,
            NChar = 20, Varnchar = 21, ByteType = 22, ByteIntType = 23, NotApplicable = 24
        }

        public static readonly List<string> PrimitiveTypes = new List<string>();
        public static readonly Hashtable PrimitivePrefixes = new Hashtable();
        public static readonly Hashtable DefaultJavaToSqlMapping = new Hashtable();
        public static readonly Hashtable SqlTypeMappings = new Hashtable();
        public static readonly Hashtable JavaTypeMappings = new Hashtable();

        public const string CHARSET_ISO88591 = "ISO88591";
        public const string CHARSET_UCS2 = "UCS2";
        public static List<string> SupportedCharSets = new List<string> { CHARSET_ISO88591, CHARSET_UCS2 };
        public static readonly int MAXIMUM_CHARACTER_LENGTH = 32708;
        public static readonly int MINIMUM_CHARACTER_LENGTH = 1;
        public static readonly int DEFAULT_CHARACTER_LENGTH = 1;


        static DataTypeHelper()
        {
            SqlTypeMappings.Add(TypeIDEnum.NotApplicable, SQL_NO_TYPE);
            SqlTypeMappings.Add(TypeIDEnum.UnknownType, SQL_UNKNOWN_TYPE);
            SqlTypeMappings.Add(TypeIDEnum.Character, SQL_CHARACTER);
            SqlTypeMappings.Add(TypeIDEnum.Varchar, SQL_VARCHAR);
            SqlTypeMappings.Add(TypeIDEnum.NumericSigned, SQL_SIGNED_NUMERIC);
            SqlTypeMappings.Add(TypeIDEnum.NumericUnsigned, SQL_UNSIGNED_NUMERIC);
            SqlTypeMappings.Add(TypeIDEnum.SmallIntSigned, SQL_SIGNED_SMALLINT);
            SqlTypeMappings.Add(TypeIDEnum.SmallIntUnsigned, SQL_UNSIGNED_SMALLINT);
            SqlTypeMappings.Add(TypeIDEnum.IntegerSigned, SQL_SIGNED_INTEGER);
            SqlTypeMappings.Add(TypeIDEnum.IntegerUnsigned, SQL_UNSIGNED_INTEGER);
            SqlTypeMappings.Add(TypeIDEnum.LargeIntSigned, SQL_SIGNED_LARGEINT);
            SqlTypeMappings.Add(TypeIDEnum.Float, SQL_FLOAT);
            SqlTypeMappings.Add(TypeIDEnum.Real, SQL_REAL);
            SqlTypeMappings.Add(TypeIDEnum.Double, SQL_DOUBLE);
            SqlTypeMappings.Add(TypeIDEnum.DecimalSigned, SQL_SIGNED_DECIMAL);
            SqlTypeMappings.Add(TypeIDEnum.DecimalUnsigned, SQL_UNSIGNED_DECIMAL);
            SqlTypeMappings.Add(TypeIDEnum.TimeStamp, SQL_TIMESTAMP);
            SqlTypeMappings.Add(TypeIDEnum.Date, SQL_DATE);
            SqlTypeMappings.Add(TypeIDEnum.Time, SQL_TIME);
            SqlTypeMappings.Add(TypeIDEnum.Interval, SQL_INTERVAL);
            SqlTypeMappings.Add(TypeIDEnum.DateTime, SQL_DATE);
            SqlTypeMappings.Add(TypeIDEnum.NChar, SQL_CHARACTER);
            SqlTypeMappings.Add(TypeIDEnum.Varnchar, SQL_VARCHAR);
            SqlTypeMappings.Add(TypeIDEnum.ByteType, SQL_SIGNED_SMALLINT);
            SqlTypeMappings.Add(TypeIDEnum.ByteIntType, SQL_SIGNED_SMALLINT);


            DefaultJavaToSqlMapping.Add(JAVA_STRING, TypeIDEnum.Varchar);
            DefaultJavaToSqlMapping.Add(JAVA_INTEGER, TypeIDEnum.IntegerSigned);
            DefaultJavaToSqlMapping.Add(JAVA_LONG, TypeIDEnum.LargeIntSigned);
            DefaultJavaToSqlMapping.Add(JAVA_FLOAT, TypeIDEnum.Real);
            DefaultJavaToSqlMapping.Add(JAVA_DOUBLE, TypeIDEnum.Double);
            DefaultJavaToSqlMapping.Add(JAVA_BIGDECIMAL, TypeIDEnum.NumericSigned);
            DefaultJavaToSqlMapping.Add(JAVA_BIGINTEGER, TypeIDEnum.NumericSigned);
            DefaultJavaToSqlMapping.Add(JAVA_DATE, TypeIDEnum.Date);
            DefaultJavaToSqlMapping.Add(JAVA_TIME, TypeIDEnum.Time);
            DefaultJavaToSqlMapping.Add(JAVA_TIMESTAMP, TypeIDEnum.TimeStamp);
            DefaultJavaToSqlMapping.Add(JAVA_PRIMITIVE_INT, TypeIDEnum.IntegerSigned);
            DefaultJavaToSqlMapping.Add(JAVA_PRIMITIVE_LONG, TypeIDEnum.LargeIntSigned);
            DefaultJavaToSqlMapping.Add(JAVA_PRIMITIVE_FLOAT, TypeIDEnum.Real);
            DefaultJavaToSqlMapping.Add(JAVA_PRIMITIVE_DOUBLE, TypeIDEnum.Double);
            DefaultJavaToSqlMapping.Add(JAVA_PRIMITIVE_SHORT, TypeIDEnum.SmallIntSigned);
            DefaultJavaToSqlMapping.Add(JAVA_RESULTSET, TypeIDEnum.NotApplicable);

            JavaTypeMappings.Add(TypeIDEnum.Character, JAVA_STRING);
            JavaTypeMappings.Add(TypeIDEnum.Varchar, JAVA_STRING);
            JavaTypeMappings.Add(TypeIDEnum.NumericSigned, JAVA_BIGDECIMAL);
            JavaTypeMappings.Add(TypeIDEnum.NumericUnsigned, JAVA_BIGDECIMAL);
            JavaTypeMappings.Add(TypeIDEnum.SmallIntSigned, JAVA_PRIMITIVE_SHORT);
            JavaTypeMappings.Add(TypeIDEnum.SmallIntUnsigned, JAVA_PRIMITIVE_SHORT);
            JavaTypeMappings.Add(TypeIDEnum.IntegerSigned, JAVA_INTEGER);
            JavaTypeMappings.Add(TypeIDEnum.IntegerUnsigned, JAVA_INTEGER);
            JavaTypeMappings.Add(TypeIDEnum.LargeIntSigned, JAVA_LONG);
            JavaTypeMappings.Add(TypeIDEnum.Float, JAVA_FLOAT);
            JavaTypeMappings.Add(TypeIDEnum.Real, JAVA_FLOAT);
            JavaTypeMappings.Add(TypeIDEnum.Double, JAVA_DOUBLE);
            JavaTypeMappings.Add(TypeIDEnum.DecimalSigned, JAVA_DOUBLE);
            JavaTypeMappings.Add(TypeIDEnum.DecimalUnsigned, JAVA_DOUBLE);
            JavaTypeMappings.Add(TypeIDEnum.TimeStamp, JAVA_TIMESTAMP);
            JavaTypeMappings.Add(TypeIDEnum.Date, JAVA_DATE);
            JavaTypeMappings.Add(TypeIDEnum.Time, JAVA_TIME);
            JavaTypeMappings.Add(TypeIDEnum.Interval, JAVA_TIME);
            JavaTypeMappings.Add(TypeIDEnum.DateTime, JAVA_DATE);
            JavaTypeMappings.Add(TypeIDEnum.NChar, JAVA_STRING);
            JavaTypeMappings.Add(TypeIDEnum.Varnchar, JAVA_STRING);
            JavaTypeMappings.Add(TypeIDEnum.ByteType, JAVA_PRIMITIVE_SHORT);
            JavaTypeMappings.Add(TypeIDEnum.ByteIntType, JAVA_PRIMITIVE_SHORT);   
        
            PrimitiveTypes.Add(JAVA_PRIMITIVE_FLOAT);
            PrimitiveTypes.Add(JAVA_PRIMITIVE_FLOAT_ARRAY);
            PrimitiveTypes.Add(JAVA_PRIMITIVE_DOUBLE);
            PrimitiveTypes.Add(JAVA_PRIMITIVE_DOUBLE_ARRAY);
            PrimitiveTypes.Add(JAVA_PRIMITIVE_INT);
            PrimitiveTypes.Add(JAVA_PRIMITIVE_INT_ARRAY);
            PrimitiveTypes.Add(JAVA_PRIMITIVE_LONG);
            PrimitiveTypes.Add(JAVA_PRIMITIVE_LONG_ARRAY);
            PrimitiveTypes.Add(JAVA_PRIMITIVE_SHORT);
            PrimitiveTypes.Add(JAVA_PRIMITIVE_SHORT_ARRAY);
   
            PrimitivePrefixes.Add(JAVA_PRIMITIVE_INT, "I");
            PrimitivePrefixes.Add(JAVA_PRIMITIVE_INT_ARRAY, "I_ARRAY");
            PrimitivePrefixes.Add(JAVA_PRIMITIVE_LONG, "L");
            PrimitivePrefixes.Add(JAVA_PRIMITIVE_LONG_ARRAY, "L_ARRAY");
            PrimitivePrefixes.Add(JAVA_PRIMITIVE_FLOAT, "F");
            PrimitivePrefixes.Add(JAVA_PRIMITIVE_FLOAT_ARRAY, "F_ARRAY");
            PrimitivePrefixes.Add(JAVA_PRIMITIVE_DOUBLE, "D");
            PrimitivePrefixes.Add(JAVA_PRIMITIVE_DOUBLE_ARRAY, "D_ARRAY");
            PrimitivePrefixes.Add(JAVA_PRIMITIVE_SHORT, "S");
            PrimitivePrefixes.Add(JAVA_PRIMITIVE_SHORT_ARRAY, "S_ARRAY");

        }

        public static bool IsPrimitive(string paramType)
        {
            if (PrimitiveTypes.Contains(paramType))
                return true;
            else
                return false;
        }

        public static TypeIDEnum MapJavaTypeToTypeId(String javaType)
        {
            if (javaType == null)
            {
                return TypeIDEnum.UnknownType;
            }

            String temp = javaType;
            if ((javaType != null) && (javaType.IndexOf("[") >= 0))
            {
                temp = javaType.Substring(0, javaType.IndexOf("["));
            }

            if(DefaultJavaToSqlMapping.Contains(temp))
            {
                return (TypeIDEnum)DefaultJavaToSqlMapping[temp];
            }
            return TypeIDEnum.UnknownType;
        }

        public static string MapTypeIdToJavaType(TypeIDEnum dataTypeId)
        {
            if (JavaTypeMappings.Contains(dataTypeId))
                return JavaTypeMappings[dataTypeId] as string;
            else
                return SQL_UNKNOWN_TYPE;
        }

        public static string MapJavaTypeToSqlType(string javaType)
        {
            TypeIDEnum typeId = MapJavaTypeToTypeId(javaType);
            if (SqlTypeMappings.Contains(typeId))
            {
                return SqlTypeMappings[typeId] as string;
            }
            else
                return SQL_UNKNOWN_TYPE;
        }

        public static string GetBaseJavaType(string javaType)
        {         
            return (javaType.IndexOf("[") > 0) ? javaType.Substring(0, javaType.IndexOf("[")) : javaType; 
        }
    }
}
