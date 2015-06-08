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
    [Flags]
    internal enum ConnectionContextOptions1: uint
    {
        SessionName = 0x80000000, // (2^31)
	    FetchAhead = 0x40000000, // (2^30)
	    CertTimestamp = 0x20000000, //(2^29)
	    ClientUsername = 0x10000000 //(2^28)
    }

    internal enum MetaDataId : short
    {
        Columns = 40,
        Tables = 54,
        ForeignKeys = 60,
        PrimaryKeys = 65,
        Procedures = 67,
        TableMVS = 1918,

        //above +9999 for JDBC
        JdbcTables = 10053,
        JdbcColumns = 10039,
    }

    internal enum ParameterMode : int
    {
        In = 1,
        InOut = 2,
        Result = 3,
        Out = 4,
        Return = 5
    }

    internal enum ConnectionOption : short
    {
        AccessMode = 101,
        AutoCommit = 102,
        TransactionIsolation = 108,
        ResetIdleTimer = 1070,
        RowsetRecovery = 2000
    }

    internal enum TransactionIsolation : int
    {
        ReadUncommitted = 1,
        ReadCommmited = 2,
        RepeatableRead = 4,
        Serializable = 8
    }

    internal enum FileSystemType : int
    {
        VarcharLong = -1,
        Char = 1,
        Numeric = 2,
        NumericUnsigned = -201,
        Decimal = 3,
        DecimalUnsigned = -301,
        DecimalLarge = -302,
        DecimalLargeUnsigned = -303,
        Integer = 4,
        IntegerUnsigned = -401,
        LargeInt = -402,
        SmallInt = 5,
        SmallIntUnsigned = -502,
        BPIntUnsigned = -503,
        Float = 6,
        Real = 7,
        Double = 8,
        DateTime = 9,
        Interval = 10,
        Varchar = 12,
        VarcharWithLength = -601,
        Bit = 14,
        BitVar = 15,
        CharDblByte = 16,
        VarcharDblByte = 17,
    }

    internal enum DateTimeCode : int
    {
        Date = 1,
        Time = 2,
        Timestamp = 3,
    }

    internal enum StatementType: int
    {
        Unknown = 0,
        Select = 0x0001,
        Update = 0x0002,
        Delete = 0x0004,
        Insert = 0x0008,
        Explain = 0x0010,
        Create = 0x0020,
        Grant = 0x0040,
        Drop = 0x0080,
        InsertParam = 0x0100,
        SelectCatalog = 0x0200,
        Smd = 0x0400,
        Call = 0x0800,
        Stats = 0x1000,
        Config = 0x2000,

        Wms = 0x4000,
        WmsOpen = 0x4001,
        WmsClose = 0x4002,

        Command = 0x3000,
        CommandOpen = 0x3001,
        CommandClose = 0x30002
    }

    internal enum QueryType : int
    {
        Other = -1,
        Unknown = 0,
        SelectUnique = 1,
        SelectNonUnique = 2,
        InsertUnique = 3,
        InsertNonUnique = 4,
        UpdateUnique = 5,
        UpdateNonUnique = 6,
        DeleteUnique = 7,
        DeleteNonUnique = 8,
        Control = 9,
        SetTransaction = 10,
        SetCatalog = 11,
        SetSchema = 12,
        CallNoResults = 13,
        CallWithResults = 14,
        SpResultSet = 15,
        InsertRWRS = 16,
        CatUtil = 17,
        ExeUtil = 18,

        BulkFetch = 10000
    }

    internal enum ReturnCode: int
    {
        Success = 0,
        SuccessWithInfo = 1,
        NoDataFound = 100
    }

    [Flags]
    internal enum BuildOptions : uint
    {
        StreamingDelayedErrorMode = 536870912, // 2^29
        Charset = 268435456, // (2^28)
        RowwiseRowset = 134217728,  // (2^27)
        PasswordSecurity = 67108864 // (2^26)
    }

    internal enum OperationId : short
    {
        AS_API_INIT = 1000,
        AS_API_GETOBJREF_PRE_R22,					//OK NSKDRVR/CFGDRVR
        AS_API_REGPROCESS,							//OK NSKSRVR/CFGSRVR
        AS_API_UPDATESRVRSTATE,						//OK NSKSRVR
        AS_API_WOULDLIKETOLIVE,						//OK NSKSRVR
        AS_API_STARTAS,								//OK CFGDRVR
        AS_API_STOPAS,								//OK CFGDRVR
        AS_API_STARTDS,								//OK CFGDRVR
        AS_API_STOPDS,								//OK CFGDRVR
        AS_API_STATUSAS,							//OK CFGDRVR
        AS_API_STATUSDS,							//OK CFGDRVR
        AS_API_STATUSDSDETAIL,						//OK CFGDRVR
        AS_API_STATUSSRVRALL,						//OK CFGDRVR
        AS_API_STOPSRVR,							//OK CFGDRVR
        AS_API_STATUSDSALL,							//OK CFGDRVR
        AS_API_DATASOURCECONFIGCHANGED,				//OK CFGSRVR
        AS_API_ENABLETRACE,							//OK CFGDRVR
        AS_API_DISABLETRACE,						//OK CFGDRVR
        AS_API_GETVERSIONAS,						//OK CFGDRVR
        AS_API_GETOBJREF,							//OK NSKDRVR/CFGDRVR

        CFG_API_INIT = 2000,
        CFG_API_GETOBJECTNAMELIST,					//OK CFGDRVR
        CFG_API_GETDATASOURCE,						//OK CFGDRVR
        CFG_API_DROPDATASOURCE,						//OK CFGDRVR
        CFG_API_SETDATASOURCE,						//OK CFGDRVR
        CFG_API_ADDNEWDATASOURCE,					//OK CFGDRVR
        CFG_API_CHECKDATASOURCENAME,				//OK CFGDRVR
        CFG_API_GETDSNCONTROL,						//OK CFGDRVR
        CFG_API_SETDSNCONTROL,						//OK CFGDRVR
        CFG_API_GETRESOURCEVALUES,					//OK CFGDRVR
        CFG_API_SETRESOURCEVALUES,					//OK CFGDRVR
        CFG_API_GETENVIRONMENTVALUES,				//OK CFGDRVR
        CFG_API_SETENVIRONMENTVALUES,				//OK CFGDRVR
        CFG_API_GETSTARTUPCONFIGVALUES,				//OK AS
        CFG_API_GETDATASOURCEVALUES,				//OK AS/CFGDRVR
        CFG_API_SETDSSTATUS,						//OK AS
        CFG_API_SETASSTATUS,						//OK AS
        CFG_API_USERAUTHENTICATE,					//OK - unused
        CFG_API_CHANGEPASSWORD,						//OK - unused
        CFG_API_STOPESPORPHANS,						//OK

        SRVR_API_INIT = 3000,
        SRVR_API_SQLCONNECT,						//OK NSKDRVR
        SRVR_API_SQLDISCONNECT,						//OK NSKDRVR
        SRVR_API_SQLSETCONNECTATTR,					//OK NSKDRVR
        SRVR_API_SQLENDTRAN,						//OK NSKDRVR
        SRVR_API_SQLPREPARE,						//OK NSKDRVR
        SRVR_API_SQLPREPARE_ROWSET,                 //OK NSKDRVR
        SRVR_API_SQLEXECUTE_ROWSET,					//OK NSKDRVR
        SRVR_API_SQLEXECDIRECT_ROWSET,				//OK NSKDRVR
        SRVR_API_SQLFETCH,
        SRVR_API_SQLFETCH_ROWSET,					//OK NSKDRVR
        SRVR_API_SQLEXECUTE,						//OK NSKDRVR
        SRVR_API_SQLEXECDIRECT,						//OK NSKDRVR
        SRVR_API_SQLEXECUTECALL,					//OK NSKDRVR
        SRVR_API_SQLFETCH_PERF,						//OK NSKDRVR
        SRVR_API_SQLFREESTMT,						//OK NSKDRVR
        SRVR_API_GETCATALOGS,						//OK NSKDRVR
        SRVR_API_STOPSRVR,							//OK AS
        SRVR_API_ENABLETRACE,						//OK AS
        SRVR_API_DISABLETRACE,						//OK AS
        SRVR_API_ENABLE_SERVER_STATISTICS,			//OK AS
        SRVR_API_DISABLE_SERVER_STATISTICS,			//OK AS
        SRVR_API_UPDATE_SERVER_CONTEXT,				//OK AS
        SRVR_API_MONITORCALL,						//OK PCDRIVER
        SRVR_API_SQLPREPARE2,						//OK PCDRIVER
        SRVR_API_SQLEXECUTE2,						//OK PCDRIVER
        SRVR_API_SQLFETCH2,							//OK PCDRIVER
        SRVR_API_SQLFASTEXECDIRECT,					//OK WMS
        SRVR_API_SQLFASTFETCH_PERF,					//OK WMS
        SRVR_API_GETSEGMENTS						//OK WMS
    }
}
