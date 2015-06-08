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
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// This class describes the basic information for a privilege such as who
    /// has the privileges as well as who granted them. Derivities from this
    /// class will add privileges import to their area.
    /// </summary>
    public class Privilege : IComparable<Privilege>
    {
        /// <summary>
        /// The various user types of the grantor and grantee.
        /// </summary>
        public enum UserType
        {
            /// <summary>
            /// Owner of the object.
            /// </summary>
            Owner,

            /// <summary>
            /// The PUBLIC user.
            /// </summary>
            Public,

            /// <summary>
            /// The SYSTEM user.
            /// </summary>
            System,

            /// <summary>
            /// The Role type.
            /// </summary>
            Role,

            /// <summary>
            /// A user that is none of the above.
            /// </summary>
            User
        };

        #region Constants

        // The string-ified user types.
        private const string OWNER_USERTYPE = "O";
        private const string PUBLIC_USERTYPE = "P";
        private const string SYSTEM_USERTYPE = "S";
        private const string ROLE_USERTYPE = "R";
        private const string USER_USERTYPE   = "U";

        #endregion

        #region Fields

        private bool _grantable = false;
        private int _grantor;
        private string _grantorName;
        private UserType _grantorType = UserType.User;
        private int _grantee;
        private string _granteeName;
        private UserType _granteeType = UserType.User;

        #endregion

        #region Properties

        /// <summary>
        /// Indicates if the grantee can grantable privileges to other users.
        /// </summary>
        public bool Grantable
        {
            get { return _grantable; }
            protected set { _grantable = value; }
        }

        /// <summary>
        /// The user id of the grantor.
        /// </summary>
        public int Grantor
        {
            get { return _grantor; }
            protected set { _grantor = value; }
        }

        /// <summary>
        /// The user name of the grantor
        /// </summary>
        public string GrantorName
        {
            get { return _grantorName; }
            protected set { _grantorName = value; }
        }

        /// <summary>
        /// The grantor's user type.
        /// </summary>
        public UserType GrantorType
        {
            get { return _grantorType; }
            protected set { _grantorType = value; }
        }

        /// <summary>
        /// The user id of the grantee-- the user with the privileges.
        /// </summary>
        public int Grantee
        {
            get { return _grantee; }
            protected set { _grantee = value; }
        }

        /// <summary>
        /// The user name of the grantee
        /// </summary>
        public string GranteeName
        {
            get { return _granteeName; }
            protected set { _granteeName = value; }
        }

        /// <summary>
        /// The grantee's user type.
        /// </summary>
        public UserType GranteeType
        {
            get { return _granteeType; }
            protected set { _granteeType = value; }
        }

        #endregion

        /// <summary>
        /// Creates a new privilege object.
        /// </summary>
        /// <param name="grantable">
        /// Indicates that the user has the ability to grantable privileges to other users.
        /// </param>
        /// <param name="grantor">The user id of the user who granted these privileges.</param>
        /// <param name="grantorType">The grantor's user type.</param>
        /// <param name="grantee">The user id of the user with the permissions.</param>
        /// <param name="granteeType">The grantee's user type.</param>
        public Privilege(bool grantable, int grantor, string grantorName, string grantorType, int grantee, string granteeName, string granteeType)
        {
            _grantable = grantable;
            _grantor = grantor;
            _grantorName = grantorName;
            _grantorType = TranslateType(grantorType);
            _grantee = grantee;
            _granteeName = granteeName;
            _granteeType = TranslateType(granteeType);
        }

        /// <summary>
        /// Creates a new privilage from another privilage object.
        /// </summary>
        /// <param name="priv">The privilage from which to copy.</param>
        public Privilege(Privilege priv)
        {
            _grantable = priv.Grantable;
            _grantor = priv.Grantor;
            _grantorName = priv.GrantorName;
            _grantorType = priv.GrantorType;
            _grantee = priv.Grantee;
            _granteeName = priv.GranteeName;
            _granteeType = priv.GranteeType;
        }

        /// <summary>
        /// Compares this Privilege to another instance.
        /// </summary>
        /// <param name="priv">The privilege to compare with this instance.</param>
        /// <returns>
        /// Less than zero means this object is less than priv.
        /// Zero means that this object is the same a priv.
        /// Greater than zero means that this object is greater than priv.
        /// </returns>
        public int CompareTo(Privilege priv)
        {
            // First, compare grantee
            int order = Grantee.CompareTo(priv.Grantee);
            if (order != 0)
            {
                return order;
            }

            // Second, compare is grantable
            if (Grantable != priv.Grantable)
            {
                // not grantable preceeds grantable.
                order = (Grantable ? -1 : 1);
                return order;
            }

            // Third, compare grantor
            order = Grantor.CompareTo(priv.Grantor);
            if (order != 0)
            {
                return order;
            }

            // Otherwise, we're the same thing.
            return 0;
        }

        /// <summary>
        /// Translates from a string-ified user type to one of the UserType types.
        /// </summary>
        /// <param name="type">One of the known user types.
        /// <seealso cref="OWNER_USERTYPE"/>
        /// <seealso cref="PUBLIC_USERTYPE"/>
        /// <seealso cref="SYSTEM_USERTYPE"/>
        /// <seealso cref="USER_USERTYPE"/>
        /// </param>
        /// <returns>A UserType. If the usertype is unknown, it returns UserType.User</returns>
        private UserType TranslateType(string type)
        {
            UserType userType = UserType.User;

            if (type.Equals(OWNER_USERTYPE, StringComparison.OrdinalIgnoreCase))
            {
                userType = UserType.Owner;
            }
            else if (type.Equals(PUBLIC_USERTYPE, StringComparison.OrdinalIgnoreCase))
            {
                userType = UserType.Public;
            }
            else if (type.Equals(SYSTEM_USERTYPE, StringComparison.OrdinalIgnoreCase))
            {
                userType = UserType.System;
            }
            else if (type.Equals(ROLE_USERTYPE, StringComparison.OrdinalIgnoreCase))
            {
                userType = UserType.Role;
            }            
            else if (type.Equals(USER_USERTYPE, StringComparison.OrdinalIgnoreCase))
            {
                userType = UserType.User;
            }
            else
            {
#if DEBUG
                // A debug build will actually throw an exception for an unexpected type.
                throw new ApplicationException("\"" + type + "\" is an unknown user type.");
#endif
            }

            return userType;
        }
    }
}
