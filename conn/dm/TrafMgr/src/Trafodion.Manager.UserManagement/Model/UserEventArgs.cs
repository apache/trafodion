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
using System.Linq;
using System.Text;

namespace Trafodion.Manager.UserManagement.Model
{
    public class UserEventArgs : EventArgs
    {
        public enum EventType { EventStart, EventProgress, EventEnd };

        string _Operation;
        string _Status;
        string _Message;
        EventType _Type;
        int _eventCount;


        public UserEventArgs(string anOperation, string aStatus, string aMessage)
        {
            this._Operation = anOperation;
            this._Status = aStatus;
            this._Message = aMessage;
        }


        public string Operation
        {
            get { return _Operation; }
            set { _Operation = value; }
        }

        public string Status
        {
            get { return _Status; }
            set { _Status = value; }
        }

        public string Message
        {
            get { return _Message; }
            set { _Message = value; }
        }

        public EventType Type
        {
            get { return _Type; }
            set { _Type = value; }
        }

        public int EventCount
        {
            get { return _eventCount; }
            set { _eventCount = value; }
        }
    }
}
