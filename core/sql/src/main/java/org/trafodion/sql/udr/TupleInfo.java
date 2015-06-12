// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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

package org.trafodion.sql.udr;
import java.util.Vector;
import java.util.Iterator;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.Date;
import java.text.DateFormat;
import java.text.SimpleDateFormat;

/**
 *  Common base class for parameter lists and row layouts
 *
 *  <p> Describes a list of scalars, which could be columns of a table
 *  or a parameter list
 */

public class TupleInfo extends TMUDRSerializableObject {

    // Functions for use by UDR writer, both at compile and at run time

    /**
     *  Get the number of columns or parameters.
     *  @return Number of columns/parameters.
     */
    public int getNumColumns() {
        return columns_.size();
    }

    /**
     *  Look up a column/parameter number by name.
     *  @param colName Name of an existing column.
     *  @return Column/parameter number.
     *  @throws UDRException
     */
    public int getColNum(String colName) throws UDRException {
        int result = 0;
        Iterator<ColumnInfo> it = columns_.iterator();
        while (it.hasNext()) {
            if (it.next().getColName() == colName)
                return result;
            result++;
        }
        throw new UDRException(38900, "Column %s not found", colName);
    }


    /**
     *  Get the column info for a column identified by its ordinal.
     *  @param colNum Column number.
     *  @return Column info.
     *  @throws UDRException
     */
    public ColumnInfo getColumn(int colNum) throws UDRException {
        if (colNum < 0 || colNum >= columns_.size())
            throw new UDRException(
                                   38900,
                                   "Trying to access column number %d but column list has only %d elements",
                                   colNum, columns_.size());
        
        return columns_.get(colNum);
    }

    /**
     *  Get the column info for a column identified by its name.
     *  @param colName Name of an existing column.
     *  @return Column info.
     *  @throws UDRException
     */
    public ColumnInfo getColumn(String colName) throws UDRException {
        return getColumn(getColNum(colName));
    }

    /**
     *  Get the type of a column.
     *  @param colNum Column number.
     *  @return Type of the column.
     *  @throws UDRException
     */
    public TypeInfo getType(int colNum) throws UDRException {
        return getColumn(colNum).getType();
    }
    
    /**
     *  Get the SQL type class.
     *  Determine whether this is a numeric character, datetime or interval type.
     *  @param colNum Column number.
     *  @return SQL type class enum.
     *  @throws UDRException
     */
    public TypeInfo.SQLTypeClassCode getSQLTypeClass(int colNum) throws UDRException {
        return getType(colNum).getSQLTypeClass();
    }

    // get values at runtime and also values of available
    // constant input parameters at compile time

    /**
     *  Get an integer value of a column or parameter
     *
     *  <p> This method is modeled after the JDBC interface.
     *
     *  <p> Use this method at runtime. It can also be used for
     *  actual parameters that are available at compile time.
     *
     *  @param colNum Column number.
     *  @return Integer value.
     *          If the value was a NULL value, then 0 is returned.
     *          The wasNull() method can be used to determine whether
     *          a NULL value was returned.
     *  @throws UDRException
     */
    public int getInt(int colNum) throws UDRException {

        long result = getLong(colNum);

        if (result < Integer.MIN_VALUE || result > Integer.MAX_VALUE)
            throw new UDRException(
                                   38900, 
                                   "Under or overflow in getInt(), %ld does not fit in an int",
                                   result);
        
        return (int) result;
        
    }

    /**
     *  Get an integer value for a column identified by name.
     *
     *  @see TupleInfo#getInt(int) const
     *
     *  @param colName Name of an existing column.
     *  @return Integer value.
     *          If the value was a NULL value, then 0 is returned.
     *          The wasNull() method can be used to determine whether
     *          a NULL value was returned.
     *  @throws UDRException
     */
    public int getInt(String colName) throws UDRException {
        return getInt(getColNum(colName));
    } 

    /**
     *  Get a long value of a column or parameter
     *
     *  <p> This method is modeled after the JDBC interface.
     *
     *  <p> Use this method at runtime. It can also be used for
     *  actual parameters that are available at compile time.
     *
     *  @param colNum Column number.
     *  @return long value.
     *          If the value was a NULL value, then 0 is returned.
     *          The wasNull() method can be used to determine whether
     *          a NULL value was returned.
     *  @throws UDRException
     */
    public long getLong(int colNum) throws UDRException {
        if (!row_.hasArray())
            throw new UDRException(
                                   38900,
                                   "Row not available for getLong() or related method");
        TypeInfo t = getType(colNum);
        if (!t.isAvailable())
            throw new UDRException(
                                   38900,
                                   "Offset for column not set, getLong() or related method not available");
        
        if (t.isAvailable() &&
            row_.getShort(t.getIndOffset()) != 0)
            {
                wasNull_ = true;
                return 0;
            }
        
        long result = 0;
        TypeInfo.SQLTypeCode tempSQLType = t.getSQLType();
        TypeInfo.SQLTypeCode origSQLType = tempSQLType;
        
        wasNull_ = false;
        
        // convert NUMERIC to the corresponding type with binary precision
        // see also code in LmTypeIsString() in file ../generator/LmExpr.cpp
        if (origSQLType == TypeInfo.SQLTypeCode.NUMERIC ||
            origSQLType == TypeInfo.SQLTypeCode.NUMERIC_UNSIGNED ||
            origSQLType == TypeInfo.SQLTypeCode.INTERVAL)
            {
                if (t.getByteLength() == 2)
                    if (origSQLType == TypeInfo.SQLTypeCode.NUMERIC_UNSIGNED)
                        tempSQLType = TypeInfo.SQLTypeCode.SMALLINT_UNSIGNED;
                    else
                        tempSQLType = TypeInfo.SQLTypeCode.SMALLINT;
                else if (t.getByteLength() == 4)
                    if (origSQLType == TypeInfo.SQLTypeCode.NUMERIC_UNSIGNED)
                        tempSQLType = TypeInfo.SQLTypeCode.INT_UNSIGNED;
                    else
                        tempSQLType = TypeInfo.SQLTypeCode.INT;
                else if (t.getByteLength() == 8)
                    tempSQLType = TypeInfo.SQLTypeCode.LARGEINT;
                // unsigned 8 byte integer is not supported
            }
        
        switch (tempSQLType)
            {
            case SMALLINT:
                result = row_.getShort(t.getDataOffset()) ; 
                break;
                
            case INT:
                result = row_.getInt(t.getDataOffset()) ; 
                break;
                
            case LARGEINT:
                result = row_.getLong(t.getDataOffset()) ; 
                break;
                
            case SMALLINT_UNSIGNED:
                result = row_.getShort(t.getDataOffset()) ;  // need to fix
                break;
                
            case INT_UNSIGNED:
                result = row_.getInt(t.getDataOffset()) ; // need to fix 
                break;
                
            case DECIMAL_LSE:
            case DECIMAL_UNSIGNED:
            {
                long fractionalPart = 0;
                boolean isNegative = false;
                boolean overflow = false;
                int dataLen = t.getByteLength();
                int data = t.getDataOffset();
                byte byteStr[] = new byte[dataLen];
                row_.position(t.getDataOffset());
                row_.get(byteStr);
                String buf = isoCharset.decode(ByteBuffer.wrap(byteStr)).toString();
                    
                if (t.getScale() == 0)
                {
                    try { result = Long.parseLong(buf);
                        if (result < 0)
                        {
                            isNegative = true;
                            result = -result ;
                        }
                    }
                    catch (Exception e1) {
                        throw new UDRException(
                                               38900,
                                               "Error converting decimal value %s to a long",
                                               buf);
                    }
                }
                else
                {
                    try { String[] parts = buf.split("\\.",2) ;
                        result = Long.parseLong(parts[0]);
                        if (parts.length == 2)
                            fractionalPart = Long.parseLong(parts[1]); 
                        if (result < 0)
                        {
                            isNegative = true;
                            result = -result ;
                        }
                    }
                    catch (Exception e1) {
                        throw new UDRException(
                                               38900,
                                               "Error converting decimal value %s (with scale) to a long",
                                               buf);
                    }
                    for (int s=0; s<t.getScale(); s++)
                        if (result <= Long.MAX_VALUE/10)
                            result *= 10;
                        else
                            overflow = true;
                    if (result <= Long.MAX_VALUE - fractionalPart)
                        result += fractionalPart;
                    else
                        overflow = true;
                }
                    
                if (isNegative)
                    if (result < Long.MAX_VALUE)
                        result = -result;
                    else
                        overflow = true;
                
                if (overflow)
                    throw new UDRException(
                                           38900,
                                           "Under or overflow occurred, converting decimal to a long");
            }
            break;
                
            case REAL:
            case DOUBLE_PRECISION:
            {
                double dresult = getDouble(colNum);
                
                if (dresult < Long.MIN_VALUE || dresult > Long.MAX_VALUE)
                    throw new UDRException(
                                           38900, 
                                           "Under or overflow in getInt() or getLong(), float value %g does not fit in a long",
                                           dresult);
                result = (long) dresult;
            }
            break;
            
            default:
                throw new UDRException(38902,
                                       "TypeInfo::getLong() and getDouble() not supported for SQL type %d",
                                       t.getSQLType().ordinal());
            }
        
        return result;
    }
    
