// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>

#include <stdio.h>

#define DESC_LEN 57
char odbcType[10];
char dsnName[50];
char user[17];
char password[30];
char insertType;
char dateType[13];
unsigned long  num_rows;
unsigned long  arr_size;
unsigned long  commit_rows;
unsigned long  rowsWithError;
unsigned long  startKey;

SQLHENV henv = NULL ;
SQLHDBC hdbc = NULL ;
SQLHSTMT hstmt = NULL;

SQLRETURN rc2;

UCHAR szSqlState[10];
SDWORD NativeError;
UCHAR szErrorMsg[501];
SWORD ErrorMsg;

SQLUSMALLINT	ParameterNumber;
SQLSMALLINT		InputOutputType;
SQLSMALLINT		ValueType;
SQLSMALLINT		ParameterType;
SQLUINTEGER		ColumnSize;
SQLSMALLINT		DecimalDigits;
SQLPOINTER		ParameterValuePtr;
SQLINTEGER		BufferLength;
SQLINTEGER 		StrLen_or_IndPtr;

SQLCHAR sqlStmt[] = "insert into table1hs (UNIQUE1, UNIQUE2, TWO, FOUR, TEN, TWENTY, "
		                                 "ONEPCT, TENPCT, TWENPCT, FIFTYPCT, HUNDPCT, "
										 "ODD1PCT, EVEN1PCT, STRINGU1) "
										 "values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

SQLCHAR sqlDel[] = "delete from table1hs";

long double start, finish;
long double elapsed;

long double start_execute, finish_execute;
long double quickest_execute, slowest_execute, diff_execute;
long double avg_execute;
unsigned long num_executes;

long double start_commit, finish_commit;
long double quickest_commit, slowest_commit, diff_commit;
long double avg_commit;
unsigned long num_commits;
/*
struct
{
	char  curr_date[6]; 
	char  prog_type[10]; 
	char  insert_fetch[6]; 
	int   array_size; 
	char  array_type[6]; 
	int   row_count;
	int   txn_count; 
	long double start_time; 
	long double end_time; 
	long double  elapsed_time; 
} stat_record;
*/
typedef struct _table_100
{
	unsigned long  unique1;int Unique1Ind;
	unsigned long  unique2; int   Unique2Ind;
	unsigned short two; int   TwoInd;
	unsigned short four; int   FourInd;
	unsigned short ten;int   TenInd;
	unsigned short twenty;int   TwentyInd;
	unsigned long  onepct;int OnepctInd;
	unsigned long  tenpct;int TenpctInd;
	unsigned long  twenpct;int TwenpctInd;
	unsigned long  fiftypct;int FiftypctInd;
	unsigned long  hundpct;int HundpctInd;
	unsigned long  odd1pct;int Odd1pctInd;
	unsigned long  even1pct;int   Even1pctInd;
	char  stringu1[DESC_LEN];int   Stringu1LenOrInd;
} table_100;

unsigned long nr = 0;
int indx, rc = 0, txns = 0;
int cbUnique1 = 0, cbUnique2 = 0, cbTwo = 0, cbFour = 0, cbTen = 0, cbTwenty = 0, cbStringu1 = SQL_NTS;
int cbOnepct = 0, cbTenpct = 0, cbTwenpct = 0,cbFiftypct = 0, cbHundpct = 0, cbOdd1pct = 0, cbEven1pct = 0;

SQLRETURN retcode;

void insert_single_rows();
void insert_rowarray_t1h();
void insert_columnarray_t1h();
void log_times();
void free_hndl();
void alloc_hndl();
void LogError();
long double getTimeMilli();

