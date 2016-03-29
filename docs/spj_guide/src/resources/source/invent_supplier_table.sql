CREATE TABLE trafodion.invent.supplier
( suppnum  NUMERIC (4) UNSIGNED NO DEFAULT NOT NULL
, suppname CHARACTER (18)       NO DEFAULT NOT NULL
, street   CHARACTER (22)       NO DEFAULT NOT NULL
, city     CHARACTER (14)       NO DEFAULT NOT NULL
, state    CHARACTER (12)       NO DEFAULT NOT NULL
, postcode CHARACTER (10)       NO DEFAULT NOT NULL
, PRIMARY KEY ( suppnum )
) ;

CREATE INDEX xsuppnam ON supplier
( suppname
) ;

INSERT INTO trafodion.invent.supplier VALUES
  (   1, 'NEW COMPUTERS INC',    '1800 KING ST.',         'SAN FRANCISCO', 'CALIFORNIA',   '94112' )
, (   2, 'DATA TERMINAL INC',    '2000 BAKER STREET',     'LAS VEGAS',     'NEVADA',       '66134' )
, (   3, 'HIGH DENSITY INC',     '7600 EMERSON',          'NEW YORK',      'NEW YORK',     '10230' )
, (   6, 'MAGNETICS INC',        '1000 INDUSTRY DRIVE',   'LEXINGTON',     'MASS',         '02159' )
, (   8, 'ATTRACTIVE CORP',      '7777 FOUNTAIN WAY',     'CHICAGO',       'ILLINOIS',     '60610' )
, (  10, 'LEVERAGE INC',         '6000 LINCOLN LANE',     'DENVER',        'COLORADO',     '80712' )
, (  15, 'DATADRIVE CORP',       '100 MAC ARTHUR',        'DALLAS',        'TEXAS',        '75244' )
, (  20, 'Macadam''S PC''s',     '106 River Road',        'New Orleans',   'Louisiana',    '67890' )
, (  25, 'Schroeder''s Ltd',     '212 Strasse Blvd West', 'Hamburg',       'Rhode Island', '22222' )
, (  30, 'O''Donnell''s Drives', '729 West Palm Beach ',  'San Antonio',   'Texas',        '78344' )
, (  35, 'Mac''Murphys PC''s',   '93323 Alemeda',         'Menlo Park',    'California',   '94025' )
, (  36, 'MAC''MURPHYS PCB''s',  '93323 Alemeda Suite B', 'Menlo Park',    'California',   '94025' )
, (  90, 'laser jets inc',       '284 blue ridge way',    'levittown',     'penna.',       '09520' )
, (  92, 'watercolors',          '84 north grand avenue', 'menlo park',    'california',   '94025' )
, (  95, 'application do''ers',  '2846 yellowwood drive', 'wayland',       'mass',         '02158' )
, (  99, 'terminals, inc.',      '2 longfellow way',      'heightstown',   'nj',           '08520' )
, ( 186, '186 Disk Makers',      '186 Dis Way',           'Dat Way',       'Wisconsin',    '00186' )
;

UPDATE STATISTICS FOR TABLE trafodion.invent.supplier ON EVERY COLUMN ;
