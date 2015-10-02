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

-----------------------------------------------------------------
--
-- sample.sql is a sample script file that creates the database.
--           Schema:   TRAFODION.CI_SAMPLE
-- -------------------------------------------------------------

-- Personnel data: EMPLOYEE, DEPT, JOB, AND PROJECT tables

   CREATE SCHEMA TRAFODION.CI_SAMPLE;
   SET SCHEMA TRAFODION.CI_SAMPLE;
   
--   SHOW TABLES EMPLOYEE
   IF RECCOUNT == 1 THEN GOTO CREATE_DEPT_TABLE;

   CREATE TABLE employee (
                           empnum          NUMERIC (4) UNSIGNED
                                           NO DEFAULT
                                           NOT NULL
--                                           constraint empnum_constrnt
--                                           check (empnum > 0 and
--                                           (empnum < 10000))
                          ,first_name      CHARACTER (15)
                                           DEFAULT ' '
                                           NOT NULL
                          ,last_name       CHARACTER (20)
                                           DEFAULT ' '
                                           NOT NULL
                          ,deptnum         NUMERIC (4)
                                           UNSIGNED
                                           NO DEFAULT
                                           NOT NULL
                          ,jobcode         NUMERIC (4) UNSIGNED
                                           DEFAULT NULL
                          ,salary          NUMERIC (8, 2) UNSIGNED
                                           DEFAULT NULL
                          ,PRIMARY KEY     (empnum)
                          )
   ;

   CREATE VIEW emplist
     AS SELECT
          empnum
         ,first_name
         ,last_name
         ,deptnum
         ,jobcode
        FROM employee
   ;


   CREATE INDEX xempname
     ON employee (
                   last_name
                  ,first_name
                  )
   ;


   CREATE INDEX xempdept
     ON employee (
                   deptnum
                  )
   ;

   
   LABEL CREATE_DEPT_TABLE
   INVOKE DEPT;
   IF ERRORCODE != 4082 THEN GOTO CREATE_JOB_TABLE; 

   CREATE TABLE dept     (
                           deptnum         NUMERIC (4) UNSIGNED
                                           NO DEFAULT
                                           NOT NULL
--                                           constraint deptnum_constrnt
--                                              check (deptnum in (1000,1500,
--                                                                 2000,2500,
--                                                                 3000,3100,
--                                                                 3200,3300,
--                                                                 3500,4000,
--                                                                 4100,9000))
                          ,deptname        CHARACTER (12)
                                           NO DEFAULT
                                           NOT NULL
                          ,manager         NUMERIC (4) UNSIGNED
                                           NO DEFAULT
                                           NOT NULL
--                                           constraint mgrnum_constrnt
--                                              check (manager > 0000 and
--                                              manager < 10000)
                          ,rptdept         NUMERIC (4) UNSIGNED
                                           DEFAULT 0
                                           NOT NULL
                          ,location        VARCHAR (18)
                                           DEFAULT ' '
                                           NOT NULL
                          ,PRIMARY KEY     (deptnum)
                          )
   ;


   CREATE VIEW mgrlist  (
                          first_name
                         ,last_name
                         ,department
                         )
     AS SELECT
          first_name
         ,last_name
         ,deptname
        FROM
          dept
         ,employee
        WHERE
          dept.manager = employee.empnum
   ;


   CREATE INDEX xdeptmgr
     ON dept     (
                   manager
                  )
   ;

   CREATE INDEX xdeptrpt
     ON dept     (
                   rptdept
                  )
   ;

   LABEL CREATE_JOB_TABLE
--   SHOW TABLES JOB
   IF ACTIVITYCOUNT == 1 THEN GOTO CREATE_PROJECT_TABLE;

   CREATE TABLE job      (
                           jobcode         NUMERIC (4) UNSIGNED
                                           NO DEFAULT
                                           NOT NULL
                          ,jobdesc         VARCHAR (18)
                                           DEFAULT ' '
                                           NOT NULL
                          ,PRIMARY KEY     (jobcode)
                          )
   ;

   LABEL CREATE_PROJECT_TABLE
   INVOKE PROJECT;
   IF ERRORCODE != 4082 THEN GOTO BEGIN_INSERTS; 

   CREATE TABLE project  (
                           projcode        NUMERIC (4) UNSIGNED
                                           NO DEFAULT
                                           NOT NULL
                          ,empnum          NUMERIC (4) UNSIGNED
                                           NO DEFAULT
                                           NOT NULL
                          ,projdesc        VARCHAR (18)
                                           DEFAULT ' '
                                           NOT NULL
                          ,start_date      DATE
                                           DEFAULT CURRENT_DATE
                                           NOT NULL
                          ,ship_timestamp  TIMESTAMP
                                           DEFAULT CURRENT_TIMESTAMP
                                           NOT NULL
                          ,est_complete    INTERVAL DAY
                                           DEFAULT INTERVAL '30' DAY
                                           NOT NULL
                          ,PRIMARY KEY     (projcode)
                          )
   ;

