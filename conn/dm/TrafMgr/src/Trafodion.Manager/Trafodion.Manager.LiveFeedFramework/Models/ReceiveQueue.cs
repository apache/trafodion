#region Copyright info
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
#endregion Copyright info

using System;
using System.Collections;
using System.ComponentModel;
using System.Threading;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// The Receive Queue used everwhere in the Live Feed Connections. This usually is used for 
    /// storing the incoming messages for later consumption. This is a circular queue. If the 
    /// max length has been reached, the new incoming element will cause the oldest element to be
    /// dropped. 
    /// </summary>
    public class ReceiveQueue
    {
        #region Fields

        private Queue _syncInputQ = Queue.Synchronized(new Queue());
        private ManualResetEvent _waitForOne = new ManualResetEvent(false);
        private string _queueName = "";

        // define the max length allowed for this queue; -1 means no limit
        private int _maxLength = -1;

        [NonSerialized()]
        private EventHandlerList theEventHandlers = new EventHandlerList();

        [NonSerialized()]
        private static readonly string theEnqueuedKey = "Enqueued";

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: Length
        /// The queue length.
        /// </summary>
        public int Length
        {
            get
            {
                lock (_syncInputQ.SyncRoot)
                {
                    return _syncInputQ.Count;
                }
            }
        }

        /// <summary>
        /// Property: Name
        /// The queue name.
        /// </summary>
        public string Name
        {
            get { return _queueName; }
        }

        /// <summary>
        /// Property: MaxLength - the max length allowed in this queue. Since this is a circular queue, if the overflow occurs, 
        /// the first element will be dropped and the new one will be enqueued. 
        /// </summary>
        public int MaxLength
        {
            get { return _maxLength; }
            set { _maxLength = value; }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// Default Constructor
        /// </summary>
        public ReceiveQueue(string aName)
        {
            _queueName = aName;
        }

        public ReceiveQueue(string aName, int aMaxLength)
        {
            _queueName = aName;
            _maxLength = aMaxLength;
        }

        #endregion Constructor

        #region Public Methods

        /// <summary>
        /// To add an element to the queue. 
        /// </summary>
        /// <param name="aData"></param>
        public void Enqueue(Object aData)
        {
            lock (_syncInputQ.SyncRoot)
            {
                if (_maxLength > 0 && _syncInputQ.Count >= _maxLength)
                {
                    // remove the first one before taking the new one
                    _syncInputQ.Dequeue();
                }
                _syncInputQ.Enqueue(aData);
                Set();
            }
        }

        /// <summary>
        /// To take out an element in the queue.
        /// </summary>
        /// <returns></returns>
        public Object Dequeue()
        {
            Object obj = null;
            lock (_syncInputQ.SyncRoot)
            {
                if (_syncInputQ.Count > 0)
                {
                    obj = _syncInputQ.Dequeue();
                }
                Reset();
            }
            return obj;
        }

        /// <summary>
        /// To clear out the queue.
        /// </summary>
        public void Clear()
        {
            lock (_syncInputQ.SyncRoot)
            {
                _syncInputQ.Clear();
            }
        }

        /// <summary>
        /// Set the event, which signifies an element jsut enqueued. 
        /// </summary>
        public void Set()
        {
            _waitForOne.Set();
        }

        /// <summary>
        /// Reset the event. 
        /// </summary>
        public void Reset()
        {
            _waitForOne.Reset();
        }

        /// <summary>
        /// Wait until an element is enqueued.
        /// </summary>
        /// <returns></returns>
        public Object WaitForDequeue()
        {
            _waitForOne.WaitOne();
            return Dequeue();
        }

        /// <summary>
        /// The delegate event handler for Enqueue.
        /// </summary>
        /// <param name="aSender"></param>
        public delegate void EnqueueHandler(object aSender);

        /// <summary>
        /// Add an event handler to, or remove one from, the list
        /// </summary>
        public event EnqueueHandler Enqueued
        {
            add { theEventHandlers.AddHandler(theEnqueuedKey, value); }
            remove { theEventHandlers.RemoveHandler(theEnqueuedKey, value); }
        }

        /// <summary>
        /// Fire an event with this connection definition as the source
        /// </summary>
        public void FireEnqueued()
        {
            // Get the list of the right kind of handlers
            EnqueueHandler theEnqueuedHandlers = (EnqueueHandler)theEventHandlers[theEnqueuedKey];

            // Check to see if there any
            if (theEnqueuedHandlers != null)
            {
                // Multicast to them all
                theEnqueuedHandlers(this);
            }
        }

        #endregion Public Methods
    }
}
