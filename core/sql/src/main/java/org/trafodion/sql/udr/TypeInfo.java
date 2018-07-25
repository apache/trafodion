// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

package org.trafodion.sql.udr;
import java.nio.ByteBuffer;


/**
 *  Data types in the UDR interface
 *
 *  <p> Describes an SQL data type and the corresponding C/C++ type,
 *  used for scalar parameters, columns of input rows and
 *  columns of result rows.
 */
public class TypeInfo extends TMUDRSerializableObject {

    /** SQL data types */
    public enum SQLTypeCode
    {
        UNDEFINED_SQL_TYPE,
            /**  16 bit integer */   
        SMALLINT,   
            /** 32 bit integer */
        INT,         
            /** 64 bit integer */
        LARGEINT,  
            /** Numeric with decimal precision */
        NUMERIC,     
            /** Numeric with leading sign embedded */
        DECIMAL_LSE,   
            /** unsigned 16 bit integer */
        SMALLINT_UNSIGNED, 
            /** unsigned 32 bit integer */
        INT_UNSIGNED,  
            /** unsigned numeric */
        NUMERIC_UNSIGNED, 
            /** unsigned decimal */
        DECIMAL_UNSIGNED,  
            /** 4 byte floating point number */
        REAL,    
            /** 8 byte floating point number */
        DOUBLE_PRECISION,  
            /** fixed length character types */
        CHAR,       
            /** varying length character types */
        VARCHAR,      
            /** date */
        DATE,    
            /** time */
        TIME,          
            /** timestamp */
        TIMESTAMP,  
            /** interval */
        INTERVAL,   
            /** Binary Large Object */
        BLOB,  
            /** Character Large Object */
        CLOB,
            /**  8 bit integer */   
        TINYINT,   
            /** unsigned 8 bit integer */
        TINYINT_UNSIGNED,
            /** boolean */
        BOOLEAN
      };
  
    /** Classes of types defined in the SQL standard */
    public enum SQLTypeClassCode
    {
        /** char and varchar types */
        CHARACTER_TYPE,
            /**  exact and approximate numerics */
        NUMERIC_TYPE,
            /** date/time/timestamp */
        DATETIME_TYPE,
            /** day/month or hour/second intervals */
        INTERVAL_TYPE,
            /**  BLOBs and CLOBs */
        LOB_TYPE,
            /**  Boolean */
        BOOLEAN_TYPE,
            /**  undefined value */
        UNDEFINED_TYPE_CLASS
    };
  
    /** More detailed type information, but not as detailed as the actual type */
    public enum SQLTypeSubClassCode
    {
        /** CHAR type */
        FIXED_CHAR_TYPE,
            /** VARCHAR type */
        VAR_CHAR_TYPE,
            /** Exact numeric */
        EXACT_NUMERIC_TYPE,
            /**  Approximate numeric (floating point) */
        APPROXIMATE_NUMERIC_TYPE,
            /** Date */
        DATE_TYPE,
            /** Time */
        TIME_TYPE,
            /** Timestamp (date + time + optional fractional seconds) */
        TIMESTAMP_TYPE,
            /** Intervals involving year and month */
        YEAR_MONTH_INTERVAL_TYPE,
            /** Intervals involving days/hours/minutes/seconds */
        DAY_SECOND_INTERVAL_TYPE,
            /** LOBs */
        LOB_SUB_CLASS,
            /** Boolean */
        BOOLEAN_SUB_CLASS,
            /** undefined value */
        UNDEFINED_TYPE_SUB_CLASS 
    };
  
    /** Character sets */
    public enum SQLCharsetCode
    {
        UNDEFINED_CHARSET,
            /**  ISO 8859-1, single byte western European characters */
        CHARSET_ISO88591, 
            /** UTF-8, 1-4 byte Unicode encoding, length is in bytes */
        CHARSET_UTF8, 
            /** UCS-2, 16 bit Unicode encoding, tolerates UTF-16 */
        CHARSET_UCS2      
    };

    /** Collations */
    public enum SQLCollationCode
    {
        /** undefined value */
        UNDEFINED_COLLATION, 
            /** System collation, which is a binary collation, except that it ignores trailining blanks */ 
        SYSTEM_COLLATION  
    };

    /** Start and end fields of interval columns */
    // Note: same values as SQLINTERVAL_CODE in file sql/cli/sqlcli.h
    public enum SQLIntervalCode
    {
        UNDEFINED_INTERVAL_CODE (0),
        INTERVAL_YEAR           (1),
        INTERVAL_MONTH          (2),
        INTERVAL_DAY            (3),
        INTERVAL_HOUR           (4),
        INTERVAL_MINUTE         (5),
        INTERVAL_SECOND         (6),
        INTERVAL_YEAR_MONTH     (7),
        INTERVAL_DAY_HOUR       (8),
        INTERVAL_DAY_MINUTE     (9),
        INTERVAL_DAY_SECOND     (10),
        INTERVAL_HOUR_MINUTE    (11),
        INTERVAL_HOUR_SECOND    (12),
        INTERVAL_MINUTE_SECOND  (13) ;
        
        private final int intervalCode_;
        
        SQLIntervalCode(int val) {
            intervalCode_ = val;
        }
          
        public int getSQLIntervalCode() {
            return intervalCode_;
        }
    }

