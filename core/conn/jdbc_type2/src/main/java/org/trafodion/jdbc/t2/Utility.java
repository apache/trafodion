/*******************************************************************************
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
 *******************************************************************************/

package org.trafodion.jdbc.t2;

import java.io.UnsupportedEncodingException;
import java.math.BigDecimal;
import java.sql.DataTruncation;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/**
This class contains a variety of methods for doing all sorts of things. 
@version 1.0
 */

class Utility {

    private static final byte key[] = Utility.UnicodeToAscii("ci4mg04-3;" + "b,hl;y'd1q" + "x8ngp93nGp" + "oOp4HlD7vm"
            + ">o(fHoPdkd" + "khp1`gl0hg" + "qERIFdlIFl" + "w48fgljksg" + "3oi5980rfd" + "4t8u9dfvkl");

    // -------------------------------------------------------------
    /**
     * This method will translate a double byte Unicode string into a single
     * byte ASCII array.
     * 
     * @param original
     *            the original string
     * 
     * @return a byte array containing the translated string
     * 
     * @exception An
     *                UnsupportedEncodingException is thrown
     */
    static byte[] UnicodeToAscii(String original) {
        try {
            byte[] utf8Bytes = original.getBytes("UTF8");
            return utf8Bytes;
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        return null;
    } // end UnicodeToAscii

    // -------------------------------------------------------------
    /**
     * This method will encrypt a byte buffer according to the encryption
     * algorithm used by the ODBC server.
     * 
     * @param original
     *            the original string
     * 
     * @return a byte array containing the translated string
     * 
     */
    static boolean Encryption(byte inBuffer[], byte outBuffer[], int inLength) {
        // Use simple encryption/decryption

        if (outBuffer != inBuffer) {
            System.arraycopy(outBuffer, 0, inBuffer, 0, inLength);
        } // end if

        for (int i = 0; i < inLength; ++i) {
            int j = i % 100;
            outBuffer[i] ^= key[j];
        }

        return true;
    } // end Encryption

    // -------------------------------------------------------------
    /**
     * This method will check a float value according to the MAX_FLOAT and
     * MIN_FLOAT values in the Java language.
     * 
     * @param the
     *            original double value to check
     * @Locale the Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkFloatBoundary(Locale locale, BigDecimal inbd) throws SQLException {
        double indbl = inbd.doubleValue();
        // double abdbl = inbd.abs().doubleValue(); Need to do MIN check as well
        if (indbl > (double) Float.MAX_VALUE) {
            throw Messages.createSQLException(null, locale, "numeric_out_of_range", inbd.toString());
        }
    } // end checkFloatBoundary

    // -------------------------------------------------------------
    /**
     * This method will check a double value according to the MAX_VALUE and
     * MIN_VALUE values in the Double class.
     * 
     * @param the
     *            original double value to check
     * @Locale the Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkDoubleBoundary(Locale locale, BigDecimal inbd) throws SQLException {
        BigDecimal maxbd = new BigDecimal(Double.MAX_VALUE);
        // need to check min as well
        // BigDecimal minbd = new BigDecimal(Double.MIN_VALUE);
        if ((inbd.compareTo(maxbd) > 0)) {
            throw Messages.createSQLException(null, locale, "numeric_out_of_range", inbd.toString());
        }

    } // end checkDoubleBoundary

    // -------------------------------------------------------------
    /**
     * This method will check a Integer value according to the
     * Interger.MAX_VALUE and Integer.MIN_VALUE values.
     * 
     * @param the
     *            original long value to check
     * @Locale the Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkIntegerBoundary(Locale locale, BigDecimal inbd) throws SQLException {
        long inlong = inbd.longValue();
        if ((inlong > Integer.MAX_VALUE) || (inlong < Integer.MIN_VALUE)) {
            throw Messages.createSQLException(null, locale, "numeric_out_of_range", String.valueOf(inlong));
        }
    } // end checkIntegerBoundary

    // -------------------------------------------------------------
    /**
     * This method will check a Long value according to the Long.MAX_VALUE*2 and
     * 0 values.
     * 
     * @param the
     *            original BigDecimal value to check
     * @Locale the Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkSignedLongBoundary(Locale locale, BigDecimal inbd) throws SQLException {
        long inlong = inbd.longValue();
        BigDecimal maxbd = new BigDecimal(Long.MAX_VALUE);
        maxbd = maxbd.add(maxbd);
        if ((inlong < 0) || (inbd.compareTo(maxbd) > 0)) {
            throw Messages.createSQLException(null, locale, "numeric_out_of_range", String.valueOf(inlong));
        }
    } // end checkIntegerBoundary

    // -------------------------------------------------------------
    /**
     * This method will check a unsigned Short value according to the
     * Short.MAX_VALUE*2 and 0 values.
     * 
     * @param the
     *            original BigDecimal value to check
     * @Locale the Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkSignedShortBoundary(Locale locale, BigDecimal inbd) throws SQLException {
        long inlong = inbd.longValue();
        long maxushort = (Short.MAX_VALUE * 2) + 1;
        if ((inlong < 0) || (inlong > maxushort)) {
            throw Messages.createSQLException(null, locale, "numeric_out_of_range", String.valueOf(inlong));
        }
    } // end checkIntegerBoundary

    // -------------------------------------------------------------
    /**
     * This method will check a unsigned Int value according to the
     * Integer.MAX_VALUE*2 and 0 values.
     * 
     * @param the
     *            original BigDecimal value to check
     * @Locale the Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkUnsignedIntegerBoundary(Locale locale, BigDecimal inbd) throws SQLException {
        long inlong = inbd.longValue();
        long maxuint = ((long) Integer.MAX_VALUE * 2L) + 1L;
        if ((inlong < 0) || (inlong > maxuint)) {
            throw Messages.createSQLException(null, locale, "numeric_out_of_range", String.valueOf(inlong));
        }
    } // end checkIntegerBoundary

    // -------------------------------------------------------------
    /**
     * This method will check a Tinyint value according to the Byte.MAX_VALUE
     * and Byte.MIN_VALUE values.
     * 
     * @param the
     *            original long value to check
     * @Locale the Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkTinyintBoundary(Locale locale, BigDecimal inbd) throws SQLException {
        long inlong = inbd.longValue();
        if ((inlong > Byte.MAX_VALUE) || (inlong < Byte.MIN_VALUE)) {
            throw Messages.createSQLException(null, locale, "numeric_out_of_range", String.valueOf(inlong));
        }
    } // end checkTinyintBoundary

    // -------------------------------------------------------------
    /**
     * This method will check a Short value according to the Short.MAX_VALUE and
     * Short.MIN_VALUE values.
     * 
     * @param the
     *            original long value to check
     * @Locale the Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkShortBoundary(Locale locale, BigDecimal inbd) throws SQLException {
        long inlong = inbd.longValue();
        if ((inlong > Short.MAX_VALUE) || (inlong < Short.MIN_VALUE)) {
            throw Messages.createSQLException(null, locale, "numeric_out_of_range", String.valueOf(inlong));
        }
    } // end checkShortBoundary

    // -------------------------------------------------------------
    /**
     * This method will extract the BigDecimal value.
     * 
     * @param the
     *            Locale to print the error message in
     * @param the
     *            original object value to extract
     * 
     * @return constructed BigDecimal value
     * 
     */
    static BigDecimal getBigDecimalValue(Locale locale, Object paramValue) throws SQLException {
        BigDecimal tmpbd;

        if (paramValue instanceof Long) {
            tmpbd = BigDecimal.valueOf(((Long) paramValue).longValue());
        } else if (paramValue instanceof Integer) {
            tmpbd = BigDecimal.valueOf(((Integer) paramValue).longValue());
        } else if (paramValue instanceof BigDecimal) {
            tmpbd = (BigDecimal) paramValue;
        } else if (paramValue instanceof String) {
            String sVal = (String) paramValue;
            if (sVal.equals("true") == true) {
                sVal = "1";
            } else if (sVal.equals("false") == true) {
                sVal = "0";
            }
            tmpbd = new BigDecimal(sVal);
        } else if (paramValue instanceof Float) {
            tmpbd = new BigDecimal(paramValue.toString());
        } else if (paramValue instanceof Double) {
            tmpbd = new BigDecimal(((Double) paramValue).toString());
        } else if (paramValue instanceof Boolean) {
            tmpbd = BigDecimal.valueOf(((((Boolean) paramValue).booleanValue() == true) ? 1 : 0));
        } else if (paramValue instanceof Byte) {
            tmpbd = BigDecimal.valueOf(((Byte) paramValue).longValue());
        } else if (paramValue instanceof Short) {
            tmpbd = BigDecimal.valueOf(((Short) paramValue).longValue());
        } else if (paramValue instanceof Integer) {
            tmpbd = BigDecimal.valueOf(((Integer) paramValue).longValue());
            // For LOB Support SB: 10/25/2004
            /*
             * else if (paramValue instanceof DataWrapper) tmpbd =
             * BigDecimal.valueOf(((DataWrapper)paramValue).longValue);
             */
        } else {
            throw Messages.createSQLException(null, locale, "object_type_not_supported", paramValue);
        }
        return tmpbd;
    } // end getBigDecimalValue

    // -------------------------------------------------------------
    /**
     * This method will check a Decimal value according to the precision in the
     * Database table.
     * 
     * @param the
     *            original BigDecimal value to check
     * @param the
     *            Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkDecimalBoundary(Locale locale, BigDecimal inbd, int precision) throws SQLException {
        if (precision > 0) {
            BigDecimal maxbd = new BigDecimal(Math.pow(10, precision));
            BigDecimal minbd = maxbd.negate();
            if ((inbd.compareTo(maxbd) >= 0) || (inbd.compareTo(minbd) < 0)) {
                throw Messages.createSQLException(null, locale, "numeric_out_of_range", inbd.toString());
            }
        }
    } // end checkBigDecimalBoundary
    //---------------------------------------------------------------
    /*code change starts
     *  MR Description:  Warnings not  being displayed  when numeric overflow occurs
     */

    /**
     * This method will check a scale value with the column in the
     * Database table.
     * 
     * @param the
     *             BigDecimal value to check
     * @param the
     *            scale to check with the BigDecimal's scale
     * 
     * @return none
     * 
     */
    static void checkScale(BigDecimal tmpbd, int scale) throws SQLException
    {
        if (tmpbd.scale() > scale)
            if (!((tmpbd.scale() == 1) && (tmpbd.toString().endsWith("0"))))
            {
                try
                {
                    Object[] messageArguments = new Object[1];
                    messageArguments[0] = new String("A numeric overflow occurred during an arithmetic computation " +
                            "or data conversion.");
                    throw Messages.createSQLWarning(null, "8411", messageArguments);
                }
                catch (SQLWarning e)
                {
                    e.printStackTrace();
                }
            }
    }

    //code change ends

    // Fix_LeadingZero - AM 08/07/2006
    private static int getExtraLen(String s) {
        int extra = 0;

        // count the trailing zero
        int inx = s.indexOf(".");
        if (inx != -1) {
            int len = s.length();
            for (int i = len - 1; i > inx; i--) {
                char ch = s.charAt(i);
                if (ch != '0') {
                    break;
                }
                extra++;
            }
        }
        // count for leading zero
        if (s.startsWith("0.") || s.startsWith("-0.")) {
            extra++;
        }

        return extra;
    }

    // -------------------------------------------------------------
    /**
     * This method will check a Decimal value according to the precision in the
     * Database table.
     * 
     * @param the
     *            original BigDecimal value to check
     * @param the
     *            Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkDecimalTruncation(int parameterIndex, Locale locale, BigDecimal inbd, int precision, int scale)
        throws SQLException {
        if (precision <= 0)
            return;

        int expectedLen = precision;

        if (scale > 0)
            expectedLen = precision + 1;

        if (inbd.signum() == -1)
            expectedLen++;
        int actualLen = 0;

        // Fix_LeadingZero - AM 08/07/2006
        expectedLen += getExtraLen(inbd.toString());
        /*
         * if( (actualLen = inbd.toString().length()) > expectedLen ){
         * //System.out.println("Length of " + inbd.toString() + " is greater
         * than " + precision); throw new DataTruncation(parameterIndex, true,
         * false, actualLen, expectedLen); }
         */
        actualLen = inbd.toString().length();
        if (precision > 0) {
            BigDecimal maxbd = new BigDecimal(Math.pow(10, precision - scale));
            BigDecimal minbd = maxbd.negate();
            if ((inbd.compareTo(maxbd) >= 0) || (inbd.compareTo(minbd) < 0)) {
                // System.out.println("Max = " + maxbd.toString() + "\nMin = " +
                // minbd + "\nInputted Val: " + inbd.toString());
                // throw new DataTruncation(parameterIndex, true, false,
                // actualLen, expectedLen);
                throw new SQLException("*** ERROR[29188] Numeric value " + inbd.doubleValue() + " is out of range [" + minbd.doubleValue() + ", " + maxbd.doubleValue() + "]; Parameter index: " + (parameterIndex) +". ["+new SimpleDateFormat("yyyy-MM-dd HH:mm:s").format(new Date())+"]", "22003", -8411);
            }
        }
    } // end checkDecimalTruncation

