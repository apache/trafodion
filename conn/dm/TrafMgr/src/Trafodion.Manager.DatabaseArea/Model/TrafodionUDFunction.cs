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

using System.Collections.Generic;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    public class TrafodionUDFunction : TrafodionRoutine
    {
        /// <summary>
        /// Constant that defines the procedure object name space
        /// </summary>
        public const string ObjectNameSpace = "UF";
        /// <summary>
        /// Constant that defines the procedure object type
        /// </summary>
        public const string ObjectType = "UR";
        /// <summary>
        /// holds function action objects that resided under this universal function
        /// </summary>
        private List<TrafodionFunctionAction> _functionActionList = null;

        #region Public Properties

        public List<TrafodionFunctionAction> TrafodionFunctionActions
        {
            get
            {
                LoadTrafodionFunctionActions();

                return _functionActionList;
            }
        }

        public void LoadTrafodionFunctionActions()
        {
            if (_functionActionList == null || _functionActionList.Count == 0)
            {
                _functionActionList = new TrafodionFunctionActionsLoader().Load(this);
            }
        }
        /// <summary>
        /// Object type for the procedure
        /// </summary>
        override public string SchemaObjectType
        {
            get
            {
                return ObjectType;
            }
        }

        /// <summary>
        /// Object name space for the procedure
        /// </summary>
        override public string SchemaObjectNameSpace
        {
            get
            {
                return ObjectNameSpace;
            }
        }

        /// <summary>
        /// Displayable Object type for the procedure
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return Properties.Resources.UserDefinedFunction;
            }
        }

        public string FormattedType
        {
            get
            {
                return (_isUniversal ? Properties.Resources.UniversalFunction: Properties.Resources.ScalarFunction);
            }
        }

        #endregion Public Properties

        public TrafodionUDFunction(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
		{
        }

        /// <summary>
        /// Resets the Function model
        /// </summary>
        override public void Refresh()
        {
            //Create a temp model
            TrafodionUDFunction aFunction = this.TheTrafodionSchema.LoadUDFunctionByName(this.InternalName);

            //IF temp model is null, the object has been removed
            //So cleanup and notify the UI
            if (aFunction == null)
            {
                this.TheTrafodionSchema.TrafodionUDFunctions.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(aFunction) != 0)
            {
                //If sql object has been recreated, attach the new sql model to the parent.
                this.TheTrafodionSchema.TrafodionUDFunctions.Remove(this);
                this.TheTrafodionSchema.TrafodionUDFunctions.Add(aFunction);
                //Notify the UI
                this.OnModelReplacedEvent(aFunction);
            }
            else
            {
                base.Refresh();
            }
        }

        public void AddAction(TrafodionFunctionAction action)
        {
            if (_functionActionList == null)
                _functionActionList = new List<TrafodionFunctionAction>();

            if(_functionActionList.Contains(action))
                return;

            _functionActionList.Add(action);
        }

        internal class TrafodionFunctionActionsLoader : TrafodionObjectsLoader<TrafodionUDFunction, TrafodionFunctionAction>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionUDFunction aTrafodionUDFunction)
            {
                return Queries.ExecuteSelectUDFActionObjectNames(aConnection, aTrafodionUDFunction);
            }

            override protected void LoadOne(List<TrafodionFunctionAction> aList, TrafodionUDFunction aTrafodionUDFunction, OdbcDataReader aReader)
            {
                TrafodionFunctionAction theTrafodionFunctionAction = aTrafodionUDFunction.TheTrafodionSchema.FindFunctionAction(aReader.GetInt64(0), aReader.GetInt64(1), aReader.GetInt64(2));
                if (theTrafodionFunctionAction != null)
                {
                    theTrafodionFunctionAction.TrafodionUDFunction = aTrafodionUDFunction;
                    aList.Add(theTrafodionFunctionAction);

                }
            }
        }
    }
}