    /** Copy constructor */
    public TypeInfo(TypeInfo type) {
        super(TMUDRObjectType.TYPE_INFO_OBJ, getCurrentVersion());
        sqlType_ = type.sqlType_;
        nullable_ = type.nullable_;
        scale_ = type.scale_;
        charset_ = type.charset_;
        intervalCode_ = type.intervalCode_;
        precision_ = type.precision_;
        collation_ = type.collation_;
        length_ = type.length_;
        dataOffset_ = type.dataOffset_;
        nullIndOffset_ = type.nullIndOffset_;
        vcLenIndOffset_ = type.vcLenIndOffset_;
        flags_ = type.flags_;
        fillers0_ = 0 ;
        fillers1_ = 0;
        fillers2_ = 0;
        fillers3_ = 0;
    }

/**
 *  Constructor with arguments
 *
 *  <p> Construct a TypeInfo object from an SQL type, with several
 *  arguments (including the SQL type). This is mostly used to create
 *  formal parameters or output columns in the compiler interface, if
 *  a more complex data type is required that is not covered by the
 *  TupleInfo.addXXXColumn() methods.
 *
 *  @param sqlType       SQL type enum to construct the type from.
 *  @param length        Length of CHAR/VARCHAR types, not needed for other types.
 *                       Note that the length for UTF-8 types is in bytes, not
 *                       characters, so this is equivalent to
 *                       <br> [VAR]CHAR (<code> <b> length </b> </code> BYTES) CHARACTER SET UTF8
 *  @param nullable      Determines the NULL / NOT NULL attribute of the type
 *                      false means NOT NULL
 *  @param scale         Scale for numeric type, fraction precision for
 *                       fractional seconds, not needed for other types.
 *  @param charset       Character set enum for CHAR/VARCHAR types, not needed
 *                       for other types.
 *  @param intervalCode  Interval code enum for intervals, not needed otherwise.
 *  @param precision     Precision for numeric types and leading precision for
 *                       interval data types.
 *  @param collation     Collation enum for CHAR/VARCHAR types, not needed for
 *                       other types. Note that only one type of collation is
 *                       currently supported.
 *  @throws UDRException
 */
    public TypeInfo(SQLTypeCode sqlType,
                    int length,
                    boolean nullable,
                    int scale,
                    SQLCharsetCode charset,
                    SQLIntervalCode intervalCode,
                    int precision,
                    SQLCollationCode collation) throws UDRException {
        super(TMUDRObjectType.TYPE_INFO_OBJ, getCurrentVersion());
        sqlType_ = sqlType.ordinal();
        nullable_ = (nullable ? 1 : 0 );
        scale_ = scale;
        charset_ = charset.ordinal();
        intervalCode_ = intervalCode.ordinal();
        precision_ = precision;
        collation_ = collation.ordinal();
        length_ = length;
        dataOffset_ = -1;
        nullIndOffset_ = -1;
        vcLenIndOffset_ = -1;
        flags_ = 0;
        fillers0_ = 0;
        fillers1_ = 0;
        fillers2_ = 0;
        fillers3_ = 0;
        
        switch (sqlType)
        {
        case TINYINT:
            length_ = 1;
            precision_ = 0;
            scale_ = 0;
            break;

        case SMALLINT:
            length_ = 2;
            precision_ = 0;
            scale_ = 0;
            break;

        case INT:
            length_ = 4;
            precision_ = 0;
            scale_ = 0;
            break;
            
        case LARGEINT:
            length_ = 8;
            precision_ = 0;
            scale_ = 0;
            break;
            
        case NUMERIC:
            length_ = convertToBinaryPrecision(precision_);
            if (scale_ < 0 || scale_ > 18)
                throw new UDRException(38900,"Scale %d of a numeric in TypeInfo::TypeInfo is out of the allowed range of 0-18", scale_);
            if (scale_ > precision_)
                throw new UDRException(38900,"Scale %d of a numeric in TypeInfo::TypeInfo is greater than precision %d", scale_, precision_);
            break;
            
        case DECIMAL_LSE:
            if (scale_ < 0 || scale_ > 18)
                throw new UDRException(38900,"Scale %d of a decimal in TypeInfo::TypeInfo is out of the allowed range of 0-18", scale_);
            if (precision_ < 1 || precision_ > 18)
                throw new UDRException(38900,"Precision %d of a decimal in TypeInfo::TypeInfo is out of the allowed range of 1-18", precision_);
            if (scale_ > precision_)
                throw new UDRException(38900,"Scale %d of a decimal in TypeInfo::TypeInfo is greater than precision %d", scale_, precision_);
            // format [-]mmmm[.sss]  - total number of digits = precision
            length_ = precision_ + 1; // add one for the sign
            if (scale_ > 0)
                length_ += 1; // for the decimal point
            break;
            
        case TINYINT_UNSIGNED:
            length_ = 1;
            precision_ = 0;
            scale_ = 0;
            break;
            
        case SMALLINT_UNSIGNED:
            length_ = 2;
            precision_ = 0;
            scale_ = 0;
            break;
            
        case INT_UNSIGNED:
            length_ = 4;
            precision_ = 0;
            scale_ = 0;
            break;
            
        case NUMERIC_UNSIGNED:
            length_ = convertToBinaryPrecision(precision_);
            if (scale_ < 0 || scale > 18)
                throw new UDRException(38900,"Scale %d of a numeric unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", scale_);
            if (scale > precision)
                throw new UDRException(38900,"Scale %d of a numeric unsigned in TypeInfo::TypeInfo is greater than precision %d", scale_, precision_);
            break;
            
        case DECIMAL_UNSIGNED:
            if (scale < 0 || scale > 18)
                throw new UDRException(38900,"Scale %d of a decimal unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", scale_);
            if (precision_ < 1 || precision_ > 18)
                throw new UDRException(38900,"Precision %d of a decimal unsigned in TypeInfo::TypeInfo is out of the allowed range of 1-18", precision_);
            if (scale > precision)
                throw new UDRException(38900,"Scale %d of a decimal unsigned in TypeInfo::TypeInfo is greater than precision %d", scale_, precision_);
            // format mmmm[.sss]  - total number of digits = precision
            length_ = precision_;
            if (scale_ > 0)
                length_ += 1; // for the decimal point
            break;
            
        case REAL:
            length_ = 4;
            break;
            
        case DOUBLE_PRECISION:
            length_ = 8;
            break;
            
        case CHAR:
            if (charset_ == SQLCharsetCode.UNDEFINED_CHARSET.ordinal())
                throw new UDRException(38900,"Charset must be specified for CHAR type in TypeInfo::TypeInfo");
            // length is the length in characters, but length_ is
            // the byte length, multiply by min bytes per char
            length_ = length * minBytesPerChar();
            if (length_ < 0)
                throw new UDRException(38900,"Length of a character type must not be negative, got %d",
                                       length_);
            if (collation_ == SQLCollationCode.UNDEFINED_COLLATION.ordinal())
                throw new UDRException(38900,"Collation must be specified for CHAR type in TypeInfo::TypeInfo");
            break;
            
        case VARCHAR:
            if (charset_ == SQLCharsetCode.UNDEFINED_CHARSET.ordinal())
                throw new UDRException(38900,"Charset must be specified for VARCHAR type in TypeInfo::TypeInfo");
            if (collation_ == SQLCollationCode.UNDEFINED_COLLATION.ordinal())
                throw new UDRException(38900,"Collation must be specified for VARCHAR type in TypeInfo::TypeInfo");
            // length is the length in characters, but length_ is
            // the byte length, multiply by min bytes per char
            length_ = length * minBytesPerChar();
            if (length_ > 32767)
                // see also CharType::CharType in ../common/CharType.cpp
                flags_ |= TYPE_FLAG_4_BYTE_VC_LEN;
            if (length_ < 0)
                throw new UDRException(38900,"Length of a varchar type must not be negative, got %d",
                                       length_);
            break;

        case CLOB:
        case BLOB:
            // BLOB and CLOB are represented by a handle that looks like a VARCHAR
            // but may contain binary data (use ISO8859-1 to be able to represent
            // binary data)
            charset_ = SQLCharsetCode.CHARSET_ISO88591.ordinal();
            // should we check the provided length if it comes from the UDR writer?
            // or just let it error out at runtime with an overflow?
            break;

        case DATE:
            // string yyyy-mm-dd
            length_ = 10;
            scale_ = 0;
            break;
            
        case TIME:
            // string hh:mm:ss[.fffffffff]
            length_ = 8;
            if (scale > 0)
                length_ += scale+1;
            if (scale < 0 || scale > 9)
                throw new UDRException(38900,"Scale %d of time in TypeInfo::TypeInfo is outside the allowed range of 0-9", scale);
            break;
            
        case TIMESTAMP:
            // string yyyy-mm-dd hh:mm:ss[.fffffffff]
            //        1234567890123456789 0123456789
            length_ = 19;
            if (scale > 0)
                length_ += scale+1;
            if (scale < 0 || scale > 9)
                throw new UDRException(38900,"Scale %d of timestamp in TypeInfo::TypeInfo is outside the allowed range of 0-9", scale);
            break;

        case INTERVAL:
        {
          int totalPrecision = 0;
          boolean allowScale = false;
          
          if (intervalCode_ == SQLIntervalCode.UNDEFINED_INTERVAL_CODE.getSQLIntervalCode())
              throw new UDRException(38900,"Interval code in TypeInfo::TypeInfo is undefined");
          if (scale < 0 || scale > 9)
            throw new UDRException(38900,"Scale %d of interval in TypeInfo::TypeInfo is outside the allowed range of 0-9", sqlType);

        // all intervals are treated like signed numbers, need to compute
        // the length from the combined precision of all parts, see method
        // IntervalType::getStorageSize() in ../common/IntervalType.cpp and
        // see also the defaults for leading precision in the SQL Reference
        // Manual. Note that the default for fraction precision in this
        // constructor is 0, the default scale for other types. This is
        // different from the default fraction precision of 6 in Trafodion
        // SQL!!

        // start with the leading precision
        if (precision == 0)
            totalPrecision = 2; // default leading precision
        else
            totalPrecision = precision;

        switch (getIntervalCode())
        {
          case INTERVAL_YEAR:
          case INTERVAL_MONTH:
          case INTERVAL_DAY:
          case INTERVAL_HOUR:
          case INTERVAL_MINUTE:
            // we are all set
            break;
          case INTERVAL_SECOND:
            // add the fraction precision (scale)
            totalPrecision += scale;
            allowScale = true;
            break;
          case INTERVAL_YEAR_MONTH:
          case INTERVAL_DAY_HOUR:
          case INTERVAL_HOUR_MINUTE:
            // leading field + 1 more field
            totalPrecision += 2;
            break;
          case INTERVAL_DAY_MINUTE:
            // leading field + 2 more fields
            totalPrecision += 4;
            break;
          case INTERVAL_DAY_SECOND:
            totalPrecision += 6 + scale;
            allowScale = true;
            break;
          case INTERVAL_HOUR_SECOND:
            totalPrecision += 4 + scale;
            allowScale = true;
            break;
          case INTERVAL_MINUTE_SECOND:
            totalPrecision += 2 + scale;
            allowScale = true;
            break;
          default:
            throw new UDRException(
                                   38900,
                                   "TypeInfo::TypeInfo() for interval type with invalid interval code");
          }

        if (scale > 0 && !allowScale)
          throw new UDRException(
               38900,
               "TypeInfo::TypeInfo(): Scale (fraction precision) should not be specified for a type when end field is not SECOND");
        
        // convert decimal to binary precision
        length_ = convertToBinaryPrecision(totalPrecision);
        if (length_ == 1)
            // intervals don't use single byte representation (yet?)
            length_ = 2;
        }
        break;

        case BOOLEAN:
            length_ = 1;
            precision_ = 0;
            scale_ = 0;
            break;

        case UNDEFINED_SQL_TYPE:
            // this case is reached when we call the default constructor,
            // type and other fields still need to be defined
            break;

        default:
            throw new UDRException(38900,"Invalid SQL Type code for the short TypeInfo constructor with an SQL code: %d", sqlType);
        }         
    }

