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
using Trafodion.Manager.Framework.Connections;
using System.Data.Odbc;

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// Summary description for TrafodionColumn.
	/// </summary>
	public class TrafodionColumn : TrafodionObject
    {
        #region Static Fields and Functions

        /// <summary>
        /// A string that represents a system class; which is a column added by the system.
        /// </summary>
        public const string SYSTEM_CLASS = "S";

        /// <summary>
        /// A string that represents a user class; which is a column added by a user.
        /// </summary>
        public const string USER_CLASS = "U";

        /// <summary>
        /// A string that represents an add-user class; which is a column added after
        /// the column's container was already created.
        /// </summary>
        public const string ADDEDCOLUMN_CLASS = "A";

        /// <summary>
        /// A string that represents a materialized view class; which is a column added
        /// to support materialized views. This is not a documented class.
        /// </summary>
        public const string MATERIALIZEDVIEW_CLASS = "M";

        /// <summary>
        /// Translates a string to a column class type.
        /// </summary>
        /// <param name="columnClass">The string to translate.</param>
        /// <returns>One of the ColumnClass types.</returns>
        public static ColumnClass TranslateColumnClassString(string columnClass)
        {
            if (columnClass.Equals(SYSTEM_CLASS, StringComparison.OrdinalIgnoreCase))
                return ColumnClass.SystemClass;
            if (columnClass.Equals(USER_CLASS, StringComparison.OrdinalIgnoreCase))
                return ColumnClass.UserClass;
            if (columnClass.Equals(ADDEDCOLUMN_CLASS, StringComparison.OrdinalIgnoreCase))
                return ColumnClass.AddedColumnClass;
            if (columnClass.Equals(MATERIALIZEDVIEW_CLASS, StringComparison.OrdinalIgnoreCase))
                return ColumnClass.MaterializedViewClass;

            throw new ApplicationException("\"" + columnClass + "\" is an unknown column class.");
        }

        #endregion

        /// <summary>
        /// The various column classes that exist.
        /// </summary>
        public enum ColumnClass
        {
            /// <summary>
            /// Indicates that the column is system-defined.
            /// </summary>
            SystemClass,

            /// <summary>
            /// Indicates that the column is user-defined.
            /// </summary>
            UserClass,

            /// <summary>
            /// Indicates that the column was added.
            /// </summary>
            AddedColumnClass,

            /// <summary>
            /// Indicates that the column was added to support materialized views.
            /// </summary>
            MaterializedViewClass
        }

        #region Fields

        private string theSQLDataType = "";
        private int theColumnNumber = 0;
        private string theDirection = "";
        private string theColumnClass = "";
        private int theColumnSize = 1;
        private string theCharacterSet = "";
        private string theEncoding = "";
        private string theCollationSequence = "";
        private int theFSDataType = 0;
        private int theColScale = 0;
        private int theColPrecision = 0;
        private bool isUpshifted = false;
        private int theNullHeaderSize = 0;
        private bool isNullable;
        private bool isNullDroppable;
        private int theVarlenHeaderSize = 0;
        private string theDefaultClass = "";
        private bool isLoggable = false;
        private int theDatetimeStartField = 0;
        private int theDatetimeEndField = 0;
        private int theDatetimeLeadingPrecision = 0;
        private int theDatetimeTrailingPrecision = 0;
        private string theDatetimeQualifier = "";
        private string theDefaultValue = "";
        private string theHeadingText = "";
        private string thePictureText = "";
        private int theColumnValueDriftPerDay = 0;
        // Use to determine if any of the fields have been changed and should be loaded
        private bool isDefaultValues = true;
        private Int64 theOutputUEC = 0;
        private bool isOptional = false;
        private string theRoutineParamType = "";

        // V2000
        private string theDateDisplayFormat = "";

        private bool caseSensitiveComparison = false;
        private bool isDisplayDataType = false;
        private TrafodionSchemaObject theTrafodionSchemaObject = null;
        private string theFormattedDefaultString = null;
        private Dictionary<string, string> theCurrentDefaultDictionary = null;

        #endregion
    
        #region Properties

        /// <summary>
        /// Reflects whether column attributes have been set
        /// </summary>
        public bool IsDefaultValues
        {
            get { return isDefaultValues; }
            set { isDefaultValues = value; }
        }

        /// <summary>
        /// The schema object in which this column resides.
        /// </summary>
        public TrafodionSchemaObject TrafodionSchemaObject
        {
            get { return theTrafodionSchemaObject; }
            set { theTrafodionSchemaObject = value; }
        }

        /// <summary>
        /// The column's data type.
        /// </summary>
        public string TheSQLDataType
        {
            get { return theSQLDataType; }
            set { theSQLDataType = value; }
        }
 
        /// <summary>
        /// The table's index which points to this column.
        /// </summary>
        public int TheColumnNumber
        {
            get { return theColumnNumber; }
            set { theColumnNumber = value; }
        }

        /// <summary>
        /// The sorting direction.
        /// </summary>
        public string TheDirection
        {
            get { return theDirection; }
            set { theDirection = value; }
        }

        /// <summary>
        /// The class of this column.
        /// </summary>
        public string TheColumnClass
        {
            get { return theColumnClass; }
            set { theColumnClass = value; }
        }

        /// <summary>
        /// The size of the column.
        /// </summary>
        public int TheColumnSize
        {
            get { return theColumnSize; }
            set { theColumnSize = value; }
        }

        /// <summary>
        /// The character set of this column (if it's a character-based column).
        /// </summary>
        public string TheCharacterSet
        {
            get { return theCharacterSet; }
            set { theCharacterSet = value; }
        }

        /// <summary>
        /// The character encoding of this column (if it's a character-based column).
        /// </summary>
        public string TheEncoding
        {
            get { return theEncoding; }
            set { theEncoding = value; }
        }

        public string TheCollationSequence
        {
            get { return theCollationSequence; }
            set { theCollationSequence = value; }
        }

        public int TheFSDataType
        {
            get { return theFSDataType; }
            set { theFSDataType = value; }
        }

        public int TheColScale
        {
            get { return theColScale; }
            set { theColScale = value; }
        }

        public int TheColPrecision
        {
            get { return theColPrecision; }
            set { theColPrecision = value; }
        }

        public bool IsUpshifted
        {
            get { return isUpshifted; }
            set { isUpshifted = value; }
        }

        public int TheNullHeaderSize
        {
            get { return theNullHeaderSize; }
            set { theNullHeaderSize = value; }
        }

        public bool IsNullable
        {
            get { return isNullable; }
            set { isNullable = value; }
        }

        public bool IsNullDroppable
        {
            get { return isNullDroppable; }
            set { isNullDroppable = value; }
        }

        public int TheVarlenHeaderSize
        {
            get { return theVarlenHeaderSize; }
            set { theVarlenHeaderSize = value; }
        }

        public string TheDefaultClass
        {
            get { return theDefaultClass; }
            set { theDefaultClass = value; }
        }

        public bool IsLoggable
        {
            get { return isLoggable; }
            set { isLoggable = value; }
        }

        public int TheDatetimeStartField
        {
            get { return theDatetimeStartField; }
            set { theDatetimeStartField = value; }
        }

        public int TheDatetimeEndField
        {
            get { return theDatetimeEndField; }
            set { theDatetimeEndField = value; }
        }

        public int TheDatetimeLeadingPrecision
        {
            get { return theDatetimeLeadingPrecision; }
            set { theDatetimeLeadingPrecision = value; }
        }

        public int TheDatetimeTrailingPrecision
        {
            get { return theDatetimeTrailingPrecision; }
            set { theDatetimeTrailingPrecision = value; }
        }

        public string TheDatetimeQualifier
        {
            get { return theDatetimeQualifier; }
            set { theDatetimeQualifier = value; }
        }

        public string TheDefaultValue
        {
            get { return theDefaultValue; }
            set { theDefaultValue = value; }
        }

        public string TheHeadingText
        {
            get { return theHeadingText; }
            set { theHeadingText = value; }
        }

        public string ThePictureText
        {
            get { return thePictureText; }
            set { thePictureText = value; }
        }

        public int TheColumnValueDriftPerDay
        {
            get { return theColumnValueDriftPerDay; }
            set { theColumnValueDriftPerDay = value; }
        }

        public string TheDateDisplayFormat
        {
            get { return theDateDisplayFormat; }
            set { theDateDisplayFormat = value; }
        }

        public bool CaseSensitiveComparison
        {
            get { return caseSensitiveComparison; }
            set { caseSensitiveComparison = value; }
        }

        public bool IsDisplayDataType
        {
            get { return isDisplayDataType; }
            set { isDisplayDataType = value; }
        }

        public bool IsOptional
        {
            get { return isOptional; }
            set { isOptional = value; }
        }

        public string RoutineParameterType
        {
            get { return theRoutineParamType; }
            set { theRoutineParamType = value; }
        }

        public Int64 OutputUEC
        {
            get { return theOutputUEC; }
            set { theOutputUEC = value; }
        }

        #endregion

        /// <summary>
        /// Constructs a new column.
        /// </summary>
        public TrafodionColumn()
		{
            if (theDataTypeFormatters == null)
            {
                theDataTypeFormatters = new Dictionary<string, DataTypeFormatter>();

                theDataTypeFormatters.Add("CHARACTER", CharacterColumnFormatter);
                theDataTypeFormatters.Add("VARCHAR", VarcharColumnFormatter);

                theDataTypeFormatters.Add("SIGNED NUMERIC", SignedNumericColumnFormatter);
                theDataTypeFormatters.Add("UNSIGNED NUMERIC", UnsignedNumericColumnFormatter);

                theDataTypeFormatters.Add("SIGNED LARGEINT", SignedLargeintColumnFormatter);

                theDataTypeFormatters.Add("SIGNED INTEGER", SignedIntegerColumnFormatter);
                theDataTypeFormatters.Add("UNSIGNED INTEGER", UnsignedIntegerColumnFormatter);

                theDataTypeFormatters.Add("SIGNED SMALLINT", SignedSmallintColumnFormatter);
                theDataTypeFormatters.Add("UNSIGNED SMALLINT", UnsignedSmallintColumnFormatter);

                theDataTypeFormatters.Add("SIGNED DECIMAL", SignedDecimalColumnFormatter);
                theDataTypeFormatters.Add("UNSIGNED DECIMAL", UnsignedDecimalColumnFormatter);

                theDataTypeFormatters.Add("DATE", DateColumnFormatter);
                theDataTypeFormatters.Add("TIME", TimeColumnFormatter);
                theDataTypeFormatters.Add("TIMESTAMP", TimestampColumnFormatter);

                theDataTypeFormatters.Add("INTERVAL", IntervalColumnFormatter);

                theDataTypeFormatters.Add("FLOAT", FloatColumnFormatter);
                theDataTypeFormatters.Add("REAL", RealColumnFormatter);
                theDataTypeFormatters.Add("DOUBLE", DoubleColumnFormatter);
            }
		}

        /// <summary>
        /// Returns the connection used to create this object.
        /// </summary>
        /// <returns>A connection.</returns>
        override public Connection GetConnection()
        {
            return TrafodionSchemaObject.GetConnection();
        }

        /// <summary>
        /// Returns the connection definition used to create this object.
        /// </summary>
        override public ConnectionDefinition ConnectionDefinition
        {
            get { return TrafodionSchemaObject.ConnectionDefinition; }
        }

        public override string ToString()
        {
            return FormattedColumnName();
        }

        #region FORMATTED

        public string FormattedColumnName()
        {
            return ExternalName;
        }

        public string FormattedDataType()
        {
            if (theDataTypeFormatters.ContainsKey(TheSQLDataType))
            {
                return theDataTypeFormatters[TheSQLDataType](this);
            }
            return TheSQLDataType;
        }

        public string FormattedNullDropable()
        {
            return IsNullDroppable ? "" : "Not Droppable";
        }

        public string FormattedDirection()
        {

            if (TheDirection.Equals("I"))
            {
                return TrafodionProcedureColumn.DirectionIn;
            }
            else if (TheDirection.Equals("O"))
            {
                return TrafodionProcedureColumn.DirectionOut;
            }
            else if (TheDirection.Equals("N"))
            {
                return TrafodionProcedureColumn.DirectionInOut;
            }
            else
                return Properties.Resources.Unknown;
        }

        public string FormattedNullable()
        {
            return IsNullable ? "" : "Not Nullable";
        }

        public string FormattedDefault()
        {
            if (theFormattedDefaultString != null)
            {
                return theFormattedDefaultString;
            }

            if (TheDefaultClass.Equals("UD"))
            {
                theFormattedDefaultString = TheDefaultValue;
            }
            else if (TheDefaultClass.Equals("ND"))
            {
                theFormattedDefaultString = "NULL";
            }
            // Support for CURRENT_...
            // "CD" is set when current date, time, or timestamp is requested
            // "UF" is set when current user is requested
            else if ((TheDefaultClass.Equals("CD")) || (TheDefaultClass.Equals("UF")))
            {
                if (theCurrentDefaultDictionary == null)
                {
                    theCurrentDefaultDictionary = new Dictionary<string, string>();
                    theCurrentDefaultDictionary.Add("DATE", "CURRENT_DATE");
                    theCurrentDefaultDictionary.Add("TIME", "CURRENT_TIME");
                    theCurrentDefaultDictionary.Add("TIMESTAMP", "CURRENT_TIMESTAMP");
                    theCurrentDefaultDictionary.Add("CHARACTER", "CURRENT_USER");
                    theCurrentDefaultDictionary.Add("VARCHAR", "CURRENT_USER");
                    theCurrentDefaultDictionary.Add("VARNCHAR", "CURRENT_USER");
                    theCurrentDefaultDictionary.Add("NCHAR", "CURRENT_USER");
                }

                theFormattedDefaultString = theCurrentDefaultDictionary[theSQLDataType];

            }
            else if (TheDefaultClass.Equals("ID") || TheDefaultClass.Equals("IA"))
            {
                if (TrafodionSchemaObject.TheTrafodionSchema.Version < 2400)
                {
                    theFormattedDefaultString = "GENERATED BY DEFAULT AS IDENTITY";
                }
                else
                {
                    theFormattedDefaultString = FormatIdentityString();
                }
            }

            // In case we didn't figure out anything
            if (TheDefaultValue == null)
            {
                theFormattedDefaultString = TheDefaultValue;
            }

            return theFormattedDefaultString;
        }

        public string FormattedHeading()
        {
            return TheHeadingText;
        }

        #endregion        

        private string FormatIdentityString()
        {
            String formattedIdentityString = "";

            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteSelectSequenceAttributes(theConnection, TrafodionSchemaObject);
                if (theReader.Read())
                {
                    //START_VALUE, INCREMENT, MIN_VALUE, MAX_VALUE, CYCLE_OPTION
                    long startValue = theReader.GetInt64(0);
                    long increment = theReader.GetInt64(1);
                    long minValue = theReader.GetInt64(2);
                    long maxValue = theReader.GetInt64(3);
                    string cycleOption = theReader.GetString(4).Trim();                    

                    //‘GENERATED { ALWAYS | BY DEFAULT } AS IDENTITY [<sequence generator spec>]

                    formattedIdentityString = String.Format("GENERATED BY {0} AS IDENTITY (START WITH {1} INCREMENT BY {2} MAXVALUE {3} MINVALUE {4} {5})",
                        new object[] { TheDefaultClass.Equals("ID") ? "DEFAULT" : "ALWAYS", startValue, increment, maxValue, minValue, 
                        cycleOption.Equals("Y") ? "CYCLE" : "NO CYCLE" });

                }
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }

            return formattedIdentityString;

        }

        #region FORMATTERS

        static private Dictionary<string, DataTypeFormatter> theDataTypeFormatters = null;

        private delegate string DataTypeFormatter(TrafodionColumn aTrafodionColumn);

        static private string DateColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Date";
        }
        static private string RealColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Real";
        }
        static private string DoubleColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Double Precision";
        }
        static private string TimeColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Time(" + aTrafodionColumn.TheDatetimeTrailingPrecision + ")";
        }

        static private string TimestampColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Timestamp(" + aTrafodionColumn.TheDatetimeTrailingPrecision + ")";
        }

        static private string SignedDecimalColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Decimal(" + aTrafodionColumn.TheColPrecision
            + "," + aTrafodionColumn.TheColScale
            + ") Signed";
        }

        static private string UnsignedDecimalColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Decimal(" + aTrafodionColumn.TheColPrecision
            + "," + aTrafodionColumn.TheColScale
            + ") Unsigned";
        }

        static private string SignedNumericColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Numeric(" + aTrafodionColumn.TheColPrecision
            + "," + aTrafodionColumn.TheColScale
            + ") Signed";
        }

        static private string UnsignedNumericColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Numeric(" + aTrafodionColumn.TheColPrecision
            + "," + aTrafodionColumn.TheColScale
            + ") Unsigned";
        }

        static private string CharacterColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            string colSize = aTrafodionColumn.TheColumnSize.ToString();
            if (aTrafodionColumn.TheCharacterSet.Equals("UTF8") || aTrafodionColumn.TheCharacterSet.Equals("SJIS"))
            {
                if (aTrafodionColumn.TheColPrecision == 0)
                {
                    colSize += " bytes";
                }
            }
            StringBuilder theStringBuilder = new StringBuilder("Char (" + colSize
            + ")");

            if (aTrafodionColumn.IsUpshifted)
            {
                theStringBuilder.Append(" Upshifted");
            }
            //if (!aTrafodionColumn.TheCharacterSet.Equals("ISO88591"))
            {
                theStringBuilder.Append(" Character Set "  + aTrafodionColumn.TheCharacterSet);
            }

            return theStringBuilder.ToString();
        }

        static private string VarcharColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            string colSize = aTrafodionColumn.TheColumnSize.ToString();
            if (aTrafodionColumn.TheCharacterSet.Equals("UTF8") || aTrafodionColumn.TheCharacterSet.Equals("SJIS"))
            {
                if (aTrafodionColumn.TheColPrecision == 0)
                {
                    colSize += " bytes";
                }
            }

            StringBuilder theStringBuilder = new StringBuilder("Varchar (" + colSize
            + ")");

            if (aTrafodionColumn.IsUpshifted)
            {
                theStringBuilder.Append(" Upshifted");
            }
            //if (!aTrafodionColumn.TheCharacterSet.Equals("ISO88591"))
            {
                theStringBuilder.Append(" Character Set " + aTrafodionColumn.TheCharacterSet);
            }

            return theStringBuilder.ToString();
        }

        static private string SignedLargeintColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Largeint";
        }

        static private string SignedIntegerColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Integer Signed";
        }

        static private string UnsignedIntegerColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Integer Unsigned";
        }

        static private string SignedSmallintColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Smallint Signed";
        }

        static private string UnsignedSmallintColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Smallint Unsigned";
        }

        static private string IntervalName(int anInt)
        {
            switch (anInt)
            {
                case 1: { return "year"; }
                case 2: { return "month"; }
                case 3: { return "day"; }
                case 4: { return "hour"; }
                case 5: { return "minute"; }
                case 6: { return "second"; }
                default: { throw new Exception("Unknown interval integer " + anInt); }
            }
        }

        static private string FloatColumnFormatter(TrafodionColumn aTrafodionColumn)
        {
            return "Float(" + aTrafodionColumn.TheColPrecision + ")";
        }

        static private string IntervalColumnFormatter(TrafodionColumn aTrafodionColumn)
		{
			int firstType = aTrafodionColumn.TheDatetimeStartField;
			int lastType  = aTrafodionColumn.TheDatetimeEndField;

            StringBuilder theStringBuilder = new StringBuilder("Interval " + IntervalName(firstType) + "(" + aTrafodionColumn.TheDatetimeLeadingPrecision);

			if (firstType == lastType) 
            {
				if (firstType == 6) // SECOND 
                {
				    theStringBuilder.Append(", " + aTrafodionColumn.TheDatetimeTrailingPrecision);
				} 
				theStringBuilder.Append(")");
			}
			else
            {
				theStringBuilder.Append(") to " + IntervalName(lastType));
				if (lastType == 6) // SECOND 
                {
				    theStringBuilder.Append("(" + aTrafodionColumn.TheDatetimeTrailingPrecision + ")");
				} 
			}
            return theStringBuilder.ToString();
        }

        #endregion

    }
}