    /**
     *  Get a long value for a column identified by name.
     *
     *  @see TupleInfo#getLong(int) const
     *
     *  @param colName Name of an existing column.
     *  @return long value.
     *  @throws UDRException
     */
    public long getLong(String colName) throws UDRException {
        return getLong(getColNum(colName));
    }

    /**
     *  Get a double value of a column or parameter
     *
     *  This method is modeled after the JDBC interface.
     *
     *  Use this method at runtime. It can also be used for
     *  actual parameters that are available at compile time.
     *
     *  @param colNum Column number.
     *  @return double value.
     *  @throws UDRException
     */
    public double getDouble(int colNum) throws UDRException {
        if (!row_.hasArray())
            throw new UDRException(
                                   38900,
                                   "Row not available for getDouble()");
        TypeInfo t = getType(colNum);
        if (!t.isAvailable())
            throw new UDRException(
                                   38900,
                                   "Offset for column not set, getDouble() method not available");
        
        if (t.isAvailable() &&
            row_.getShort(t.getIndOffset()) != 0)
            {
                wasNull_ = true;
                return 0.0;
            }
        
        double result = 0.0;
        wasNull_ = false;
        
        switch (t.getSQLType())
        {
        case REAL:
            result = row_.getFloat(t.getDataOffset()) ; 
            break;
            
        case DOUBLE_PRECISION:
            result = row_.getDouble(t.getDataOffset()) ; 
            break;
            
        case SMALLINT:
        case INT:
        case LARGEINT:
        case NUMERIC:
        case DECIMAL_LSE:
        case SMALLINT_UNSIGNED:
        case INT_UNSIGNED:
        case NUMERIC_UNSIGNED:
        case DECIMAL_UNSIGNED:
        case INTERVAL:
            result = (double)getLong(colNum);
            break;
            
        default:
            throw new UDRException(38900,
                                   "getDouble() not supported for SQL type %d",
                                   t.getSQLType().ordinal());
        }
        
        return result;
    }

    /**
     *  Get double value of a column/parameter identified by name.
     *
     *  @see TupleInfo#getDouble(int colNum) const
     *
     *  @param colName Name of an existing column.
     *  @return double value.
     *  @throws UDRException
     */
    public double getDouble(String colName) throws UDRException {
       return getDouble(getColNum(colName)); 
    }
    
    /**
     *  Get a string value of a column or parameter
     *
     *  <p> This method is modeled after the JDBC interface.
     *
     *  <p> Use this method at runtime. It can also be used for
     *  actual parameters that are available at compile time.
     *
     *  @param colNum Column number.
     *  @return String value.
     *          If the value was a NULL value, an empty string
     *          is returned. The wasNull() method can be used to
     *          determine whether a NULL value was returned.
     *  @throws UDRException
     */
    public String getString(int colNum) throws UDRException {
        TypeInfo t = getType(colNum);
        TypeInfo.SQLTypeCode sqlType = t.getSQLType();
        
        switch (sqlType)
        {
        case DECIMAL_LSE:
        case DECIMAL_UNSIGNED:
        case DATE:
        case TIME:
        case TIMESTAMP:
        case BLOB:  
        {
            // these types are stored as strings
            byte[] buf;
            buf = getRaw(colNum);
            if (buf != null) {
                String result;
                try {result = new String(buf, "ISO-8859-1"); }
                catch (Exception e1) {
                    throw new UDRException(
                                           38900,
                                           "Could not convert binary data of type %d to string", 
                                           sqlType.ordinal()); }
                return result;
            }
            else
                return "";
        }
         
        case CHAR:
        case VARCHAR:
        case CLOB:
        {
            byte[] buf;
            buf = getRaw(colNum);
            if (buf != null) {
                String result;
                try {
                    switch (t.getCharset())
                    {
                    case CHARSET_ISO88591:
                        result = new String(buf, "ISO-8859-1"); 
                        break ;
                    case CHARSET_UTF8:
                        result = new String(buf, "UTF8"); 
                        break ; 
                    case CHARSET_UCS2:
                        result = new String(buf, "UTF16"); 
                        break ; 
                    default:
                         throw new UDRException(
                                           38900,
                                           "Could not convert binary data of type %d with charset %d to string", 
                                           sqlType.ordinal(), t.getCharset().ordinal());
                    }
                }
                catch (Exception e1) {
                    throw new UDRException(
                                           38900,
                                           "Could not convert binary data of type %d with charset %d to string", 
                                           sqlType.ordinal(), t.getCharset().ordinal()); }
                    return result;
            }
            else
                return "";
            
        }
        case SMALLINT:
        case INT:
        case LARGEINT:
        case NUMERIC:
        case SMALLINT_UNSIGNED:
        case INT_UNSIGNED:
        case NUMERIC_UNSIGNED:
        {
            long num = getLong(colNum);
            if (wasNull_)
                return "";
            
            return Long.toString(num);
        }

        case REAL:
        case DOUBLE_PRECISION:
        {
            double num = getDouble(colNum);
            // see also constants SQL_FLOAT_FRAG_DIGITS and
            // SQL_DOUBLE_PRECISION_FRAG_DIGITS in file 
            // trafodion/core/sql/common/SQLTypeDefs.h
            int numSignificantDigits = 17;
            if (sqlType == TypeInfo.SQLTypeCode.REAL)
                numSignificantDigits = 7;
            if (wasNull_)
                return "";
            
            return String.format("%*lf", numSignificantDigits, num);
        }

        case INTERVAL:
        {
            long longVal       = getLong(colNum);
            long fractionalVal = 0;
            TypeInfo typ = getType(colNum);
            TypeInfo.SQLIntervalCode intervalCode = typ.getIntervalCode();
            int precision      = typ.getPrecision();
            int scale          = typ.getScale();
            String sign   = "";
            String dot    = (scale == 0 ? "" : ".");
            
            if (wasNull_)
                return "";
            
            if (longVal < 0)
            {
                longVal = -longVal;
                sign = "-";
            }

            // split the number into integer and fractional values
            for (int d=0; d<scale; d++)
            {
                fractionalVal = 10*fractionalVal + longVal % 10;
                longVal /= 10;
            }
            
            switch (intervalCode)
            {
            case INTERVAL_YEAR:
            case INTERVAL_MONTH:
            case INTERVAL_DAY:
            case INTERVAL_HOUR:
            case INTERVAL_MINUTE:
                // Example: "59"
                return String.format("%s%*ld", sign, precision, longVal);

            case INTERVAL_SECOND:
                // Example: "99999.000001"
                return String.format("%s%*ld%s%0*ld", sign, precision, longVal,
                                     dot, scale, fractionalVal);

            case INTERVAL_YEAR_MONTH:
                // Example: "100-01"
                return String.format("%s%*ld-%02d", sign, precision, (long) (longVal/12),
                                     (int)  (longVal%12));

            case INTERVAL_DAY_HOUR:
                // Example: "365 06"
                return String.format("%s%*ld %02d", sign, precision, (long) (longVal/24),
                                     (int)  (longVal%24));
                
            case INTERVAL_DAY_MINUTE:
                // Example: "365:05:49"
                return String.format("%s%*ld %02d:%02d", sign, precision, 
                                     (long) (longVal/1440),
                                     (int)  (longVal%1440/60),
                                     (int)  (longVal%60));

            case INTERVAL_DAY_SECOND:
                // Example: "365:05:49:12.00"
                return String.format("%s%*ld %02d:%02d:%02d%s%0*ld", sign, precision,
                                     (long) (longVal/86400),
                                     (int)  (longVal%86400/3600),
                                     (int)  (longVal%3600/60),
                                     (int)  (longVal%60),
                                     dot, scale, fractionalVal);

            case INTERVAL_HOUR_MINUTE:
                // Example: "12:00"
                return String.format("%s%*ld:%02d", sign, precision, (long) (longVal/60),
                                     (int)  (longVal%60));

            case INTERVAL_HOUR_SECOND:
                // Example: "100:00:00"
                return String.format("%s%*ld:%02d:%02d%s%0*ld", sign, precision,
                                     (long) (longVal/3600),
                                     (int)  (longVal%3600/60),
                                     (int)  (longVal%60),
                                     dot, scale, fractionalVal);

            case INTERVAL_MINUTE_SECOND:
                // Example: "3600:00.000000"
                return String.format("%s%*ld:%02d%s%0*ld", sign, precision,
                                     (long) (longVal/60),
                                     (int)  (longVal%60),
                                     dot, scale, fractionalVal);
            default:
                throw new UDRException(
                                   38900,
                                   "Invalid interval code in TupleInfo::getString()");
            }
      }

        default:
            throw new UDRException(
                                   38900,
                                   "Type %d not yet supported in getString()",
                                   sqlType.ordinal());
        }
        
    }