    /**
     *  Constructor with all parameters except collation
     *
     *  @param sqlType       SQL type enum to construct the type from.
     *  @param length        Length of CHAR/VARCHAR types, not needed for other types.
     *                       Note that the length for UTF-8 types is in bytes, not
     *                       characters, so this is equivalent to
     *                       <br> [VAR]CHAR (<code> <b> length </b> </code> BYTES) CHARACTER SET UTF8
     *  @param nullable      Determines the NULL / NOT NULL attribute of the type
     *                       false means NOT NULL
     *  @param scale         Scale for numeric type, fraction precision for
     *                       fractional seconds, not needed for other types.
     *  @param charset       Character set enum for CHAR/VARCHAR types, not needed
     *                       for other types.
     *  @param intervalCode  Interval code enum for intervals, not needed otherwise.
     *  @param precision     Precision for numeric types and leading precision for
     *                       interval data types.
     *  @throws UDRException
     */
    public TypeInfo(SQLTypeCode sqlType,
                    int length,
                    boolean nullable,
                    int scale,
                    SQLCharsetCode charset,
                    SQLIntervalCode intervalCode,
                    int precision) throws UDRException {
        this(sqlType,
             length,
             nullable,
             scale,
             charset,
             intervalCode,
             precision,
             SQLCollationCode.SYSTEM_COLLATION);
    }
    
