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


namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// 
	/// </summary>
    public class TrafodionTriggerUsage : TrafodionSchemaObject
		
    {
        #region Member Variables

        public string _isSubjectTable;
        public string _operation;
        private TrafodionSchemaObject theTrafodionObject;

        #endregion

        public TrafodionSchemaObject TheTrafodionObject
        {
            get { return theTrafodionObject; }
        }

        override public string SchemaObjectType
        {
            get
            {
                return theTrafodionObject.SchemaObjectType;
            }
        }

        /// <summary>
        /// Displayable Object type for the trigger
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return theTrafodionObject.DisplayObjectType;
            }
        }
        override public string SchemaObjectNameSpace
        {
            get
            {
                return theTrafodionObject.SchemaObjectNameSpace;
            }
        }


        public TrafodionTriggerUsage(TrafodionSchema aTrafodionSchema, TrafodionSchemaObject aTrafodionObject, string operation, string isSubjectTable)
            : base(aTrafodionSchema, aTrafodionObject.InternalName, aTrafodionObject.UID, aTrafodionObject.TheCreateTime, aTrafodionObject.TheRedefTime, aTrafodionObject.TheSecurityClass, aTrafodionObject.Owner) 
		{
            theTrafodionObject = aTrafodionObject;
            _operation = operation;
            _isSubjectTable = isSubjectTable;
		}
                     
	}

}


