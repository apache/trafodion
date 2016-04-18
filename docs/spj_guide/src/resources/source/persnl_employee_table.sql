CREATE TABLE trafodion.persnl.employee
( empnum     NUMERIC (4)    UNSIGNED NO DEFAULT  NOT NULL
, first_name CHARACTER (15)          DEFAULT ' ' NOT NULL
, last_name  CHARACTER (20)          DEFAULT ' ' NOT NULL
, deptnum    NUMERIC (4)    UNSIGNED NO DEFAULT  NOT NULL
, jobcode    NUMERIC (4)    UNSIGNED DEFAULT     NULL
, salary     NUMERIC (8, 2) UNSIGNED DEFAULT     NULL
, PRIMARY KEY ( empnum )
) ;

ALTER TABLE trafodion.persnl.employee
   ADD CONSTRAINT empnum_constrnt CHECK ( empnum BETWEEN 0001 AND 9999 )
   ;

CREATE INDEX xempname ON employee
( last_name
, first_name
) ;

CREATE INDEX xempdept ON employee
( deptnum
) ;

CREATE VIEW trafodion.persnl.emplist AS
SELECT
  empnum
, first_name
, last_name
, deptnum
, jobcode
FROM employee
;

INSERT INTO trafodion.persnl.employee VALUES
  (   1, 'ROGER',    'GREEN',      9000, 100,  175500.00 )
, (  23, 'JERRY',    'HOWARD',     1000, 100,  137000.10 )
, (  29, 'JANE',     'RAYMOND',    3000, 100,  136000.00 )
, (  32, 'THOMAS',   'RUDLOFF',    2000, 100,  138000.40 )
, (  39, 'KLAUS ',   'SAFFERT',    3200, 100,   75000.00 )
, (  43, 'PAUL',     'WINTER',     3100, 100,   90000.00 )
, (  65, 'RACHEL',   'MCKAY',      4000, 100,  118000.00 )
, (  72, 'GLENN',    'THOMAS',     3300, 100,   80000.00 )
, (  75, 'TIM',      'WALKER',     3000, 300,   32000.00 )
, (  87, 'ERIC',     'BROWN',      4000, 400,   89000.00 )
, (  89, 'PETER',    'SMITH',      3300, 300,   37000.40 )
, (  93, 'DONALD',   'TAYLOR',     3100, 300,   33000.00 )
, ( 104, 'DAVID',    'STRAND',     4000, 400,   69000.00 )
, ( 109, 'STEVE',    'COOK',       4000, 400,   68000.00 )
, ( 111, 'SHERRIE',  'WONG',       3500, 100,   70000.00 )
, ( 178, 'JOHN',     'CHOU',       3500, 900,   28000.00 )
, ( 180, 'MANFRED',  'CONRAD',     4000, 450,   32000.00 )
, ( 201, 'JIM',      'HERMAN',     3000, 300,   19000.00 )
, ( 202, 'LARRY',    'CLARK',      1000, 500,   25000.75 )
, ( 203, 'KATHRYN',  'HALL',       4000, 400,   96000.00 )
, ( 205, 'GINNY',    'FOSTER',     3300, 900,   30000.00 )
, ( 206, 'DAVE',     'FISHER',     3200, 900,   25000.00 )
, ( 207, 'MARK',     'FOLEY',      4000, 420,   33000.00 )
, ( 208, 'SUE',      'CRAMER',     1000, 900,   19000.00 )
, ( 209, 'SUSAN',    'CHAPMAN',    1500, 900,   17000.00 )
, ( 210, 'RICHARD',  'BARTON',     1000, 500,   29000.00 )
, ( 211, 'JIMMY',    'SCHNEIDER',  1500, 600,   26000.00 )
, ( 212, 'JONATHAN', 'MITCHELL',   1500, 600,   32000.00 )
, ( 213, 'ROBERT',   'WHITE',      1500, 100,   90000.00 )
, ( 214, 'JULIA',    'KELLY',      1000, 500,   50000.00 )
, ( 215, 'WALTER',   'LANCASTER',  4000, 450,   33000.50 )
, ( 216, 'JOHN',     'JONES',      4000, 450,   40000.00 )
, ( 217, 'MARLENE',  'BONNY',      4000, 900,   24000.90 )
, ( 218, 'GEORGE',   'FRENCHMAN',  4000, 420,   36000.00 )
, ( 219, 'DAVID',    'TERRY',      2000, 250,   27000.12 )
, ( 220, 'JOHN',     'HUGHES',     3200, 300,   33000.10 )
, ( 221, 'OTTO',     'SCHNABL',    3200, 300,   33000.00 )
, ( 222, 'MARTIN',   'SCHAEFFER',  3200, 300,   31000.00 )
, ( 223, 'HERBERT',  'KARAJAN',    3200, 300,   29000.00 )
, ( 224, 'MARIA',    'JOSEF',      4000, 420,   18000.10 )
, ( 225, 'KARL',     'HELMSTED',   4000, 450,   32000.00 )
, ( 226, 'HEIDI',    'WEIGL',      3200, 300,   22000.00 )
, ( 227, 'XAVIER',   'SEDLEMEYER', 3300, 300,   30000.00 )
, ( 228, 'PETE',     'WELLINGTON', 3100, 300,   32000.20 )
, ( 229, 'GEORGE',   'STRICKER',   3100, 300,   32222.00 )
, ( 230, 'ROCKY',    'LEWIS',      2000, 200,   24000.00 )
, ( 231, 'HERB',     'ALBERT',     3300, 300,   33000.00 )
, ( 232, 'THOMAS',   'SPINNER',    4000, 450,   45000.00 )
, ( 233, 'TED',      'MCDONALD',   2000, 250,   29000.00 )
, ( 234, 'MARY',     'MILLER',     2500, 100,   56000.00 )
, ( 235, 'MIRIAM',   'KING',       2500, 900,   18000.00 )
, ( 321, 'BILL',     'WINN',       2000, 900,   32000.00 )
, ( 337, 'DINAH',    'CLARK',      9000, 900,   37000.00 )
, ( 343, 'ALAN',     'TERRY',      3000, 900,   39500.00 )
, ( 557, 'BEN',      'HENDERSON',  4000, 400,   65000.00 )
, ( 568, 'JESSICA',  'CRINER',     3500, 300,   39500.00 )
, ( 990, 'THOMAS',   'STIBBS',     3500, NULL,      NULL )
, ( 991, 'WAYNE',    'O''NEIL',    3500, NULL,      NULL )
, ( 992, 'BARRY',    'KINNEY',     3500, NULL,      NULL )
, ( 993, 'PAUL',     'BUSKETT',    3100, NULL,      NULL )
, ( 994, 'EMMY',     'BUSKETT',    3100, NULL,      NULL )
, ( 995, 'WALT',     'FARLEY',     3100, NULL,      NULL )
;

UPDATE STATISTICS FOR TABLE trafodion.persnl.employee ON EVERY COLUMN ;
