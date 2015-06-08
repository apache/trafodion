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
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Net;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Trafodion.Manager.Framework;

// Ported from NPA SQ R1.0 to support Explain plan feature.
namespace Trafodion.Manager.DatabaseArea.NCC
{
    /// <summary>
    /// Utilities ported from NPA.
    /// </summary>
	public class NCCUtils {

        #region Members

        #region Static Members

        private const String TRACE_SUB_AREA_NAME = "Query Plan";

		/**
		 *  Minimum IGrid column width.
		 */
		public const int  MIN_IGRID_COLUMN_WDIDTH = 64;

		/**
		 *  Maximum IGrid column width.
		 */
		public const int  MAX_IGRID_COLUMN_WDIDTH = 500;

		/**
		 *  Static variable to hold the number format information for the current locale.
		 */
		private static NumberFormatInfo _numFormatInfo = null;

        /**
         *  Constant for stream not a valid binary format. Cryptic, eh!! That's what .NET throws when we
         *  try and open a text file as a serialized object. Rather than hardcode and check for errors, 
         *  just a constant here for any deserialization/load errors.
         */
        public const String IS_NOT_A_BINARY_FILE = "The input stream is not a valid binary format";

		#endregion  /*  End of  region  Static Members.  */

		#endregion  /*  End of  region  Members.  */

		#region Public Methods

		/**
		 *  <summary>
		 *  Returns whether or not the specified ghostname is resolvable --
		 *  really means we can get its DNS entry!! 
		 *  </summary>
		 *
		 *  <param name="hostName">The hostname to check if it can be resolved</param>
		 *  <remarks>Not used at this time</remarks>
		 */
		public static bool IsHostResolvable(String hostName) 
        {
			try {
				IPHostEntry hostEntry = null;
				hostEntry = Dns.GetHostEntry(hostName);

				if (0 >= hostEntry.AddressList.Length)
					return false;   //  Couldn't resolve the host name.

                if (Logger.IsTracingEnabled) {
					foreach (IPAddress ip in hostEntry.AddressList)
                        Logger.OutputToLog(
                                TraceOptions.TraceOption.DEBUG,
                                TraceOptions.TraceArea.Database,
                                TRACE_SUB_AREA_NAME,
                                "NCCUtils::isHostReachable(): HostName " + hostName + " resolved to " + ip.ToString() );
				}

			} catch (Exception e) {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Database,
                        TRACE_SUB_AREA_NAME,
                        "NCCUtils::isHostReachable(): Error resolving " + hostName + ". Details = " + e.Message);
                }

				return false;
			}

