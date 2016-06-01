CREATE TABLE trafodion.persnl.dept
( deptnum  NUMERIC (4) UNSIGNED NO DEFAULT  NOT NULL
, deptname CHARACTER (12)       NO DEFAULT  NOT NULL
, manager  NUMERIC (4) UNSIGNED NO DEFAULT  NOT NULL
, rptdept  NUMERIC (4) UNSIGNED DEFAULT 0   NOT NULL
, location VARCHAR (18)         DEFAULT ' ' NOT NULL
, PRIMARY KEY ( deptnum )
) ;

CREATE INDEX xdeptmgr ON dept
( manager
) ;

CREATE INDEX xdeptrpt ON dept
( rptdept
) ;

ALTER TABLE trafodion.persnl.dept
   ADD CONSTRAINT mgrnum_constrnt
   CHECK (manager BETWEEN 0000 AND 9999)
   ;

ALTER TABLE trafodion.persnl.dept
   ADD CONSTRAINT deptnum_constrnt
      CHECK ( deptnum IN
              ( 1000
              , 1500
              , 2000
              , 2500
              , 3000
              , 3100
              , 3200
              , 3300
              , 3500
              , 4000
              , 4100
              , 9000
              )
           ) 
           ;
3
CREATE VIEW trafodion.persnl.mgrlist
( first_name
, last_name
, department
)
AS SELECT
  first_name
, last_name
, deptname
FROM dept, employee
WHERE dept.manager = employee.empnum
;

INSERT INTO trafodion.persnl.dept VALUES
  ( 1000, 'FINANCE',       23, 9000, 'CHICAGO'     )
, ( 1500, 'PERSONNEL',    213, 1000, 'CHICAGO'     )
, ( 2000, 'INVENTORY',     32, 9000, 'LOS ANGELES' )
, ( 2500, 'SHIPPING',     234, 2000, 'PHOENIX'     )
, ( 3000, 'MARKETING',     29, 9000, 'NEW YORK'    )
, ( 3100, 'CANADA SALES',  43, 3000, 'TORONTO'     ) 
, ( 3200, 'GERMNY SALES',  39, 3000, 'FRANKFURT'   )
, ( 3300, 'ENGLND SALES',  72, 3000, 'LONDON'      )
, ( 3500, 'ASIA SALES',   111, 3000, 'HONG KONG'   )
, ( 4000, 'RESEARCH',      65, 9000, 'NEW YORK'    )
, ( 4100, 'PLANNING',      87, 4000, 'NEW YORK'    )
, ( 9000, 'xxCORPORATE',    1, 9000, 'CHICAGO'     )
;

UPDATE STATISTICS FOR TABLE trafodion.persnl.dept ON EVERY COLUMN ;