int main (int argc, char *argv[])
{
	if (argc < 9)
	{
		printf("\nUsage: bind_array1 <MX o MP> <num of rows to insert> <array size> <commit rows> <R (row-wise) or C (column-wise) or S (single row at a time)>"
			   " <data source name> <user id> <password> [<start key> [<%rows with error 0..100>]]\n");
		goto terminate;
	}
	else
	{
		strcpy(odbcType, argv[1]);
		num_rows = atoi(argv[2]);
		arr_size = atoi(argv[3]);
		commit_rows = atoi(argv[4]);
		insertType = argv[5][0];
		strcpy(dsnName, argv[6]);
		strcpy(user, argv[7]);
		strcpy(password, argv[8]);
		if (argc >=9)
		{
			startKey = atoi(argv[9]);
			if (argc >=10)
			{
				rowsWithError = atoi(argv[10]);
				// calculate the row intervals that will fail
				if (rowsWithError != 0)
				{
					rowsWithError = 100 / rowsWithError;
				}
				else if (rowsWithError > 100)
				{
					rowsWithError = 100;
				}
			}
			else
				rowsWithError = 0;
		}
		else
		{
			startKey = 101;
		}
	}

	if (!SQL_SUCCEEDED((retcode = SQLAllocEnv(&henv))))
	{
		LogError();
		goto terminate;
	}
	if (!SQL_SUCCEEDED((retcode = SQLAllocConnect( henv, &hdbc))))
	{
		LogError();
		goto terminate;
	}
	if (!SQL_SUCCEEDED((retcode = SQLConnect(hdbc, (unsigned char *)dsnName, SQL_NTS, (unsigned char *)user, SQL_NTS, (unsigned char *)password, SQL_NTS))))
	{
		LogError();
		goto terminate;
	}
	if (!SQL_SUCCEEDED((retcode = SQLAllocStmt(hdbc, &hstmt))))
	{
		LogError();
		goto terminate;
	}
	if (!SQL_SUCCEEDED((retcode = SQLSetConnectAttr(hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF, SQL_NTS))))
	{
		LogError();
		goto terminate;
	}
	free_hndl();
	
	/*
	alloc_hndl();

	// Delete the rows first
	if (!SQL_SUCCEEDED((retcode = SQLExecDirect(hstmt, sqlDel, SQL_NTS))))
	{
		LogError();	
		//printf("\nDelete Return Code = %d", retcode);
	}
	if (!SQL_SUCCEEDED((retcode = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT))))
		LogError();

	free_hndl();
	*/

	alloc_hndl();

	if (!SQL_SUCCEEDED((retcode = SQLPrepare(hstmt, sqlStmt, SQL_NTS))))
	{
		LogError();	
		//printf("Insert SQLPrepare Return Code = %d\n", retcode);
		goto terminate;
	}
	if (odbcType[1] == 'P')
		insert_single_rows();
	else
	{
		switch (insertType)
			{
				case 'S':
				case 's':
					insert_single_rows();
					break;

				case 'R':
				case 'r':
					insert_rowarray_t1h();
					break;


				case 'C':
				case 'c':
					insert_columnarray_t1h();
					break;
			}
	}

	alloc_hndl();
	log_times();

terminate:
	if( hstmt != NULL ) SQLFreeStmt( hstmt, SQL_DROP );
	if( hdbc  != NULL ) SQLDisconnect( hdbc );
	if( hdbc  != NULL ) SQLFreeConnect( hdbc );
	if( henv  != NULL ) SQLFreeEnv( henv );

	return 0;
}