    /**
     *  Constructor with all parameters except precision and collation
     *
     *  @param sqlType       SQL type enum to construct the type from.
     *  @param length        Length of CHAR/VARCHAR types, not needed for other types.
     *                       Note that the length for UTF-8 types is in bytes, not
     *                       characters, so this is equivalent to
     *                       <br> [VAR]CHAR (<code> <b> length </b> </code> BYTES) CHARACTER SET UTF8
     *  @param nullable      Determines the NULL / NOT NULL attribute of the type
     *                       false means NOT NULL
     *  @param scale         Scale for numeric type, fraction precision for
     *                       fractional seconds, not needed for other types.
     *  @param charset       Character set enum for CHAR/VARCHAR types, not needed
     *                       for other types.
     *  @param intervalCode  Interval code enum for intervals, not needed otherwise.
     *  @throws UDRException
     */
    public TypeInfo(SQLTypeCode sqlType,
                    int length,
                    boolean nullable,
                    int scale,
                    SQLCharsetCode charset,
                    SQLIntervalCode intervalCode) throws UDRException {
        this(sqlType,
             length,
             nullable,
             scale,
             charset,
             intervalCode,
             0, // precision
             SQLCollationCode.SYSTEM_COLLATION);
    }

    /**
     *  Constructor with type, length, nullable, scale and charset
     *
     *  @param sqlType       SQL type enum to construct the type from.
     *  @param length        Length of CHAR/VARCHAR types, not needed for other types.
     *                       Note that the length for UTF-8 types is in bytes, not
     *                       characters, so this is equivalent to
     *                       <br> [VAR]CHAR (<code> <b> length </b> </code> BYTES) CHARACTER SET UTF8
     *  @param nullable      Determines the NULL / NOT NULL attribute of the type
     *                       false means NOT NULL
     *  @param scale         Scale for numeric type, fraction precision for
     *                       fractional seconds, not needed for other types.
     *  @param charset       Character set enum for CHAR/VARCHAR types, not needed
     *                       for other types.
     *  @throws UDRException
     */
    public TypeInfo(SQLTypeCode sqlType,
                    int length,
                    boolean nullable,
                    int scale,
                    SQLCharsetCode charset) throws UDRException {
        this(sqlType,
             length,
             nullable,
             scale,
             charset,
             SQLIntervalCode.UNDEFINED_INTERVAL_CODE,
             0, // precision
             SQLCollationCode.SYSTEM_COLLATION);
    }

    /**
     *  Constructor with type, length, nullable and scale
     *
     *  @param sqlType       SQL type enum to construct the type from.
     *  @param length        Length of CHAR/VARCHAR types, not needed for other types.
     *                       Note that the length for UTF-8 types is in bytes, not
     *                       characters, so this is equivalent to
     *                       <br> [VAR]CHAR (<code> <b> length </b> </code> BYTES) CHARACTER SET UTF8
     *  @param nullable      Determines the NULL / NOT NULL attribute of the type
     *                       false means NOT NULL
     *  @param scale         Scale for numeric type, fraction precision for
     *                       fractional seconds, not needed for other types.
     *  @throws UDRException
     */
    public TypeInfo(SQLTypeCode sqlType,
                    int length,
                    boolean nullable,
                    int scale) throws UDRException {
        this(sqlType,
             length,
             nullable,
             scale,
             SQLCharsetCode.CHARSET_UTF8,
             SQLIntervalCode.UNDEFINED_INTERVAL_CODE,
             0, // precision
             SQLCollationCode.SYSTEM_COLLATION);
    }

    /**
     *  Constructor with type, length, and nullable
     *
     *  @param sqlType       SQL type enum to construct the type from.
     *  @param length        Length of CHAR/VARCHAR types, not needed for other types.
     *                       Note that the length for UTF-8 types is in bytes, not
     *                       characters, so this is equivalent to
     *                       <br> [VAR]CHAR (<code> <b> length </b> </code> BYTES) CHARACTER SET UTF8
     *  @param nullable      Determines the NULL / NOT NULL attribute of the type
     *                       false means NOT NULL
     *  @throws UDRException
     */
    public TypeInfo(SQLTypeCode sqlType,
                    int length,
                    boolean nullable) throws UDRException {
        this(sqlType,
             length,
             nullable,
             0, //scale
             SQLCharsetCode.CHARSET_UTF8,
             SQLIntervalCode.UNDEFINED_INTERVAL_CODE,
             0, // precision
             SQLCollationCode.SYSTEM_COLLATION);
    }

    /**
     *  Constructor with type, and length
     *
     *  @param sqlType       SQL type enum to construct the type from.
     *  @param length        Length of CHAR/VARCHAR types, not needed for other types.
     *                       Note that the length for UTF-8 types is in bytes, not
     *                       characters, so this is equivalent to
     *                       <br> [VAR]CHAR (<code> <b> length </b> </code> BYTES) CHARACTER SET UTF8
     *  @throws UDRException
     */
    public TypeInfo(SQLTypeCode sqlType,
                    int length) throws UDRException {
        this(sqlType,
             length,
             false, // nullable
             0, //scale
             SQLCharsetCode.CHARSET_UTF8,
             SQLIntervalCode.UNDEFINED_INTERVAL_CODE,
             0, // precision
             SQLCollationCode.SYSTEM_COLLATION);
    }

    /**
     *  Constructor with type
     *  @param  sqlType       SQL type enum to construct the type from.
     *  @throws UDRException
     */
    public TypeInfo(SQLTypeCode sqlType) throws UDRException {
        this(sqlType,
             0, // length
             false, // nullable
             0, // scale
             SQLCharsetCode.CHARSET_UTF8,
             SQLIntervalCode.UNDEFINED_INTERVAL_CODE,
             0, // precision
             SQLCollationCode.SYSTEM_COLLATION);
    }
    
     /**
     *  Default constructor
     *  @throws UDRException
     */
    public TypeInfo() throws UDRException {
        this(SQLTypeCode.UNDEFINED_SQL_TYPE,
             0, // length
             false, // nullable
             0, // scale
             SQLCharsetCode.CHARSET_UTF8,
             SQLIntervalCode.UNDEFINED_INTERVAL_CODE,
             0, // precision
             SQLCollationCode.SYSTEM_COLLATION);
    }

