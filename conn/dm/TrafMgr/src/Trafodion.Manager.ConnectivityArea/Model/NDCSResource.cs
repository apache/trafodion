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

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    ///The NDCS Resource associated with a Datasource 
    /// </summary>
    public class NDCSResource
    {
        #region member variables
        private string _theAttributeName;
        private long _theLimit;
        private long _theActionID;

        public static string[] ActionNames = new string[] {"", "LOG", "STOP", "", "", "LOG_WITH_INFO" };

        #endregion

        #region Properties
        /// <summary>
        /// The name of the NDCSResource
        /// </summary>
        public string AttributeName
        {
            get { return _theAttributeName; }
            set { _theAttributeName = value; }
        }

        /// <summary>
        /// The Limit of the NDCSResource
        /// </summary>
        public long Limit
        {
            get { return _theLimit; }
            set { _theLimit = value; }
        }

        /// <summary>
        /// The action of the NDCSResource
        /// </summary>
        public long ActionID
        {
            get { return _theActionID; }
            set { _theActionID = value; }
        }

        /// <summary>
        /// Compares the object passed to see if they are the same
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public override bool Equals(object obj)
        {
            NDCSResource resource = obj as NDCSResource;
            if (resource != null)
            {
                if (NDCSObject.AreStringsEqual(this.AttributeName, resource.AttributeName)
                    && (this.Limit == resource.Limit)
                    && (this.ActionID == resource.ActionID))
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Merges the passed NDCSResource object to the current object
        /// </summary>
        /// <param name="aResource"></param>
        public void Copy(NDCSResource aResource)
        {
            if (aResource != null)
            {
                this.AttributeName = aResource.AttributeName;
                this.Limit = aResource.Limit;
                this.ActionID = aResource.ActionID;
            }
        }

        #endregion
    }
}