void insert_single_rows()
{
	unsigned long unique1;
	unsigned long  unique2;
	unsigned short two;
	unsigned short four;
	unsigned short ten;
	unsigned short twenty;
	unsigned long  onepct;
	unsigned long  tenpct;
	unsigned long  twenpct;
	unsigned long  fiftypct;
	unsigned long  hundpct;
	unsigned long  odd1pct;
	unsigned long  even1pct;
	char  stringu1[DESC_LEN];

	unsigned int  intxn = 1, tl = 0;

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &unique1,0,(long *)&cbUnique1))))
		LogError();
	
	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &unique2,0,(long *)&cbUnique2))))
		LogError();
	
	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, &two,0,(long *)&cbTwo))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, &four,0,(long *)&cbFour))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, &ten,0,(long *)&cbTen))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, &twenty,0,(long *)&cbTwenty))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &onepct,0,(long *)&cbOnepct))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 8, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &tenpct,0,(long *)&cbTenpct))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 9, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &twenpct,0,(long *)&cbTwenpct))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 10, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &fiftypct,0,(long *)&cbFiftypct))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 11, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &hundpct,0,(long *)&cbHundpct))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 12, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &odd1pct,0,(long *)&cbOdd1pct))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 13, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &even1pct,0,(long *)&cbEven1pct))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 14, SQL_PARAM_INPUT, SQL_C_CHAR, 
									SQL_CHAR,DESC_LEN,0, stringu1,DESC_LEN,(long *)&cbStringu1))))
		LogError();
	
	unique1  = startKey;
	unique2  = startKey;
	two      = 100;
	four     = 100;
	ten      = 100;
	twenty   = 100;
	onepct   = 100;
	tenpct   = 100;
	twenpct  = 100;
	fiftypct = 100;
	hundpct  = 100;
	odd1pct  = 100;
	even1pct = 100;
	strcpy(stringu1,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123");

	quickest_execute = 2147483647L;
	slowest_execute = 0;
	num_executes = 0;

	quickest_commit = 2147483647L;
	slowest_commit = 0;
	num_commits = 0;

	//time(&start);
	start = getTimeMilli();

	// Loop for number of rows to insert
	for (nr = 0; nr < num_rows; nr++)
	{
		unique1++; 
		unique2++;
		intxn = 1;		// we're in a txn loop
		tl++;			// increment the txn loop counter
		
		//time(&start_execute);
		start_execute = getTimeMilli();
		if (!SQL_SUCCEEDED((retcode = SQLExecute(hstmt))))
			LogError();
		//time(&finish_execute);
		finish_execute = getTimeMilli();
		//diff_execute = (long)difftime(finish_execute, start_execute);
		diff_execute = finish_execute - start_execute;
		if (diff_execute > slowest_execute)
			slowest_execute = diff_execute;
		if (diff_execute < quickest_execute)
			quickest_execute = diff_execute;
		avg_execute += diff_execute;
		num_executes++;

		if (tl >= commit_rows)   // if the number of inserts in this loop is at least 3200, commit the txn.
		{
			//time(&start_commit);
			start_commit = getTimeMilli();
			if (!SQL_SUCCEEDED((retcode = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT))))
				LogError();
			else
			{
				//time(&finish_commit);
				finish_commit = getTimeMilli();
				//diff_commit = (long)difftime(finish_commit, start_commit);
				diff_commit = finish_commit - start_commit;
					slowest_commit = diff_commit;
				if (diff_commit < quickest_commit)
					quickest_commit = diff_commit;
				avg_commit += diff_commit;
				num_commits++;
				txns++;
				intxn = 0;	 // we've just committed the txn, so start a new one
				tl = 0;      // reset the txn loop counter
			}
		}
	}
	if (intxn)			// if we've done inserts, but haven't already committed the txn, commit it.
	{
		//time(&start_commit);
		start_commit = getTimeMilli();
		if (!SQL_SUCCEEDED((retcode = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT))))
			LogError();
		else
		{
			txns++;
			//time(&finish_commit);
			finish_commit = getTimeMilli();
			//diff_commit = (long)difftime(finish_commit, start_commit);
			diff_commit = finish_commit - start_commit;
			if (diff_commit > slowest_commit)
				slowest_commit = diff_commit;
			if (diff_commit < quickest_commit)
				quickest_commit = diff_commit;
			avg_commit += diff_commit;
			num_commits++;
		}
	}
	//time(&finish);
	finish = getTimeMilli();
	//elapsed = difftime(finish, start);
	elapsed = finish - start;
	avg_execute = avg_execute/num_executes;
	avg_commit = avg_commit/num_commits;
}

