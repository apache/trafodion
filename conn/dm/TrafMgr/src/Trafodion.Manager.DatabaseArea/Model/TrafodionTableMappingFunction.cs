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

namespace Trafodion.Manager.DatabaseArea.Model
{
    public class TrafodionTableMappingFunction : TrafodionRoutine
    {

        /// <summary>
        /// Constant that defines the table mapping function object name space
        /// </summary>
        public const string ObjectNameSpace = "TA";
        /// <summary>
        /// Constant that defines the table mapping function object type
        /// </summary>
        public const string ObjectType = "UR";

        public TrafodionTableMappingFunction(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
		{
        }

        /// <summary>
        /// Resets the Function model
        /// </summary>
        override public void Refresh()
        {
            ////Create a temp model
            //TrafodionUDFunction aFunction = this.TheTrafodionSchema.LoadUDFunctionByName(this.InternalName);

            ////IF temp model is null, the object has been removed
            ////So cleanup and notify the UI
            //if (aFunction == null)
            //{
            //    this.TheTrafodionSchema.TrafodionUDFunctions.Remove(this);
            //    OnModelRemovedEvent();
            //    return;
            //}
            //if (this.CompareTo(aFunction) != 0)
            //{
            //    //If sql object has been recreated, attach the new sql model to the parent.
            //    this.TheTrafodionSchema.TrafodionUDFunctions.Remove(this);
            //    this.TheTrafodionSchema.TrafodionUDFunctions.Add(aFunction);
            //    //Notify the UI
            //    this.OnModelReplacedEvent(aFunction);
            //}
            //else
            //{
            //    base.Refresh();
            //}
        }
    }
}
