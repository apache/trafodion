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
using Trafodion.Manager.Framework;
using System;
using System.Data;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// 
    /// </summary>
    public class TrafodionSequence : TrafodionSchemaObject
    {

        #region Fields
        
        /// <summary>
        /// Object name space
        /// </summary>
        public const string ObjectNameSpace = "TA";
        /// <summary>
        /// Object type
        /// </summary>
        public const string ObjectType = "SG";

        #endregion


        /// <summary>
        /// Creates a new TrafodionView object. The parent of instances of this object are TrafodionSchema objects.
        /// </summary>
        /// <param name="aTrafodionSchema">The parent object of this instance.</param>
        /// <param name="anInternalName">The internal name of the view.</param>
        /// <param name="aUID">The unique identifer of the object.</param>
        /// <param name="aCreateTime">The creation time of the object.</param>
        /// <param name="aRedefTime">The redefinition time of the object.</param>
        /// <param name="aSecurityClass">The security class of the object.</param>
        /// <param name="anOwner">The owner of the object.</param>
        public TrafodionSequence(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
        {
        }

        /// <summary>
        /// The internal object type string for a view.
        /// </summary>
        override public string SchemaObjectType
        { 
            get
            {
                return ObjectType; 
            }
        }

        /// <summary>
        /// Displayable Object type for the view
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return Properties.Resources.View;
            }
        }
        
        /// <summary>
        /// The internal object name space for a view.
        /// </summary>
        override public string SchemaObjectNameSpace
        { 
            get
            {
                return ObjectNameSpace; 
            }
        }


        /// <summary>
        /// The view model resets it state
        /// </summary>
        public override void Refresh()
        {
            //Create a temp view model
            TrafodionSequence aSequence = this.TheTrafodionSchema.LoadSequenceByName(this.InternalName);

            if (aSequence == null)
            {
                //IF temp model is null, the object has been removed
                //So cleanup and notify the UI
                this.TheTrafodionSchema.TrafodionSequences.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(aSequence) != 0)
            {
                //If sql object has been recreated, attach the new sql model to the parent.
                this.TheTrafodionSchema.TrafodionSequences.Remove(this);
                this.TheTrafodionSchema.TrafodionSequences.Add(aSequence);
                //Notify the UI
                this.OnModelReplacedEvent(aSequence);
            }
            else
            {
                base.Refresh();
            }
        }

    }
}
