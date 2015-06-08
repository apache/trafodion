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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;


namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// Base class for the NDCS Model objects
    /// </summary>
    abstract public class NDCSObject : IComparable
    {
        #region Member variables


        public enum NDCS_MODEL_ACTION
        {
            MODEL_REPLACED = 1, ADD_DATASOURCE = 2, ALTER_DATASOURCE = 3, DELETE_DATASOURCE = 4
        };

        /// <summary>
        /// ENUM StopMode
        /// </summary>
        public enum StopMode { STOP_IMMEDIATE = 0, STOP_DISCONNECT = 1 };

        /// <summary>
        /// Handle to notify changes in the model
        /// </summary>
        public event EventHandler<NDCSModelEventArgs> ModelChangedEvent;
        /// <summary>
        /// Handler to notify removal of the model
        /// </summary>
        public event EventHandler ModelRemovedEvent;
        /// <summary>
        /// Handler to notify the replacement of the model
        /// </summary>
        public event EventHandler<NDCSModelEventArgs> ModelReplacedEvent;

        #endregion Member variables

        #region Properties

        /// <summary>
        /// Name of the NDCSObject
        /// </summary>
        abstract public string Name { get; set; }
        /// <summary>
        /// The Connection definition associated with the NDCSObject instance
        /// </summary>
        abstract public ConnectionDefinition ConnectionDefinition { get; }

        abstract public string DDLText { get; }

        #endregion Properties

        #region Constructors
        /// <summary>
        /// Constructs the NDCS model object
        /// </summary>
        public NDCSObject()
        {

        }
        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Refreshes the model
        /// </summary>
        virtual public void Refresh()
        {
        }

        /// <summary>
        /// Compares this model to another model object
        /// </summary>
        /// <param name="aNDCSObject"></param>
        /// <returns></returns>
        public int CompareTo(Object aNDCSObject)
        {
            if(this.GetType() == aNDCSObject.GetType())
            {
                return Name.CompareTo(((NDCSObject)aNDCSObject).Name);
            }
            else
                return -1;
        }


        /// <summary>
        /// Helper method to check if two objects are equal
        /// </summary>
        /// <param name="aObject"></param>
        /// <param name="anotherObject"></param>
        /// <returns></returns>
        public static bool AreObjectsEqual(object aObject, Object anotherObject)
        {
            //both are null hence equal
            if ((aObject == null) && (anotherObject == null))
            {
                return true;
            }
            //only one of them is null hence not equal
            if ((aObject == null) || (anotherObject == null))
            {
                return false;
            }
            //Finally since none of them are null, compare them
            return (aObject.Equals(anotherObject));
        }

        /// <summary>
        /// Helper method to check if two objects are equal
        /// </summary>
        /// <param name="aObject"></param>
        /// <param name="anotherObject"></param>
        /// <returns></returns>
        public static bool AreStringsEqual(string aString, string anotherString)
        {
            //both are null hence equal
            if ((aString == null) && (anotherString == null))
            {
                return true;
            }
            //only one of them is null hence not equal
            if ((aString == null) || (anotherString == null))
            {
                return false;
            }
            //Finally since none of them are null, compare them
            return (aString.Equals(anotherString, StringComparison.InvariantCultureIgnoreCase));
        }

        #endregion Public methods

        #region protected methods
        /// <summary>
        /// Raise the model changed event
        /// </summary>
        protected virtual void OnModelChangedEvent(NDCS_MODEL_ACTION action, NDCSObject aNDCSObject)
        {
            EventHandler<NDCSModelEventArgs> handler = ModelChangedEvent;
            if (handler != null)
            {
                handler(this, new NDCSModelEventArgs(action, aNDCSObject));
            }
        }

        /// <summary>
        /// Raise the model removed event
        /// </summary>
        protected virtual void OnModelRemovedEvent()
        {
            EventHandler handler = ModelRemovedEvent;
            if (handler != null)
            {
                handler(this, new EventArgs());
            }
        }

        /// <summary>
        /// Raise the model replaced event
        /// </summary>
        /// <param name="newNDCSObject">The new model that replaces this model</param>
        protected virtual void OnModelReplacedEvent(NDCSObject newNDCSObject)
        {
            EventHandler<NDCSModelEventArgs> handler = ModelReplacedEvent;

            if (handler != null)
            {
                handler(this, new NDCSModelEventArgs(NDCS_MODEL_ACTION.MODEL_REPLACED, newNDCSObject));
            }
        }
        #endregion protected methods
        
        /// <summary>
        /// Event argument for the ModelReplacedEvent
        /// </summary>
        public class NDCSModelEventArgs : EventArgs
        {
            private NDCS_MODEL_ACTION _ndcsModelAction;
            private NDCSObject _theNDCSObject;

            /// <summary>
            /// Constructs the event argument for ModelReplaced event
            /// </summary>
            /// <param name="aNDCSObject"></param>
            public NDCSModelEventArgs(NDCS_MODEL_ACTION ndcsModelAction, NDCSObject aNDCSObject)
            {
                _ndcsModelAction = ndcsModelAction; 
                _theNDCSObject = aNDCSObject;
            }
            /// <summary>
            /// The action id that uniquely identifies this ndcs event
            /// </summary>
            public NDCS_MODEL_ACTION Action
            {
                get { return _ndcsModelAction; }
            }
            /// <summary>
            /// The NDCSObject that replaces the old model
            /// </summary>
            public NDCSObject NDCSObject
            {
                get { return _theNDCSObject; }
            }
        }
    }
}
