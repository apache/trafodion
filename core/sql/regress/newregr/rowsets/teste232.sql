-- @@@ START COPYRIGHT @@@
--
-- Licensed to the Apache Software Foundation (ASF) under one
-- or more contributor license agreements.  See the NOTICE file
-- distributed with this work for additional information
-- regarding copyright ownership.  The ASF licenses this file
-- to you under the Apache License, Version 2.0 (the
-- "License"); you may not use this file except in compliance
-- with the License.  You may obtain a copy of the License at
--
--   http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing,
-- software distributed under the License is distributed on an
-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
-- KIND, either express or implied.  See the License for the
-- specific language governing permissions and limitations
-- under the License.
--
-- @@@ END COPYRIGHT @@@
/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
//
// @@@ END COPYRIGHT @@@ 
**********************************************************************/
        /*  Tests for compound statements containing rowsets and assignment 
          statements, on or below DP2. */ 
        
        #include <stdio.h>
        #include <string.h>
        #include <stdlib.h>
        
          EXEC SQL
            MODULE CAT.SCH.TTEMPM NAMES ARE ISO88591;
          
        EXEC SQL
        BEGIN DECLARE SECTION;
	/* First Insert */
          ROWSET [10000] int arr1A, arr1B;
          ROWSET [10000] short intarrInd1;
          ROWSET [5] char planOperators1[20];
	  ROWSET [5] short  chararrInd1;

	/* Second Insert */
          ROWSET [5000] int arr2A, arr2B;
          ROWSET [5000] short intarrInd2;
          ROWSET [5] char planOperators2[20];
	  ROWSET [5] short  chararrInd2;

          int a, b,i;
          int SQLCODE;
          char stmt_buffer[500];
        EXEC SQL END DECLARE SECTION;

        void test_insert();
        void test_select_into();
        void test_validate_plan();
        
        int main (int argc, char **argv)
        {
        
        
	  EXEC SQL CONTROL QUERY DEFAULT GENERATE_EXPLAIN  'ON' ;
          printf("Testing CS and Rowsets in DP2\n");
        
          /* Initialize data */
        
          for(i=0;i< 10000;i++) {
          arr1A[i] = i;
          }
        
          for(i=0;i< 5000;i++) {
          arr2A[i] = i;
          }

          test_insert();

          test_select_into();

          test_validate_plan();
          return 0;
        }

        void test_insert()
         {
           EXEC SQL INSERT INTO t232rt1 VALUES (:arr1A);

            if (SQLCODE != 0) {
           printf("Fail: Insert into t232rt1 table. SQLCODE = %ld\n", SQLCODE);
            EXEC SQL ROLLBACK;
            return ;
           }

           EXEC SQL INSERT INTO t232rt2 VALUES (:arr2A);

          if (SQLCODE != 0) {
         printf("Fail: Insert into t232rt2 table. SQLCODE = %ld\n", SQLCODE);
           EXEC SQL ROLLBACK;
           return ;
          }
        
           EXEC SQL COMMIT ;
        
           printf ("Done with Inserts\n\n");
         }

        void test_select_into()
         {
            EXEC SQL
              control query default opts_push_down_dam '1';
        
            EXEC SQL select t1 into :arr1B :intarrInd1 from t232rt1;

            if (SQLCODE != 0) {
            printf("Fail: Select into t232rt1 table. SQLCODE = %ld\n\n", SQLCODE);
             EXEC SQL ROLLBACK;
             return ;
            }

            EXEC SQL select t1 into :arr2B :intarrInd2 from t232rt2;
            if (SQLCODE != 0) {
            printf("Fail: Select into t232rt2 table. SQLCODE = %ld\n\n", SQLCODE);
            EXEC SQL ROLLBACK;
            return ;
            }
        }

       void test_validate_plan()
        {
	    EXEC SQL select operator into :planOperators1 :chararrInd1
            from table(explain('CAT.SCH.TTEMPM','SQLMX_DEFAULT_STATEMENT_9'));

            if (SQLCODE != 0) {
             printf("Fail: Select from module CAT.SCH.TTEMPM for \
                     rowsize > 30000. SQLCODE = %ld\n\n", SQLCODE);
            EXEC SQL ROLLBACK;
            return ;
            }

	    EXEC SQL select operator into :planOperators2 :chararrInd2
            from table(explain('CAT.SCH.TTEMPM','SQLMX_DEFAULT_STATEMENT_11'));

            if (SQLCODE != 0) {
            printf("Fail: Select from module CAT.SCH.TTEMPM for \
                    rowsize < 30000. SQLCODE = %ld\n\n", SQLCODE);
            EXEC SQL ROLLBACK;
            return ;
           }
            
          printf("Plan when row size > 30000.\n \
Expect Pack node to be above DP2  \n\n");
	  a = 0;
	  printf("%d %s\n", a++, planOperators1[0]);
	  printf("%d %s\n", a++, planOperators1[1]);
	  printf("%d %s\n", a++, planOperators1[2]);
	  printf("%d %s\n", a++, planOperators1[3]);
        
          printf("Plan when row size < 30000.\n \
Expect Pack node to be below DP2 \n\n");
	  a = 0;
	  printf("%d %s\n", a++, planOperators2[0]);
	  printf("%d %s\n", a++, planOperators2[1]);
	  printf("%d %s\n", a++, planOperators2[2]);
	  printf("%d %s\n", a++, planOperators2[3]);
       }