void insert_rowarray_t1h()
{
	SQLUSMALLINT i, *ParamStatusArray;
	SQLUINTEGER ParamsProcessed;
	unsigned int intxn = 1, tl = 0;
	table_100 *t1hArray;

	ParamStatusArray = (SQLUSMALLINT *)malloc (arr_size * sizeof(SQLUSMALLINT));
	t1hArray = (table_100 *)malloc (arr_size * sizeof(table_100));

	// Set the SQL_ATTR_PARAM_BIND_TYPE statement attribute to use column-wise binding.
	if (!SQL_SUCCEEDED((retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAM_BIND_TYPE, (void *)sizeof(table_100), 0))))
		LogError();

	// Specify the number of elements in each parameter array.
	if (!SQL_SUCCEEDED((retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMSET_SIZE, (void *)arr_size, 0))))
		LogError();

	// Specify an array in which to return the status of each set of parameters.
	if (!SQL_SUCCEEDED((retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAM_STATUS_PTR, ParamStatusArray, 0))))
		LogError();

	// Specify an SQLUINTEGER value in which to return the number of sets of parameters
	// processed.
	if (!SQL_SUCCEEDED((retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &ParamsProcessed, 0))))
		LogError();

	// Bind the parameters in row-wise fashion.
	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &t1hArray[0].unique1,0,(long *)&t1hArray[0].Unique1Ind))))
		LogError();
	
	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &t1hArray[0].unique2,0,(long *)&t1hArray[0].Unique2Ind))))
		LogError();
	
	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, &t1hArray[0].two,0,(long *)&t1hArray[0].TwoInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, &t1hArray[0].four,0,(long *)&t1hArray[0].FourInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, &t1hArray[0].ten,0,(long *)&t1hArray[0].TenInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, &t1hArray[0].twenty,0,(long *)&t1hArray[0].TwentyInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &t1hArray[0].onepct,0,(long *)&t1hArray[0].OnepctInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 8, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &t1hArray[0].tenpct,0,(long *)&t1hArray[0].TenpctInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 9, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &t1hArray[0].twenpct,0,(long *)&t1hArray[0].TwenpctInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 10, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &t1hArray[0].fiftypct,0,(long *)&t1hArray[0].FiftypctInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 11, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &t1hArray[0].hundpct,0,(long *)&t1hArray[0].HundpctInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 12, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &t1hArray[0].odd1pct,0,(long *)&t1hArray[0].Odd1pctInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 13, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, &t1hArray[0].even1pct,0,(long *)&t1hArray[0].Even1pctInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 14, SQL_PARAM_INPUT, SQL_C_CHAR, 
									SQL_CHAR,DESC_LEN,0, t1hArray[0].stringu1,DESC_LEN,(long *)&t1hArray[0].Stringu1LenOrInd))))
		LogError();

	quickest_execute = 2147483647L;
	slowest_execute = 0;
	num_executes = 0;

	quickest_commit = 2147483647L;
	slowest_commit = 0;
	num_commits = 0;
	
	//time(&start);
	start = getTimeMilli();

	for (nr = 0; nr < num_rows;)				// Loop for number of rows to insert
	{
		for (i = 0; i < arr_size; i++)		// Set part ID, description, and price.
		{
			if (nr >= num_rows)
			{
				// Specify the number of elements in each parameter array.
				if (!SQL_SUCCEEDED((retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMSET_SIZE, (void *)i, 0))))
					LogError();
				break;
			}

			intxn = 1;                          
			t1hArray[i].unique1 = startKey + nr;
			if ((rowsWithError != 0) && ((nr % rowsWithError) == 0))
			{
				t1hArray[i].unique2 = startKey;	// inject an error (duplicated key)
			}
			else
			{
				t1hArray[i].unique2 = startKey + nr;
			}
			t1hArray[i].two = 100;
			t1hArray[i].four = 100;
			t1hArray[i].ten = 100;
			t1hArray[i].twenty = 100;
			t1hArray[i].onepct = 100;
			t1hArray[i].tenpct = 100;
			t1hArray[i].twenpct = 100;
			t1hArray[i].fiftypct = 100;
			t1hArray[i].hundpct = 100;
			t1hArray[i].odd1pct = 100;
			t1hArray[i].even1pct = 100;
			strcpy(t1hArray[i].stringu1, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123");

			t1hArray[i].Unique1Ind = 0;
			t1hArray[i].Unique2Ind = 0;
			t1hArray[i].TwoInd = 0;
			t1hArray[i].FourInd = 0;
			t1hArray[i].TenInd = 0;
			t1hArray[i].TwentyInd = 0;
			t1hArray[i].OnepctInd = 0;
			t1hArray[i].TenpctInd = 0;
			t1hArray[i].TwenpctInd = 0;
			t1hArray[i].FiftypctInd = 0;
			t1hArray[i].HundpctInd = 0;
			t1hArray[i].Odd1pctInd = 0;
			t1hArray[i].Even1pctInd = 0;
			t1hArray[i].Stringu1LenOrInd = SQL_NTS;
			nr++;		// increment the number of rows inserted
			tl++;       // increment the number of inserts in this transaction loop
		}
		// Execute the statement.
		//time(&start_execute);
		start_execute = getTimeMilli();
		if (!SQL_SUCCEEDED((retcode = SQLExecute(hstmt))))
		{	
			free (ParamStatusArray);
			free (t1hArray);
			LogError();
			return;
		}

		//time(&finish_execute);
		finish_execute = getTimeMilli();
		//diff_execute = (long)difftime(finish_execute, start_execute);
		diff_execute = finish_execute - start_execute;
		if (diff_execute > slowest_execute)
			slowest_execute = diff_execute;
		if (diff_execute < quickest_execute)
			quickest_execute = diff_execute;
		avg_execute += diff_execute;
		num_executes++;

		if (tl >= commit_rows)  // if the number of inserts in this loop is at least 3200, commit the txn
		{
			//time(&start_commit);
			start_commit = getTimeMilli();
			if (!SQL_SUCCEEDED((retcode = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT))))
				LogError();
			else
			{
				//time(&finish_commit);
				finish_commit = getTimeMilli();
				//diff_commit = (long)difftime(finish_commit, start_commit);
				diff_commit = finish_commit - start_commit;
				if (diff_commit > slowest_commit)
					slowest_commit = diff_commit;
				if (diff_commit < quickest_commit)
					quickest_commit = diff_commit;
				avg_commit += diff_commit;
				num_commits++;
				txns++;
				intxn = 0;	// we've done a txn, so reset the txn indicator
				tl = 0;    // reset the txn loop counter
			}
		}
	}
	if (intxn)   // if we've done inserts, but haven't committed yet, commit
	{
		//time(&start_commit);
		start_commit = getTimeMilli();
		if (!SQL_SUCCEEDED((retcode = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT))))
			LogError();
		else
		{
			txns++;
			//time(&finish_commit);
			finish_commit = getTimeMilli();
			//diff_commit = (long)difftime(finish_commit, start_commit);
			diff_commit = finish_commit - start_commit;
			if (diff_commit > slowest_commit)
				slowest_commit = diff_commit;
			if (diff_commit < quickest_commit)
				quickest_commit = diff_commit;
			avg_commit += diff_commit;
			num_commits++;
		}
	}
	free (ParamStatusArray);
	free (t1hArray);
	//time(&finish);
	finish = getTimeMilli();
	elapsed = finish - start;
	avg_execute = avg_execute/num_executes;
	avg_commit = avg_commit/num_commits;
}

