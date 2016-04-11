CREATE TABLE trafodion.persnl.job
( jobcode NUMERIC (4)  UNSIGNED NO DEFAULT  NOT NULL
, jobdesc VARCHAR (18)          DEFAULT ' ' NOT NULL
, PRIMARY KEY ( jobcode )
) ;

INSERT INTO trafodion.persnl.job VALUES
  ( 100, 'MANAGER' )
, ( 200, 'PRODUCTION SUPV' )
, ( 250, 'ASSEMBLER' )
, ( 300, 'SALESREP' )
, ( 400, 'SYSTEM ANALYST' )
, ( 420, 'ENGINEER' )
, ( 450, 'PROGRAMMER' )
, ( 500, 'ACCOUNTANT' )
, ( 600, 'ADMINISTRATOR' )
, ( 900, 'SECRETARY' )
;

UPDATE STATISTICS FOR TABLE trafodion.persnl.job ON EVERY COLUMN ;
