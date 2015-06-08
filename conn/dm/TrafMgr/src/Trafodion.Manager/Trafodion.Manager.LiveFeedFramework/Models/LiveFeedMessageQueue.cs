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
using System.Collections.Generic;
using System.Windows.Forms;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// Class to represent a LiveFeed Queue registered to the LiveFeed broker. Every subscription to the broker
    /// requires to have a queue registered to the broker. Therefore, this class holds all of the subscriptions
    /// using this queue. The queue name has to be unique to the broker. 
    /// [NOTE] For M6 implementation, a LiveFeed connection has only one LiveFeed Queue. 
    /// </summary>
    public class LiveFeedMessageQueue
    {
        #region Fields

        private LiveFeedConnection _theLiveFeedConnection = null;
        private string _name = null;
        private UInt32 _count = 0;

        // The list of subscriptions via this queue; a queue could subscribe to many different publications
        private List<LiveFeedSubscription> _theSubscriptions = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: The queue name
        /// </summary>
        public string Name
        {
            get { return _name; }
        }

        /// <summary>
        /// Property: Number of subscriptions on this queue
        /// </summary>
        public UInt32 Count
        {
            get { return _count; }
            set { _count = value; }
        }


        /// <summary>
        /// Property: Subscriptions - all of the subscriptions for this queue
        /// </summary>
        public List<LiveFeedSubscription> Subscriptions
        {
            get { return _theSubscriptions; }
        }

        /// <summary>
        /// Property: LiveFeedConnection - the LiveFeedConnection, which this queue is belong to. 
        /// </summary>
        public LiveFeedConnection LiveFeedConnection
        {
            get { return _theLiveFeedConnection; }
        }

        #endregion Properties

        #region Constructor 

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="anLiveFeedConnection"></param>
        /// <param name="aPublication"></param>
        public LiveFeedMessageQueue(LiveFeedConnection anLiveFeedConnection)
        {
            _theLiveFeedConnection = anLiveFeedConnection;
            _theSubscriptions = new List<LiveFeedSubscription>();
            _name = GetUniqueueName(_theLiveFeedConnection);
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// The static method used to generate a unique queue name. 
        /// </summary>
        /// <param name="aLiveFeedConnection"></param>
        /// <returns></returns>
        public static string GetUniqueueName(LiveFeedConnection aLiveFeedConnection)
        {
            return string.Format("{0}:{1}:{2}:{3}", "TrafodionManager", aLiveFeedConnection.LiveFeedCustomRegistrationID, aLiveFeedConnection.ConnectionDefn.Name, DateTime.Now.Ticks);
        }

        /// <summary>
        /// To subscribe a publcation to the broker via this queue
        /// </summary>
        /// <param name="aPublication"></param>
        /// <returns></returns>
        public LiveFeedSubscription Subscribe(string aPublication)
        {
            LiveFeedSubscription subscription = null;

            if (null == _theSubscriptions)
            {
                // This is the first subscription for the connection.  
                _theSubscriptions = new List<LiveFeedSubscription>();
            }

            subscription = new LiveFeedSubscription(_theLiveFeedConnection, aPublication);
            _theSubscriptions.Add(subscription);

            return subscription;
        }

        /// <summary>
        /// To unsubscribe a publication from the borker. 
        /// </summary>
        /// <param name="aSubscription"></param>
        public void UnSubscribe(LiveFeedSubscription aSubscription)
        {
            _theSubscriptions.Remove(aSubscription);
        }

        /// <summary>
        /// to unsubscribe a publication 
        /// </summary>
        /// <param name="aPublication"></param>
        public void UnSubscribe(string aPublication)
        {
            foreach (LiveFeedSubscription sub in _theSubscriptions)
            {
                if ((sub == null) || (sub.PublicationName == aPublication))
                {
                    _theSubscriptions.Remove(sub);
                    return;
                }
            }
        }

        #endregion Public methods
    }
}