    // this constructor should not be used by UDR writers
    public TypeInfo(SQLTypeCode sqlType,
                    boolean nullable,
                    int scale,
                    SQLCharsetCode charset,
                    SQLIntervalCode intervalCode,
                    int precision,
                    SQLCollationCode collation,
                    int length) throws UDRException {
        this(sqlType,
             length,
             nullable,
             scale,
             charset,
             intervalCode,
             precision,
             collation);
    }

    /**
     *  Get the SQL type.
     *  @return SQL type enum.
     */
    public SQLTypeCode getSQLType() {
        switch (sqlType_)
        {
        case 0:
            return SQLTypeCode.UNDEFINED_SQL_TYPE;
        case 1: 
            return SQLTypeCode.SMALLINT;   
        case 2:
            return SQLTypeCode.INT;         
        case 3:
            return SQLTypeCode.LARGEINT;  
        case 4:
            return SQLTypeCode.NUMERIC;     
        case 5:
            return SQLTypeCode.DECIMAL_LSE;   
        case 6:
            return SQLTypeCode.SMALLINT_UNSIGNED; 
        case 7:
            return SQLTypeCode.INT_UNSIGNED;  
        case 8:
            return SQLTypeCode.NUMERIC_UNSIGNED; 
        case 9:
            return SQLTypeCode.DECIMAL_UNSIGNED;  
        case 10:
            return SQLTypeCode.REAL;    
        case 11:
            return SQLTypeCode.DOUBLE_PRECISION;  
        case 12:
            return SQLTypeCode.CHAR;       
        case 13:
            return SQLTypeCode.VARCHAR;      
        case 14:
            return SQLTypeCode.DATE;    
        case 15:
            return SQLTypeCode.TIME;          
        case 16:
            return SQLTypeCode.TIMESTAMP;  
        case 17:
            return SQLTypeCode.INTERVAL;   
        case 18:
            return SQLTypeCode.BLOB;  
        case 19:
            return SQLTypeCode.CLOB;
        case 20: 
            return SQLTypeCode.TINYINT;   
        case 21:
            return SQLTypeCode.TINYINT_UNSIGNED; 
        case 22:
            return SQLTypeCode.BOOLEAN; 
        default:
            return SQLTypeCode.UNDEFINED_SQL_TYPE;  
        }
    }
    
    /**
     *  Get the SQL type class.
     *  Determine whether this is a numeric character, datetime or interval type.
     *  @return SQL type class enum.
     */
    public SQLTypeClassCode getSQLTypeClass() {
        switch (sqlType_)
        {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 20:
        case 21:
            return SQLTypeClassCode.NUMERIC_TYPE;
      
        case 12:
        case 13:
            return SQLTypeClassCode.CHARACTER_TYPE;
            
        case 14:
        case 15:
        case 16:
            return SQLTypeClassCode.DATETIME_TYPE;
            
        case 17:
            return SQLTypeClassCode.INTERVAL_TYPE;
      
        case 18:
        case 19:
            return SQLTypeClassCode.LOB_TYPE;
            
        case 22:
            return SQLTypeClassCode.BOOLEAN_TYPE;

        case 0:
        default:
            break;
        }

        return SQLTypeClassCode.UNDEFINED_TYPE_CLASS;
    }

    /**
     *  Get the SQL type subclass.
     *  This goes to one more level of detail beyond the type class,
     *  like exact/approximate numeric, char/varchar, etc.
     *  @return SQL type subclass enum.
     */
    public SQLTypeSubClassCode getSQLTypeSubClass() {
        switch (sqlType_)
        {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 20:
        case 21:
            return SQLTypeSubClassCode.EXACT_NUMERIC_TYPE;
        case 10:
        case 11:
            return SQLTypeSubClassCode.APPROXIMATE_NUMERIC_TYPE;
        
        case 12:
            return SQLTypeSubClassCode.FIXED_CHAR_TYPE;
        case 13:
            return SQLTypeSubClassCode.VAR_CHAR_TYPE;
            
        case 14:
            return SQLTypeSubClassCode.DATE_TYPE;
        case 15:
            return SQLTypeSubClassCode.TIME_TYPE;
        case 16:
            return SQLTypeSubClassCode.TIMESTAMP_TYPE;
            
        case 17:
            switch (intervalCode_)
            {
            case 1:
            case 2:
            case 7:
                return SQLTypeSubClassCode.YEAR_MONTH_INTERVAL_TYPE;
                
            case 3:
            case 4:
            case 5:
            case 6:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
                return SQLTypeSubClassCode.DAY_SECOND_INTERVAL_TYPE;
                
            case 0:
            default:
                break;
            }
            break ;
            
        case 18:
        case 19:
            return SQLTypeSubClassCode.LOB_SUB_CLASS;

        case 22:
            return SQLTypeSubClassCode.BOOLEAN_SUB_CLASS;
            
        case 0:
        default:
            break;
        }

        return SQLTypeSubClassCode.UNDEFINED_TYPE_SUB_CLASS;
    }

    /**
     *  Get whether the type is nullable.
     *  @return True for nullable types, false for non-nullable types.
     */
    public boolean getIsNullable() {
        return (nullable_ != 0);
    }
    
    /**
     *  Get the scale of the data type.
     *
     *  <p> For integer, largeint, etc. types the scale is 0, since these are
     *  integer data types. For NUMERIC and DECIMAL types, a scale can
     *  be specified. Timestamp and some interval data types have a
     *  "fraction precision" value, which is the number of digits
     *  allowed after the decimal point for seconds. This fraction precision
     *  is returned as the scale, since can be considered the scale of
     *  the seconds part. For other data types like CHAR, the scale
     *  value is meaningless.
     *  @return Scale (digits after the decimal point) for numeric types,
     *          fraction precision (digits of fractional seconds) for intervals.
     */
    public int getScale() {
        return scale_ ;
    }

    /**
     *  Get the character set of the data type.
     *  @return Character set enum.
     */
    public SQLCharsetCode getCharset() {
        switch (charset_)
        {
        case 1:
            return SQLCharsetCode.CHARSET_ISO88591; 
        case 2:
            return SQLCharsetCode.CHARSET_UTF8; 
        case 3:
            return SQLCharsetCode.CHARSET_UCS2;
        case 0:
        default:
            break;
        }
        return SQLCharsetCode.UNDEFINED_CHARSET;
    }