    /**
    *  Get a string value of a column or parameter identified by name.
    *
    *  <p> This method is modeled after the JDBC interface.
    *
    *  <p> Use this method at runtime. It cannot be used for
    *  actual parameters that are available at compile time, use
    *  getString(int colNum) instead, since actual parameters are not named.
    *
    *  @param colName Name of an existing column.
    *  @return String value.
    *          If the value was a NULL value, an empty string
    *          is returned. The wasNull() method can be used to
    *          determine whether a NULL value was returned.
    *  @throws UDRException
    */
    public String getString(String colName) throws UDRException {
        return getString(getColNum(colName));
    }

    /**
     *  Get a pointer to the raw data value of a column.
     *
     *  Using this method requires knowledge of the data layout
     *  for the different types used in UDRs. This method can be
     *  useful for performance optimizations, when converting longer
     *  string values to std::string is undesirable. Note that the
     *  pointer to the raw value is valid only until a new row
     *  is read or the existing row is emitted.
     *
     *  Use this method at runtime. It can also be used for
     *  actual parameters that are available at compile time.
     *
     *  @param colNum Column number.
     *  @param byteLen Length, in bytes, of the value returned.
     *  @return Pointer to the raw column value in the row buffer.
     *  @throws UDRException
     */
    byte[] getRaw(int colNum) throws UDRException {
        if (!row_.hasArray())
            throw new UDRException(
                                   38900,
                                   "Row not available for getRaw()");
        TypeInfo t = getType(colNum);
        if (!t.isAvailable())
            throw new UDRException(
                                   38900,
                                   "Offset for column not set, getRaw() method not available");
        
        if (t.isAvailable() &&
            row_.getShort(t.getIndOffset()) != 0)
            {
                wasNull_ = true;
                return null;
            }
        
        byte[] result;
        int byteLen = 0;
        wasNull_ = false;
        
        switch (t.getSQLType())
        {
        case VARCHAR:
        case BLOB:
        case CLOB:
            if ((t.getFlags() & TypeInfo.TYPE_FLAG_4_BYTE_VC_LEN) == 1)
            {                
                byteLen  = row_.getInt(t.getVcLenIndOffset()) ; 
            }
            else
            {
                byteLen  = row_.getShort(t.getVcLenIndOffset()) ;   
            }
            break ;
            
        case UNDEFINED_SQL_TYPE:
            throw new UDRException(38900,
                                   "getString()/getRaw() not supported for SQL type %d",
                                   t.getSQLType().ordinal());
            
        default:
            byteLen = t.getByteLength();
            if (t.getSQLType() == TypeInfo.SQLTypeCode.CHAR)
                switch (t.getCharset())
                 {
                 case CHARSET_ISO88591:
                 case CHARSET_UTF8:
                     // trim trailing blanks from the value
                     while (byteLen > 0 && row_.get(t.getDataOffset()+byteLen-1) == 0x20) // blank space
                         byteLen--;
                     break;
                 case CHARSET_UCS2:
                     // trim trailing little-endian UCS2 blanks
                     // from the value
                     while (byteLen > 1 &&
                            row_.getChar(t.getDataOffset()+byteLen-2) == ' ') // char is 2 bytes UCS2x
                         byteLen -= 2;
                     break;
                 default:
                     throw new UDRException(
                                            38900,
                                            "Unsupported character set in TupleInfo#getRaw(): %d",
                                            t.getCharset());
                 }
            break;
        }
        result = new byte[byteLen];
        row_.position(t.getDataOffset());
        row_.get(result);
        return result;
    }

    /**
     *  Get a datetime or interval column value as time_t
     *
     *  <p> This method can be used to convert column values with
     *  a datetime type or a day-second interval type to the
     *  POSIX type time_t. Note that this may result in the loss
     *  of fractional seconds.
     *
     *  <p> Use this method at runtime. It can also be used for
     *  actual parameters that are available at compile time.
     *
     *  @param colNum Column number.
     *  @throws UDRException
     */
    public Date getTime(int colNum) throws UDRException {
          if (!row_.hasArray())
            throw new UDRException(
                                   38900,
                                   "Row not available for getTime()");
        TypeInfo t = getType(colNum);
        if (!t.isAvailable())
            throw new UDRException(
                                   38900,
                                   "Offset for column not set, getTime() method not available");
        
        if (t.isAvailable() &&
            row_.getShort(t.getIndOffset()) != 0)
            {
                wasNull_ = true;
                return new Date(0);
            }
        
        long result;
        Date resultDate;
        wasNull_ = false;

        if (t.getSQLType() == TypeInfo.SQLTypeCode.INTERVAL)
        {
            long longVal = getLong(colNum);
            if (wasNull_)
                return new Date(0);

            // convert the interval value to seconds
            
            // NOTE: This relies on the assumption that time_t
            //       uses seconds as its unit, which is true for
            //       current Linux systems but may not always remain
            //       true
            switch (t.getIntervalCode())
            {
            case INTERVAL_DAY:
                result = longVal * 86400;
                break;
            case INTERVAL_HOUR:
            case INTERVAL_DAY_HOUR:
                result = longVal * 3600;
                break;
            case INTERVAL_MINUTE:
            case INTERVAL_DAY_MINUTE:
            case INTERVAL_HOUR_MINUTE:
                result = longVal * 60;
                break;
            case INTERVAL_SECOND:
            case INTERVAL_DAY_SECOND:
            case INTERVAL_HOUR_SECOND:
            case INTERVAL_MINUTE_SECOND:
            {
                // scale the value down and ignore fractional seconds
                for (int s=0; s<t.getScale(); s++)
                    longVal /= 10;

                result = longVal;
            }
          break;
            default:
                throw new UDRException(
                                       38900,
                                       "getTime() is not supported for year-month intervals");
            }
            resultDate = new Date(result*1000) ; // convert to milliseconds
        } // intervals
        else
        {
            String val = getString(colNum);
            if (wasNull_)
                return new Date(0);
            DateFormat df;

            try {
                switch (t.getSQLType())
                {
                case DATE:
                    // yyyy-mm-dd
                    df = new SimpleDateFormat ("yyyy-MM-dd");
                    resultDate = df.parse(val);
                    break;
                    
                case TIME:
                    df = new SimpleDateFormat ("HH:mm:ss");
                    resultDate = df.parse(val);
                    break;
                    
                case TIMESTAMP:
                    df = new SimpleDateFormat ("yyyy-MM-dd HH:mm:ss");
                    resultDate = df.parse(val);
                    break;
                    
                default:
                    throw new UDRException(38900,
                                           "getTime() not supported for SQL type %d",
                                           t.getSQLType().ordinal());
                }
            }
            catch (java.text.ParseException e1) {
                throw new UDRException(
                                       38900,
                                       "Unable to parse datetime string %s for conversion to Date",
                                       val);
            }
            if ( resultDate == null ) {
                throw new UDRException(
                                       38900,
                                       "Unable to parse datetime string %s for conversion to Date",
                                       val);
                }
        }
        return resultDate;
    }

    /**
     *  Check whether a parameter is available at compile-time.
     *
     *  <p> Use this method to check in the compiler interfaces whether
     *  an actual parameter is a constant value that can be read
     *  at compile time. If this method returns true, the value
     *  can be accessed with the getInt(), getString() etc. methods.
     *
     *  @param colNum Column number.
     *  @return true if the parameter value is available.
     *  @throws UDRException
     */
    public boolean isAvailable(int colNum) throws UDRException {
        return (row_.hasArray() &&
                colNum < columns_.size() &&
                getType(colNum).isAvailable());   
    }
    
