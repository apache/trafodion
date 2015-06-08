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
using System.IO;
using System.Xml;
using System.Xml.Serialization;

namespace Trafodion.Manager.MetricMiner
{
    public class HistoryLogger
    {
        
        Object locker = new Object();
        public delegate void NewEntriesAdded(List<HistorylogElement> logEntries);
        public delegate void EntriesRemoved(List<HistorylogElement> logEntries);

        public event NewEntriesAdded OnNewEntriesAdded;
        public event EntriesRemoved  OnEntriesRemoved;
        String logFileName = null;

        private HistoryLogger()
        {
            logFileName = Path.Combine(System.Windows.Forms.Application.StartupPath, "HistoryLog.log");
            TextWriter tw = null;
            try
            {
                tw = new StreamWriter(logFileName, true);
            }
            catch
            {
                logFileName = Path.Combine(Trafodion.Manager.Framework.Persistence.HomeDirectory, "HistoryLog.log");
                tw = new StreamWriter(logFileName, true);
            }
            finally
            {
                tw.Close();
            }
        }

        private static HistoryLogger _theInstance = new HistoryLogger();

        /// <summary>
        /// Get instance of the logger
        /// </summary>
        public static HistoryLogger Instance
        {
            get
            {
                return _theInstance;
            }
        }

        /// <summary>
        /// Add a single entry to the log file
        /// </summary>
        /// <param name="logEntry"></param>
        public void AddToLog(HistorylogElement logEntry)
        {
                List<HistorylogElement> logEntries = new List<HistorylogElement>();
                logEntries.Add(logEntry);
               
                lock (locker)
                {
                    //write the entries to the file
                    writeToFile(logEntries);
                }

                //notify listeners that new logf elements have been added
                if (this.OnNewEntriesAdded != null)
                {
                    OnNewEntriesAdded(logEntries);
                }
           
        }

        /// <summary>
        /// Add multiple entries to the log
        /// </summary>
        /// <param name="logEntries"></param>
        public void AddToLog(List<HistorylogElement> logEntries)
        {
            lock (locker)
            {
                //write the entries to the file
                writeToFile(logEntries);
            }

            //notify listeners that new logf elements have been added
            if (this.OnNewEntriesAdded != null)
            {
                OnNewEntriesAdded(logEntries);
            }
        }

        /// <summary>
        /// Delete the selected contents from the file
        /// </summary>
        public void DeleteLogContents(List<HistorylogElement> logEntries)
        {
            List<HistorylogElement> deletedEntries = new List<HistorylogElement>();

            lock (locker)
            {
                //Get the entire content from the log files
                List<HistorylogElement> logEntriesFromFile = LoadLogContents();

                //remove the ones specified in the list
                foreach(HistorylogElement hle in logEntries)
                {
                    bool deleted = logEntriesFromFile.Remove(hle);
                    if (deleted)
                    {
                        deletedEntries.Add(hle);
                    }
                }

                //save the entire content back
                writeToFile(logEntriesFromFile);
            }
            //notify listeners that log entries have been removed
            if (OnEntriesRemoved != null)
            {
                OnEntriesRemoved(deletedEntries);
            }
        }

        //Returns the contents of the log file 
        public List<HistorylogElement> GetLogContents()
        {
            lock (locker)
            {
                return LoadLogContents();
            }
        }
        /// <summary>
        /// Load the entire contents from the file system
        /// </summary>
        private List<HistorylogElement> LoadLogContents()
        {
            List<HistorylogElement> logEntries = new List<HistorylogElement>();
            //load the entire content from the file and return back as an arraylist
            XmlSerializer s = new XmlSerializer(typeof(Historylog));
            TextReader reader = null;
            StringBuilder sb = new StringBuilder();
            StringWriter sw = new StringWriter(sb);

            Historylog log = null;
            try
            {
                reader = new StreamReader(logFileName);
                sw.Write("<?xml version=\"1.0\" encoding=\"utf-16\"?><Historylog xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><LogContents>");
                sw.Write(reader.ReadToEnd());
                sw.Write("</LogContents></Historylog>");
                //string line = reader.ReadLine();
                //while (line != null)
                //{
                //    sw.WriteLine(line);
                //    line = reader.ReadLine();
                //}



                log = (Historylog)s.Deserialize(new StringReader(sb.ToString()));
                logEntries = log.LogContents;
                reader.Close();
            }
            catch (Exception ex)
            {
                
            }
            return logEntries;
        }