    /**
     *  Get the interval code for start/end fields.
     *  @return Interval code enum, indicating start and end fields of an interval type.
     */
    public SQLIntervalCode getIntervalCode() {
        switch (intervalCode_)
        {
        case 1:
            return SQLIntervalCode.INTERVAL_YEAR;
        case 2:
            return SQLIntervalCode.INTERVAL_MONTH;
        case 3:
            return SQLIntervalCode.INTERVAL_DAY;
        case 4:
            return SQLIntervalCode.INTERVAL_HOUR;
        case 5:
            return SQLIntervalCode.INTERVAL_MINUTE;
        case 6:
            return SQLIntervalCode.INTERVAL_SECOND;
        case 7:
            return SQLIntervalCode.INTERVAL_YEAR_MONTH;
        case 8:
            return SQLIntervalCode.INTERVAL_DAY_HOUR;
        case 9:
            return SQLIntervalCode.INTERVAL_DAY_MINUTE;
        case 10:
            return SQLIntervalCode.INTERVAL_DAY_SECOND;
        case 11:
            return SQLIntervalCode.INTERVAL_HOUR_MINUTE;
        case 12:
            return SQLIntervalCode.INTERVAL_HOUR_SECOND;
        case 13:
            return SQLIntervalCode.INTERVAL_MINUTE_SECOND;
        case 0:
        default:
            break;
        }
        return SQLIntervalCode.UNDEFINED_INTERVAL_CODE;
    }

    /**
     *  Get the precision (max. number of significant digits).
     *
     *  The precision is the maximum number of digits before the decimal
     *  point a value can have. For interval types, this is the "leading
     *  precision". For example, an INTEGER value can range from
     *  -2,147,483,648 to 2,147,483,647. It's precision is 10, since the
     *  longest number has 10 digits. Note that not all 10 digit numbers
     *  can be represented in an integer. This is called binary
     *  precision. NUMERIC and DECIMAL types have decimal precision,
     *  meaning that a NUMERIC(10,0) type can represent values from
     *  -9,999,999,999 to +9,999,999,999.
     *
     *  @return Precision of numeric types or interval types.
     */
    public int getPrecision() {
        return precision_;
    }

    public int getFlags() {
        return flags_ ;
    }
    
    /**
     *  Get the collation for char/varchar data types.
     *
     *  <p> Note that, currently, only one collation is supported.
     *  This default collation is a binary collation, except that
     *  trailing blanks are ignored.
     *
     *  @return Collation enum.
     */
    public SQLCollationCode getCollation() {
        if (collation_ == 0)
            return SQLCollationCode.UNDEFINED_COLLATION ;
        else
            return SQLCollationCode.SYSTEM_COLLATION;
    }

    /**
     *  Get the length of a value of the type.
     *
     *  <p> Getting the length is useful for CHAR/VARCHAR data types
     *  but probably not as useful for other types that may have
     *  an internal representation unknown to a UDR writer.
     *  This returns the length in bytes, not in characters.
     *
     *  @see TypeInfo#getMaxCharLength()
     *
     *  @return Length in bytes.
     */
    public int getByteLength() {
        return length_ ;
    }

    /**
     *  Get the maximum number of characters that can be stored in this type.
     *
     *  <p> This method should be used only for character types that
     *  have a fixed-width encoding. For variable-length encoding, like
     *  UTF-8, the method returns the highest possible number of characters
     *  (assuming single byte characters in the case of UTF-8). Right now,
     *  UTF-8 data types all have byte semantics, meaning there is no
     *  limit for the number of characters stored in a type, it is only
     *  limited by the number of bytes. The method returns 0 for numeric
     *  types. It returns the length of the string representation for
     *  types that are represented by a string, like datetime types.
     *
     *  @see TypeInfo#getByteLength()
     *
     *  @return Length in bytes.
     *  @throws UDRException
     */
    public int getMaxCharLength() throws UDRException {
        switch (getSQLTypeClass())
        {
        case CHARACTER_TYPE:
            return length_ / minBytesPerChar();
        case NUMERIC_TYPE:
            return 0;
        case DATETIME_TYPE:
            // return the length of the string representation
            // in ISO88591/UTF-8
            return length_;
        case INTERVAL_TYPE:
            return 0;
        default:
            throw new UDRException(
                                   38900,
                                   "Called TypeInfo::getMaxCharLength() on an unsupported type: %d",
                                   sqlType_);
        }
    }

    /**
     *  Set the nullable attribute of a type
     *
     *  Use this method to set types created locally in the UDF
     *  to be nullable or not nullable.
     *
     *  @param nullable true to set the type to nullable, false
     *                  to give the type the NOT NULL attibute.
     */
    public void setNullable(boolean nullable) {
        nullable_ = (nullable ? 1 : 0 );
    }

    // UDR writers can ignore these methods

    
    boolean isAvailable() {
        return (dataOffset_ >= 0);
    }

    int minBytesPerChar() throws UDRException {
        switch (getCharset())
        {
        case CHARSET_ISO88591:
        case CHARSET_UTF8:
            return 1;
        case CHARSET_UCS2:
            return 2;
        default:
            throw new UDRException(
                                   38900, "Minimum bytes per char not defined for charset %d",
                                   charset_);
        }
    }

    int convertToBinaryPrecision(int decimalPrecision) throws UDRException {
        if (decimalPrecision < 1 || decimalPrecision > 18)
            throw new UDRException(
                                   38900,
                                   "Decimal precision %d is out of the allowed range of 1-18",
                                   decimalPrecision);
        
        if (decimalPrecision < 3)
            return 1;
        else if (decimalPrecision < 5)
            return 2;
        else if (decimalPrecision < 10)
            return 4;
        else
            return 8;
    }