    /**
     *  Get columns of a row as a delimited string.
     *
     *  <p> This method is useful to interface with tools that take a delimited
     *  record format. It is also useful for printing rows
     *  @see UDRInvocationInfo.DebugFlags#TRACE_ROWS
     *
     *  <p> Only use this method at runtime.
     *
     *  <p> Note: This method may return a string that contains multiple
     *        character sets, if columns with different character sets
     *        are involved. Using this method with UCS-2 columns is not
     *        recommended.
     *
     *  @return String     String in which the result delimited row
     *                     will be returned.
     *  @param delim       US ASCII field delimiter to use.
     *  @param quote       Whether to quote character field values that contain
     *                     the delimiter symbol or a quote symbol. Quote symbols
     *                     will be duplicated to escape them.
     *  @param quoteSymbol US ASCII quote character to use, if quote is true.
     *  @param firstColumn First column to read.
     *  @param lastColumn  Last column to read (inclusive) or -1 to read until
     *                     the last column in the row.
     *  @throws UDRException
     */
    public String getDelimitedRow(char delim,
                                  boolean quote,
                                  char quoteSymbol,
                                  int firstColumn,
                                  int lastColumn) throws UDRException {
        // read all columns and form a delimited text row from them

        // if quote is true, then quote any text that contains the delimiter
        // and also double any quotes appearing in the text 
        int nc = getNumColumns();
        
        if (firstColumn >= nc ||
            firstColumn < 0 ||
            lastColumn < -1 ||
            lastColumn > 0 && lastColumn >= nc)
            throw new UDRException(
                                   38900,
                                   "Invalid column range %d to %d in getDelimitedRow for a tuple with %d columns",
                                   firstColumn,
                                   lastColumn,
                                   nc);
        if (lastColumn == -1)
            lastColumn = nc-1;
        
        StringBuilder sb = new StringBuilder() ;
        String quoteString = String.valueOf(quoteSymbol); // does this have to be escaped for certain symbols ?
        String delimString = String.valueOf(delim);
        int currentStartLength=0, quoteIndex=0, delimIndex=0;

        for (int i=firstColumn; i<=lastColumn; i++)
        {
            if (i>firstColumn)
                sb.append(delim);

            currentStartLength = sb.length() ;
            sb.append(getString(i));
            if (!wasNull())
            {
                if (quote)
                {
                    boolean quoteTheString=false;
                    quoteIndex = sb.indexOf(quoteString, currentStartLength);
                    delimIndex = sb.indexOf(delimString, currentStartLength);
                    // replace all quotes with two quotes
                    while(quoteIndex != -1)
                    {
                        sb.insert(quoteIndex, quoteSymbol);
                        quoteIndex = sb.indexOf(quoteString, quoteIndex+2);
                        quoteTheString = true;
                    }
                    
                    // if we found a quote or a delimiter in the
                    // string, then quote it
                    if (quoteTheString || (delimIndex != -1))
                    {
                        sb.insert(currentStartLength, quoteSymbol);
                        sb.append(quoteSymbol);
                    }
                } // quote
            } // value is not NULL
        } // loop over columns
        return sb.toString();
    }

    /**
     *  Get all columns of a row as a delimited string, with '|' as field delimiter
     *
     *  <p> This method is useful to interface with tools that take a delimited
     *  record format. It is also useful for printing rows
     *  @see UDRInvocationInfo.DebugFlags#TRACE_ROWS
     *
     *  <p> Only use this method at runtime.
     *
     *  <p> Note: This method may return a string that contains multiple
     *        character sets, if columns with different character sets
     *        are involved. Using this method with UCS-2 columns is not
     *        recommended.
     *
     *  @return String   String in which the result delimited row
     *                     will be returned.
     *  @throws UDRException
     */
    public String getDelimitedRow( ) throws UDRException {
        return getDelimitedRow('|', false, '"', 0, -1) ;
    } 
    
    /**
    *  Check whether the last value returned from a getInt() etc. method was NULL.
    *
    *  <p> This method is modeled after the JDBC interface.
    *
    *  @return true if the last value returned from a getInt(), getString()
    *               etc. method was a NULL value, false otherwise.
    */
    public boolean wasNull() {
        return wasNull_ ;
    }
    
    // non-const methods, used during runtime only

    /**
     *  Set an output column to a specified integer value.
     *
     *  <p> Use this method at runtime.
     *
     *  @param colNum Index/ordinal of the column to set.
     *  @param val    The new integer value for the column to set.
     *  @throws UDRException
     */
    public void setInt(int colNum, int val) throws UDRException {
        setLong(colNum, val);
    }
    /**
     *  Set an output column to a specified long value.
     *
     *  <p> Use this method at runtime.
     *
     *  @param colNum Index/ordinal of the column to set.
     *  @param val    The new long value for the column to set.
     *  @throws UDRException
     */
    public void setLong(int colNum, long val) throws UDRException {
        TypeInfo t = getType(colNum);
        if (!row_.hasArray() ||
            t.getDataOffset() < 0)
            throw new UDRException(38900, "setInt() or setLong() on a non-existent value");

        // set NULL indicator to 0
        if (t.getIndOffset() >= 0)
            row_.putShort(t.getIndOffset(), (short)0);

        TypeInfo.SQLTypeCode tempSQLType = t.getSQLType();

        // convert NUMERIC to the corresponding type with binary precision
        if (tempSQLType == TypeInfo.SQLTypeCode.NUMERIC ||
            tempSQLType == TypeInfo.SQLTypeCode.NUMERIC_UNSIGNED ||
            tempSQLType == TypeInfo.SQLTypeCode.INTERVAL)
            {
                if (t.getByteLength() == 2)
                    if (tempSQLType == TypeInfo.SQLTypeCode.NUMERIC_UNSIGNED)
                        tempSQLType = TypeInfo.SQLTypeCode.SMALLINT_UNSIGNED;
                    else
                        tempSQLType = TypeInfo.SQLTypeCode.SMALLINT;
                else if (t.getByteLength() == 4)
                    if (tempSQLType == TypeInfo.SQLTypeCode.NUMERIC_UNSIGNED)
                        tempSQLType = TypeInfo.SQLTypeCode.INT_UNSIGNED;
                    else
                        tempSQLType = TypeInfo.SQLTypeCode.INT;
                else if (t.getByteLength() == 8)
                    tempSQLType = TypeInfo.SQLTypeCode.LARGEINT;
                // unsigned 8 byte integer is not supported
            }
        
        switch (tempSQLType)
        {
        case SMALLINT:
            if (val > Short.MAX_VALUE || val < Short.MIN_VALUE)
                throw new UDRException(
                                       38900,
                                       "Overflow/Underflow when assigining to SMALLINT type");
            row_.putShort(t.getDataOffset(), (short) val);
            break;
            
        case INT:
            if (val > Integer.MAX_VALUE || val < Integer.MIN_VALUE)
                throw new UDRException(
                                       38900,
                                       "Overflow/Underflow when assigining to INT type");
            row_.putInt(t.getDataOffset(), (int) val);
            break;

        case LARGEINT:
            row_.putLong(t.getDataOffset(), val);
            break;

        case SMALLINT_UNSIGNED:
            if (val < 0 || val > Short.MAX_VALUE)
                throw new UDRException(
                                       38900,
                                       "Overflow/underflow when assigning to SMALLINT UNSIGNED type");
            row_.putShort(t.getDataOffset(), (short) val);
            break;

        case INT_UNSIGNED:
            if (val < 0 || val > Integer.MAX_VALUE)
                throw new UDRException(
                                       38900,
                                       "Overflow/underflow when assigning to an INT UNSIGNED type");
            row_.putInt(t.getDataOffset(), (int) val);
            break;

        case DECIMAL_LSE:
        case DECIMAL_UNSIGNED:
        {
            boolean overflow = false;
            boolean isNegative = false;
            int remainingLength = t.getByteLength();
            int neededLengthWithoutSign = t.getPrecision() + (t.getScale() > 0 ? 1 : 0);
            byte buf[] = new byte[19];
            int remainingBufLength = buf.length;
            int bufIndex = 0;
            
            final long maxvals[] = {9L,
                                    99L,
                                    999L,
                                    9999L,
                                    99999L,
                                    999999L,
                                    9999999L,
                                    99999999L,
                                    999999999L,
                                    9999999999L,
                                    99999999999L,
                                    999999999999L,
                                    9999999999999L,
                                    99999999999999L,
                                    999999999999999L,
                                    9999999999999999L,
                                    99999999999999999L,
                                    999999999999999999L};
            
            if (t.getPrecision() < 1 || t.getPrecision() > 18 ||
                t.getScale() < 0 || t.getScale() > t.getPrecision())
                throw new UDRException(
                                       38900, "Invalid precision (%d) or scale (%d) for a decimal data type",
                                       t.getPrecision(), t.getScale());
            
            // right now precision is limited to 18, but may need to use this code for BigNum
            // so we add a check for this future case
            if (t.getByteLength() > remainingBufLength)
                throw new UDRException(
                                       38900, "Decimal precision %d is not supported by setLong(), limit is %d",
                                       t.getPrecision(), remainingBufLength);
            
            if (val < 0)
            {
                if (tempSQLType == TypeInfo.SQLTypeCode.DECIMAL_UNSIGNED)
                    throw new UDRException(
                                           38900,
                                           "Trying to assign a negative value to a DECIMAL UNSIGNED type");
                val = -val;
                isNegative = true;
                buf[bufIndex] = 0x2D; // ascii for '-'
                bufIndex++;
                remainingLength--;
                remainingBufLength--;
            }

            // add enough blanks to print the number right-adjusted
            while (neededLengthWithoutSign < remainingLength)
            {
                buf[bufIndex] = 0x20 ; // ascii for ' '
                bufIndex++;
                remainingLength--;
                remainingBufLength--;
            }

            // sanity check, t.getByteLength() should have enough space for sign,
            // precision and decimal point
            if (remainingLength < neededLengthWithoutSign)
                throw new UDRException(
                                       38900,
                                       "Internal error, field length too short in setLong() (%d, %d)",
                                       remainingLength, neededLengthWithoutSign);
            
            // validate limits for decimal precision
            if (val > maxvals[t.getPrecision()-1])
                throw new UDRException(
                                       38900,
                                       "Overflow occurred while converting value %ld to a DECIMAL(%d, %d)",
                                       val, t.getPrecision(), t.getScale());
            String s;
            if (t.getScale() == 0)
            {
                s = String.format("%0*ld", t.getPrecision(), val);
            }
            else
            {
                long fractionalValue = 0;
                long multiplier = 1;
                
                for (int i=0; i<t.getScale(); i++)
                {
                    fractionalValue += multiplier * (val % 10);
                    val /= 10;
                    multiplier *= 10;
                }
                s = String.format("%0*ld.%0*ld",
                                  t.getPrecision()-t.getScale(),
                                  val,
                                  t.getScale(),
                                  fractionalValue);
            }
            
            ByteBuffer bb = isoCharset.encode(s.substring(0,remainingBufLength));
            row_.position(t.getDataOffset());
            row_.put(buf,0,bufIndex).put(bb);
        }
        break;

        case REAL:
        case DOUBLE_PRECISION:
            setDouble(colNum, val);
            break;
            
        default:
            throw new UDRException(38900,
                                   "setLong(), setInt() or related is not supported for data type %d",
                                   t.getSQLType().ordinal());
        }
    }

