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
using System.Data;
using System.Collections;
using TenTec.Windows.iGridLib;
using System.IO;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.WorkloadArea.Model
{
    public class WMSUtils
    {
        public const string WARN_LEVEL_NOWARN = "NOWARN";
        public const string WARN_LEVEL_LOW = "LOW";
        public const string WARN_LEVEL_MEDIUM = "MEDIUM";
        public const string WARN_LEVEL_HIGH = "HIGH";

        private const string TRACE_SUB_AREA_NAME = "WMS Utilities";
        private const double TEN_THOUSAND_YEARS = (double)10000 * 365.25 * 24 * 60 * 60 * 1000;
        private const double ONE_YEAR = (double)10000 * 1 * 24 * 60 * 60 * 1000;

        public static string convertJulianTimeStamp(object timeInJulian)
        {
            string timeStamp = timeInJulian.ToString();
            Int64 julianTime = Convert.ToInt64(timeInJulian);
            if (julianTime == -1 || julianTime == 0)
                return "";

            if (julianTime < 0)
                return timeInJulian.ToString();

            /*
                >>values (juliantimestamp(timestamp '0001-01-01 00:00:00.000000'));
                  (EXPR)
                  --------------------
                  148731163200000000
            */
            Int64 startTime = 148731163200000000;   //0001-01-01 00:00:00.000000
            Int64 elapseTime = (julianTime - startTime) * 10;
            DateTime dt = new DateTime(elapseTime);
            ////dt = dt.ToLocalTime();
            ////timeStamp = String.Format("{0:0000}-{1:00}-{2:00} {3:00}:{4:00}:{5:00}.{6:D}",
            //timeStamp = String.Format("{0:0000}-{1:00}-{2:00} {3:00}:{4:00}:{5:00}.{6:000000}",
            //                          dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second, dt.Millisecond);
            //timeStamp = dt.ToString("yyyy-MM-dd HH:mm:ss.FFFFFF");
            timeStamp = Utilities.GetTrafodionSQLLongDateTime(dt, false);
            return timeStamp;


        }

        public static string convertJulianTimeStampLCT(object timeInJulian)
        {
            string timeStamp = timeInJulian.ToString();
            Int64 julianTime = Convert.ToInt64(timeInJulian);
            if (julianTime <= 0)
                return timeInJulian.ToString();

            Int64 startTime = 148731163200000000;   //0001-01-01 00:00:00.000000
            Int64 elapseTime = (julianTime - startTime) * 10;
            DateTime dt = new DateTime(elapseTime);
            timeStamp = String.Format("{0:yyyy-MM-dd HH:mm:ss.FFFFFF tt}", dt);
            return timeStamp;
        }

        //input argument value - in sec output hhh:mm:ss
        public static string formatInt2Time(Int64 value)
        {
            int hour = (int)(value / 3600);
            int remainder = (int)(value % 3600);
            int minute = remainder / 60;
            int second = remainder % 60;
            string formatValue = String.Format("{0:000}:{1:00}:{2:00}", hour, minute, second);

            return formatValue;
        }

        //input argument value - in microsec output ss.sss
        public static string formatInt2TimeMicroSec(Int64 value)
        {
            Int64 sec = value / 1000000;
            Int64 msec = value / 1000;
            msec = ((value % 1000) >= 500) ? msec + 1 : msec;
            string formatValue = String.Format("{0:#,0}.{1:000}", sec, msec);

            return formatValue;
        }

        public static string FormatTimeFromMilliseconds(double milliseconds)
        {            
            if (milliseconds >= TimeSpan.MaxValue.TotalMilliseconds)
            {
                return string.Format("{0:0,0.00}", milliseconds/1000);
            }
            TimeSpan myTimeSpan = TimeSpan.FromMilliseconds(milliseconds);
            return string.Format("{0:000}:{1:00}:{2:00}.{3:000}", (int)myTimeSpan.TotalHours, myTimeSpan.Minutes, myTimeSpan.Seconds, myTimeSpan.Milliseconds);
        }

        public static DataTable getAssoicatedDataTable(string serviceName, DataTable dtConn, DataTable dtComp, DataTable dtExec)
        {
            DataTable dtNew = new DataTable();
            string[] columns = { "TYPE", "RULE_NAME", "SERVICE_NAME" };
            Type typeString = System.Type.GetType("System.String");
            Type[] types = { typeString, typeString, typeString };
            for (int c = 0; c < columns.Length; c++)
            {
                DataColumn dcNew = new DataColumn(columns[c], types[c]);
                dtNew.Columns.Add(dcNew);
            }

            int rows = dtConn.Rows.Count;
            for (int r = 0; r < rows; r++)
            {
                if (serviceName.Equals(dtConn.Rows[r]["SERVICE_NAME"].ToString()))
                {
                    DataRow drNew = dtNew.NewRow();
                    drNew[0] = dtConn.Rows[r]["TYPE"];
                    drNew[1] = dtConn.Rows[r]["RULE_NAME"];
                    drNew[2] = dtConn.Rows[r]["SERVICE_NAME"];
                    dtNew.Rows.Add(drNew);
                }
            }

            rows = dtComp.Rows.Count;
            for (int r = 0; r < rows; r++)
            {
                DataRow drNew = dtNew.NewRow();
                drNew[0] = dtComp.Rows[r]["TYPE"];
                drNew[1] = dtComp.Rows[r]["RULE_NAME"];
                drNew[2] = dtComp.Rows[r]["SERVICE_NAME"];
                dtNew.Rows.Add(drNew);
            }

            rows = dtExec.Rows.Count;
            for (int r = 0; r < rows; r++)
            {
                DataRow drNew = dtNew.NewRow();
                drNew[0] = dtExec.Rows[r]["TYPE"];
                drNew[1] = dtExec.Rows[r]["RULE_NAME"];
                drNew[2] = dtExec.Rows[r]["SERVICE_NAME"];
                dtNew.Rows.Add(drNew);
            }

            return dtNew;
        }

        public static void renameColumnNames(ref DataTable dataTable)
        {
            for (int i = 0; i < dataTable.Columns.Count; i++)
            {
                dataTable.Columns[i].ColumnName = TrafodionIGridUtils.ConvertUnderlineToBreak(dataTable.Columns[i].ColumnName);
            }
        }

        public static void renameColumnNamesSpace(ref DataTable dataTable)
        {
            for (int i = 0; i < dataTable.Columns.Count; i++)
            {
                dataTable.Columns[i].ColumnName = dataTable.Columns[i].ColumnName.Replace("_", " ");
            }
        }

        public static string getCharSet(ConnectionDefinition connectionDefn)
        {
            string columnNameCharsetPrefix = Trafodion.Manager.DatabaseArea.Model.Queries.GetColumnEncoding(connectionDefn, "MANAGEABILITY", "INSTANCE_REPOSITORY", connectionDefn.MetricQueryView, "QUERY_ID");
            return (string.IsNullOrEmpty(columnNameCharsetPrefix) ? columnNameCharsetPrefix : "_" + columnNameCharsetPrefix);
        }

        public static String getFormattedElapsedTime(TimeSpan runTime)
        {
            String retValue = "";

            try
            {
                if (runTime.Ticks < 0)
                    runTime = new TimeSpan(0);

                int numHours = runTime.Hours + (runTime.Days * 24);
                retValue = String.Format("{0:00}", numHours) + ":" +
                            String.Format("{0:00}", runTime.Minutes) + ":" +
                            String.Format("{0:00}", runTime.Seconds) + "." +
                            String.Format("{0:000}", runTime.Milliseconds);
            }
            catch (Exception)
            { }

            return retValue;

        }

        public static String getFormattedElapsedTime(DateTime startTime, DateTime endTime)
        {
            String retValue = "";

            try
            {
                TimeSpan runTime = endTime.Subtract(startTime);
                retValue = getFormattedElapsedTime(runTime);
            }
            catch (Exception)
            { }

            return retValue;

        }

        public static double calculateDeltaCounterValue(String counterName, DataRow currRow, DataRow prevRow)
        {
            double deltaValue = 0;

            try
            {
                double currentValue = double.Parse(currRow[counterName].ToString());
                double previousValue = 0;
                if (null != prevRow)
                    previousValue = double.Parse(prevRow[counterName].ToString());

                deltaValue = currentValue - previousValue;
            }
            catch (Exception)
            { }

            return deltaValue;
        }
        
    }
}