void insert_columnarray_t1h()
{
	unsigned long  *unique1;
	unsigned long  *unique2;
	unsigned short *two;
	unsigned short *four;
	unsigned short *ten;
	unsigned short *twenty;
	unsigned long  *onepct;
	unsigned long  *tenpct;
	unsigned long  *twenpct;
	unsigned long  *fiftypct;
	unsigned long  *hundpct;
	unsigned long  *odd1pct;
	unsigned long  *even1pct;
	char  *stringu1;

	long  *Unique1Ind;
	long  *Unique2Ind;
	int   *TwoInd;
	int   *FourInd;
	int   *TenInd;
	int   *TwentyInd;
	int   *OnepctInd;
	int   *TenpctInd;
	int   *TwenpctInd;
	int   *FiftypctInd;
	int   *HundpctInd;
	int   *Odd1pctInd;
	int   *Even1pctInd;
	int   *Stringu1LenOrInd;
	SQLUSMALLINT i, intxn = 1, tl = 0, *ParamStatusArray;
	SQLUINTEGER ParamsProcessed;

	unique1 = (unsigned long  *) malloc(arr_size * sizeof(unsigned long));
	unique2 = (unsigned long  *) malloc(arr_size * sizeof(unsigned long));
	two = (unsigned short  *) malloc(arr_size * sizeof(unsigned short));
	four = (unsigned short  *) malloc(arr_size * sizeof(unsigned short));
	ten = (unsigned short  *) malloc(arr_size * sizeof(unsigned short));
	twenty = (unsigned short  *) malloc(arr_size * sizeof(unsigned short));
	onepct = (unsigned long  *) malloc(arr_size * sizeof(unsigned long));
	tenpct = (unsigned long  *) malloc(arr_size * sizeof(unsigned long));
	twenpct = (unsigned long  *) malloc(arr_size * sizeof(unsigned long));
	fiftypct = (unsigned long  *) malloc(arr_size * sizeof(unsigned long));
	hundpct = (unsigned long  *) malloc(arr_size * sizeof(unsigned long));
	odd1pct = (unsigned long  *) malloc(arr_size * sizeof(unsigned long));
	even1pct = (unsigned long  *) malloc(arr_size * sizeof(unsigned long));

	stringu1 = (char *) malloc(arr_size * DESC_LEN);

	Unique1Ind = (long  *) malloc(arr_size * sizeof(long));
	Unique2Ind = (long  *) malloc(arr_size * sizeof(long));
	TwoInd = (int  *) malloc(arr_size * sizeof(int));
	FourInd = (int  *) malloc(arr_size * sizeof(int));
	TenInd = (int  *) malloc(arr_size * sizeof(int));
	TwentyInd = (int  *) malloc(arr_size * sizeof(int));
	OnepctInd = (int  *) malloc(arr_size * sizeof(int));
	TenpctInd = (int  *) malloc(arr_size * sizeof(int));
	TwenpctInd = (int  *) malloc(arr_size * sizeof(int));
	FiftypctInd = (int  *) malloc(arr_size * sizeof(int));
	HundpctInd = (int  *) malloc(arr_size * sizeof(int));
	Odd1pctInd = (int  *) malloc(arr_size * sizeof(int));
	Even1pctInd = (int  *) malloc(arr_size * sizeof(int));
	Stringu1LenOrInd = (int  *) malloc(arr_size * sizeof(int));
	ParamStatusArray = (SQLUSMALLINT *) malloc(arr_size * sizeof(SQLUSMALLINT));

	// Set the SQL_ATTR_PARAM_BIND_TYPE statement attribute to use column-wise binding.
	if (!SQL_SUCCEEDED(retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0)))
		LogError();

	// Specify the number of elements in each parameter array.
	if (!SQL_SUCCEEDED(retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMSET_SIZE, (void *)arr_size, 0)))
		LogError();

	// Specify an array in which to return the status of each set of parameters.
	if (!SQL_SUCCEEDED(retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAM_STATUS_PTR, ParamStatusArray, 0)))
		LogError();

	// Specify an SQLUINTEGER value in which to return the number of sets of parameters
	// processed.
	if (!SQL_SUCCEEDED(retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &ParamsProcessed, 0)))
		LogError();

	// Bind the parameters in column-wise fashion.
	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, unique1,0,Unique1Ind))))
		LogError();
	
	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, unique2,0,Unique2Ind))))
		LogError();
	
	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, two,0,(long *)TwoInd))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, four,0,(long *)FourInd))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, ten,0,(long *)TenInd))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_USHORT,
							    	SQL_INTEGER,5,0, twenty,0,(long *)TwentyInd))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, onepct,0,(long *)OnepctInd))))
		LogError(); 

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 8, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, tenpct,0,(long *)TenpctInd))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 9, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, twenpct,0,(long *)TwenpctInd))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 10, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, fiftypct,0,(long *)FiftypctInd))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 11, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, hundpct,0,(long *)HundpctInd))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 12, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, odd1pct,0,(long *)Odd1pctInd))))
		LogError();	

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 13, SQL_PARAM_INPUT, SQL_C_ULONG,
							    	SQL_INTEGER,5,0, even1pct,0,(long *)Even1pctInd))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLBindParameter(hstmt, 14, SQL_PARAM_INPUT, SQL_C_CHAR, 
									SQL_CHAR,DESC_LEN,0, stringu1, DESC_LEN,(long *)Stringu1LenOrInd))))
		LogError();

	quickest_execute = 2147483647L;
	slowest_execute = 0;
	num_executes = 0;

	quickest_commit = 2147483647L;
	slowest_commit = 0;
	num_commits = 0;

	//time(&start);	// Get the start time
	start = getTimeMilli();

	for (nr = 0; nr < num_rows;)					// Loop for number of rows to insert
	{
		for (i = 0; i < arr_size; i++)			// loop for the number in the array
		{
			if (nr >= num_rows)
			{
				// Specify the number of elements in each parameter array.
				if (!SQL_SUCCEEDED((retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMSET_SIZE, (void *)i, 0))))
					LogError();
				break;
			}
			unique1 [i] = startKey + nr;
			if ((rowsWithError != 0) && ((nr % rowsWithError) == 0))
			{
				unique2[i] = startKey;	// inject an error (duplicated key)
			}
			else
			{
				unique2[i] = startKey + nr;
			}
			two     [i] = 100;
			four    [i] = 100;
			ten     [i] = 100;
			twenty  [i] = 100;
			onepct  [i] = 100;
			tenpct  [i] = 100;
			twenpct [i] = 100;
			fiftypct[i] = 100;
			hundpct [i] = 100;
			odd1pct [i] = 100;
			even1pct[i] = 100;
			strcpy(&stringu1[i*DESC_LEN], "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123");

			Unique1Ind[i]      = 0;
			Unique2Ind[i]      = 0;
			TwoInd[i]          = 0;
			FourInd[i]         = 0;
			TenInd[i]          = 0;
			TwentyInd[i]       = 0;
			OnepctInd[i]       = 0;
			TenpctInd[i]       = 0;
			TwenpctInd[i]      = 0;
			FiftypctInd[i]     = 0;
			HundpctInd[i]      = 0;
			Odd1pctInd[i]      = 0;
			Even1pctInd[i]     = 0;
			Stringu1LenOrInd[i]= SQL_NTS;
			intxn = 1;			// we're in a txn loop
			nr++;				// increment the numbers of rows inserted
			tl++;				// increment the number of rows inserted for this txn loop
		}
		
			// Execute the statement.
		//time(&start_execute);
		start_execute = getTimeMilli();
		if (!SQL_SUCCEEDED((retcode = SQLExecute(hstmt))))
		{
			LogError();
			free (unique1);
			free (unique2);
			free (two);
			free (four);
			free (ten);
			free (twenty);
			free (onepct);
			free (tenpct);
			free (twenpct);
			free (fiftypct);
			free (hundpct);
			free (odd1pct);
			free (even1pct);

			free (stringu1);

			free (Unique1Ind);
			free (Unique2Ind);
			free (TwoInd);
			free (FourInd);
			free (TenInd);
			free (TwentyInd);
			free (OnepctInd);
			free (TenpctInd);
			free (TwenpctInd);
			free (FiftypctInd);
			free (HundpctInd);
			free (Odd1pctInd);
			free (Even1pctInd);
			free (Stringu1LenOrInd);
			free (ParamStatusArray);

			return;
		}
		//time(&finish_execute);
		finish_execute = getTimeMilli();
		//diff_execute = (long)difftime(finish_execute, start_execute);
		diff_execute = finish_execute - start_execute;
		if (diff_execute > slowest_execute)
			slowest_execute = diff_execute;
		if (diff_execute < quickest_execute)
			quickest_execute = diff_execute;
		avg_execute += diff_execute;
		num_executes++;

		//printf("Executing the insert for row %lu \n",nr); 
		if (tl >= commit_rows)			// if we've inserted at least 3200 rows, commit the txn
		{
			//time(&start_commit);
			start_commit = getTimeMilli();
			if (!SQL_SUCCEEDED((retcode = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT))))
				LogError();
			else
			{
				//time(&finish_commit);
				finish_commit = getTimeMilli();
				//diff_commit = (long)difftime(finish_commit, start_commit);
				diff_commit = finish_commit - start_commit;
				if (diff_commit > slowest_commit)
					slowest_commit = diff_commit;
				if (diff_commit < quickest_commit)
					quickest_commit = diff_commit;
				avg_commit += diff_commit;
				num_commits++;
				intxn = 0;	// we've just committed a txn
				txns++;		
				tl = 0;		// reset the txn loop counter
			}
		}
	}
	if (intxn)		// if we've inserted rows, but haven't committed the last set, commit.
	{
		//time(&start_commit);
		start_commit = getTimeMilli();
		if (!SQL_SUCCEEDED((retcode = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT))))
			LogError();
		else
		{
			txns++;
			//time(&finish_commit);
			finish_commit = getTimeMilli();
			//diff_commit = (long)difftime(finish_commit, start_commit);
			diff_commit = finish_commit - start_commit;
			if (diff_commit > slowest_commit)
				slowest_commit = diff_commit;
			if (diff_commit < quickest_commit)
				quickest_commit = diff_commit;
			avg_commit += diff_commit;
			num_commits++;
		}
	}	

	free (unique1);
	free (unique2);
	free (two);
	free (four);
	free (ten);
	free (twenty);
	free (onepct);
	free (tenpct);
	free (twenpct);
	free (fiftypct);
	free (hundpct);
	free (odd1pct);
	free (even1pct);
	free (stringu1);
	free (Unique1Ind);
	free (Unique2Ind);
	free (TwoInd);
	free (FourInd);
	free (TenInd);
	free (TwentyInd);
	free (OnepctInd);
	free (TenpctInd);
	free (TwenpctInd);
	free (FiftypctInd);
	free (HundpctInd);
	free (Odd1pctInd);
	free (Even1pctInd);
	free (Stringu1LenOrInd);
	free (ParamStatusArray);

	//time(&finish);    // Get the end time
	finish = getTimeMilli();
	//elapsed = (difftime(finish, start));  //Get the elapsed time
	elapsed = finish - start;
	avg_execute = avg_execute/num_executes;
	avg_commit = avg_commit/num_commits;
}	