    String toString(boolean longForm) {
        StringBuilder sb = new StringBuilder();
        switch (getSQLType())
        {
        case UNDEFINED_SQL_TYPE:
            sb.append("undefined_sql_type");
            break;
        case TINYINT:
            sb.append("TINYINT");
            break;
        case SMALLINT:
            sb.append("SMALLINT");
            break;
        case INT:
            sb.append("INT");
            break;
        case LARGEINT:
            sb.append("LARGEINT");
            break;
        case NUMERIC:
            sb.append(String.format("NUMERIC(%d,%d)",getPrecision(), getScale()));
            break;
        case DECIMAL_LSE:
            sb.append(String.format("DECIMAL(%d,%d)",getPrecision(), getScale()));
            break;
        case TINYINT_UNSIGNED:
            sb.append("TINYINT UNSIGNED");
            break;
        case SMALLINT_UNSIGNED:
            sb.append("SMALLINT UNSIGNED");
            break;
        case INT_UNSIGNED:
            sb.append("INT UNSIGNED");
            break;
        case NUMERIC_UNSIGNED:
            sb.append(String.format("NUMERIC(%d,%d) UNSIGNED",getPrecision(), getScale()));
            break;
        case DECIMAL_UNSIGNED:
            sb.append(String.format("DECIMAL(%d,%d) UNSIGNED",getPrecision(), getScale()));
            break;
        case REAL:
            sb.append("REAL");
            break;
        case DOUBLE_PRECISION:
            sb.append("DOUBLE PRECISION");
            break;
        case CHAR:
        case VARCHAR:
            StringBuilder csName = new StringBuilder();
            switch(getCharset())
            {
            case UNDEFINED_CHARSET:
                csName.append("undefined");
                break;
            case CHARSET_ISO88591:
                csName.append("ISO88591");
                break;
            case CHARSET_UTF8:
                csName.append("UTF8");
                break;
            case CHARSET_UCS2:
                csName.append("UCS2");
                break;
            default:
                csName.append("invalid charset!");
                break;
            }

            try 
                {
                    sb.append(String.format("%s(%d%s) CHARACTER SET ",
                                            (sqlType_ == SQLTypeCode.CHAR.ordinal() ? "CHAR" : "VARCHAR"),
                                            getMaxCharLength(),
                                            (getCharset() == SQLCharsetCode.CHARSET_UTF8 ? " BYTES" : "")));
                }
            catch (UDRException e1)
                {
                    sb.append(String.format("%s(undefined-max-length%s) CHARACTER SET ",
                                        (sqlType_ == SQLTypeCode.CHAR.ordinal() ? "CHAR" : "VARCHAR"),
                                        (getCharset() == SQLCharsetCode.CHARSET_UTF8 ? " BYTES" : "")));
                }
            sb.append(csName.toString());
            break;
        case DATE:
            sb.append("DATE");
            break;
        case TIME:
            sb.append("TIME");
            if (scale_ > 0)
                sb.append(String.format("(%d)", scale_));
            break;
        case TIMESTAMP:
            sb.append(String.format("TIMESTAMP(%d)",scale_));
            break;
        case INTERVAL:
            switch (getIntervalCode())
            {
            case UNDEFINED_INTERVAL_CODE:
                sb.append("INTERVAL with undefined subtype!");
                break;
            case INTERVAL_YEAR:
                sb.append(String.format("INTERVAL YEAR(%d)",getPrecision()));
                break;
            case INTERVAL_MONTH:
                sb.append(String.format("INTERVAL MONTH(%d)",getPrecision()));
                break;
            case INTERVAL_DAY:
                sb.append(String.format("INTERVAL DAY(%d)",getPrecision()));
                break;
            case INTERVAL_HOUR:
                sb.append(String.format("INTERVAL HOUR(%d)",getPrecision()));
                break;
            case INTERVAL_MINUTE:
                sb.append(String.format("INTERVAL MINUTE(%d)",getPrecision()));
                break;
            case INTERVAL_SECOND:
                sb.append(String.format("INTERVAL SECOND(%d,%d)",getPrecision(), 
                                        getScale()));
                break;
            case INTERVAL_YEAR_MONTH:
                sb.append(String.format("INTERVAL YEAR(%d) TO MONTH",
                                        getPrecision()));
                break;
            case INTERVAL_DAY_HOUR:
                sb.append(String.format("INTERVAL DAY(%d) TO HOUR",
                                        getPrecision()));
                break;
            case INTERVAL_DAY_MINUTE:
                sb.append(String.format("INTERVAL DAY(%d) TO MINUTE",
                                        getPrecision()));
                break;
            case INTERVAL_DAY_SECOND:
                sb.append(String.format("INTERVAL DAY(%d) TO SECOND(%d)",
                                        getPrecision(), getScale()));
                break;
            case INTERVAL_HOUR_MINUTE:
                sb.append(String.format("INTERVAL HOUR(%d) TO MINUTE",
                                        getPrecision()));
                break;
            case INTERVAL_HOUR_SECOND:
                sb.append(String.format("INTERVAL HOUR(%d) TO SECOND(%d)",
                                        getPrecision(), getScale()));
                break;
            case INTERVAL_MINUTE_SECOND:
                sb.append(String.format("INTERVAL MINUTE(%d) TO SECOND(%d)",
                                        getPrecision(), getScale()));
                break;
            default:
                sb.append("invalid interval code!");
                break;
            }
            break;
        case BLOB:
            sb.append("BLOB");
            break;
        case CLOB:
            sb.append("CLOB");
            break;
        case BOOLEAN:
            sb.append("BOOLEAN");
            break;
        default:
            sb.append("invalid SQL type!");
            break;
        }

        if (nullable_ == 0)
            sb.append(" NOT NULL");

        if (longForm && dataOffset_ >= 0)
        {
            sb.append(String.format(" offsets: (nullInd=%d, vcLen=%d, data=%d)",
                                    nullIndOffset_, vcLenIndOffset_, dataOffset_));
        }
        return sb.toString();
    }