    /**
     * Set an output column to a specified double value.
     *
     *  <p> Use this method at runtime.
     *
     *  @param colNum Index/ordinal of the column to set.
     *  @param val    The new double value for the column to set.
     *  @throws UDRException
     */
    public void setDouble(int colNum, double val) throws UDRException {
        TypeInfo t = getType(colNum);
        if (!row_.hasArray() ||
            t.getDataOffset() < 0)
            throw new UDRException(38900, "setDouble() on a non-existent value");

        // set NULL indicator to 0
        if (t.getIndOffset() >= 0)
            row_.putShort(t.getIndOffset(), (short)0);

        TypeInfo.SQLTypeCode sqlType = t.getSQLType();
        
        switch (sqlType)
            {
            case REAL:
                if (val > Float.MAX_VALUE || val < Float.MIN_VALUE)
                throw new UDRException(
                                       38900,
                                       "Overflow/Underflow when assigining to REAL type");
                row_.putFloat(t.getDataOffset(), (float) val);
                break;
                
            case DOUBLE_PRECISION:
                row_.putDouble(t.getDataOffset(), val);
                break;
                
            default:
                throw new UDRException(38900,
                                   "setDouble() is not supported for data type %d",
                                   sqlType.ordinal());
            }
    }

    /**
     *  Set an output column to a specified string value.
     *
     *  <p> Use this method at runtime.
     *
     *  @param colNum Index/ordinal of the column to set.
     *  @param val    The new string value for the column to set.
     *  @throws UDRException
     */
    
    public void setString(int colNum, String val) throws UDRException {
        TypeInfo t = getType(colNum);
        if (!row_.hasArray() ||
            t.getDataOffset() < 0)
            throw new UDRException(38900, "setString() on a non-existent value");
        
        // set NULL indicator to 0
        if (t.getIndOffset() >= 0)
        {
            if (val == null)
            {
                setNull(colNum);
                return ;
            }
            else
                row_.putShort(t.getIndOffset(), (short)0);
        }

        TypeInfo.SQLTypeCode sqlType = t.getSQLType();
        boolean isApproxNumeric = false;
        int stringLen = val.length();
        ByteBuffer bb;
        
        // a string overflow will raise an exception
        if (stringLen > t.getByteLength())
            throw new UDRException(38900,
                                   "setString() with a string of length %d on a column with length %d",
                                   stringLen,
                                   t.getByteLength());
        
        switch (sqlType)
        {
        case DATE:
        case TIME:
        case TIMESTAMP:
        case BLOB:
            if ((stringLen != t.getByteLength()) && (sqlType != TypeInfo.SQLTypeCode.BLOB))
                throw new UDRException(38900,
                                       "setString() with a string of length %d on a column with length %d",
                                       stringLen,
                                       t.getByteLength());
            bb = isoCharset.encode(val);
            row_.position(t.getDataOffset());
            row_.put(bb);
            if (sqlType == TypeInfo.SQLTypeCode.BLOB)
            {
                // set the varchar length indicator
                if (t.getVcLenIndOffset() < 0)
                    throw new UDRException(38900,
                                           "Internal error, BLOB without length indicator");
                
                if ((t.getFlags() & TypeInfo.TYPE_FLAG_4_BYTE_VC_LEN) == 1)
                    row_.putInt(t.getVcLenIndOffset(),stringLen);
                else
                    row_.putShort(t.getVcLenIndOffset(),(short)stringLen);
            }
            break ;
        case CHAR:
        case VARCHAR:
        case CLOB:
            // for these types, copy the string and pad with blanks
            // of the appropriate charset for fixed-length strings
            row_.position(t.getDataOffset());

            if (sqlType == TypeInfo.SQLTypeCode.VARCHAR ||
                sqlType == TypeInfo.SQLTypeCode.CLOB)
            {
                // set the varchar length indicator
                if (t.getVcLenIndOffset() < 0)
                    throw new UDRException(38900,
                                           "Internal error, VARCHAR/CLOB without length indicator");
                
                if ((t.getFlags() & TypeInfo.TYPE_FLAG_4_BYTE_VC_LEN) == 1)
                    row_.putInt(t.getVcLenIndOffset(),stringLen);
                else
                    row_.putShort(t.getVcLenIndOffset(),(short) stringLen);
            }
            row_.position(t.getDataOffset());
                // fill fixed character value with blanks of the appropriate
                // character set
                switch (t.getCharset())
                {
                case CHARSET_ISO88591:
                    bb = isoCharset.encode(val);
                    row_.put(bb);
                    for (int i = 0; (sqlType == TypeInfo.SQLTypeCode.CHAR && (i < t.getByteLength() - stringLen)); i++)
                        row_.put(t.getDataOffset()+stringLen+i, (byte)0x20); // space
                    break;
                case CHARSET_UTF8:
                    bb = utf8Charset.encode(val);
                    row_.put(bb);
                    stringLen = bb.capacity();
                    for (int i = 0; (sqlType == TypeInfo.SQLTypeCode.CHAR && (i < t.getByteLength() - stringLen)); i++)
                        row_.put(t.getDataOffset()+stringLen+i, (byte)0x20); // space
                    break;
                case CHARSET_UCS2:
                    row_.asCharBuffer().put(val);
                    int paddedLen = stringLen*2;
                    
                    // pad with little-endian UCS-2 blanks
                    while (sqlType == TypeInfo.SQLTypeCode.CHAR && paddedLen+1 < t.getByteLength())
                    {
                        row_.putChar(' ');
                        paddedLen += 2;
                    }
                    break;
                default:
                    throw new UDRException(
                                           38900,
                                           "Unsupported character set in TupleInfo::setString(): %d",
                                           t.getCharset().ordinal());
                }

            break;

        case REAL:
        case DOUBLE_PRECISION:
            double dval;
            try {dval = Double.parseDouble(val);}
            catch (Exception e1) {
                throw new UDRException(
                                       38900,
                                       "Error in setString() \"%s\" is not a double value",
                                       val);
            }
            setDouble(colNum, dval);
            break ;
                        
        case SMALLINT:
        case INT:
        case LARGEINT:
        case NUMERIC:
        case DECIMAL_LSE:
        case SMALLINT_UNSIGNED:
        case INT_UNSIGNED:
        case NUMERIC_UNSIGNED:
        case DECIMAL_UNSIGNED:
            long lval;
            try {lval = Long.parseLong(val);}
            catch (Exception e1) {
                throw new UDRException(
                                       38900,
                                       "Error in setString() \"%s\" is not a long value",
                                       val);
            }
            setLong(colNum, lval);
            break;
        
        case INTERVAL:
        {
            String trimmedval = val.trim(); // remove leading and trailing blanks
            long years, months, days, hours, minutes, seconds, singleField;
            long result = 0;
            long fractionalVal = 0;
            boolean isNegative = false;
            String parts[], parts2[], parts3[];

            if (trimmedval.charAt(0) == '-')
                isNegative = true;
            
            try {
                switch (t.getIntervalCode())
                {
                case INTERVAL_YEAR:
                case INTERVAL_MONTH:
                case INTERVAL_DAY:
                case INTERVAL_HOUR:
                case INTERVAL_MINUTE:
                    result = Long.parseLong(trimmedval);
                    if (isNegative)
                        result = -result;
                    break;
                case INTERVAL_SECOND:
                    parts = trimmedval.split("\\.",2) ;
                    result = Long.parseLong(parts[0]);
                    if (isNegative)
                        result = -result;
                    if (parts.length == 2)
                        fractionalVal = Long.parseLong(parts[1]); 
                    break;
                case INTERVAL_YEAR_MONTH: // negative string and exceptions for missing parts left
                    parts = trimmedval.split("-") ;
                    years = Long.parseLong(parts[isNegative ? 1 : 0]);
                    months = Long.parseLong(parts[isNegative ? 2 : 0]);
                    result = years * 12 + months;
                    break;
                case INTERVAL_DAY_HOUR: // exceptions for missing parts left
                    parts = trimmedval.split(" ",2) ;
                    days = Long.parseLong(parts[0]);
                    if (isNegative)
                        days = -days;
                    hours = Long.parseLong(parts[1]);
                    result = days * 24 + hours;
                    break;
                case INTERVAL_DAY_MINUTE:
                    parts = trimmedval.split(" ",2) ;
                    days = Long.parseLong(parts[0]);
                    if (isNegative)
                        days = -days;
                    parts2 = parts[1].split(":",2);
                    hours = Long.parseLong(parts2[0]);
                    minutes = Long.parseLong(parts2[1]);
                    result = days * 1440 + hours * 60 + minutes;
                    break;
                case INTERVAL_DAY_SECOND:
                    parts = trimmedval.split(" ",2) ;
                    days = Long.parseLong(parts[0]);
                    if (isNegative)
                        days = -days;
                    parts2 = parts[1].split(":",3);
                    hours = Long.parseLong(parts2[0]);
                    minutes = Long.parseLong(parts2[1]);
                    parts3 = parts2[2].split("\\.",2);
                    seconds = Long.parseLong(parts3[0]);
                    if (parts3.length == 2)
                        fractionalVal = Long.parseLong(parts3[1]); 
                    result = days * 86400 + hours * 3600 + minutes * 60 + seconds;
                    break;
                case INTERVAL_HOUR_MINUTE:
                    parts = trimmedval.split(":",2) ;
                    hours = Long.parseLong(parts[0]);
                    if (isNegative)
                        hours = -hours;
                    minutes = Long.parseLong(parts[1]);
                    result = hours * 60 + minutes;
                    break;
                case INTERVAL_HOUR_SECOND:
                    parts = trimmedval.split(":",3) ;
                    hours = Long.parseLong(parts[0]);
                    if (isNegative)
                        hours = -hours;
                    minutes = Long.parseLong(parts[1]);
                    parts2 = parts[2].split("\\.",2);
                    seconds = Long.parseLong(parts2[0]);
                    if (parts2.length == 2)
                        fractionalVal = Long.parseLong(parts2[1]);
                    result = hours * 3600 + minutes * 60 + seconds;
                    break;
                case INTERVAL_MINUTE_SECOND:
                    parts = trimmedval.split(":",2) ;
                    minutes = Long.parseLong(parts[0]);
                    if (isNegative)
                        minutes = -minutes;
                    parts2 = parts[1].split("\\.",2);
                    seconds = Long.parseLong(parts2[0]);
                    if (parts2.length == 2)
                        fractionalVal = Long.parseLong(parts2[1]); 
                    result = minutes * 60 + seconds;
                    break;
                default:
                    throw new UDRException(
                                           38900,
                                           "Invalid interval code in TupleInfo::setString()");
                } 
            }
            catch (Exception e1) { // need to explicitly list out exceptions here and exclude UDRException
                throw new UDRException(
                                       38900,
                                       "Error in setString(), \"%s\" is not an interval value for interval code %d",
                                       trimmedval, t.getIntervalCode().SQLIntervalCode());
            }
            // if the fractional seconds are not 0, complain if fraction
            // precision is 0.
            if (fractionalVal > 0 && t.getScale() == 0)
                throw new UDRException(
                                       38900,
                                       "Encountered a fractional second part in a string value for an interval type that doesn't allow fractional values: %s",
                                       trimmedval);
            
            if (t.getScale() > 0)
            {
                long fractionOverflowTest = fractionalVal;
                
                // scale up the result
                for (int s=0; s<t.getScale(); s++)
                {
                    result *= 10;
                    fractionOverflowTest /= 10;
                }
                
                if (fractionOverflowTest != 0)
                    throw new UDRException(
                                           38900,
                                           "Fractional value %ld exceeds allowed range for interval fraction precision %d",
                                           fractionalVal, t.getScale());
                
                // add whole and fractional seconds (could overflow in extreme cases)
                result += fractionalVal;
            }
            
            // could overflow in extreme cases
            if (isNegative)
                result = -result;
            
            // result could exceed allowed precision, will cause an executor error when processed further
            setLong(colNum, result);
        }
        break;
        
    case UNDEFINED_SQL_TYPE:
        default:
            throw new UDRException(38900,
                                   "setString() is not yet supported for data type %d",
                                   sqlType.ordinal());
        }
    }

