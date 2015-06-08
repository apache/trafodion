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


namespace Trafodion.Manager.DatabaseArea.Model
{
    public class TrafodionFunctionAction : TrafodionRoutine
    {
        /// <summary>
        /// Constant that defines the function action object name space
        /// </summary>
        public const string ObjectNameSpace = "AC";
        /// <summary>
        /// Constant that defines the function action object type
        /// </summary>
        public const string ObjectType = "UR";

        private TrafodionUDFunction _sqlMxUDFunction;

        public TrafodionUDFunction TrafodionUDFunction
        {
            get
            {
                return _sqlMxUDFunction;
            }
            set
            {
                _sqlMxUDFunction = value;
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

        public TrafodionFunctionAction(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
        {
        }

        public TrafodionFunctionAction(TrafodionUDFunction aTrafodionUDFunction, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionUDFunction.TheTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
        {
            _sqlMxUDFunction = aTrafodionUDFunction;
        }


        /// <summary>
        /// Resets the Function model
        /// </summary>
        override public void Refresh()
        {
            //Create a temp model
            TrafodionFunctionAction aFunctionAction = TheTrafodionSchema.LoadFunctionActionByName(this.InternalName);

            //IF temp model is null, the object has been removed
            //So cleanup and notify the UI
            if (aFunctionAction == null)
            {
                this.TheTrafodionSchema.TrafodionFunctionActions.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(aFunctionAction) != 0)
            {
                //If sql object has been recreated, attach the new sql model to the parent.
                this.TheTrafodionSchema.TrafodionFunctionActions.Remove(this);
                this.TheTrafodionSchema.TrafodionFunctionActions.Add(aFunctionAction);
                //Notify the UI
                this.OnModelReplacedEvent(aFunctionAction);
            }
            else
            {
                base.Refresh();
            }
        }
    }
}
