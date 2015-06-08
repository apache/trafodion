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
using System.Reflection;

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// Class that is used for configuring attributes for event handlers
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = true)]
    public class EventHandlerForAttribute : Attribute
    {
        public EventHandlerForAttribute(string className, string eventName) 
        { 
            this._ClassName = className; 
            this._EventName = eventName;
        }
        private string _ClassName;           //The class name whose event I can handle
        private string _EventName;           //The event that I can handle
        private MethodInfo _Method;          //The method that this attribute is for

        /// <summary>
        /// The class name whose event I can handle
        /// </summary>
        public string ClassName
        {
            get { return _ClassName; }
            set { _ClassName = value; }
        }

        /// <summary>
        /// The event that I can handle
        /// </summary>
        public string EventName
        {
            get { return _EventName; }
            set { _EventName = value; }
        }

        /// <summary>
        /// The method that this attribute is for
        /// </summary>
        public MethodInfo Method
        {
            get { return _Method; }
            set { _Method = value; }
        }

        public override String ToString()
        {
            return _ClassName + " : " + _EventName;
        }
    }

}
