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
using System.Data.Odbc;
using Trafodion.Manager;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;

using System.Collections;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Timers;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Connections
{
    [Serializable]
    public class NSMThreshold
    {
        private int _maxThresholdValue;

        public int MaxThresholdValue
        {
            get { return _maxThresholdValue; }
            set { _maxThresholdValue = value; }
        }

        private SystemMetric _sysMetric;
        public SystemMetric SysMetric
        {
            get { return _sysMetric; }
        }

        public NSMThreshold(SystemMetric aSysMetric, int thresholdValue)
        {
            _sysMetric = aSysMetric;
            _maxThresholdValue = thresholdValue;
        }
    }

    /// <summary>
    /// This class exists to suppress change events on connection definitions as they are being loaded and saved.
    /// See "if (aConnectionDefinition is ScratchConnectionDefinition)" in the persistence code above.
    /// </summary>
    [Serializable]
    public class NSMServerConfigurationDefinition
    {
        //Connection options
        private int _portNumber = 4746;
        private int _portNumberINC = 0;
        private bool _useIncPort = false;

        //Refresh rate settings
        private int _refreshRate = 2;        

        //private bool fetchConnectivity = true;
        //private bool fetchTransactions = true;
        //private bool fetchDisk = true;

        private bool fetchConnectivity = false;
        private bool fetchTransactions = false;
        private bool fetchDisk = false;

        //ArrayList of threshold values
        private ArrayList _thresholdValues = new ArrayList();

        public ArrayList ThresholdValues
        {
            get { return _thresholdValues; }
        }

        public int RefreshRate
        {
            get 
            {
                IDataStatus dataStatus = new RefreshRateStatus(_refreshRate);
                return dataStatus.NewValue;
            }
            set 
            {
                IDataStatus dataStatus = new RefreshRateStatus(value);
                _refreshRate = dataStatus.NewValue;
            }
        }

        public int PortNumber
        {
            get 
            {
                IDataStatus dataStatus = new PortNumberStatus(_portNumber);
                return dataStatus.NewValue;
            }
            set
            {
                IDataStatus dataStatus = new PortNumberStatus(value);
                _portNumber = dataStatus.NewValue;
            }
        }

        public int PortNumberINC
        {
            get { return _portNumberINC; }
            set { _portNumberINC = value; }
        }

        public bool UseIncPort
        {
            get { return _useIncPort; }
            set { _useIncPort = value; }
        }

        public bool FetchDisk
        {
            get { return fetchDisk; }
            set { fetchDisk = value; }
        }

        public bool FetchConnectivity
        {
            get { return fetchConnectivity; }
            set { fetchConnectivity = value; }
        }

        public bool FetchTransactions
        {
            get { return fetchTransactions; }
            set { fetchTransactions = value; }
        }

        /// <summary>
        /// Creates a new uninitialized connection definition
        /// </summary>
        public NSMServerConfigurationDefinition()
        {
            this.AddThreshold(MetricTypes.CPUBusy);            
            this.AddThreshold(MetricTypes.FreeMemory);            
        }

        private void AddThreshold(SystemMetric aSysMetric)
        {
            _thresholdValues.Add(new NSMThreshold(aSysMetric, aSysMetric.MaxValThreshold));
        }

        private void AddThreshold(SystemMetric aSysMetric, int aThresholdValue)
        {
            _thresholdValues.Add(new NSMThreshold(aSysMetric, aThresholdValue));
        }

        public int GetThresholdForMetric(SystemMetric aSysMetric)
        {
            foreach (NSMThreshold thresh in this._thresholdValues)
            {
                if (thresh.SysMetric.Equals(aSysMetric))
                    return thresh.MaxThresholdValue;
            }

            return -1;
        }

        public bool SetThresholdForMetric(SystemMetric aSysMetric, string aNewThreshold)
        {
            int parsedThreshold = 0;
            if (!Int32.TryParse(aNewThreshold, out parsedThreshold))
                return false;

            return SetThresholdForMetric(aSysMetric, parsedThreshold);
        }

        public bool SetThresholdForMetric(SystemMetric aSysMetric, int aNewThreshold)
        {
            for (int i = 0; i < this._thresholdValues.Count; i++)
            {
                NSMThreshold thresh = ((NSMThreshold)this._thresholdValues[i]);
                if (thresh.SysMetric.Equals(aSysMetric))
                {
                    ((NSMThreshold)this._thresholdValues[i]).MaxThresholdValue = aNewThreshold;
                    return true;
                }
            }

            return false;
        }
    }



    /// <summary>
    /// A connection.  It has an open ODBC connection generated from a given connection definition.
    /// </summary>
    public class NSMConnection
    {
        public static readonly string DefaultSessionName = "MANAGEABILITY";
        private static readonly string _sessionKey = ";SESSION=";
        private bool _isServerActive = false;
        private NSMServerConfigurationDefinition _nsmServerConfigDef = new NSMServerConfigurationDefinition();
        private ArrayList _CPUDownList = new ArrayList();
        private int _failedConnections = -1;

        public delegate void ChangingHandler(object sender, int ReconnectCount);
        public event ChangingHandler ConnectionEvent;

        public ArrayList CPUDownList
        {
            get { return _CPUDownList; }
            set { _CPUDownList = value; }
        }


        //private  = new Dictionary<string, int>();
        System.Timers.Timer RefreshClock = new System.Timers.Timer();

        private static Dictionary<ConnectionDefinition, NSMConnection> _theNSMConnections = new Dictionary<ConnectionDefinition, NSMConnection>(new MyConnectionDefinitionComparer());
        public static Dictionary<ConnectionDefinition, NSMConnection> NSMConnections
        {
            get
            {
                return _theNSMConnections;
            }
        }

        /// <summary>
        /// Finds an instance of NSMConnection for this connection definition. If a system does not exists, a new instance is created
        /// </summary>
        /// <param name="connectionDefinition"></param>
        /// <returns></returns>
        public static NSMConnection FindNSMConnection(ConnectionDefinition connectionDefinition)
        {
            return FindNSMConnection(connectionDefinition, true);
        }

        /// <summary>
        /// Finds an instance of NSMConnection for this connection definition. If a system does not exists, a new instance is created
        /// </summary>
        /// <param name="connectionDefinition"></param>
        /// <returns></returns>
        public static NSMConnection FindNSMConnection(ConnectionDefinition connectionDefinition, bool createIfNotFound)
        {
            if (null == connectionDefinition)
                return new NSMConnection();

            NSMConnection nsmConnection = null;
            IEqualityComparer<ConnectionDefinition> test = _theNSMConnections.Comparer;
            bool conExists = _theNSMConnections.TryGetValue(connectionDefinition, out nsmConnection);
            if ((!conExists || nsmConnection == null) && createIfNotFound)
            {
                //Create a new NSM connection based on the Connection Definition
                nsmConnection = new NSMConnection(connectionDefinition);
                _theNSMConnections.Add(connectionDefinition, nsmConnection);
            }

            return nsmConnection;
        }

        public void UpdateConfigDef(NSMServerConfigurationDefinition aInConfigDef)
        {
            this.Close();
            if (null != this.theSocketConnection)
            {
                theSocketConnection.Close();
                theSocketConnection = null;
            }
                

            this._nsmServerConfigDef.PortNumber = aInConfigDef.PortNumber;
            this._nsmServerConfigDef.RefreshRate = aInConfigDef.RefreshRate;
            this._nsmServerConfigDef.PortNumberINC = aInConfigDef.PortNumberINC;
            this._nsmServerConfigDef.UseIncPort = aInConfigDef.UseIncPort;


            foreach (NSMThreshold thresh in aInConfigDef.ThresholdValues)
            {
                this._nsmServerConfigDef.SetThresholdForMetric(thresh.SysMetric, thresh.MaxThresholdValue);
            }

            //Set which System Status stats we ask for and expect
            this._nsmServerConfigDef.FetchConnectivity = aInConfigDef.FetchConnectivity;
            this._nsmServerConfigDef.FetchDisk = aInConfigDef.FetchDisk;
            this._nsmServerConfigDef.FetchTransactions = aInConfigDef.FetchTransactions;


            this.BindAndInitialize();
        }


        public bool IsServerActive
        {
            get { return _isServerActive; }
        }
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition to use</param>
        public NSMConnection()
        {

        }


        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition to use</param>
        public NSMConnection(ConnectionDefinition aConnectionDefinition)
        {

            // Save it
            theConnectionDefinition = aConnectionDefinition;

            //Load Persisted MaxValueSettings
            //ConnectionDefinition.Changed += new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);

            //// Open an ODBC connection
            //theOdbcConnection = GetOpenOdbcConnection(ConnectionDefinition);

            //// Tell anyone interested that a connection has been opened
            //FireOpened(ConnectionDefinition);
        }

        /// <summary>
        /// If the connection definition has changed/removed, the static dictionary is updated accordingly
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            //FindNSMConnection(aConnectionDefinition). = false;
            //_theNSMConnections.(aConnectionDefinition);
        }

        void RefreshClock_Elapsed(object sender, ElapsedEventArgs e)
        {
            try
            {
                //Send initial 'handshake' message to the server
                string SERVER_REQUEST = this.buildServerRequest();
                SendMessageToServer(SERVER_REQUEST);                

                SocketPacket theSocPkt = new SocketPacket();
                theSocPkt.thisSocket = theSocketConnection;
                theSocketConnection.ReceiveTimeout = 3000;                

                if (0 != theSocketConnection.Available) 
                {
                    int iRx = theSocketConnection.Receive(theSocPkt.dataBuffer);
                    string RecievedData = OnDataReceived(theSocPkt, iRx);                    
                    ParseUpdateData(0, RecievedData);
                }   
            }
            catch (SocketException ex)
            {
                this.handleSocketException();
                //MessageBox.Show("NSM Error");                
                
                //RefreshClock.Elapsed -= new ElapsedEventHandler(RefreshClock_Elapsed);
            }
            catch (Exception ex)
            {
                //MessageBox.Show("NSM Error");
            }
        }

        /// <summary>
        /// Close our NSM connection
        /// </summary>
        public void Close()
        {
            if (null != this.RefreshClock)
                this.RefreshClock.Stop();

            //if (null != this._isServerActive)
                this._isServerActive = false;
        }

        public void Shutdown()
        {
            Close();
            if (null != this.theSocketConnection)
            {
                theSocketConnection.Close();
                theSocketConnection = null;
            }
        }

        // delegate declaration 
        public delegate void UpdateHandler(object sender, SystemSnapshot ea);

        // event declaration 
        public event UpdateHandler DataRefresh;

        private string GetMetricRequestString(SystemMetric aSystemMetric)
        {
            return aSystemMetric.Type + this._nsmServerConfigDef.GetThresholdForMetric(aSystemMetric).ToString().PadRight(8, ' ');
        }

        private string buildServerRequest()
        {
            //string BASE_SERVER_REQUEST = "B100     D300     C50000   P10000   S50      M8192    Q20      ";

            string metricServerRequest =
                GetMetricRequestString(MetricTypes.CPUBusy) +                
                GetMetricRequestString(MetricTypes.FreeMemory);


            bool ConnectionLight = this._nsmServerConfigDef.FetchConnectivity;
            bool TransactionsLight = this._nsmServerConfigDef.FetchTransactions;
            bool DisksLight = this._nsmServerConfigDef.FetchDisk;

            //Manage System Summary Widget Code
            char ConnectionChar = '0';
            char TransactionChar = '0';
            char DiskChar = '0';

            if (ConnectionLight)
                ConnectionChar = '1';

            if (TransactionsLight)
                TransactionChar = '1';

            if (DisksLight)
                DiskChar = '1';

            string SystemSummaryChar = "00000" + ConnectionChar + TransactionChar + DiskChar;
            char myhex = (char)(Convert.ToInt32(SystemSummaryChar, 2));
            return (metricServerRequest + myhex);
        }

        public void BindAndInitialize()
        {
            try
            {
                connectToServer();

                this._isServerActive = true;

                //Start clock
                RefreshClock = new System.Timers.Timer();
                RefreshClock.Elapsed += new ElapsedEventHandler(RefreshClock_Elapsed);
                double refreshTimeInMillSecs = (this._nsmServerConfigDef.RefreshRate * 1000);
                RefreshClock.Interval = refreshTimeInMillSecs; 
				RefreshClock.Start();
            }
            catch (SocketException se)
            {
                //handleSocketErrors(se, false);
            }
        }

        void SendMessageToServer(string iMessage)
        {
            try
            {
                IPAddress remoteIP = Dns.GetHostEntry(this.theConnectionDefinition.Host).AddressList[0];//IPAddress.Parse (textBoxIP.Text);
                int iPortNo = System.Convert.ToInt32(this._nsmServerConfigDef.PortNumber);
                IPEndPoint ipEnd = new IPEndPoint(remoteIP, iPortNo);

                byte[] byData = System.Text.Encoding.ASCII.GetBytes(iMessage);
                theSocketConnection.SendTo(byData, ipEnd);
            }
            catch
            {
            }
        }

        private void connectToServer()
        {
            try
            {

                // Create the socket instance
                theSocketConnection = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);

                IPEndPoint anyEP = null;
                if (!this._nsmServerConfigDef.UseIncPort)
                {
                    this._nsmServerConfigDef.PortNumberINC = 0;
                }

                if (this._nsmServerConfigDef.UseIncPort && this._nsmServerConfigDef.PortNumberINC > 0)
                {
                    anyEP = new IPEndPoint(IPAddress.Any, this._nsmServerConfigDef.PortNumberINC);
                }
                else
                {
                    anyEP = new IPEndPoint(IPAddress.Any, 0);
                }

                // Connect to the remote host
                theSocketConnection.Bind(anyEP);

                //Send initial '}|handshake' message to the server
                string SERVER_REQUEST = this.buildServerRequest();
                SendMessageToServer("I" + SERVER_REQUEST);

                //Wait for the 'handshake response' asynchronously
                WaitForData(new AsyncCallback(OnInitDataReceived));

                //setGraphBackColor(this.initLineGraphBG, this.initBarGraphBG);
            }
            catch (Exception e)
            {
                handleSocketException();                
                //throw new Exception("Server not able to connect");
            }
        }

        public void WaitForData(AsyncCallback _inCallback)
        {
            try
            {
                //validate callback
                SocketPacket theSocPkt = new SocketPacket();
                theSocPkt.thisSocket = theSocketConnection;
                theSocketConnection.ReceiveTimeout = 3000;
                // Start listening to the data asynchronously

                //dataBuffer = new byte[BufferSize];

                //m_result = 
                theSocketConnection.BeginReceive(theSocPkt.dataBuffer, 0, SocketPacket.BufferSize, SocketFlags.None, _inCallback, theSocPkt);
            }
            catch (SocketException se)
            {
                handleSocketException();
                //ThreadPool.QueueUserWorkItem(new WaitCallback(handleSocketErrors), se);
            }
        }

        public class SocketPacket
        {
            public System.Net.Sockets.Socket thisSocket;
            public const int BufferSize = 10240;
            public byte[] dataBuffer = new byte[BufferSize];
        }

        public void OnInitDataReceived(IAsyncResult asyn)
        {
            //this._responseReceived = true;
            try
            {
                SocketPacket theSockPacket = (SocketPacket)asyn.AsyncState;
                int numRead = theSockPacket.thisSocket.EndReceive(asyn);

                String serverData = OnDataReceived(theSockPacket, numRead);
                // OnDataReceived(asyn);

                //Run System Compatibility Check
                if (serverData != "")
                {
                    ParseInitData(serverData);
                }

                this._failedConnections = -1;
                if (null != ConnectionEvent)
                    ConnectionEvent(this, _failedConnections);
            }
            catch (SocketException se)
            {
                handleSocketException();
            }
            catch (Exception e)
            {
            }
        }

        //Pull the raw data from the server
        public string OnDataReceived(SocketPacket theSockId, int iRx)
        {
            try
            {
                //was in finally but...
                //numConnectionFailures = 0;

                int BufferSize = 10240;
                char[] chars = new char[BufferSize];
                System.Text.Decoder d = System.Text.Encoding.GetEncoding(1252).GetDecoder();
                int charLen = d.GetChars(theSockId.dataBuffer, 0, iRx, chars, 0);
                String szData = new System.String(chars);

                this._failedConnections = -1;
                if (null != ConnectionEvent)
                    ConnectionEvent(this, _failedConnections);

                return szData;

            }
            catch (ObjectDisposedException)
            {
                return "";
            }
            catch (SocketException se)
            {
                handleSocketException();
                return "";
            }
        }

        private void handleSocketException()
        {
            if (this._failedConnections < 0)
                this._failedConnections = 0;

            _failedConnections++;

            if (null != ConnectionEvent)
                ConnectionEvent(this, _failedConnections);
        }

        private int _numberOfSystems = 0;
        ///
        /// This Method is for parsing the initial 'handshake' message recieved from the server
        /// It walks through the string and populates the 'this.connInfo' object.
        ///
        private void ParseInitData(string serverData)
        {
            int x = 0;

            if (serverData != "")
            {
                //this.CurrentST = SliceType.INITMETRIC;
                string _connInfo = "Version: " + serverData.Substring(x, 3) + '\n';
                //this._connInfo = "Type: " + this._serverData.Substring(0, 1) + '\n';

                x += 3;
                _connInfo += "Sampling Type: " + (int)serverData.Substring(x, 1).ToCharArray()[0] + '\n';
                x++;
                this._numberOfSystems = (int)serverData.Substring(x, 1).ToCharArray()[0];
                _connInfo += "# of Systems: " + _numberOfSystems + '\n';
                //Loop through each system
                x++;
                for (int i = 0; i < _numberOfSystems; i++)
                {
                    String sysName = serverData.Substring(x, 8);//.ToCharArray()[0];
                    _connInfo += "    System: " + sysName + '\n';
                    x += 8;
                }
                ParseUpdateData(x, serverData);
            }
        }

        //Eventually, return a data-object with all the info
        private void ParseUpdateData(int iMarker, string serverData)
        {
            SystemSnapshot m_currentSnapshot = new SystemSnapshot();
            m_currentSnapshot.SampleTypes = new Hashtable();
            m_currentSnapshot.SystemSummary = null;
            m_currentSnapshot.CPUDownList = new ArrayList();

            this.CPUDownList.Clear();
            int x = iMarker;

            lock (this)
            {
                int xS = 0;
                for (xS = 0; xS < 7; xS++)
                {
                    String SamplingType = serverData.Substring(x, 1);
                    ArrayList SamplingArray = new ArrayList();
                    x++;

                    //bool CpuSubset = true;

                    ArrayList SystemArray = new ArrayList();
                    for (int i = 0; i < _numberOfSystems; i++)
                    {
                        //For each system...

                        //CPUcount varies by system. Maintain a hash to store info on each system's CPU count
                        int CPUcount = (int)serverData.Substring(x, 1).ToCharArray()[0];
                        x++;
                        ArrayList CPUArray = new ArrayList();

                        //for each CPU

                        for (int j = 0; j < CPUcount; j++)
                        {
                            int currentBar = (int)serverData.Substring(x, 1).ToCharArray()[0];
                            x++;


                            if (SamplingType == "B")
                            {
                                if (currentBar == 255)
                                {
                                    this.CPUDownList.Add(i + "," + j);
                                    m_currentSnapshot.CPUDownList.Add(i + "," + j);
                                }
                            }
                            CPUArray.Add(currentBar);
                            /* if currentBar = 255, CPU is DOOWN*/
                        }
                        SystemArray.Add(CPUArray);
                    }

                    try
                    {
                        //Check if the metric for the current snapshot already exists 
                        //(i.e. there has been an asynchronous timing error)
                        if (m_currentSnapshot.SampleTypes.ContainsKey(SamplingType))
                        {
                            m_currentSnapshot.SampleTypes[SamplingType] = SystemArray;
                        }
                        else
                        {
                            m_currentSnapshot.SampleTypes.Add(SamplingType, SystemArray);
                        }
                    }
                    catch
                    {

                    }
                }
                String RemainingData = serverData.Substring(x);
                String SummaryFlag = serverData.Substring(x, 1);
                x++;

                if (SummaryFlag == "S")
                {
                    int ConnectionFlag = int.Parse(serverData.Substring(x, 1));
                    x++;
                    int TransactionFlag = int.Parse(serverData.Substring(x, 1));
                    x++;
                    int DiskFlag = int.Parse(serverData.Substring(x, 1));

                    int SysSummaryCount = (ConnectionFlag + TransactionFlag + DiskFlag);


                    Hashtable SysStateHash = new Hashtable();
                    //int stringPlaceCounter = x;

                    //SummaryFlag - Data
                    //RemainingData = this._serverData.Substring(x + 1);
                    string CurrentToken = "";

                    x++;
                    String SystemType = "";
                    String SystemSummary = "";
                    int SystemState = -1;
                    int numMessages = 0;

                    ArrayList ErrorArray = new ArrayList();

                    //For each system summary state
                    for (int h = 0; h < SysSummaryCount; h++)
                    {
                        Hashtable SystemHash = new Hashtable();
                        SystemType = serverData.Substring(x, 4);
                        x += 4;
                        //string remainingData = serverData.Substring(x);
                        SystemState = Int16.Parse(serverData.Substring(x++, 1));//.ToCharArray()[0];                       

                        //Parse Messages relating to summary state********************************
                        int frontMarker = x;
                        int backMarker = serverData.IndexOf('\t', x + 1);
                        CurrentToken = serverData.Substring(frontMarker + 1, backMarker - frontMarker - 1);
                        numMessages = Int16.Parse(CurrentToken);

                        DateTime MessageTime = new DateTime();
                        String SegmentNumber = "0";

                        try
                        {
                            //Loop and accumulate System Messages
                            for (int p = 0; p < numMessages; p++)
                            {

                                frontMarker = serverData.IndexOf('\t', backMarker);
                                backMarker = serverData.IndexOf('\t', frontMarker + 1);
                                //System Message
                                CurrentToken = serverData.Substring(frontMarker + 1, backMarker - frontMarker - 1);
                                SegmentNumber = CurrentToken;
                                if (SegmentNumber == "0")
                                {
                                    SegmentNumber = "All Segments";
                                }

                                frontMarker = serverData.IndexOf('\t', backMarker);
                                backMarker = serverData.IndexOf('\t', frontMarker + 1);

                                CurrentToken = serverData.Substring(frontMarker + 1, backMarker - frontMarker - 1);
                                MessageTime = DateTime.Parse(CurrentToken.Replace(";", " "));

                                frontMarker = serverData.IndexOf('\t', backMarker);
                                backMarker = serverData.IndexOf('\t', frontMarker + 1);
                                CurrentToken = serverData.Substring(frontMarker + 1, backMarker - frontMarker - 1); //System message

                                //Add message to an array
                                ErrorArray.Add(new ErrorTableData(AspectFromName(SystemType), SegmentNumber, MessageTime, CurrentToken));
                            }

                        }
                        catch (Exception e)
                        {

                        }

                        x = backMarker + 1;

                        SystemHash.Add("state", SystemState);
                        SystemHash.Add("summary", SystemSummary);
                        SysStateHash.Add(SystemType, SystemHash);
                    }

                    SysStateHash.Add("ErrorArray", ErrorArray);
                    m_currentSnapshot.SystemSummary = SysStateHash;
                }
            }
            if (DataRefresh != null)
            {
                DataRefresh(this, m_currentSnapshot);
            }
            else
            {
                this.Close();
            }
        }

        private SystemAspect AspectFromName(string inSystemType)
        {
            switch (inSystemType)
            {
                case "NDCS":
                    return SystemAspect.Connectivity;
                case "TMFS":
                    return SystemAspect.Transactions;
                case "DISK":
                    return SystemAspect.Disk;
                default:
                    return SystemAspect.General;
            }
        }

        // The connection definition that we are using
        private ConnectionDefinition theConnectionDefinition;
        private Socket theSocketConnection;

        /// <summary>
        /// Readonly property for the connection definition that we are using
        /// </summary>
        public ConnectionDefinition TheConnectionDefinition
        {
            get { return theConnectionDefinition; }
        }

        /// <summary>
        /// Our underlying ODBC connection
        /// </summary>
        private OdbcConnection theOdbcConnection = null;

        /// <summary>
        /// Handler for ODBC connection opened
        /// </summary>
        /// <param name="aConnectionDefinition">The associated connection definition</param>
        public delegate void OpenedHandler(ConnectionDefinition aConnectionDefinition);

        /// <summary>
        /// Handler for ODBC connection closed
        /// </summary>
        /// <param name="aConnectionDefinition">The associated connection definition</param>
        public delegate void ClosedHandler(ConnectionDefinition aConnectionDefinition);


        /// <summary>
        /// The list of parites interested in opening and/or closing ODBC conncetions
        /// </summary>
        static private EventHandlerList theEventHandlers = new EventHandlerList();

        /// <summary>
        /// The key for opened event listeners
        /// </summary>
        private static readonly string theOpenedKey = "OdbcConnectionOpened";

        /// <summary>
        /// The key for closed event listeners
        /// </summary>
        private static readonly string theClosedKey = "OdbcConnectionClosed";

        /// <summary>
        /// Add an ODBC connection opened handler
        /// </summary>
        static public event OpenedHandler Opened
        {
            add { theEventHandlers.AddHandler(theOpenedKey, value); }
            remove { theEventHandlers.RemoveHandler(theOpenedKey, value); }
        }

        /// <summary>
        /// Tell everyone interrested that an ODBC connection has been opened
        /// </summary>
        /// <param name="aConnectionDefinition">The associated connection definition</param>
        static private void FireOpened(ConnectionDefinition aConnectionDefinition)
        {
            OpenedHandler theOpenedHandlers = (OpenedHandler)theEventHandlers[theOpenedKey];

            if (theOpenedHandlers != null)
            {
                theOpenedHandlers(aConnectionDefinition);
            }
        }

        /// <summary>
        /// Add an ODBC connection closed handler
        /// </summary>
        static public event ClosedHandler Closed
        {
            add { theEventHandlers.AddHandler(theClosedKey, value); }
            remove { theEventHandlers.RemoveHandler(theClosedKey, value); }
        }

        /// <summary>
        /// Tell everyone interrested that an ODBC connection has been closed
        /// </summary>
        /// <param name="aConnectionDefinition">The associated connection definition</param>
        static private void FireClosed(ConnectionDefinition aConnectionDefinition)
        {
            ClosedHandler theClosedHandlers = (ClosedHandler)theEventHandlers[theClosedKey];

            if (theClosedHandlers != null)
            {
                theClosedHandlers(aConnectionDefinition);
            }
        }

    }

    public struct SystemSnapshot
    {
        public Hashtable SampleTypes;
        public Hashtable SystemSummary;
        public ArrayList CPUDownList;
    }

    public interface IDataStatus
    {
        bool IsValidBeforeReset
        {
            get;
        }
        /// <summary>
        /// NewValue will be the same as old value if the old value is valid
        /// </summary>
        int NewValue
        {
            get;
        }

        int MinValue
        {
            get;
        }
        int MaxValue
        {
            get;
        }
    }

    public class RefreshRateStatus : IDataStatus
    {
        #region IDataStatus Members

        public bool IsValidBeforeReset
        {
            get { return _isValidBeforeReset; }
        }

        public int NewValue
        {
            get { return _newValue; }
        }

        public int MinValue
        {
            get
            {
                return 2;
            }
        }

        public int MaxValue
        {
            get
            {
                return 500;
            }
        }

        #endregion

        private bool _isValidBeforeReset = true;
        private int _newValue = 0;
        public RefreshRateStatus(int aRefreshRate)
        {
            DataStatusHelper.CheckData(aRefreshRate, MinValue, MaxValue, 
                out _isValidBeforeReset, out _newValue);
        }
        public RefreshRateStatus(string aRefreshRate)
        {
            DataStatusHelper.CheckData(aRefreshRate, MinValue, MaxValue,
                out _isValidBeforeReset, out _newValue);
        }
    }

    public class PortNumberStatus : IDataStatus
    {
        #region IDataStatus Members

        public bool IsValidBeforeReset
        {
            get { return _isValidBeforeReset; }
        }

        public int NewValue
        {
            get { return _newValue; }
        }

        public int MinValue
        {
            get
            {
                return 1000;
            }
        }
        public int MaxValue
        {
            get
            {
                return 65535;
            }
        }

        #endregion

        private bool _isValidBeforeReset = true;
        private int _newValue = 0;
        public PortNumberStatus(int aPortNumber)
        {
            DataStatusHelper.CheckData(aPortNumber, MinValue, MaxValue,
                out _isValidBeforeReset, out _newValue);
        }
        public PortNumberStatus(string aPortNumber)
        {
            DataStatusHelper.CheckData(aPortNumber, MinValue, MaxValue,
                out _isValidBeforeReset, out _newValue);
        }
    }

    public class FixedIncomingStatus : IDataStatus
    {
        #region IDataStatus Members

        public bool IsValidBeforeReset
        {
            get { return _isValidBeforeReset; }
        }

        public int NewValue
        {
            get { return _newValue; }
        }

        public int MinValue
        {
            get
            {
                return 0;
            }
        }
        public int MaxValue
        {
            get
            {
                return 65535;
            }
        }

        #endregion

        private bool _isValidBeforeReset = true;
        private int _newValue = 0;
        public FixedIncomingStatus(int aPortNumber)
        {
            DataStatusHelper.CheckData(aPortNumber, MinValue, MaxValue,
                out _isValidBeforeReset, out _newValue);
        }
        public FixedIncomingStatus(string aPortNumber)
        {
            DataStatusHelper.CheckData(aPortNumber, MinValue, MaxValue,
                out _isValidBeforeReset, out _newValue);
        }
    }

    public class CPUBusySettingStatus : IDataStatus
    {
        #region IDataStatus Members

        public bool IsValidBeforeReset
        {
            get { return _isValidBeforeReset; }
        }

        public int NewValue
        {
            get { return _newValue; }
        }

        public int MinValue
        {
            get
            {
                return 1;
            }
        }
        public int MaxValue
        {
            get
            {
                return 100;
            }
        }

        #endregion
        private bool _isValidBeforeReset = true;
        private int _newValue = 0;
        public CPUBusySettingStatus(int aCPUBusySetting)
        {
            DataStatusHelper.CheckData(aCPUBusySetting, MinValue, MaxValue,
                out _isValidBeforeReset, out _newValue);
        }

        public CPUBusySettingStatus(string aCPUBusySetting)
        {
            DataStatusHelper.CheckData(aCPUBusySetting, MinValue, MaxValue,
                out _isValidBeforeReset, out _newValue);
        }
    }

    public class Other100SettingStatus : IDataStatus
    {
        #region IDataStatus Members

        public bool IsValidBeforeReset
        {
            get { return _isValidBeforeReset; }
        }

        public int NewValue
        {
            get { return _newValue; }
        }

        public int MinValue
        {
            get { return 1; }
        }

        public int MaxValue
        {
            get { return 99999999; }
        }

        #endregion

        private bool _isValidBeforeReset = true;
        private int _newValue = 0;
        public Other100SettingStatus(int a100SettingValue)
        {
            DataStatusHelper.CheckData(a100SettingValue, MinValue, MaxValue,
                out _isValidBeforeReset, out _newValue);
        }

        public Other100SettingStatus(string a100SettingValue)
        {
            DataStatusHelper.CheckData(a100SettingValue, MinValue, MaxValue,
                out _isValidBeforeReset, out _newValue);
        } 
    }


    public enum SystemAspect
    {
        Connectivity,
        Disk,
        Transactions,
        General
    };



    public class ErrorTableData
    {
        private SystemAspect m_eErrorType;
        private String m_iSegmentNumber;
        private DateTime m_iTimestamp;
        private string m_sErrorString;

        public ErrorTableData(SystemAspect inType, String inSegmentNumber, DateTime inTimestamp, string inErrorString)
        {
            m_eErrorType = inType;
            m_iSegmentNumber = inSegmentNumber;
            m_iTimestamp = inTimestamp;
            m_sErrorString = inErrorString;

        }

        public ErrorTableData()
        {

        }

        //MessageString -- "Details"
        public string ErrorString
        {
            get { return m_sErrorString; }
            set { m_sErrorString = value; }
        }

        //Timestamp -- "DateTime"
        public DateTime Timestamp
        {
            get { return m_iTimestamp; }
            set { m_iTimestamp = value; }
        }

        //SegmentNumber -- "SegNum"
        public String SegmentNumber
        {
            get { return m_iSegmentNumber; }
            set { m_iSegmentNumber = value; }
        }

        //MessageType -- "Entity"
        public SystemAspect ErrorType
        {
            get { return m_eErrorType; }
            set { m_eErrorType = value; }
        }
    }

    public class DataStatusHelper
    {
        public static void CheckData(int aValue, int aMinValue, int aMaxValue,
            out bool aIsValidBeforeReset,out int aNewValue)
        {
            if (aValue < aMinValue)
            {
                aIsValidBeforeReset = false;
                aNewValue = aMinValue;
            }
            else if (aValue > aMaxValue)
            {
                aIsValidBeforeReset = false;
                aNewValue = aMaxValue;
            }
            else
            {
                aIsValidBeforeReset = true;
                aNewValue = aValue;
            }
        }

        public static void CheckData(string aValue, int aMinValue, int aMaxValue,
            out bool aIsValidBeforeReset, out int aNewValue)
        {
            if (string.IsNullOrEmpty(aValue))
            {
                aIsValidBeforeReset = false;
                aNewValue = aMinValue;
            }
            else
            {
                int number = 0;
                bool test = int.TryParse(aValue, out number);
                if (!test)
                {
                    aIsValidBeforeReset = false;
                    aNewValue = aMinValue;
                    return;
                }
                DataStatusHelper.CheckData(number, aMinValue, aMaxValue,
                    out aIsValidBeforeReset, out aNewValue);
            }
        }
    }
       

}