    // -------------------------------------------------------------
    /**
     * This method will check a Long value according to the Long.MAX_VALUE and
     * Long.MIN_VALUE values.
     * 
     * @param the
     *            original long value to check
     * @param the
     *            Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkLongBoundary(Locale locale, BigDecimal inbd) throws SQLException {
        if ((inbd.compareTo(long_maxbd) > 0) || (inbd.compareTo(long_minbd) < 0)) {
            throw Messages.createSQLException(null, locale, "numeric_out_of_range", inbd.toString());
        }
    } // end checkBigDecimalBoundary

    // -------------------------------------------------------------
    /**
     * This method will check a Double and long value are the same.
     * 
     * @param the
     *            original double value to check
     * @param the
     *            original long value to check
     * @param the
     *            Locale to print the error message in
     * 
     * @return none
     * 
     */
    static void checkLongTruncation(int parameterindex, BigDecimal inbd) throws SQLException {
        long inlong = inbd.longValue();
        double indbl = inbd.doubleValue();

        if ((double) inlong != indbl) {
            int sizeLong = String.valueOf(inlong).length();
            int sizeDbl = String.valueOf(indbl).length();
            // throw new DataTruncation(parameterindex, true, false,
            // sizeLong, sizeDbl);

            DataTruncation dt = new DataTruncation(parameterindex, true, false, sizeLong, sizeDbl);
            dt.setNextException(new SQLException("DataTruncation", "22003", -8411));
            throw dt;
        }
    } // end checkLongTruncation