void free_hndl()
{
	SQLFreeStmt(hstmt, SQL_CLOSE);
}

void alloc_hndl()
{
	if (!SQL_SUCCEEDED((retcode = SQLAllocStmt( hdbc, &hstmt))))
	{
		LogError();
	}
}

void log_times()
{
	char curDate[20];
	int	 cbcurDate = SQL_NTS;
	char array_type[5];
	char msg[1000];

	SQLCHAR sqlStmt[] = "Select {fn CURDATE()} from dtp";

	retcode = SQLAllocStmt( hdbc, &hstmt);
	retcode = SQLExecDirect(hstmt, sqlStmt, SQL_NTS);
	retcode = SQLBindCol(hstmt,1,SQL_C_CHAR,  &curDate,20, (long *)&cbcurDate);

	retcode = SQLFetch(hstmt); 
	printf("MYDATE= %s  \n", curDate);
/* Joe - this causes an error
	if (!SQL_SUCCEEDED((retcode = SQLSetConnectAttr(hdbc, SQL_ATTR_ACCESS_MODE, SQL_MODE_READ_WRITE, SQL_NTS))))
	{
		LogError();
		return;
	}
*/
	free_hndl();
	alloc_hndl();
/*
	stat_record.start_time = start;
	stat_record.end_time = finish;
	stat_record.elapsed_time = elapsed; 
	stat_record.array_size = arr_size;
	stat_record.row_count = nr;
	stat_record.txn_count = txns;
*/	
	switch (insertType)
	{
		case 'S':
		case 's':
			strcpy(array_type, "SIN\0");
			break;

		case 'C':
		case 'c':
			strcpy(array_type, "COL\0");
			break;

		case 'R':
		case 'r':
			strcpy(array_type, "ROW\0");
			break;
	}

	sprintf(msg, "INSERT INTO STATSP2 (curr_date, prog_type, insert_fetch, row_count, "
					"start_time, end_time, elapsed_time, "  
					"array_size, array_type, txn_count, num_executes, quickest_execute, slowest_execute, avg_execute, num_commits, quickest_commit, slowest_commit, avg_commit) "
	 "VALUES ({d'%s'}, 'ODBC/%s', 'INSERT', %lu, %6.0Lf, %6.0Lf, %6.0Lf, %d, '%s', %d, %ld, %6.0Lf, %6.0Lf, %8.3Lf, %ld, %6.0Lf, %6.0Lf, %8.3Lf)",
				curDate, odbcType, nr, start, finish, elapsed, arr_size, array_type, txns, num_executes, quickest_execute, slowest_execute, avg_execute, num_commits, quickest_commit, slowest_commit, avg_commit);

	printf("\n%s\n", msg);

	if (!SQL_SUCCEEDED((retcode = SQLExecDirect(hstmt, (unsigned char *)msg, SQL_NTS))))
		LogError();

	if (!SQL_SUCCEEDED((retcode = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT))))
		LogError();
}

void LogError( )
{

	printf("\nRetcode = %d, sqlState = %s, errorMsg = %s", retcode, (char*)szSqlState, (char*)szErrorMsg); 
	while( SQLError( henv, hdbc, NULL, szSqlState, &NativeError, szErrorMsg, 500, &ErrorMsg) == SQL_SUCCESS){
		printf("\n%s\n",(char*)szErrorMsg );
	}
	while( SQLError( henv, hdbc, hstmt, szSqlState, &NativeError, szErrorMsg, 500, &ErrorMsg) == SQL_SUCCESS){
		printf("\n%s\n",(char*)szErrorMsg );
	}
}

long double getTimeMilli()
{
	long double CurrentTime;
	struct _timeb tb;
	_ftime(&tb);
	CurrentTime =(long double) (tb.time * 1000.0) + (tb.millitm);
	return CurrentTime;
}