    /**
     *  Set a datetime or interval output column to a value specified as Date
     *
     *  <p> This method cannot be used with year-month intervals or data types that
     *  are not datetime or interval types. It is not possible to set fractional
     *  seconds with this method.
     *
     *  <p> Use this method at runtime.
     *
     *  @param colNum    Index/ordinal of the column to set.
     *  @param val       The new Date value for the column to set.
     *  @throws UDRException
     */
    public void setTime(int colNum, Date val) throws UDRException {
        TypeInfo t = getType(colNum);
        if (!row_.hasArray() ||
            t.getDataOffset() < 0)
            throw new UDRException(38900, "setDouble() on a non-existent value");

        // set NULL indicator to 0
        if (t.getIndOffset() >= 0)
            row_.putShort(t.getIndOffset(), (short)0);

        TypeInfo.SQLTypeCode sqlType = t.getSQLType();
        if (sqlType == TypeInfo.SQLTypeCode.INTERVAL)
        {
            long tVal = val.getTime()/1000; // convert from millisecond units to seconds to stay in sync with C++ use of time_t
            long result = 0;

            // convert the time_t value to the base units of the interval
            
            // NOTE: This relies on the assumption that time_t
            //       uses seconds as its unit, which is true for
            //       current Linux systems but may not always remain
            //       true. It may also some day become bigger than long.
            switch (t.getIntervalCode())
            {
            case INTERVAL_DAY:
                result = tVal/86400;
                break;
            case INTERVAL_HOUR:
            case INTERVAL_DAY_HOUR:
                result = tVal/3600;
                break;
            case INTERVAL_MINUTE:
            case INTERVAL_DAY_MINUTE:
            case INTERVAL_HOUR_MINUTE:
                result = tVal/60;
                break;
            case INTERVAL_SECOND:
            case INTERVAL_DAY_SECOND:
            case INTERVAL_HOUR_SECOND:
            case INTERVAL_MINUTE_SECOND:
            {
                // scale the value up
                for (int s=0; s<t.getScale(); s++)
                    tVal *= 10;
                
                result = tVal;
            }
            break;
            default:
                throw new UDRException(
                                       38900,
                                       "getTime() is not supported for year-month intervals");
            }
            
            setLong(colNum, result);
        } // intervals
        else
        {
            String resultDate ;
            DateFormat df;
            switch (sqlType)
            {
            case DATE:
                // yyyy-mm-dd
                df = new SimpleDateFormat ("yyyy-MM-dd");
                resultDate = df.format(val);
                break;
                
            case TIME:
                df = new SimpleDateFormat ("HH:mm:ss");
                resultDate = df.format(val);
                break;
                
            case TIMESTAMP:
                df = new SimpleDateFormat ("yyyy-MM-dd HH:mm:ss");
                resultDate = df.format(val);
                break;

            default:
                throw new UDRException(38900,
                                   "setTime() not supported for SQL type %d",
                                   sqlType.ordinal());
            }

            // add fraction (with value 0) if needed
            if (t.getScale() > 0) {
                resultDate.concat(".");
                for (int s=0; s<t.getScale(); s++)
                    resultDate.concat("0");
            }
            
            setString(colNum, resultDate);
        } // types other than intervals
    }

