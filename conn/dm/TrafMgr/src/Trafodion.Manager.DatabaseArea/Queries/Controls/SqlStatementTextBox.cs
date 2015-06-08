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
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{

    /// <summary>
    /// Our standard way of representing SQL statements
    /// </summary>
    public class SqlStatementTextBox : TrafodionRichTextBox
    {
        static private Font boldFont = new Font("Trebuchet MS", 10.75F, FontStyle.Bold);
        static private Font regularFont = new Font("Tahoma", 8.25F, FontStyle.Regular);
        static private Color regColor = Color.FromKnownColor(KnownColor.ControlText);
        static private Font textFont = new Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));

        public SqlStatementTextBox()
        {
            Multiline = true;
            WordWrap = false;
            BackColor = System.Drawing.Color.WhiteSmoke;
            Font = textFont;
            //Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            
        }

        private int _nesting = -1;
        
        protected override void OnTextChanged(EventArgs e)
        {
            base.OnTextChanged(e);

            _nesting++;
            if (_nesting == 0)
            {
                // SOmetimes DDL coming in from the Trafodion has both \r\n and \n which confuses the textbox.  It seems
                // to pick whichever it sees first and then puts in "blots" for occurrences of the other.
                Text.Replace("\r\n", "\n");

                //Save the location of the cursor
                int cursorLoc = SelectionStart;

                // TODO - Syntax highlighting
                //SyntaxHighlight(Text);

                //Set the cursor postion after syntax highlighting
                //SelectionStart = cursorLoc;
                //SelectionLength = 0;
            }
            _nesting--;

        }

        private void colorRTBTextSelection(RichTextBox rtbox, String regExp, Font selFont, Color selColor,
                                           bool insertCRLF)
        {
            try
            {
                MatchCollection stopWords = Regex.Matches(rtbox.Text, regExp,
                                                        RegexOptions.Multiline | RegexOptions.IgnoreCase);

                int sqlLen = rtbox.Text.Length;
                String suffix = "";

                if (insertCRLF)
                    suffix = Environment.NewLine + "     ";

                if ((null != stopWords) && (0 < stopWords.Count))
                {
                    for (int idx = 0; idx < stopWords.Count; idx++)
                    {
                        String value = stopWords[idx].Value;
                        int startIdx = stopWords[idx].Index;
                        bool addSuffixToWord = insertCRLF;

                        if ((-1 < startIdx) && (sqlLen > startIdx))
                        {
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
                                rtbox.SelectedText = word.ToUpper();


                        }
                    }
                }

            }
            catch (Exception e)
            {
            }

        }
        //Colorize the Sql text and organize
        private void SyntaxHighlight(String sqlText)
        {
            lock (this)
            {
            }

            this.Text = sqlText;

            String sqlObjectsRE = @"(CATALOG|SCHEMA|INDEX|MATERIALIZED\s+VIEW|MVGROUP|PROCEDURE|TABLE|TRIGGER|VIEW|SYNONYM)";

            String sqlRE = @"\b(UPDATE\s*STATISTICS|CALL|EXECUTE|" +
                            @"SELECT\s+\x5B\s*(FIRST|ANY)\s+\d+\s*\x5D\s+(ALL|DISTINCT)|" +
                            @"SELECT\s+\x5B\s*(FIRST|ANY)\s+\d+\s*\x5D|SELECT\s+(ALL|DISTINCT)|SELECT|SEL|" +
                            @"DELETE|DEL|UPDATE|UPD|MERGE|INSERT|INS|" +
                            @"ALTER\s+" + sqlObjectsRE + "|" +
                            @"CREATE\s+" + sqlObjectsRE + "|" +
                            @"CREATE VOLATILE\s+" + sqlObjectsRE + "|" +
                            @"CREATE UNIQUE\s+" + sqlObjectsRE + "|" +
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




            //EventHandler textChangeHandler = this.TextChanged;
            try
            {
                //this.TextChanged -= textChangeHandler;          
                this.SelectAll();
                this.SelectionColor = regColor;
                this.SelectionFont = regularFont;

                // Change the color for the SQL statement/clauses/functions.
                // RR:  Need to do the line breaks and tabs still in the colorization. 
                colorRTBTextSelection(this, sqlRE, boldFont, Color.MidnightBlue, false /* true */);
                colorRTBTextSelection(this, sqlClausesRE, boldFont, Color.ForestGreen, false /* true */);
                colorRTBTextSelection(this, sqlFunctionsRE, boldFont, Color.Brown, false);

                this.Select(this.Text.Length, this.Text.Length);
                this.SelectionColor = regColor;
                this.SelectionFont = regularFont;
                this.ClearUndo();  //This prevents undoing color changes on a CTRL+Z etc.

            }
            finally
            {
                // this.TextChanged += textChangeHandler;
            }

        }
        protected override void OnSelectionChanged(EventArgs e)
        {
            //this.OnVisibleChanged
            //SelectionFont = new Font("Tahoma", 12, FontStyle.Bold);
            //SelectionColor = System.Drawing.Color.Red;
            base.OnSelectionChanged(e);
        }

        public void SyntaxHighlightAll()
        {
            _nesting++;
            SyntaxHighlight(Text);
            _nesting--;
        }

        public void ResetSyntax()
        {
            this.SelectAll();
            this.SelectionColor = regColor;
            this.SelectionFont = regularFont;
        }

    }

}