        /// <summary>
        /// Helper methods to write to file
        /// Note: This method must always be called within a lock
        /// </summary>
        /// <param name="logEntries"></param>
        private void writeToFile(List<HistorylogElement> logEntries)
        {
            TextWriter tw = null;
            try
            {
                //open file
                tw = new StreamWriter(logFileName, true);
                
                //Get the hlm xml
                string xml = getXMLForLogElements(logEntries);

                //write it to the file
                tw.WriteLine(xml);


            }
            catch (Exception ex)
            {

            }
            finally
            {
                //close the file
                if ((tw != null))
                {
                    tw.Close();
                }
            }
        }

        /// <summary>
        /// Method to get the XML for just the log entries part.
        /// </summary>
        /// <param name="logEntries"></param>
        /// <returns></returns>
        private string getXMLForLogElements(List<HistorylogElement> logEntries)
        {
            string logXml = null;
            XmlSerializer s = new XmlSerializer(typeof(Historylog));
            StringBuilder sb = new StringBuilder();
            StringWriter sw = new StringWriter(sb);

            Historylog log = new Historylog();
            log.LogContents = logEntries;
            s.Serialize(sw, log);

            //get the part log element parts only
            logXml = sb.ToString();
            string startPattern = "<LogContents>";
            string endPattern = "</LogContents>";
            int idx = logXml.IndexOf(startPattern, StringComparison.InvariantCultureIgnoreCase);
            if (idx >= 0)
            {
                logXml = logXml.Substring(idx + startPattern.Length);
                logXml = logXml.Substring(0, logXml.LastIndexOf(endPattern, StringComparison.InvariantCultureIgnoreCase)).Trim();
            }

            return logXml;
        }

    }

    [Serializable]
    [XmlRoot("Historylog")]
    [XmlInclude(typeof(HistorylogElement))]
    public class Historylog 
    {
        [XmlArray("LogContents")]
        [XmlArrayItem("hle")]
        List<HistorylogElement> _theLogContents = new List<HistorylogElement>();

        public List<HistorylogElement> LogContents
        {
            get { return _theLogContents; }
            set { _theLogContents = value; }
        }
    }

    public class HistorylogElement : IComparable 
    {
        public enum QueryStatus {Success, Error, Cancelled }
        private const int THE_MAX_ONE_LINE_LENGTH = 120;
        private long        _theId;
        private string      _theSqlText;
        private DateTime    _theExecutionTime;
        private QueryStatus _theQueryStatus;
        private String      _theConnectionAttributes;
        private String      _theExecutionStats;

        [XmlElement("id")]
        public long Id
        {
            get { return _theId; }
            set { _theId = value; }
        }

        [XmlElement("es")]
        public String ExecutionStats
        {
            get { return _theExecutionStats; }
            set { _theExecutionStats = value; }
        }

        [XmlElement("ca")]
        public String ConnectionAttributes
        {
            get { return _theConnectionAttributes; }
            set { _theConnectionAttributes = value; }
        }

        [XmlElement("qs")]
        public QueryStatus TheQueryStatus
        {
            get { return _theQueryStatus; }
            set { _theQueryStatus = value; }
        }

        [XmlElement("et")]
        public DateTime ExecutionTime
        {
            get { return _theExecutionTime; }
            set { _theExecutionTime = value; }
        }

        [XmlElement("st")]
        public string SqlText
        {
            get { return _theSqlText; }
            set { _theSqlText = value; }
        }


        public int CompareTo(object obj)
        {
            if (obj is HistorylogElement)
            {
                HistorylogElement otherElement = (HistorylogElement)obj;
                if (otherElement.Id == _theId)
                {
                    return 0;
                }
                else
                {
                    //Could return a false match if the timestamps are the same. Revisit later
                    return this._theExecutionTime.CompareTo(otherElement._theExecutionTime);
                }
            }
            else
            {
                return -1;
            }
        }

        private string OneLineSummary
        {
            get
            {
                string theString = _theSqlText;
                int newLineIndex = theString.IndexOf("\n");
                if (newLineIndex > 0)
                {
                    if (newLineIndex <= THE_MAX_ONE_LINE_LENGTH && theString.Length >= THE_MAX_ONE_LINE_LENGTH)
                    {
                        theString = theString.Substring(0, THE_MAX_ONE_LINE_LENGTH).Insert(THE_MAX_ONE_LINE_LENGTH, "...");
                    }
                    else
                    {
                        theString = theString.Substring(0, newLineIndex).Insert(newLineIndex, "...");
                    }
                }
                theString = theString.Substring(0, Math.Min(THE_MAX_ONE_LINE_LENGTH, theString.Length));
                theString = theString.Replace("\n", " ");
                theString = theString.Replace("\t", " ");
                return theString;
            }
        }

        public override string ToString()
        {
            return OneLineSummary;
        }
    }


}
