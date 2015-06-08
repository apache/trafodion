// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    public class ReceivedPackage
    {
        private object _data = null;
        private string _publication = null;
        private DateTime _timestamp;
        private string _theQueueName = null;

        public object Data
        {
            get { return _data; }
            set { _data = value; }
        }

        public string Publication
        {
            get { return _publication; }
            set { _publication = value; }
        }

        public DateTime Timestamp
        {
            get { return _timestamp; }
            set { _timestamp = value; }
        }

        public string QueueName
        {
            get { return _theQueueName; }
            set { _theQueueName = value; }
        }

        public ReceivedPackage(string aQueueName, string publication, object data, DateTime timestamp)
        {
            QueueName = aQueueName;
            Data = data;
            Publication = publication;
            Timestamp = timestamp;
        }

        public ReceivedPackage(string publication, object data, DateTime timestamp)
        {
            QueueName = "default";
            Publication = publication;
            Data = data;
            Timestamp = timestamp;
        }
    }
}
