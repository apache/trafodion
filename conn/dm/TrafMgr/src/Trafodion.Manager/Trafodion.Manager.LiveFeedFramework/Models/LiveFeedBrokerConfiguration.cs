//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Net;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// This class represents a Live Feed Broker.
    /// </summary>
    [Serializable]
    public class LiveFeedBrokerConfiguration
    {
        #region Fields

        private ConnectionDefinition _theConnectionDefinition = null;

        //Connection options
        public const string LocalHost = "127.0.0.1";
        public const int DefaultPortNumber = -1;//5672
        public const int DefaultSessionTimeout = 500;        // 50 seconds
        public const int DefaultMaxRetries = 3;
        public const int DefaultSessionMonitorTimeout = 30; // 30 seconds

        private int _port = DefaultPortNumber;
        private string _host = LocalHost;
        private int _maxRetries = DefaultMaxRetries;
        private int _sessionTimeout = DefaultSessionTimeout;
        private int _sessionRetryTimer = DefaultSessionMonitorTimeout;

        public delegate void LiveFeedBrokerChanged(object sender, LiveFeedBrokerChangedEventArgs eArgs);
        public event LiveFeedBrokerChanged OnBrokerChanged;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: ConnectionDefn - the connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        /// <summary>
        /// Property: Port Number
        /// </summary>
        public int Port
        {
            get 
            {
                if (string.IsNullOrEmpty(_theConnectionDefinition.LiveFeedPort))
                    return DefaultPortNumber;
                else
                {
                    try
                    {
                        _port = int.Parse(_theConnectionDefinition.LiveFeedPort);
                    }
                    catch (Exception)
                    {
                        return DefaultPortNumber;
                    }
                }
                return _port; 
            }
            set 
            { 
                _port = value;
                FireOnBrokerChanged(new LiveFeedBrokerChangedEventArgs(LiveFeedBrokerChangedEventArgs.Reason.PortNumber));
            }
        }

        /// <summary>
        /// Property: Port Number in text
        /// </summary>
        public string PortNumber
        {
            get { return _port.ToString(); }
            set
            {
                try
                {
                    if (value.Equals("Default Port Number"))
                        Port = DefaultPortNumber;
                    else                        
                        Port = int.Parse(value);
                }
                catch (Exception)
                {
                    // if error, just use the original port number
                }
            }
        }

        /// <summary>
        /// Property: Host address
        /// </summary>
        public string Host
        {
            get 
            {
                return _theConnectionDefinition.LiveFeedHostName; 
                //return _host; 
            }
            set { _host = value; }
        }

        /// <summary>
        /// Property: Maximum retries
        /// </summary>
        public int MaxRetries
        {
            get { return _maxRetries; }
            set { _maxRetries = value; }
        }

        /// <summary>
        /// Property: SocketAddr
        /// The broker socket address.  Mainly used for display. 
        /// </summary>
        public string SocketAddr
        {
            get
            {
                IPHostEntry iphe = Dns.Resolve(Host);
                return (string.Format("{0}:{1}", iphe.AddressList[0].ToString(), Port));
            }
        }

        /// <summary>
        /// Property: Session timeout value in seconds
        /// </summary>
        public int SessionTimeout
        {
            get { return _sessionTimeout; }
            set { _sessionTimeout = value; }
        }

        /// <summary>
        /// Property: Timer monitoring session timeouts
        /// </summary>
        public int SessionRetryTimer
        {
            get { return _sessionRetryTimer; }
            set 
            { 
                _sessionRetryTimer = value;
                FireOnBrokerChanged(new LiveFeedBrokerChangedEventArgs(LiveFeedBrokerChangedEventArgs.Reason.SessionRetryTimer));
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public string SessionRetryTimerNumber
        {
            get 
            {
                if (string.IsNullOrEmpty(_theConnectionDefinition.LiveFeedRetryTimer))
                    return DefaultSessionMonitorTimeout.ToString();
                else
                {
                    try
                    {
                        _sessionRetryTimer = int.Parse(_theConnectionDefinition.LiveFeedRetryTimer);
                    }
                    catch (Exception)
                    {
                        return DefaultSessionMonitorTimeout.ToString();
                    }
                }
               
                return _sessionRetryTimer.ToString(); 
            }
            set
            {
                try
                {
                    SessionRetryTimer = int.Parse(value);
                }
                catch (Exception)
                {
                    // if error, just use the original port number
                }
            }
        }

        #endregion Properties


        #region Constructors

        /// <summary>
        /// Default constructor: Creates a new uninitialized connection definition
        /// </summary>
        public LiveFeedBrokerConfiguration()
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aHost"></param>
        public LiveFeedBrokerConfiguration(ConnectionDefinition aConnectionDefinition)
        {
            ConnectionDefn = aConnectionDefinition;
            Host = aConnectionDefinition.LiveFeedHostName;
            PortNumber = aConnectionDefinition.LiveFeedPort;
            SessionRetryTimerNumber = aConnectionDefinition.LiveFeedRetryTimer;
        }

        #endregion Constructors

        #region Private methods

        /// <summary>
        /// Fire the state change event
        /// </summary>
        /// <param name="e"></param>
        public void FireOnBrokerChanged(LiveFeedBrokerChangedEventArgs e)
        {
            if (OnBrokerChanged != null)
            {
                OnBrokerChanged(this, e);
            }
        }

        #endregion Private methods
    }

    /// <summary>
    /// Class for connection state change event
    /// </summary>
    public class LiveFeedBrokerChangedEventArgs : EventArgs
    {
        public enum Reason
        {
            HostName = 0, 
            PortNumber,
            SessionRetryTimer
        }

        private Reason _theReason;

        /// <summary>
        /// Property: Reason for the changed
        /// </summary>
        public Reason ChangedReason
        {
            get { return _theReason; }
            set { _theReason = value; }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="originalState"></param>
        /// <param name="currentState"></param>
        public LiveFeedBrokerChangedEventArgs(Reason aReason)
        {
            _theReason = aReason;
        }
    }
}
