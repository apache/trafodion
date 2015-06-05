/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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
********************************************************************/

namespace Trafodion.Data
{
    using System;
    using System.Text.RegularExpressions;
    using System.Collections.Generic;
    using System.Data;

    /// <summary>
    /// Collection of static regular expressions, arrays, and methods used through out the provider.
    /// </summary>
    internal class TrafodionDBUtility
    {
        /// <summary>
        /// Regular expression for validating numeric format.  Used in BigNum conversion.
        /// </summary>
        public static readonly Regex ValidateNumeric = new Regex("^-{0,1}\\d*\\.{0,1}\\d+$", RegexOptions.Compiled);

        /// <summary>
        /// Regular expression for trimming leading 0s.  Used in BigNum conversion.
        /// </summary>
        public static readonly Regex TrimNumericString = new Regex("0*", RegexOptions.Compiled);

        /// <summary>
        /// Regular expression for finding wild char characters in metadata parameters.
        /// </summary>
        public static readonly Regex FindWildCards = new Regex("[%_]", RegexOptions.Compiled);

        /// <summary>
        /// Regular expression for stripping out SQL comments.
        /// </summary>
        public static readonly Regex RemoveComments = new Regex("--.*[\r\n]|/\\*(.|[\r\n])*?\\*/", RegexOptions.Compiled);

        /// <summary>
        /// Regular expression for finding out particular num in the sql.
        /// </summary>
        public static readonly Regex FindRowSize = new Regex("[0-9]+", RegexOptions.Compiled);

        /// <summary>
        /// Regular expression for checking if the sql is RWRS User Load
        /// </summary>
        public static readonly Regex FindRowwiseUserLoad = new Regex(@"user\s+load", RegexOptions.Compiled);

        /// <summary>
        /// Regular expression for removing string literals.
        /// </summary>
        public static readonly Regex RemoveStringLiterals = new Regex("\"[^\"]*\"|'[^']*'", RegexOptions.Compiled);

        /// <summary>
        /// Regular expression for tokenizing words.
        /// </summary>
        public static readonly Regex Tokenize = new Regex("[^a-zA-Z]+", RegexOptions.Compiled);

        /// <summary>
        /// Most commonly used date format.
        /// </summary>
        public const string DateFormat = "yyyy-MM-dd";

        /// <summary>
        /// Commonly used time formats.   Precision 0 through 6.
        /// </summary>
        public static readonly string[] TimeFormat =
        {
            "HH:mm:ss", "HH:mm:ss.f", "HH:mm:ss.ff", "HH:mm:ss.fff",
            "HH:mm:ss.ffff", "HH:mm:ss.fffff", "HH:mm:ss.ffffff"
        };

        /// <summary>
        /// Commonly used Timestamp formats.  Precision 0 through 6
        /// </summary>
        public static readonly string[] TimestampFormat =
        {
            "yyyy-MM-dd HH:mm:ss", "yyyy-MM-dd HH:mm:ss.f",
            "yyyy-MM-dd HH:mm:ss.ff", "yyyy-MM-dd HH:mm:ss.fff",
            "yyyy-MM-dd HH:mm:ss.ffff", "yyyy-MM-dd HH:mm:ss.fffff",
            "yyyy-MM-dd HH:mm:ss.ffffff"
        };

        /// <summary>
        /// Array of powers of 10 that can be referenced by using the exponent as the offset
        /// </summary>
        public static readonly long[] PowersOfTen =
        {
            1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000,
            1000000000, 10000000000, 100000000000, 1000000000000,
            10000000000000, 100000000000000, 1000000000000000, 10000000000000000,
            100000000000000000, 1000000000000000000
        };

        /*
         * the following section is a hack implemented to work around that Visual Studio 2008 does not use the proper delimiter
         * with its query builder.  We need to switch  [ ] to " before executing the statement without breaking
         * string literals or valid SQL syntax
         *
         * (""[^""]*"")                              -- find text enclosed in double quotes
         * ('[^']*')                                 -- find text enclosed in single quotes
         * ((\[)(FIRST|ANY|LAST)\s(\d)*?(\]))        -- find text in the form [<keyword> ####], keywords: FIRST, LAST, ANY
         *
         */

        /// <summary>
        /// Regular expression for removing valid brackets.
        /// </summary>
        public static readonly Regex RemoveValidBrackets = new Regex(@"(""[^""]*"")|('[^']*')|((\[)(FIRST|ANY|LAST)\s(\d)*?(\]))", RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Multiline);

        /// <summary>
        /// Regular expression for finding all the brackets.
        /// </summary>
        public static readonly Regex FindBrackets = new Regex(@"[\[\]]", RegexOptions.Compiled);

