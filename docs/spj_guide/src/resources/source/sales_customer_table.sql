CREATE TABLE trafodion.sales.customer
( custnum  NUMERIC (4) UNSIGNED NO DEFAULT   NOT NULL
, custname CHARACTER (18)       NO DEFAULT   NOT NULL
, street   CHARACTER (22)       NO DEFAULT   NOT NULL
, city     CHARACTER (14)       NO DEFAULT   NOT NULL
, state    CHARACTER (12)       DEFAULT ' '  NOT NULL
, postcode CHARACTER (10)       NO DEFAULT   NOT NULL
, credit   CHARACTER (2)        DEFAULT 'C1' NOT NULL
, PRIMARY KEY ( custnum )
) ;

INSERT INTO trafodion.sales.customer VALUES
  (   21, 'CENTRAL UNIVERSITY', 'UNIVERSITY WAY',        'PHILADELPHIA',  'PENNSYLVANIA', '19104',    'A1' )
, (  123, 'BROWN MEDICAL CO',   '100 CALIFORNIA STREET', 'SAN FRANCISCO', 'CALIFORNIA',   '94944',    'C2' )
, (  143, 'STEVENS SUPPLY',     '2020 HARRIS STREET',    'DENVER',        'COLORADO',     '80734',    'A2' )
, (  324, 'PREMIER INSURANCE',  '3300 WARBASH',          'LUBBOCK',       'TEXAS',        '76308',    'A1' )
, (  543, 'FRESNO STATE BANK',  '2300 BROWN BLVD',       'FRESNO',        'CALIFORNIA',   '93921',    'B3' )
, (  926, 'METALL-AG.',         '12 WAGNERRING',         'FRANKFURT',     'WEST GERMANY', '34',       'D4' )
, ( 1234, 'DATASPEED',          '300 SAN GABRIEL WAY',   'NEW YORK',      'NEW YORK',     '10014',    'C1' )
, ( 3210, 'BESTFOOD MARKETS',   '3333 PHELPS STREET',    'LINCOLN',       'NEBRASKA',     '68134',    'A4' )
, ( 3333, 'NATIONAL UTILITIES', '6500 TRANS-CANADIENNE', 'QUEBEC',        'CANADA',       'H4T 1X4',  'A1' )
, ( 5635, 'ROYAL CHEMICALS',    '45 NEW BROAD STREET',   'LONDON',        'ENGLAND',      'EC2M 1NH', 'B2' )
, ( 7654, 'MOTOR DISTRIBUTING', '2345 FIRST STREET',     'CHICAGO',       'ILLINOIS',     '60610',    'E4' )
, ( 7777, 'SLEEPWELL HOTELS',   '9000 PETERS AVENUE',    'DALLAS',        'TEXAS',        '75244',    'B1' )
, ( 9000, 'BUNKNOUGHT INN',     '4738 RALPH STREET',     'BAYONNE',       'NEW JERSEY',   '09520',    'C1' )
, ( 9010, 'HOTEL OREGON',       '333 PORTLAND AVE.',     'MEDFORD',       'OREGON',       '97444',    'C2' )
, ( 9033, 'ART SUPPLIES, INC.', '22 SWEET ST.',          'PITTSBURGH',    'PENNA.',       '08333',    'C3' )
;

UPDATE STATISTICS FOR TABLE trafodion.sales.customer ON EVERY COLUMN;