    /**
     * This method sets the round mode behaviour for the driver. Accepted values
     * are: static int ROUND_CEILING Rounding mode to round towards positive
     * infinity. static int ROUND_DOWN Rounding mode to round towards zero.
     * static int ROUND_FLOOR Rounding mode to round towards negative infinity.
     * static int ROUND_HALF_DOWN Rounding mode to round towards "nearest
     * neighbor" unless both neighbors are equidistant, in which case round
     * down. static int ROUND_HALF_EVEN Rounding mode to round towards the
     * "nearest neighbor" unless both neighbors are equidistant, in which case,
     * round towards the even neighbor. static int ROUND_HALF_UP Rounding mode
     * to round towards "nearest neighbor" unless both neighbors are
     * equidistant, in which case round up. static int ROUND_UNNECESSARY
     * Rounding mode to assert that the requested operation has an exact result,
     * hence no rounding is necessary. static int ROUND_UP Rounding mode to
     * round away from zero. The default behaviour is to do ROUND_DOWN.
     * 
     * @param ref
     *            roundMode
     */
    static int getRoundingMode(String roundMode) {
        int op_roundMode = BigDecimal.ROUND_DOWN;
        if (roundMode == null) {
            op_roundMode = BigDecimal.ROUND_DOWN;
        } else if (roundMode.equals("ROUND_CEILING")) {
            op_roundMode = BigDecimal.ROUND_CEILING;
        } else if (roundMode.equals("ROUND_DOWN")) {
            op_roundMode = BigDecimal.ROUND_DOWN;
        } else if (roundMode.equals("ROUND_FLOOR")) {
            op_roundMode = BigDecimal.ROUND_FLOOR;
        } else if (roundMode.equals("ROUND_HALF_UP")) {
            op_roundMode = BigDecimal.ROUND_HALF_UP;
        } else if (roundMode.equals("ROUND_UNNECESSARY")) {
            op_roundMode = BigDecimal.ROUND_UNNECESSARY;
        } else if (roundMode.equals("ROUND_HALF_EVEN")) {
            op_roundMode = BigDecimal.ROUND_HALF_EVEN;
        } else if (roundMode.equals("ROUND_HALF_DOWN")) {
            op_roundMode = BigDecimal.ROUND_HALF_DOWN;
        } else if (roundMode.equals("ROUND_UP")) {
            op_roundMode = BigDecimal.ROUND_UP;
        } else {
            try {
                op_roundMode = getRoundingMode(Integer.parseInt(roundMode));
            } catch (Exception ex) {
                op_roundMode = BigDecimal.ROUND_DOWN;
            }

        }
        return op_roundMode;
    }

