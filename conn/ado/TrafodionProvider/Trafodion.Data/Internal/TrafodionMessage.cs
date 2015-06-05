/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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
********************************************************************/

using System;

namespace Trafodion.Data
{
    //these names correspond directly to the Messages.resx file which contains the associated text

    //24500-24999 reserved for ADO.NET
    internal enum TrafodionDBMessage
    {
        //generic
        InternalError,   //used twice currently
        CommunicationFailure,

        //getobjref
        NoServerHandle,
        DSNotAvailable,
        PortNotAvailable,
        TryAgain,

        //others
        UnsupportedCommandType,


        //warnings
        ConnectedToDefaultDS,




        InvalidTransactionState,
        InvalidConnectionState,
        InvalidTrafodionDBDbType,
        UnsupportedIsolationLevel,
        ParameterCountMismatch,
        StmtInvalid
    }
}