			return true;
		}  /*  End of  isHostResolvable  method.  */

		/**
		 *  <summary>
		 *  Returns the number format for the current locale.
		 *  </summary>
		 *
		 *  <param name="numDecimalDigits">Precision for number of digits in the decimal place</param>
		 */
		public static String GetNumberFormatForCurrentLocale(int numDecimalDigits) 
        {
			return GetLocaleNumberFormat(numDecimalDigits, false);
		}  /*  End of  getNumberFormatForCurrentLocale  method.  */

		/**
		 *  <summary>
		 *  Returns the number format for the current locale zero padded if so desired.
		 *  </summary>
		 *
		 *  <param name="numDecimalDigits">Precision for number of digits in the decimal place</param>
		 *  <param name="zeroPad">Whether or not to zero-pad</param>
		 */
		public static String GetLocaleNumberFormat(int numDecimalDigits, bool zeroPad) 
        {
			return "{0:N" + numDecimalDigits + "}";
		}  /*  End of  getLocaleNumberFormat  method.  */

		/**
		 *  <summary>
		 *  Returns the number format to use for parsing -- this is the data sent to and from Trafodion.
		 *  We handle the number formatting on the clients -- Trafodion sends a number something like :
		 *	1,234,567.89  --  but this may be displayed in German as 1.234.567,89  -- ',' is the new '.'!!
		 *  Most European locales follow a similar convention.
		 *  </summary>
		 */
		public static NumberFormatInfo GetNumberFormatInfoForParsing() 
        {
			if (null != _numFormatInfo)
				return _numFormatInfo;

			CultureInfo ci = System.Globalization.CultureInfo.InvariantCulture;
			_numFormatInfo = (NumberFormatInfo) ci.NumberFormat.Clone();
			_numFormatInfo.NumberDecimalSeparator = ".";

			return _numFormatInfo;
		}   /*  End of  getNumberFormatInfoForParsing.  */

		/**
		 *  <summary>
		 *  Saves the specified "stream" (String) of qeries to a script file.
		 *  </summary>
		 *
         **/
		public static void SaveQueriesToScriptFile(String queryTextAndInfo) 
        {
			SaveFileDialog saveDialog = new SaveFileDialog();
			saveDialog.AddExtension = true;
			saveDialog.CreatePrompt = false;
			saveDialog.DefaultExt = "sql";
			saveDialog.OverwritePrompt = true;

			saveDialog.FileName = DateTime.Now.ToString("NPA-MMM dd yyyy - hh_mm_ss tt") + ".sql";

			DialogResult result = saveDialog.ShowDialog();
			if (DialogResult.OK != result)
				return;

			StreamWriter theFile = null;
			try {
				theFile = new StreamWriter(saveDialog.FileName);
				theFile.WriteLine(queryTextAndInfo);

			} 
            catch(Exception e) 
            {
				MessageBox.Show("\nError saving selected queries to script file.\n" +
							    "\t File Name = " + saveDialog.FileName + "\n\n" +
								"Problem: \t Unable to write information to the specified file. \n\n" +
								"Solution: \t Please see the error details for recovery information.\n\n" +
								"Details: \t " + e.Message + "\n\n", "Generate Consolidated SQL Script Error",
								MessageBoxButtons.OK, MessageBoxIcon.Error);
			} 
            finally 
            {
				if (null != theFile)
					theFile.Close();
			}
		}  /*  End of  saveQueriesToScriptFile  method.  */

        /**
         *  <summary>
         *  Colorizes the specified SQL text and adds it to the specified RichTextBox control.
         *  </summary>
         *
         *  <param name="sqlText">Query Text</param>
         *  <param name="theRichTextBox">Ending Time</param>
         *  <remarks>Since: NPA2.0</remarks>
         */
        public static void ColorizeSQLText(String sqlText, RichTextBox theRichTextBox) 
        {
            if (null == theRichTextBox)
                return; 

            String sqlObjectsRE = @"(INDEX|MATERIALIZED\s+VIEW|MVGROUP|PROCEDURE|TABLE|TRIGGER|VIEW)";

            String sqlRE = @"\b(UPDATE\s*STATISTICS|CALL|EXECUTE|" +
                            @"SELECT\s+\x5B\s*(FIRST|ANY)\s+\d+\s*\x5D\s+(ALL|DISTINCT)|" +
                            @"SELECT\s+\x5B\s*(FIRST|ANY)\s+\d+\s*\x5D|SELECT\s+(ALL|DISTINCT)|SELECT|SEL|" +
                            @"DELETE|DEL|UPDATE|UPD|MERGE|INSERT|INS|" +
                            @"ALTER\s+" + sqlObjectsRE + "|" +
                            @"CREATE\s+" + sqlObjectsRE + "|" +
                            @"DROP\s+" + sqlObjectsRE + "|" +
                            @"SET\s+(SCHEMA|TABLE\s+TIMEOUT|TRANSACTION)|" +
                            @"GRANT|REVOKE|BEGIN\s+WORK|COMMIT\s+WORK|ROLLBACK\s+WORK|" +
                            @"LOCK\s+TABLE|POPULATE\s+INDEX|PURGEDATA|REORG|MAINTAIN|" +
                            @"SHOWDDL|INVOKE|CONTROL\s+QUERY\s+DEFAULT|SHOWCONTROL|SHOWSHAPE|INFOSTATS)\b";

            String sqlClausesRE = @"\b(FROM|WHERE|HAVING|GROUP\s+BY|ORDER\s+BY\s+\w+(ASCENDING|DESCENDING)|" +
                                  @"ORDER\s+BY\s+\w+(ASC|DESC)|ORDER\s+BY|VALUES|USING|DEFAULT|" +
                                  @"IN\s+(SHARE|EXCLUSIVE)\s+MODE|UNION\s+ALL|UNION|" +
                                  @"NATURAL(\s+(INNER|LEFT\s+OUTER|LEFT|RIGHT\s+OUTER|RIGHT))\s+JOIN|" +
                                  @"FOR\s+(READ UNCOMMITTED|READ COMMITTED|SERIALIZABLE|REPEATABLE READ|SKIP CONFLICT)\s+ACCESS|" +
                                  @"KEY\s+BY|SAMPLE|SEQUENCE\s+BY|TRANSPOSE|AFTER\s+LAST\s+ROW)\b";

            String sqlFunctionsRE = @"\b(ABS|ACOS|ADD_MONTHS|ASCII|ASIN|ATAN2|ATAN|AVG|CASE|CAST|CEILING|" +
                                    @"CHAR|CHAR_LENGTH|COALESCE|CODE_VALUE|CONCAT|CONVERTTIMESTAMP|COS|COSH|COUNT|" +
                                    @"CURRENT_DATE|CURRENT_TIMESTAMP|CURRENT_TIME|CURRENT|DATE_ADD|DATE_SUB|DATEADD|DATEDIFF|" +
                                    @"DATEFORMAT|DATE_PART|DATE_TRUNC|DAYNAME|DAYOFMONTH|DAYOFWEEK|DAYOFYEAR|DAY|" +
                                    @"DECODE|DEGREES|DIFF1|DIFF2|EXP|EXTRACT|FLOOR|HOUR|INSERT|ISNULL|" +
                                    @"JULIANTIMESTAMP|LASTNOTNULL|LCASE|LEFT|LOCATE|LOG10|LOG|LOWER|LPAD|LTRIM|" +
                                    @"MAXIMUM|MAX|MINUTE|MIN|MOD|MONTHNAME|MONTH|" +
                                    @"MOVINGAVG|MOVINGCOUNT|MOVINGMAX||MOVINGMIN|MOVINGSTDDEV|MOVINGSUM|MOVINGVARIANCE|" +
                                    @"NULLIFZERO|NULLIF|NVL|OCTET_LENGTH|OFFSET|PI|POSITION|" +
                                    @"POWER|QUARTER|RADIANS|RUNNINGRANK|RANK|RUNNINGSTDDEV|REPEAT|REPLACE|" +
                                    @"RIGHT|ROUND|ROWS\s+SINCE\s+CHANGED|ROWS\s+SINCE|RPAD|RTRIM|" +
                                    @"RUNNINGAVG|RUNNINGCOUNT|RUNNINGMAX|RUNNINGMIN|RUNNINGRANK|" +
                                    @"RUNNINGSTDDEV|RUNNINGSUM|RUNNINGVARIANCE|SECOND|SIGN|" +
                                    @"SINH|SIN|SPACE|SQRT|STDDEV|SUBSTRING|SUBSTR|SUM|TANH|TAN|" +
                                    @"THIS|TRANSLATE|TRIM|UCASE|UPPER|UPSHIFT|VARIANCE|WEEK|YEAR|ZEROIFNULL|FORMAT)\b";

            Font boldFont = new Font("Trebuchet MS", 10.75F, FontStyle.Bold);
            Font regularFont = new Font("Arial", 8.25F, FontStyle.Regular);
            Color regColor = Color.FromKnownColor(KnownColor.ControlText);


            theRichTextBox.Text = sqlText;

            theRichTextBox.SelectAll();
            theRichTextBox.SelectionColor = regColor;
            theRichTextBox.SelectionFont = regularFont;

            // Change the color for the SQL statement/clauses/functions.
            // RR:  Need to do the line breaks and tabs still in the colorization. 
            ColorRTBTextSelection(theRichTextBox, sqlRE, boldFont, Color.MidnightBlue, false /* true */);
            ColorRTBTextSelection(theRichTextBox, sqlClausesRE, boldFont, Color.ForestGreen, false /* true */);
            ColorRTBTextSelection(theRichTextBox, sqlFunctionsRE, boldFont, Color.Brown, false);

            theRichTextBox.Select(theRichTextBox.Text.Length, theRichTextBox.Text.Length);
            theRichTextBox.SelectionColor = regColor;
            theRichTextBox.SelectionFont = regularFont;
            theRichTextBox.ClearUndo();   // This prevents undoing color changes on a CTRL+Z etc.
        }  /*  End of  colorizeSQLText(String, RichTextBox)  method.  */

		#endregion  /*  End of  region  Public Methods.  */

        #region  Private Methods

        /// <summary>
        /// ColorRTBTextSelection
        /// </summary>
        /// <param name="rtbox"></param>
        /// <param name="regExp"></param>
        /// <param name="selFont"></param>
        /// <param name="selColor"></param>
        /// <param name="insertCRLF"></param>
        private static void ColorRTBTextSelection(RichTextBox rtbox, String regExp, Font selFont, Color selColor, bool insertCRLF) 
        {
            try 
            {
                MatchCollection stopWords = Regex.Matches(rtbox.Text, regExp,
                                                          RegexOptions.Multiline | RegexOptions.IgnoreCase);

                int sqlLen = rtbox.Text.Length;
                String suffix = "";

                if (insertCRLF)
                    suffix = Environment.NewLine + "     ";

                if ((null != stopWords) && (0 < stopWords.Count)) {
                    for (int idx = 0; idx < stopWords.Count; idx++) {
                        String value = stopWords[idx].Value;
                        int startIdx = stopWords[idx].Index;
                        bool addSuffixToWord = insertCRLF;

                        if ((-1 < startIdx) && (sqlLen > startIdx)) {
                            rtbox.Select(startIdx, value.Length + suffix.Length);
                            String word = rtbox.SelectedText;

                            rtbox.SelectionColor = selColor;
                            rtbox.SelectionFont = selFont;

                            if (addSuffixToWord && !word.EndsWith(suffix))
                            {
                                rtbox.Select(startIdx, value.Length);
                                word = rtbox.SelectedText;

                                rtbox.SelectionColor = selColor;
                                rtbox.SelectionFont = selFont;

                                if (0 < word.Length)
                                    rtbox.SelectedText = word.ToUpper() + suffix;
                            }
                            else
                            {
                                rtbox.SelectedText = word.ToUpper();
                            }
                        }
                    }
                }

            } 
            catch (Exception e)
            {
                Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Database,
                        TRACE_SUB_AREA_NAME,
                        "NCCUtils::colorRTBTextSelection(): Error with Regular Expression: " + regExp + "\n\tException: " + e.Message);
            }
        }  /*  End of  colorRTBTextSelection  method.  */

		#endregion  /*  End of  region  Private Methods.  */

	}  /*  End of  class  NCCUtils.  */
} 