    void putEncodedStringIntoByteBuffer(ByteBuffer tgt, ByteBuffer src) throws UDRException
    {
        // Copy the byte buffer with encoded characters into the target buffer.
        // For fixed-length strings, remove or add trailing blanks as necessary.
        // For variable length strings, set the varLen indicator.
        int trimmedLen = src.limit();
        int minBytesPerChar = minBytesPerChar();
        boolean isFixedChar = (vcLenIndOffset_ < 0); // fixed chars can be trimmed
        boolean okToTrim = true;

        if (trimmedLen > length_)
            {
                if (isFixedChar)
                    if (minBytesPerChar == 1)
                        {
                            // are all the extra characters blanks?
                            for (int i=length_; i<trimmedLen; i++)
                                if (src.get(i) != (byte) ' ')
                                    okToTrim = false;
                        }
                    else if (getCharset() == SQLCharsetCode.CHARSET_UCS2)
                        {
                            // are all the extra bytes little-endian UCS-2 blanks?
                            for (int i=length_/2; i<trimmedLen/2; i++)
                                if (src.get(2*i) != (byte) ' ' ||
                                    src.get(2*i+1) != 0)
                                    okToTrim = false;
                            if (okToTrim)
                                trimmedLen = length_;
                        }
                    else
                        // don't know how to trim this charset
                        okToTrim = false;
                else
                    // not allowed to trim varchars
                    okToTrim = false;

                if (okToTrim)
                    trimmedLen = length_;
                else
                    throw new UDRException(38900,
                                           "String overflow, trying to assign a string with an encoded length of %d bytes (charset %d) into a target type %d bytes wide",
                                           trimmedLen,
                                           charset_,
                                           length_);
            }

        // transfer "trimmedLen" bytes from the beginning of src into tgt,
        // starting at dataOffset_
        src.limit(trimmedLen);
        src.rewind();
        tgt.position(dataOffset_);
        tgt.put(src);

        if (isFixedChar)
            if (minBytesPerChar == 1)
                while (trimmedLen < length_)
                    {
                        tgt.put((byte) ' ');
                        trimmedLen++;
                    }
            else
               while (trimmedLen < length_)
                    {
                        tgt.putShort((short) ' ');
                        trimmedLen += 2;
                    }
        else
            {
                // set the varchar length indicator
                if ((flags_ & TYPE_FLAG_4_BYTE_VC_LEN) == 1)
                    tgt.putInt(vcLenIndOffset_,trimmedLen);
                else
                    tgt.putShort(vcLenIndOffset_,(short) trimmedLen);
            }
    }

    static short getCurrentVersion() { return 1; }

    @Override
    int serializedLength() throws UDRException{
        return super.serializedLength() +  (17 * serializedLengthOfInt());
    }
        
    @Override
    int serialize(ByteBuffer outputBuffer) throws UDRException{

      int origPos = outputBuffer.position();

      super.serialize(outputBuffer);
      
      serializeInt(64, outputBuffer);
      serializeInt(sqlType_, outputBuffer);
      serializeInt(nullable_, outputBuffer);
      serializeInt(scale_, outputBuffer);
      serializeInt(charset_, outputBuffer);
      serializeInt(intervalCode_, outputBuffer);
      serializeInt(precision_, outputBuffer);
      serializeInt(collation_, outputBuffer);
      serializeInt(length_, outputBuffer);
      serializeInt(dataOffset_, outputBuffer);
      serializeInt(nullIndOffset_, outputBuffer);
      serializeInt(vcLenIndOffset_, outputBuffer);
      serializeInt(flags_, outputBuffer);
      serializeInt(fillers0_, outputBuffer);
      serializeInt(fillers1_, outputBuffer);
      serializeInt(fillers2_, outputBuffer);
      serializeInt(fillers3_, outputBuffer);

      int bytesSerialized = outputBuffer.position() - origPos;

      validateSerializedLength(bytesSerialized);

      return bytesSerialized;
    }

    @Override
    int deserialize(ByteBuffer inputBuffer) throws UDRException{

      int origPos = inputBuffer.position();

      super.deserialize(inputBuffer);
      validateObjectType(TMUDRObjectType.TYPE_INFO_OBJ);

      // consume the total length of the next 16 integers
      int totalLen = deserializeInt(inputBuffer);
      if (totalLen != 64)
          throw new UDRException(38900, "Expecting 64 bytes of data for TypeInfo");

      sqlType_ = deserializeInt(inputBuffer);
      nullable_ = deserializeInt(inputBuffer);
      scale_ = deserializeInt(inputBuffer);
      charset_ = deserializeInt(inputBuffer);
      intervalCode_ = deserializeInt(inputBuffer);
      precision_ = deserializeInt(inputBuffer);
      collation_ = deserializeInt(inputBuffer);
      length_ = deserializeInt(inputBuffer);
      dataOffset_ = deserializeInt(inputBuffer);
      nullIndOffset_ = deserializeInt(inputBuffer);
      vcLenIndOffset_ = deserializeInt(inputBuffer);
      flags_ = deserializeInt(inputBuffer);
      fillers0_ = deserializeInt(inputBuffer);
      fillers1_ = deserializeInt(inputBuffer);
      fillers2_ = deserializeInt(inputBuffer);
      fillers3_ = deserializeInt(inputBuffer);
     
      int bytesDeserialized = inputBuffer.position() - origPos;
      validateSerializedLength(bytesDeserialized);

      return bytesDeserialized;
    }

    void setOffsets(int dataOffset, int indOffset, int vcOffset) {
        nullIndOffset_  = indOffset;
        vcLenIndOffset_ = vcOffset;
        dataOffset_     = dataOffset;
    }
    int getDataOffset() {
        return dataOffset_ ;
    }
    int getIndOffset() {
        return nullIndOffset_ ;
    }
    int getVcLenIndOffset() {
        return vcLenIndOffset_ ;
    }
    

    // flags
    static final int TYPE_FLAG_4_BYTE_VC_LEN    = 0x00000001;
    
    private int /*SQLTypeCode */ sqlType_;      
    private int nullable_; // boolean
    private int scale_; // scale for exact numeric, fraction precision for datetime/interval
    private int /*SQLCharsetCode   */ charset_;       // for character types
    private int /*SQLIntervalCode  */ intervalCode_;  // for interval types
    private int  precision_;  // decimal precision for exact numerics, leading interval precision for intervals
    private int /*SQLCollationCode */ collation_;     // for character types
    private int   length_;        // for numeric (decimal precision) and character types
    private int   dataOffset_;    // offset in record for data portion
    private int   nullIndOffset_; // offset in record for 2 or 4 byte varchar length
    private int   vcLenIndOffset_;// offset in record for 2 byte null indicator
    private int   flags_;         // bit flags
    private int   fillers0_, fillers1_, fillers2_, fillers3_ ;    
    // for adding more fields without versioning
}
