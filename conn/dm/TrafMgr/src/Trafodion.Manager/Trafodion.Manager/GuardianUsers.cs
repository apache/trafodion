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
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// Class used to store the mapping of Guardian UserName's to User Name (ie. 255,255 -> super.super)
    /// There is one static instance of a dictionary holding a group of Guardain users per connection
    /// defined by class GuardianUsers.   _allConnnections.
    /// There is one dictionary per class GuardianUsers to hold UserName -> Username mapping.  _useridToUsernameDictionary
    /// </summary>
    public class GuardianUsers
    {
        private ConnectionDefinition _connectionDef;
        private Dictionary<int, string> _useridToUsernameDictionary = new Dictionary<int, string>();
        private static Dictionary<ConnectionDefinition, GuardianUsers> _allConnnections = new Dictionary<ConnectionDefinition, GuardianUsers>(new MyConnectionDefinitionComparer());
       
 
        public static string UseridToUsername(ConnectionDefinition aConnectionDefinition, int aUserid)
        {
            if (!_allConnnections.ContainsKey(aConnectionDefinition))
            {
                _allConnnections.Add(aConnectionDefinition, new GuardianUsers(aConnectionDefinition));
            }
  
            return _allConnnections[aConnectionDefinition].UseridToUsername(aUserid); 
		}

        // constructor is private because it is only used by static method UseridToUsername in this class
        private GuardianUsers(ConnectionDefinition aConnectionDefinition)
        {
            _connectionDef = aConnectionDefinition;
            _useridToUsernameDictionary.Add(-1, "Public");
            _useridToUsernameDictionary.Add(-2, "System");
        }

    	private string UseridToUsername(int aUserid)
	    {

            
            // See if we have userID name string in dictionary.
            if (!(_useridToUsernameDictionary.ContainsKey(aUserid)))
            {
                Connection aConnection = new Connection(_connectionDef);
                //We dont have userID name string, find and add it to dictionary.

                try
                {
                    OdbcDataReader theReader;

                    OdbcCommand theQuery = new OdbcCommand(String.Format("SELECT USER({0}) FROM (values(1)) a;",
                        new object[] {Convert.ToString(aUserid)}));

                    theQuery.Connection = aConnection.OpenOdbcConnection;
                    theReader = Utilities.ExecuteReader(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, "Guardian Users", true);
                    theReader.Read();
                    _useridToUsernameDictionary.Add(aUserid, theReader.GetString(0).Trim());

                }
                catch(Exception ex)
                {
                    _useridToUsernameDictionary.Add(aUserid, aUserid.ToString());
                }
                finally
                {
                    if (aConnection != null)
                    {
                        aConnection.Close();
                    }
                }
            }

            return _useridToUsernameDictionary[aUserid];
	    }

    }
   
}