    /**
     * This method sets the round mode behaviour for the driver. Accepted values
     * are: static int ROUND_CEILING Rounding mode to round towards positive
     * infinity. static int ROUND_DOWN Rounding mode to round towards zero.
     * static int ROUND_FLOOR Rounding mode to round towards negative infinity.
     * static int ROUND_HALF_DOWN Rounding mode to round towards "nearest
     * neighbor" unless both neighbors are equidistant, in which case round
     * down. static int ROUND_HALF_EVEN Rounding mode to round towards the
     * "nearest neighbor" unless both neighbors are equidistant, in which case,
     * round towards the even neighbor. static int ROUND_HALF_UP Rounding mode
     * to round towards "nearest neighbor" unless both neighbors are
     * equidistant, in which case round up. static int ROUND_UNNECESSARY
     * Rounding mode to assert that the requested operation has an exact result,
     * hence no rounding is necessary. static int ROUND_UP Rounding mode to
     * round away from zero. The default behaviour is to do ROUND_DOWN.
     * 
     * @param ref
     *            roundMode
     */
    static int getRoundingMode(int roundMode) {
        if ((roundMode == BigDecimal.ROUND_CEILING) || (roundMode == BigDecimal.ROUND_DOWN)
                || (roundMode == BigDecimal.ROUND_UP) || (roundMode == BigDecimal.ROUND_FLOOR)
                || (roundMode == BigDecimal.ROUND_HALF_UP) || (roundMode == BigDecimal.ROUND_UNNECESSARY)
                || (roundMode == BigDecimal.ROUND_HALF_EVEN) || (roundMode == BigDecimal.ROUND_HALF_DOWN)) {
            return roundMode;
        } else {
            return BigDecimal.ROUND_DOWN;
        }
    }

    static BigDecimal setScale(BigDecimal tmpbd, int scale, int roundingMode) throws SQLException {
        try {
            if (scale > -1) {
                tmpbd = tmpbd.setScale(scale, roundingMode);
            }
        } catch (ArithmeticException aex) {
            throw new SQLException(aex.getMessage());
        }
        return tmpbd;
    }

    static final BigDecimal long_maxbd = BigDecimal.valueOf(Long.MAX_VALUE);
    static final BigDecimal long_minbd = BigDecimal.valueOf(Long.MIN_VALUE);
} // end class Utility
