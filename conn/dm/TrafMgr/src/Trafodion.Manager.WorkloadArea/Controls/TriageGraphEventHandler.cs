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

using ZedGraph;
namespace Trafodion.Manager.WorkloadArea.Controls
{
	public class TriageGraphEventHandler : GraphEventHandler {

		private TriageHelper _theTriageHelper = null;

        public TriageGraphEventHandler(TriageHelper ws)
        {
            this._theTriageHelper = ws;
		}

		public override String PointValueEvent(PointPair pt) {

            DateTime endTime = new DateTime();
            DateTime startTime = new DateTime();
			
            string runTime = "";
			string elapsedTime = "";
			try {			

                endTime = DoubleXDateToDateTime(pt.Y);
                startTime = DoubleXDateToDateTime(pt.Z);
                runTime = TriageHelper.getFormattedElapsedTime(startTime, endTime);
                
				elapsedTime = "   Elapsed Time: " + runTime;

			} catch (Exception) {
				elapsedTime = "";
			}

			String timeQueryEnded = endTime.ToString() + "    [Completed] ";
            if ((null != this._theTriageHelper) && (null != pt.Tag) &&
				(null != pt.Tag.ToString())) {
                    this._theTriageHelper.HighlightQuery(pt.Tag.ToString(), startTime.ToString());

                    if (this._theTriageHelper.isHighlightedQueryRunning())
                    {
					    timeQueryEnded = "    [Running] ";
                        try
                        {
                            if (TimeSpan.Parse(runTime).TotalSeconds > TriageHelper.TWELVE_HOURS.TotalSeconds)
                                timeQueryEnded = "    [Abnormally Terminated] ";
                        }
                        catch(Exception ex)
                        {
                            //To Do Something here. I plan to put the original logic here...
                        }
				}
			}

			return "Query ID: " + pt.Tag + Environment.NewLine +
				    elapsedTime + 
					"   Start Time: " + startTime.ToString() +
					"   End Time: " + timeQueryEnded;
		}
        /**
         *This Method is to Convert PointPairValue to DateTime. While default convertion will lose milliSecond information,           
         * so write this method instead     
         */
        private DateTime DoubleXDateToDateTime(double xDate) 
        {
            DateTime returnDateTime = DateTime.Now;
            int numYear, numMonth, numberDay, numHour, numMinute, numSecond, numMilliSecond = 0;
            XDate.XLDateToCalendarDate(xDate, out numYear, out numMonth, out numberDay, out numHour, out numMinute, out numSecond, out numMilliSecond);

            returnDateTime = new DateTime(numYear, numMonth, numberDay, numHour, numMinute, numSecond, numMilliSecond);

            return returnDateTime;
        }

	}
}
