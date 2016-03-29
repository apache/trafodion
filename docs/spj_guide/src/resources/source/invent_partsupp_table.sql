CREATE TABLE trafodion.invent.partsupp
( partnum      NUMERIC (4) UNSIGNED NO DEFAULT NOT NULL
, suppnum      NUMERIC (4) UNSIGNED NO DEFAULT NOT NULL
, partcost     NUMERIC (8, 2)       NO DEFAULT NOT NULL
, qty_received NUMERIC (5) UNSIGNED DEFAULT 0  NOT NULL
, PRIMARY KEY ( partnum, suppnum )
) ;

CREATE INDEX XSUPORD ON partsupp
( suppnum
) ;

CREATE VIEW trafodion.invent.view207
( partnumber
, partdescrpt
, suppnumber
, supplrname
, partprice
, qtyreceived
)
AS SELECT
  x.partnum
, partdesc
, x.suppnum
, suppname
, partcost
, qty_received
FROM
  trafodion.invent.partsupp x
, trafodion.sales.parts p
, trafodion.invent.supplier s
WHERE x.partnum = p.partnum
  AND x.suppnum = s.suppnum
;

CREATE VIEW trafodion.invent.view207n
( partnumber
, partdescrpt
, suppnumber
, supplrname
, partprice
, qtyreceived
)
AS SELECT
  x.partnum
, p.partdesc
, s.suppnum
, s.suppname
, x.partcost
, x.qty_received
FROM trafodion.invent.supplier s
LEFT JOIN trafodion.invent.partsupp x ON s.suppnum = x.suppnum
LEFT JOIN trafodion.sales.parts p     ON x.partnum = p.partnum
;

CREATE VIEW trafodion.invent.viewcust
( custnumber
, cusname
, ordernum
)
AS SELECT
  c.custnum
, c.custname
, o.ordernum
FROM trafodion.sales.customer c
LEFT JOIN trafodion.sales.orders o ON c.custnum = o.custnum
;

CREATE VIEW trafodion.invent.viewcs AS SELECT
  custname
FROM trafodion.sales.customer
UNION SELECT
  suppname
FROM trafodion.invent.supplier ;

INSERT INTO trafodion.invent.partsupp VALUES
  (  212,  1, 2000.00,  20 )
, (  212,  3, 1900.00,  35 )
, (  244,  1, 2400.00,  50 )
, (  244,  2, 2200.00,  66 )
, (  255,  1, 3300.00,  35 )
, (  255,  3, 3000.00,  46 )
, ( 2001,  1,  700.00, 100 )
, ( 2001,  2,  750.00,  55 )
, ( 2002,  1, 1000.00, 120 )
, ( 2002,  6, 1100.00,  20 )
, ( 2003,  1, 1300.00, 100 )
, ( 2003,  2, 1400.00,  50 )
, ( 2003, 10, 1450.00,  50 )
, ( 2402,  1,  200.00,  35 )
, ( 2403,  1,  300.00, 200 )
, ( 2405,  1,  500.00,  40 )
, ( 2405,  6,  450.00,  50 )
, ( 3103,  1, 3200.00, 200 )
, ( 3103, 15, 3300.00, 100 )
, ( 3201,  1,  380.00,  36 )
, ( 3205,  1,  425.00, 150 )
, ( 3210,  6,  470.00,  10 )
, ( 3210, 15,  450.00,  25 )
, ( 4102,  6,   20.00, 115 )
, ( 4102,  8,   19.00, 140 )
, ( 4102, 15,   21.00,  30 )
, ( 5100,  6,  100.00,  50 )
, ( 5100,  8,  105.00,  40 )
, ( 5100, 15,   95.00,  60 )
, ( 5101,  8,  135.00,  33 )
, ( 5101, 15,  125.00,  43 )
, ( 5103,  8,  265.00,  20 )
, ( 5103, 15,  250.00,  58 )
, ( 5110,  1,  335.00, 100 )
, ( 5110,  2,  350.00,  36 )
, ( 5504,  2,   85.00,  10 )
, ( 5504,  6,   75.00,  10 )
, ( 5504, 15,   78.00,  10 )
, ( 5505, 15,  200.00, 100 )
, ( 6201,  1,  100.00, 110 )
, ( 6301,  1,  150.00, 230 )
, ( 6400,  1,  390.00,  50 )
, ( 6401,  2,  500.00,  20 )
, ( 6401,  3,  480.00,  38 )
, ( 6500,  2,   60.00, 140 )
, ( 6500,  3,   65.00,  32 )
, ( 6603,  2,   25.00, 150 )
, ( 7102, 10,  165.00, 100 )
, ( 7301,  1,  300.00,  32 )
;

UPDATE STATISTICS FOR TABLE trafodion.invent.partsupp ON EVERY COLUMN ;
