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
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.WorkloadArea.Model
{
    /// <summary>
    /// Base class for the Wms Model objects
    /// </summary>
    abstract public class WmsObject : IComparable, ICloneable
    {

        #region Member variables
        /// <summary>
        /// Handler to notify the replacement of the model
        /// </summary>
        public event EventHandler<WmsModelEventArgs> WmsModelEvent;

        protected Connection _connection = null;
        protected ConnectionDefinition _connectionDefinition;

        #endregion Member variables

        #region Properties

        /// <summary>
        /// Name of the WmsObject
        /// </summary>
        abstract public string Name { get; set; }

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
        }
        /// <summary>
        /// The SQL command string for altering the Wms object
        /// </summary>
        abstract public string AlterCommandString { get; }

        /// <summary>
        /// The SQL command string for creating the Wms object
        /// </summary>
        abstract public string CreateCommandString { get; }

        abstract public string DDLText { get; }

        #endregion Properties

        /// <summary>
        /// Constructs the Wms model object
        /// </summary>
        public WmsObject(ConnectionDefinition aConnectionDefinition)
        {
            _connectionDefinition = aConnectionDefinition;
        }
        /// <summary>
        /// Clone a shallow copy of this model
        /// </summary>
        /// <returns></returns>
        public object Clone()
        {
            //return NCCUtils.DeepCopy(this);
            return this.MemberwiseClone();
        }

        /// <summary>
        /// Refreshes the model
        /// </summary>
        abstract public void Refresh();

        protected void GetConnection()
        {
            if (_connection == null)
            {
                _connection = new Connection(ConnectionDefinition);
            }
        }        

        /// <summary>
        /// Compares this model to another model object
        /// </summary>
        /// <param name="aWmsObject"></param>
        /// <returns></returns>
        public int CompareTo(Object aWmsObject)
        {
            if (this.GetType() == aWmsObject.GetType())
            {
                return Name.CompareTo(((WmsObject)aWmsObject).Name);
            }
            else
                return -1;
        }


        /// <summary>
        /// Checks if the specified object name is a WMS system object or is a reserved WMS name.
        /// </summary>
        /// <param name="aWmsObject"></param>
        /// <returns></returns>
        public bool isASystemObject(String objectName)
        {
            if ((null != objectName) && objectName.StartsWith("HPS_"))
                return true;

            return false;
        }


        /// <summary>
        /// Identifies if this service is system created
        /// </summary>
        public bool isASystemService(String serviceName)
        {
            if (null == serviceName)
                return false;

            if (serviceName.Equals("TRAFODION_DEFAULT_SERVICE") || serviceName.Equals("HPS_TRANSPORTER") ||
                serviceName.Equals("HPS_MANAGEABILITY"))
                return true;

            return false;
        }


        /// <summary>
        /// Raise the model replaced event
        /// </summary>
        /// <param name="newWmsObject">The new model that replaces this model</param>
        protected virtual void OnWmsModelEvent(WmsCommand.WMS_ACTION wmsAction, WmsObject wmsObject)
        {
            EventHandler<WmsModelEventArgs> handler = WmsModelEvent;

            if (handler != null)
            {
                handler(this, new WmsModelEventArgs(wmsAction, wmsObject));
            }
        }

        /// <summary>
        /// Event argument for the ModelReplacedEvent
        /// </summary>
        public class WmsModelEventArgs : EventArgs
        {
            private WmsCommand.WMS_ACTION _wmsAction;
            private WmsObject _wmsObject;

            /// <summary>
            /// Constructs the event argument for ModelReplaced event
            /// </summary>
            /// <param name="wmsObject"></param>
            public WmsModelEventArgs(WmsCommand.WMS_ACTION wmsAction, WmsObject wmsObject)
            {
                _wmsAction = wmsAction;
                _wmsObject = wmsObject;
            }
            /// <summary>
            /// The action id that uniquely identifies this wms event
            /// </summary>
            public WmsCommand.WMS_ACTION WmsAction
            {
                get { return _wmsAction; }
            }
            /// <summary>
            /// The WmsObject that replaces the old model
            /// </summary>
            public WmsObject WmsObject
            {
                get { return _wmsObject; }
            }
        }
    }
}
