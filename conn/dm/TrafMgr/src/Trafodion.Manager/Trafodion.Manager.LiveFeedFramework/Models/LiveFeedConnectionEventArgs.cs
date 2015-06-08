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


namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// Class for connection state change event
    /// </summary>
    public class LiveFeedConnectionEventArgs : EventArgs
    {
        private LiveFeedConnection.LiveFeedConnectionState _originalState;
        private LiveFeedConnection.LiveFeedConnectionState _currentState;
        private Exception _exception;
        private string _reason;

        /// <summary>
        /// Property: Reason for the state change
        /// </summary>
        public string Reason
        {
            get { return _reason; }
            set { _reason = value; }
        }

        /// <summary>
        /// Property: Any exception occured caused the state change
        /// </summary>
        public Exception Exception
        {
            get { return _exception; }
            set { _exception = value; }
        }

        /// <summary>
        /// Property: The current state 
        /// </summary>
        public LiveFeedConnection.LiveFeedConnectionState CurrentState
        {
            get { return _currentState; }
            set { _currentState = value; }
        }

        /// <summary>
        /// Property: The original (previous) state
        /// </summary>
        public LiveFeedConnection.LiveFeedConnectionState OriginalState
        {
            get { return _originalState; }
            set { _originalState = value; }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="originalState"></param>
        /// <param name="currentState"></param>
        public LiveFeedConnectionEventArgs(LiveFeedConnection.LiveFeedConnectionState originalState, LiveFeedConnection.LiveFeedConnectionState currentState)
        {
            _originalState = originalState;
            _currentState = currentState;
            _exception = null;
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="originalState"></param>
        /// <param name="currentState"></param>
        /// <param name="e"></param>
        public LiveFeedConnectionEventArgs(LiveFeedConnection.LiveFeedConnectionState originalState, LiveFeedConnection.LiveFeedConnectionState currentState, Exception e)
            : base()
        {
            _originalState = originalState;
            _currentState = currentState;
            _exception = e;
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="originalState"></param>
        /// <param name="currentState"></param>
        /// <param name="reason"></param>
        public LiveFeedConnectionEventArgs(LiveFeedConnection.LiveFeedConnectionState originalState, LiveFeedConnection.LiveFeedConnectionState currentState, String reason)
            : base()
        {
            _originalState = originalState;
            _currentState = currentState;
            _reason = reason;
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="originalState"></param>
        /// <param name="currentState"></param>
        /// <param name="error"></param>
        /// <param name="reason"></param>
        /// <param name="e"></param>
        public LiveFeedConnectionEventArgs(LiveFeedConnection.LiveFeedConnectionState originalState, LiveFeedConnection.LiveFeedConnectionState currentState, string reason, Exception e)
            : base()
        {
            _originalState = originalState;
            _currentState = currentState;
            _exception = e;
            _reason = reason;
        }
    }
}
