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
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.SecurityArea.Model
{
    public class Credentials : SecurityObject
    {
        #region private variables
        string _systemName;
        string _commonName;
        string _orgName;
        string _orgUnit;
        string _city;
        string _state;
        int _keySize;
        string _country;
        string _certificate;
        string _caCertificate;
 
        #endregion private variables

        #region Public Properties

        public string SystemName
        {
            get { return _systemName; }
            set { _systemName = value; }
        }

        public string CommonName
        {
            get { return _commonName; }
            set { _commonName = value; }
        }

        public string OrgName
        {
            get { return _orgName; }
            set { _orgName = value; }
        }

        public string OrgUnit
        {
            get { return _orgUnit; }
            set { _orgUnit = value; }
        }

        public string City
        {
            get { return _city; }
            set { _city = value; }
        }

        public string State
        {
            get { return _state; }
            set { _state = value; }
        }

        public string Country
        {
            get { return _country; }
            set { _country = value; }
        }

        public int KeySize
        {
            get { return _keySize; }
            set { _keySize = value; }
        }

        public string Subject
        {
            get 
            {
                List<string> entries = new List<string>();

                if (!string.IsNullOrEmpty(_commonName))
                    entries.Add("CN=" + _commonName);
                if (!string.IsNullOrEmpty(_orgName))
                    entries.Add("O=" + _orgName);
                if (!string.IsNullOrEmpty(_orgUnit))
                    entries.Add("OU=" + _orgUnit);
                if (!string.IsNullOrEmpty(_city))
                    entries.Add("L=" + _city);
                if (!string.IsNullOrEmpty(_state))
                    entries.Add("ST=" + _state);
                if (!string.IsNullOrEmpty(_country))
                    entries.Add("C=" + _country);

                StringBuilder sb = new StringBuilder();
                for (int i = 0; i < entries.Count; i++)
                {
                    sb.Append(i < entries.Count - 1 ? entries[i] + "," : entries[i]);
                } 
                return sb.ToString();
            }
        }

        public string Certificate
        {
            get { return _certificate; }
            set { _certificate = value; }
        }


        public string CACertificate
        {
            get { return _caCertificate; }
            set { _caCertificate = value; }
        }

        #endregion Public Properties

        public Credentials(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
        }

        public string CreateCertificate()
        {
            string certString = "";

            if (!GetConnection())
                return certString;

            try
            {

                Queries.ExecuteCreateCertificate(CurrentConnection, this, out certString);
            }
            finally
            {
                CloseConnection();
            }
            return certString;
        }

        public String GenerateCSR()
        {
            string csrString = "";

            if (!GetConnection())
                return csrString;

            try
            {
                Queries.ExecuteCreateCSR(CurrentConnection, this, out csrString);
            }
            finally
            {
                CloseConnection();
            }
            return csrString;
        }

        public void DeployCertificate()
        {
            if (!GetConnection())
                return;

            try
            {
                Queries.ExecuteInsertCertificate(CurrentConnection, this);
            }
            finally
            {
                CloseConnection();
            }
        }
    }
}