    /**
     * Set the result row from a string with delimited field values.
     *
     *  <p> This method can be used to read delimited text files and
     *  conveniently produce a result table from them. For example,
     *  if the following string is passed in as row:
     *  @code skip1|'skip|2'|3|'delim|and''Quote'|5 @endcode
     *  This call:
     *  @code setFromDelimitedRow(
     *      row,  // row
     *      '|',  // delim
     *      true, // quote
     *      '\'', // quoteSymbol (single quote)
     *      10,   // firstColumnToSet
     *      11,   // lastColumnToSet
     *      2);   // numDelimColsToSkip @endcode
     *  would set output column 10 to 3 and output column 11 to delim|and'Quote.
     *
     *  <p> Note: The delimited row may need to contain strings of multiple
     *        character sets. Using this method with non-ascii UTF8 data is not
     *        recommended, since that might require special handling.
     *
     *  @see TupleInfo#getDelimitedRow(char,boolean,char,int,int)
     *
     *  @param row    A string with delimited field values to read.
     *  @param delim  Delimiter between field values. Use a US ASCII symbol
     *                as the delimiter.
     *  @param quote  true if the method should assume that text fields
     *                use quotes to quote special symbols like delimiters
     *                that are embedded within fields, and that quote symbols
     *                embedded in text fields will be doubled.
     *  @param quoteSymbol US ASCII Quote symbol used to quote text. Meaningful
     *                     only if quote is set to true.
     *  @param firstColumnToSet First column in the output table to be set
     *                          from the delimited row (0-based).
     *  @param lastColumnToSet  Last column in the output table to be set
     *                          (inclusive) or -1 to indicate to set all
     *                          remaining columns of the table.
     *  @param numDelimColsToSkip Number of fields to skip in the delimited
     *                            row before using the values to set output
     *                            columns.
     *  @return                 Number of character read from row. Can be used
     *                           to process text that is not yet consumed.
     *  @throws UDRException
     */
    public int setFromDelimitedRow(String row,
                                   char delim,
                                   boolean quote,
                                   char quoteSymbol,
                                   int firstColumnToSet,
                                   int lastColumnToSet,
                                   int numDelimColsToSkip) throws UDRException 
    {
        int nc = getNumColumns();
        // virtual start column number of the first column in the delimited row
        // we may need to skip some values to reach the first one to use
        int startCol = firstColumnToSet-numDelimColsToSkip;
        
        if (firstColumnToSet >= nc ||
            firstColumnToSet < 0 ||
            lastColumnToSet < -1 ||
            lastColumnToSet > 0 && (lastColumnToSet >= nc ||
                                    firstColumnToSet > lastColumnToSet))
            throw new UDRException(
                                   38900,
                                   "Invalid column range %d to %d in setFromDelimitedRow for a tuple with %d columns",
                                   firstColumnToSet,
                                   lastColumnToSet,
                                   nc);
        if (lastColumnToSet == -1)
            lastColumnToSet = nc-1;
        
        int rowIndex = 0;
        for (int i=startCol; i<=lastColumnToSet; i++)
        {
            // skip over whitespace
            while (row.charAt(rowIndex) == ' ' || row.charAt(rowIndex) == '\t') rowIndex++;
            
            // make sure we have a delimiter for columns other than the first
            if (i>startCol)
            {
                if (row.charAt(rowIndex) != delim)
                    throw new UDRException(
                                           38900,
                                           "Expected delimiter at position %d in string %s",
                                           rowIndex, row);
                
                // skip over the delimiter and white space
                rowIndex++;
                while (row.charAt(rowIndex) == ' ' || row.charAt(rowIndex) == '\t') rowIndex++;
            }
            
            // find the end of the column value
            int endIndex = rowIndex;
            
            if (quote && row.charAt(rowIndex) == quoteSymbol)
            {
                // read and set a quoted string
                boolean embeddedQuote = false;
                boolean done = false;
                
                endIndex = ++rowIndex;
                
                // find the matching end to the quote
                while (endIndex < row.length() && !done)
                    if (row.charAt(endIndex) == quoteSymbol)
                        if (row.charAt(endIndex+1) == quoteSymbol)
                        {
                            // skip over both quotes
                            embeddedQuote = true;
                            endIndex += 2;
                        }
                        else
                            // found the terminating quote
                            done = true;
                  else
                      endIndex++;
                
                if (!done)
                    throw new UDRException(
                                           38900,
                                           "missing quote at the end of column %d in string %s",
                                           i, row);
                
                if (embeddedQuote)
                {
                    // need to transform the double doublequotes
                    // in a separate buffer
                    String unquotedVal = row.substring(rowIndex, endIndex).replaceAll("'","");
                    if (i >= firstColumnToSet)
                        // set from the transformed string
                        setString(i, unquotedVal);
                }
                else
                {
                    if (i >= firstColumnToSet)
                        // set from the value between the quotes
                        setString(i, row.substring(rowIndex, endIndex));
                    // skip over the trailing quote
                    endIndex++;
                }
                
            }
            else
            {
              // rowIndex points to the beginning of the field value
              // find the next delimiter or the end of the
              // record and treat white space only as a NULL
              // value
                boolean isNull = true;
                
                while (endIndex < row.length() && row.charAt(endIndex) != delim)
                {
                    if (isNull && row.charAt(endIndex) != ' ' && row.charAt(endIndex) != '\t')
                        isNull = false;
                    
                    endIndex++;
              }
                
                if (i >= firstColumnToSet)
                {
                    if (isNull)
                        setNull(i);
                    else
                        setString(i, row.substring(rowIndex, endIndex));
              }
            }
            
            // set the current character pointer to the
            // character just past of what we have consumed
            rowIndex = endIndex;
        }
      
        return rowIndex;
    }

    /**
     * Set the result row from a string with delimited field values.
     *
     *  <p> Calls other version of setFromDelimitedRow with these arguments
     *  @code setFromDelimitedRow(
     *      row,  // row
     *      '|',  // delim
     *      false, // quote
     *      '"', // quoteSymbol (single quote)
     *      0,   // firstColumnToSet
     *      -1,   // lastColumnToSet
     *      0);   // numDelimColsToSkip @endcode
     *
     *  <p> Note: The delimited row may need to contain strings of multiple
     *        character sets. Using this method with non-ascii UTF8 data is not
     *        recommended, since that might require special handling.
     *
     *  @see TupleInfo#getDelimitedRow( )
     *
     *  @param row    A string with delimited field values to read.
     *  @return       Number of character read from row. Can be used
     *                to process text that is not yet consumed.
     *  @throws UDRException
     */
    public int setFromDelimitedRow(String row) throws UDRException 
    {
        return setFromDelimitedRow(row, '|', false, '"', 0, -1, 0);
    }
    /**
     * Set an output column to a NULL value.
     *
     *  <p> Use this method at runtime.
     *
     *  @param colNum Index/ordinal of the column to set to NULL.
     *  @throws UDRException
     */
    public void setNull(int colNum) throws UDRException {
        TypeInfo t = getType(colNum);
        if (!row_.hasArray() ||
            t.getDataOffset() < 0)
            throw new UDRException(38900, "setNull() on a non-existent value");

        if (t.getIndOffset() < 0)
            throw new UDRException(38900,
                                   "Trying to set a non-nullable value to NULL");
        
        row_.putShort(t.getIndOffset(), (short)-1);
    }

    // non-const methods, used during compile time only
    /**
     *  Add a new column.
     *
     *  Only use this method from within the following method:
     *  <ul>
     *  <li> UDR::describeParamsAndColumns()
     * </ul>
     *
     *  @param column Info of the new column to add.
     */
    public void addColumn(ColumnInfo column)
    {
        ColumnInfo newCol = new ColumnInfo(column);
        columns_.add(newCol);
    }

    // for convenient adding of columns of a common type
    /**
     *  Add an integer output column.
     *
     *  <p> The new column is added at the end.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns() method.
     *
     *  @param colName    Name of the column to add. Use UPPER CASE letters,
     *                    digits and underscore, otherwise you will need to
     *                    use delimited column names with matching case in
     *                    Trafodion.
     *  @param isNullable true if the added column should be nullable,
     *                    false if the added column should have the NOT NULL
     *                    constraint.
     *  @throws UDRException
     */
    public void addIntColumn(String colName, boolean isNullable) throws UDRException
    {
        addColumn(new ColumnInfo(colName, 
                                 new TypeInfo(TypeInfo.SQLTypeCode.INT,0,isNullable)));
    }
    
    /**
     *  Add a long output column.
     *
     *  <p> The new column is added at the end.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns() method.
     *
     *  @param colName    Name of the column to add. Use UPPER CASE letters,
     *                    digits and underscore, otherwise you will need to
     *                    use delimited column names with matching case in
     *                    Trafodion.
     *  @param isNullable true if the added column should be nullable,
     *                    false if the added column should have the NOT NULL
     *                    constraint.
     *  @throws UDRException
     */
    public void addLongColumn(String colName, boolean isNullable) throws UDRException
    {
        addColumn(new ColumnInfo(colName, 
                                 new TypeInfo(TypeInfo.SQLTypeCode.LARGEINT,0,isNullable)));
    }

    /**
     *  Add a fixed character output column.
     *
     *  <p> The new column is added at the end.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns() method.
     *
     *  @param colName    Name of the column to add. Use UPPER CASE letters,
     *                    digits and underscore, otherwise you will need to
     *                    use delimited column names with matching case in
     *                    Trafodion.
     *  @param length     Length of the new character column.
     *                    For single-byte and variable byte character sets,
     *                    the length is specified in bytes. For UTF-8, this
     *                    is equivalent to CHAR(length BYTES) in SQL. For UCS2,
     *                    the length is in UCS2 16-bit characters.
     *  @param isNullable true if the added column should be nullable,
     *                    false if the added column should have the NOT NULL
     *                    constraint.
     *  @param charset    Character set of the new column.
     *  @param collation  Collation of the new column.
     *  @throws UDRException
     */
    public void addCharColumn(String colName,
                              int length,
                              boolean isNullable,
                              TypeInfo.SQLCharsetCode charset,
                              TypeInfo.SQLCollationCode collation) throws UDRException {
        addColumn(new ColumnInfo(colName, 
                                 new TypeInfo(TypeInfo.SQLTypeCode.CHAR,
                                              length,
                                              isNullable,
                                              0,
                                              charset,
                                              TypeInfo.SQLIntervalCode.UNDEFINED_INTERVAL_CODE,
                                              0,
                                              collation)));   
    }

