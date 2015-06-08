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
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// Any SQL object that has a name and a UID.
	/// </summary>
	abstract public class TrafodionObject : IComparable
	{
        private string _internalName = null;
        private string _externalName = null;
        private long _uid = 0;
        private bool _allowShowDDL = true;
        /// <summary>
        /// Show DDL text of the object
        /// </summary>
        protected string _ddlText = null;

        /// <summary>
        /// Enum of model change events
        /// </summary>
        public enum ChangeEvent 
        { 
            ProcedureCreated = 0, 
            ProcedureDropped = 1,
            PrivilegeGranted = 2,
            PrivilegeRevoked = 3,
            LibraryCreated = 4,
            LibraryDropped = 5,
            LibraryAltered = 6,
            ViewValidated=7,
            SchemaViewValidated=8 //Click Schema Name, and select "views" tab, then do the validation operation
        };       

        public enum PrivilegeAction
        {
            GRANT = 0,
            REVOKE = 1
        };
        /// <summary>
        /// List of event handlers that handle changes to the model
        /// </summary>
        public event EventHandler<TrafodionModelChangeEventArgs> ModelChangedEvent;
        /// <summary>
        /// List of event handlers that handle the model removed event
        /// </summary>
        public event EventHandler ModelRemovedEvent;
        /// <summary>
        /// List of event handlers that handle the model replaced event
        /// </summary>
        public event EventHandler<TrafodionModelEventArgs> ModelReplacedEvent;

        static private string[] _columnNames = { "Name", "Metadata UID" };

        /// <summary>
        /// Default constructor
        /// </summary>
        public TrafodionObject()
        {
        }

        /// <summary>
        /// Constructs the sql object 
        /// </summary>
        /// <param name="anInternalName">Name of the object</param>
        /// <param name="aUID">UID of the object</param>
        public TrafodionObject(String anInternalName, long aUID)
        {
            InternalName = anInternalName;
            UID = aUID;
        }

        /// <summary>
        /// Raise the model changed event
        /// </summary>
        protected virtual void OnModelChangedEvent(TrafodionModelChangeEventArgs changeEvent)
        {
            EventHandler<TrafodionModelChangeEventArgs> handler = ModelChangedEvent;
            if (handler != null)
            {
                handler(this, changeEvent);
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
        /// <param name="newTrafodionObject">The new model that replaces this model</param>
        protected virtual void OnModelReplacedEvent(TrafodionObject newTrafodionObject)
        {
            EventHandler<TrafodionModelEventArgs> handler = ModelReplacedEvent; 

            if (handler != null)
            {
                handler(this, new TrafodionModelEventArgs(newTrafodionObject));
            }
        }

        /// <summary>
        /// Refresh the object model. Derived should override this method as needed
        /// </summary>
        virtual public void Refresh()
        {
            _ddlText = null;
        }

        public static bool Exists<T>(List<T> trafodionObjectList, string objectName) where T : TrafodionObject
        {
            T trafodionObject = trafodionObjectList.Find(delegate(T aTrafodionObject)
            {
                return aTrafodionObject.ExternalName == objectName;
            });
            return trafodionObject != null;
        }

        /// <summary>
        /// Compares this model to another model object
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        public int CompareTo(Object aTrafodionObject)
        {
            long result = this.UID - ((TrafodionObject)aTrafodionObject).UID;
            return (int)result;
        }

        /// <summary>
        /// Gets the connection definition associated with this object
        /// </summary>
        abstract public ConnectionDefinition ConnectionDefinition { get; }
        /// <summary>
        /// Gets the Connection object
        /// </summary>
        /// <returns></returns>
        abstract public Connection GetConnection();

        /// <summary>
        /// The internal name of the sql object
        /// </summary>
		public string InternalName
		{
            get { return _internalName; }
			set
			{
				_internalName = value;
                _externalName = TrafodionName.ExternalInternalFormSame(_internalName) ? _internalName : TrafodionName.ExternalForm(_internalName);
			}
		}

        /// <summary>
        /// The external name of the sql object
        /// </summary>
        public string ExternalName
		{
            get { return _externalName; }
			set
			{
                _externalName = value;
                _internalName = TrafodionName.ExternalInternalFormSame(_externalName) ? _externalName : TrafodionName.InternalForm(_externalName);
			}
		}

        /// <summary>
        /// The UID of the sql object
        /// </summary>
        public long UID
		{
            get { return _uid; }
            set {_uid = value; }
		}

        /// <summary>
        /// The 3 part ansi name of the sql object
        /// </summary>

        virtual public string RealAnsiName
        {
            get { return ExternalName; }
        }

        /// <summary>
        /// Displayable ansi name of the sql object
        /// </summary>
        virtual public string VisibleAnsiName
        {
            get { return ExternalName; }
        }

        /// <summary>
        /// Returns a string array of column names for this sql object
        /// </summary>
        static public string[] ColumnNames
        {
            get { return _columnNames; }
        }

        /// <summary>
        /// The DDL text provided by SHOWDDL
        /// </summary>
        virtual public string DDLText
        {
            get
            {
                if (_ddlText == null || _ddlText.Length == 0)
                    _ddlText = TrafodionObjectDDLLoader.LoadDDL(this);

                return _ddlText;
            }
        }
        /// <summary>
        /// Indicates if DDL information can be displayed for this object
        /// </summary>
        virtual public bool AllowShowDDL
        {
            get { return _allowShowDDL; }
            set { _allowShowDDL = value; }
        }

        /// <summary>
        /// Returns the ansi schema name of the sql object
        /// </summary>
        public string SchemaName
        {
            get
            {
                string[] nameParts = RealAnsiName.Split('.');
                if (nameParts.Length > 1)
                    return nameParts[1];
                return null;
            }
        }

        public static bool SupportsPrivileges(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionSchema ||
                aTrafodionObject is TrafodionTable ||
                aTrafodionObject is TrafodionMaterializedView ||
                aTrafodionObject is TrafodionView ||
                aTrafodionObject is TrafodionProcedure ||
                aTrafodionObject is TrafodionLibrary
                )
            {
                return true;
            }
            return false;
        }
        
        /// <summary>
        /// Argument for a TrafodionModel replace event
        /// </summary>
        public class TrafodionModelEventArgs : EventArgs
        {
            private TrafodionObject _sqlMxObject;

            /// <summary>
            /// Constructs the model replace event
            /// </summary>
            /// <param name="sqlMxObject">Identifies the new model that replaces the old</param>
            public TrafodionModelEventArgs(TrafodionObject sqlMxObject)
            {
                _sqlMxObject = sqlMxObject;
            }
            /// <summary>
            /// The new Sql object model that is replacing the old model
            /// </summary>
            public TrafodionObject TrafodionObject
            {
                get { return _sqlMxObject; }
            }
        }
        /// <summary>
        /// Event argument for a sql model change event
        /// </summary>
        public class TrafodionModelChangeEventArgs : EventArgs
        {
            private TrafodionObject _sqlMxObject;
            private ChangeEvent _eventId;
 
            /// <summary>
            /// Constructs a model change event
            /// </summary>
            /// <param name="eventId"></param>
            /// <param name="sqlMxObject"></param>
            public TrafodionModelChangeEventArgs(ChangeEvent eventId, TrafodionObject sqlMxObject)
            {
                _eventId = eventId;
                _sqlMxObject = sqlMxObject;
            }

            /// <summary>
            /// Identifies the sql object that initiated a change in this model
            /// For example, when a procedure is added to the schema, the procedure model
            /// triggers a procedure created event on the schema model.
            /// </summary>
            public TrafodionObject ChangeInitiator
            {
                get { return _sqlMxObject; }
            }

            /// <summary>
            /// ChangeEvent that identifies the type of model change
            /// </summary>
            public ChangeEvent EventId
            {
                get { return _eventId; }
            }
        }
    }
}