-- -------------------------------------------------------------------------------
-- Insert data into the CI_SAMPLE schema
-- -------------------------------------------------------------------------------
LABEL BEGIN_INSERTS
INSERT INTO employee
   VALUES (   1,'ROGER'   ,'GREEN'     ,9000, 100,175500.00 ),
          (  23,'JERRY'   ,'HOWARD'    ,1000, 100,137000.10 ),
          (  29,'JANE'    ,'RAYMOND'   ,3000, 100,136000.00 ),
          (  32,'THOMAS'  ,'RUDLOFF'   ,2000, 100,138000.40 ),
          (  39,'KLAUS '  ,'SAFFERT'   ,3200, 100, 75000.00 ),
          (  43,'PAUL'    ,'WINTER'    ,3100, 100, 90000.00 ),
          (  65,'RACHEL'  ,'MCKAY'     ,4000, 100,118000.00 ),
          (  72,'GLENN'   ,'THOMAS'    ,3300, 100, 80000.00 ),
          (  75,'TIM'     ,'WALKER'    ,3000, 300, 32000.00 ),
          (  87,'ERIC'    ,'BROWN'     ,4000, 400, 89000.00 ),
          (  89,'PETER'   ,'SMITH'     ,3300, 300, 37000.40 ),
          (  93,'DONALD'  ,'TAYLOR'    ,3100, 300, 33000.00 ),
          ( 104,'DAVID'   ,'STRAND'    ,4000, 400, 69000.00 ),
          ( 109,'STEVE'   ,'COOK'      ,4000, 400, 68000.00 ),
          ( 111,'SHERRIE' ,'WONG'      ,3500, 100, 70000.00 ),
          ( 178,'JOHN'    ,'CHOU'      ,3500, 900, 28000.00 ),
          ( 180,'MANFRED' ,'CONRAD'    ,4000, 450, 32000.00 ),
          ( 201,'JIM'     ,'HERMAN'    ,3000, 300, 19000.00 ),
          ( 202,'LARRY'   ,'CLARK'     ,1000, 500, 25000.75 ),
          ( 203,'KATHRYN' ,'HALL'      ,4000, 400, 96000.00 ),
          ( 205,'GINNY'   ,'FOSTER'    ,3300, 900, 30000.00 ),
          ( 206,'DAVE'    ,'FISHER'    ,3200, 900, 25000.00 ),
          ( 207,'MARK'    ,'FOLEY'     ,4000, 420, 33000.00 ),
          ( 208,'SUE'     ,'CRAMER'    ,1000, 900, 19000.00 ),
          ( 209,'SUSAN'   ,'CHAPMAN'   ,1500, 900, 17000.00 ),
          ( 210,'RICHARD' ,'BARTON'    ,1000, 500, 29000.00 ),
          ( 211,'JIMMY'   ,'SCHNEIDER' ,1500, 600, 26000.00 ),
          ( 212,'JONATHAN','MITCHELL'  ,1500, 600, 32000.00 ),
          ( 213,'ROBERT'  ,'WHITE'     ,1500, 100, 90000.00 ),
          ( 214,'JULIA'   ,'KELLY'     ,1000, 500, 50000.00 ),
          ( 215,'WALTER'  ,'LANCASTER' ,4000, 450, 33000.50 ),
          ( 216,'JOHN'    ,'JONES'     ,4000, 450, 40000.00 ),
          ( 217,'MARLENE' ,'BONNY'     ,4000, 900, 24000.90 ),
          ( 218,'GEORGE'  ,'FRENCHMAN' ,4000, 420, 36000.00 ),
          ( 219,'DAVID'   ,'TERRY'     ,2000, 250, 27000.12 ),
          ( 220,'JOHN'    ,'HUGHES'    ,3200, 300, 33000.10 ),
          ( 221,'OTTO'    ,'SCHNABL'   ,3200, 300, 33000.00 ),
          ( 222,'MARTIN'  ,'SCHAEFFER' ,3200, 300, 31000.00 ),
          ( 223,'HERBERT' ,'KARAJAN'   ,3200, 300, 29000.00 ),
          ( 224,'MARIA'   ,'JOSEF'     ,4000, 420, 18000.10 ),
          ( 225,'KARL'    ,'HELMSTED'  ,4000, 450, 32000.00 ),
          ( 226,'HEIDI'   ,'WEIGL'     ,3200, 300, 22000.00 ),
          ( 227,'XAVIER'  ,'SEDLEMEYER',3300, 300, 30000.00 ),
          ( 228,'PETE'    ,'WELLINGTON',3100, 300, 32000.20 ),
          ( 229,'GEORGE'  ,'STRICKER'  ,3100, 300, 32222.00 ),
          ( 230,'ROCKY'   ,'LEWIS'     ,2000, 200, 24000.00 ),
          ( 231,'HERB'    ,'ALBERT'    ,3300, 300, 33000.00 ),
          ( 232,'THOMAS'  ,'SPINNER'   ,4000, 450, 45000.00 ),
          ( 233,'TED'     ,'MCDONALD'  ,2000, 250, 29000.00 ),
          ( 234,'MARY'    ,'MILLER'    ,2500, 100, 56000.00 ),
          ( 235,'MIRIAM'  ,'KING'      ,2500, 900, 18000.00 ),
          ( 321,'BILL'    ,'WINN'      ,2000, 900, 32000.00 ),
          ( 337,'DINAH'   ,'CLARK'     ,9000, 900, 37000.00 ),
          ( 343,'ALAN'    ,'TERRY'     ,3000, 900, 39500.00 ),
          ( 557,'BEN'     ,'HENDERSON' ,4000, 400, 65000.00 ),
          ( 568,'JESSICA' ,'CRINER'    ,3500, 300, 39500.00 ),
          ( 990,'thomas'  ,'stibbs'    ,3500,null,null      ),
          ( 991,'Wayne'   ,'O''Neil'   ,3500,null,null      ),
          ( 992,'Barry'   ,'Kinney'    ,3500,null,null      ),
          ( 993,'Paul'    ,'Buskett'   ,3100,null,null      ),
          ( 994,'Emmy'    ,'Buskett'   ,3100,null,null      ),
          ( 995,'Walt'    ,'Farley'    ,3100,null,null      );


 UPDATE STATISTICS FOR TABLE TRAFODION.CI_SAMPLE.employee ON (last_name,first_name);