    /**
     *  Add a fixed character output column with UTF8 charset and SYSTEM_COLLATION
     *
     *  <p> The new column is added at the end.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns() method.
     *
     *  @param colName    Name of the column to add. Use UPPER CASE letters,
     *                    digits and underscore, otherwise you will need to
     *                    use delimited column names with matching case in
     *                    Trafodion.
     *  @param length     Length of the new character column.
     *                    For single-byte and variable byte character sets,
     *                    the length is specified in bytes. For UTF-8, this
     *                    is equivalent to CHAR(length BYTES) in SQL. For UCS2,
     *                    the length is in UCS2 16-bit characters.
     *  @param isNullable true if the added column should be nullable,
     *                    false if the added column should have the NOT NULL
     *                    constraint.
     *  @throws UDRException
     */
    public void addCharColumn(String colName,
                              int length,
                              boolean isNullable) throws UDRException {
        addCharColumn(colName, length, isNullable, 
                      TypeInfo.SQLCharsetCode.CHARSET_UTF8,
                      TypeInfo.SQLCollationCode.SYSTEM_COLLATION);
    }

    /**
     *  Add a VARCHAR output column.
     *
     *  <p> The new column is added at the end.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns() method.
     *
     *  @param colName    Name of the column to add. Use UPPER CASE letters,
     *                    digits and underscore, otherwise you will need to
     *                    use delimited column names with matching case in
     *                    Trafodion.
     *  @param length     Length of the new character column.
     *                    For single-byte and variable byte character sets,
     *                    the length is specified in bytes. For UTF-8, this
     *                    is equivalent to CHAR(length BYTES) in SQL. For UCS2,
     *                    the length is in UCS2 16-bit characters.
     *  @param isNullable true if the added column should be nullable,
     *                    false if the added column should have the NOT NULL
     *                    constraint.
     *  @param charset    Character set of the new column.
     *  @param collation  Collation of the new column.
     *  @throws UDRException
     */
    public void addVarCharColumn(String colName,
                                 int length,
                                 boolean isNullable,
                                 TypeInfo.SQLCharsetCode charset,
                                 TypeInfo.SQLCollationCode collation) throws UDRException {
        addColumn(new ColumnInfo(colName, 
                                 new TypeInfo(TypeInfo.SQLTypeCode.VARCHAR,
                                              length,
                                              isNullable,
                                              0,
                                              charset,
                                              TypeInfo.SQLIntervalCode.UNDEFINED_INTERVAL_CODE,
                                              0,
                                              collation)));   
    }

    /**
     *  Add a VARCHAR output column with UTF8 charset and SYSTEM_COLLATION
     *
     *  <p> The new column is added at the end.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns() method.
     *
     *  @param colName    Name of the column to add. Use UPPER CASE letters,
     *                    digits and underscore, otherwise you will need to
     *                    use delimited column names with matching case in
     *                    Trafodion.
     *  @param length     Length of the new character column.
     *                    For single-byte and variable byte character sets,
     *                    the length is specified in bytes. For UTF-8, this
     *                    is equivalent to CHAR(length BYTES) in SQL. For UCS2,
     *                    the length is in UCS2 16-bit characters.
     *  @param isNullable true if the added column should be nullable,
     *                    false if the added column should have the NOT NULL
     *                    constraint.
     *  @throws UDRException
     */
    public void addVarCharColumn(String colName,
                                 int length,
                                 boolean isNullable) throws UDRException {
        addVarCharColumn(colName, length, isNullable, 
                         TypeInfo.SQLCharsetCode.CHARSET_UTF8,
                         TypeInfo.SQLCollationCode.SYSTEM_COLLATION);
    }

    /**
     *  Add multiple columns to the table-valued output.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns() method.
     *
     *  @param columns Vector of ColumnInfo objects describing the columns to add.
     */
    public void addColumns(Vector<ColumnInfo> columns) {
        Iterator<ColumnInfo> it = columns.iterator();
        while (it.hasNext()) {
            addColumn(it.next());
        }
    }

    /**
     *  Add a new column at a specified position.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns() method.
     *
     *  @param column   ColumnInfo object describing the new column.
     *  @param position Position/ordinal number of the new column.
     *                  All existing columns with ordinal numbers
     *                  greater or equal to position will be shifted by one.
     */
    public void addColumnAt(ColumnInfo column, int position) {
        ColumnInfo newCol = new ColumnInfo(column);
        columns_.add(position, newCol);
    }

    /**
     *  Delete a column of the table-valued output.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns() method.
     *
     *  @param i Position/ordinal (0-based) of column to be deleted.
     *  @throws UDRException
     */
    public void deleteColumn(int i) throws UDRException {
        try { columns_.remove(i); }
        catch (ArrayIndexOutOfBoundsException e1)
            { throw new UDRException(38906, "Column number %d not found", i); }
    }

    /**
     *  Delete a column with a specified column name.
     *
     *  <p> The first column that matches the specified column name
     *  will be deleted.
     *
     *  @param name Name of the column to be deleted.
     *  @throws UDRException
     */
    public void deleteColumn(String name) throws UDRException {
        deleteColumn(getColNum(name));
    }

    // Functions for debugging
    /**
     *  Print the object, for use in debugging.
     *
     *  @see UDR#debugLoop()
     *  @see UDRInvocationInfo.DebugFlags#PRINT_INVOCATION_INFO_AT_RUN_TIME
     */
    public void print() {
        StringBuilder sb = new StringBuilder();
        sb.append(String.format("    Number of columns        : %d\n", getNumColumns()));
        sb.append(String.format("    Columns                  : \n"));
        for (int c=0; c<getNumColumns(); c++)
        {   
            try {
                sb.append(String.format("        %s\n", getColumn(c).toString(true))); }
            catch (UDRException e1) {
                sb.append(String.format(" column number %d does not exist\n",c)); }
        }
        if (recordLength_ >= 0)
            sb.append(String.format("    Record length            : %d\n", recordLength_));
        System.out.print(sb.toString());
    } 
    
    // UDR writers can ignore these methods
    public TupleInfo(TMUDRObjectType objType, short version) {
        super(objType, version);
        recordLength_ = -1;
        wasNull_ = false;
        columns_ = new Vector<ColumnInfo>();
    }
    
    public static short getCurrentVersion() { return 1; }

    public int serializedLength() throws UDRException {
      int result = super.serializedLength() +
        2 * serializedLengthOfInt();

      for (int c=0; c<getNumColumns(); c++)
        result += getColumn(c).serializedLength();

      return result;
    }

    public int serialize(ByteBuffer outputBuffer) throws UDRException{
      int origPos = outputBuffer.position();

      super.serialize(outputBuffer);

      serializeInt(getNumColumns(),
                   outputBuffer);

      for (int c=0; c<getNumColumns(); c++)
        getColumn(c).serialize(outputBuffer);

      serializeInt(recordLength_,
                   outputBuffer);

      int bytesSerialized = outputBuffer.position() - origPos;

      if (getObjectType() == TMUDRObjectType.TUPLE_INFO_OBJ)
        validateSerializedLength(bytesSerialized);

      return bytesSerialized;
    }

    public int deserialize(ByteBuffer inputBuffer) throws UDRException{

      int origPos = inputBuffer.position();

      super.deserialize(inputBuffer);

      int numCols = deserializeInt(inputBuffer);
      
      columns_ = new Vector<ColumnInfo>();

      for (int c=0; c < numCols; c++)
      {
        ColumnInfo ci = new ColumnInfo();

        ci.deserialize(inputBuffer);

        addColumn(ci);
      }

      recordLength_ = deserializeInt(inputBuffer);

      row_ = null;
      wasNull_ = false;

      int bytesDeserialized = inputBuffer.position() - origPos;
      if (getObjectType() == TMUDRObjectType.TUPLE_INFO_OBJ)
        validateDeserializedLength(bytesDeserialized);

      return bytesDeserialized;
    }
    
    public ByteBuffer getRow(){
        return row_ ;
    }
    public int getRecordLength() {
        return recordLength_;
    }


    protected void setRecordLength(int len) {
        recordLength_ = len;
    }
    protected void setRow(byte[] rowByteArr) {
        row_ = ByteBuffer.wrap(rowByteArr);
    }

    private Vector<ColumnInfo>  columns_;
    private int recordLength_;
    private ByteBuffer row_;
    private boolean wasNull_;
    private static final Charset isoCharset = Charset.forName("ISO-8859-1");
    private static final Charset utf8Charset = Charset.forName("UTF-8");
    
}