        /// <summary>
        /// Parses a SQL string and replaces [ ] delimiters with " " .
        /// </summary>
        /// <param name="str">The SQL string to be parsed.</param>
        /// <returns>A SQL string with TrafodionDB compatible delimiters.</returns>
        public static string ConvertBracketIdentifiers(string str)
        {
            // save the raw characters
            char[] chars = str.ToCharArray();

            // replace all the valid brackets and string literals with whitespace equal to the length removed
            str = RemoveValidBrackets.Replace(str, (Match m) => new string(' ', m.Value.Length));

            // find the remaining bracket offsets
            MatchCollection mc = FindBrackets.Matches(str);

            // use the offsets in the temp str to replace the original
            foreach (Match m in mc)
            {
                chars[m.Index] = '"';
            }

            return new string(chars);
        }

        /*
         * The following section contains all the type mappings between TrafodionDB's custom types and ADO.NET
         *  standard types.
         */

        private static readonly Dictionary<DbType, TrafodionDBDbType> DbTypeMapping;
        private static readonly Dictionary<TrafodionDBDbType, DbType> TrafodionDBDbTypeMapping;

        static TrafodionDBUtility()
        {
            //DbType to TrafodionDBDbType
            DbTypeMapping = new Dictionary<DbType, TrafodionDBDbType>(27);
            DbTypeMapping.Add(DbType.AnsiString, TrafodionDBDbType.Varchar);
            DbTypeMapping.Add(DbType.AnsiStringFixedLength, TrafodionDBDbType.Char);
            //DbTypeMapping.Add(DbType.Binary, TrafodionDBDbType.Undefined);
            //DbTypeMapping.Add(DbType.Boolean, TrafodionDBDbType.Undefined);
            //DbTypeMapping.Add(DbType.Byte, TrafodionDBDbType.Undefined);
            //DbTypeMapping.Add(DbType.Currency, TrafodionDBDbType.Undefined);
            DbTypeMapping.Add(DbType.Date, TrafodionDBDbType.Date);
            DbTypeMapping.Add(DbType.DateTime, TrafodionDBDbType.Timestamp);
            //DbTypeMapping.Add(DbType.DateTime2, TrafodionDBDbType.Undefined);
            //DbTypeMapping.Add(DbType.DateTimeOffset, TrafodionDBDbType.Undefined);
            DbTypeMapping.Add(DbType.Decimal, TrafodionDBDbType.Decimal);
            DbTypeMapping.Add(DbType.Double, TrafodionDBDbType.Double);
            //DbTypeMapping.Add(DbType.Guid, TrafodionDBDbType.Undefined);
            DbTypeMapping.Add(DbType.Int16, TrafodionDBDbType.SmallInt);
            DbTypeMapping.Add(DbType.Int32, TrafodionDBDbType.Integer);
            DbTypeMapping.Add(DbType.Int64, TrafodionDBDbType.LargeInt);
            //DbTypeMapping.Add(DbType.Object, TrafodionDBDbType.Undefined);
            //DbTypeMapping.Add(DbType.SByte, TrafodionDBDbType.Undefined);
            DbTypeMapping.Add(DbType.Single, TrafodionDBDbType.Float);
            DbTypeMapping.Add(DbType.String, TrafodionDBDbType.Varchar); // wrong
            DbTypeMapping.Add(DbType.StringFixedLength, TrafodionDBDbType.Char); // wrong
            DbTypeMapping.Add(DbType.Time, TrafodionDBDbType.Time);
            DbTypeMapping.Add(DbType.UInt16, TrafodionDBDbType.SmallIntUnsigned);
            DbTypeMapping.Add(DbType.UInt32, TrafodionDBDbType.IntegerUnsigned);
            //DbTypeMapping.Add(DbType.UInt64, TrafodionDBDbType.Undefined);
            DbTypeMapping.Add(DbType.VarNumeric, TrafodionDBDbType.Numeric);
            //DbTypeMapping.Add(DbType.Xml, TrafodionDBDbType.Undefined);

            // TrafodionDBDbType to DbType
            TrafodionDBDbTypeMapping = new Dictionary<TrafodionDBDbType,DbType>(100);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Undefined, DbType.Object);

            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Char, DbType.StringFixedLength); //wrong
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Date, DbType.Date);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Decimal, DbType.Decimal);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.DecimalUnsigned, DbType.Decimal); // wrong
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Double, DbType.Double);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Float, DbType.Single);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Integer, DbType.Int32);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.IntegerUnsigned, DbType.UInt32);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Interval, DbType.Object); // wrong
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.LargeInt, DbType.Int64);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Numeric, DbType.VarNumeric);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.NumericUnsigned, DbType.VarNumeric); // wrong
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Real, DbType.Single);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.SmallInt, DbType.Int16);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.SmallIntUnsigned, DbType.UInt16);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Time, DbType.Time);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Timestamp, DbType.DateTime);
            TrafodionDBDbTypeMapping.Add(TrafodionDBDbType.Varchar, DbType.String); //wrong
        }

        public static TrafodionDBDbType MapDbType(DbType type)
        {
            return DbTypeMapping.ContainsKey(type) ? DbTypeMapping[type] : TrafodionDBDbType.Undefined;
        }

        public static DbType MapTrafodionDBDbType(TrafodionDBDbType type)
        {
            return TrafodionDBDbTypeMapping.ContainsKey(type) ? TrafodionDBDbTypeMapping[type] : DbType.Object;
        }
    }
}