-- MAINTAIN TABLE EMPLOYEE, ALL;
---------------------------------------------------------------------

INSERT INTO dept
   VALUES ( 1000,'FINANCE'     ,  23,9000,'CHICAGO'     ),
          ( 1500,'PERSONNEL'   , 213,1000,'CHICAGO'     ),
          ( 2000,'INVENTORY'   ,  32,9000,'LOS ANGELES' ),
          ( 2500,'SHIPPING'    , 234,2000,'PHOENIX'     ),
          ( 3000,'MARKETING'   ,  29,9000,'NEW YORK'    ),
          ( 3100,'CANADA SALES',  43,3000,'TORONTO'     ),
          ( 3200,'GERMNY SALES',  39,3000,'FRANKFURT'   ),
          ( 3300,'ENGLND SALES',  72,3000,'LONDON'      ),
          ( 3500,'ASIA SALES'  , 111,3000,'HONG KONG'   ),
          ( 4000,'RESEARCH'    ,  65,9000,'NEW YORK'    ),
          ( 4100,'PLANNING'    ,  87,4000,'NEW YORK'    ),
          ( 9000,'CORPORATE'   ,   1,9000,'CHICAGO'     );

-- MAINTAIN TABLE DEPT, ALL;

---------------------------------------------------------------------

INSERT INTO job
   VALUES ( 100, 'MANAGER' ),
          ( 200, 'PRODUCTION SUPV' ),
          ( 250, 'ASSEMBLER' ),
          ( 300, 'SALESREP' ),
          ( 400, 'SYSTEM ANALYST' ),
          ( 420, 'ENGINEER' ),
          ( 450, 'PROGRAMMER' ),
          ( 500, 'ACCOUNTANT' ),
          ( 600, 'ADMINISTRATOR' ),
          ( 900, 'SECRETARY' );

-- MAINTAIN TABLE JOB, ALL;

---------------------------------------------------------------------
INSERT INTO project
   VALUES ( 1000,9657,'SALT LAKE CITY'
           ,DATE '1996-04-10'
           ,TIMESTAMP '1996-04-21:08:15:00.00'
           ,INTERVAL '15' DAY ),

          ( 2000,9658,'ROSS PRODUCTS'
           ,DATE '1996-06-10'
           ,TIMESTAMP '1996-07-21:08:30:00.0000'
           ,INTERVAL '30' DAY ),

          ( 2500,9659,'MONTANA TOOLS'
           ,DATE '1996-10-10'
           ,TIMESTAMP '1996-12-21:09:00:00.0000'
           ,INTERVAL '60' DAY ),

          ( 3000,9660,'AHAUS TOOL/SUPPLY'
           ,DATE '1996-08-21'
           ,TIMESTAMP '1996-10-21:08:10:00.0000'
           ,INTERVAL '60' DAY ),

          ( 4000,9661,'THE WORKS'
           ,DATE '1996-09-21'
           ,TIMESTAMP '1996-10-21:10:15:00.0000'
           ,INTERVAL '30' DAY ),

          ( 5000,9662,'THE WORKS'
           ,DATE '1996-09-28'
           ,TIMESTAMP '1996-10-28:09:25:01.1111'
           ,INTERVAL '30' DAY );

--  MAINTAIN TABLE PROJECT, ALL;

----------------------------------------------------------------
 get tables;
 SELECT * FROM employee;

 SELECT * FROM dept;

 SELECT * FROM project;

 SELECT * FROM job;

----------------------------------------------------------------
-- End of sample.sql file
----------------------------------------------------------------
;
