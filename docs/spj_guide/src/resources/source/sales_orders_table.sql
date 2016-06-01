CREATE TABLE trafodion.sales.orders
( ordernum   NUMERIC (6) UNSIGNED NO DEFAULT                NOT NULL
, order_date DATE                 DEFAULT DATE '2011-07-01' NOT NULL
, deliv_date DATE                 DEFAULT DATE '2011-08-01' NOT NULL
, salesrep   NUMERIC (4) UNSIGNED DEFAULT 0                 NOT NULL
, custnum    NUMERIC (4) UNSIGNED NO DEFAULT                NOT NULL
, PRIMARY KEY ( ordernum )
) ;

ALTER TABLE trafodion.sales.orders
   ADD CONSTRAINT trafodion.sales.date_constrnt CHECK ( deliv_date >= order_date )
   ;

CREATE INDEX xordrep ON orders
( salesrep
) ;

CREATE INDEX xordcus ON orders
( custnum
) ;

CREATE VIEW trafodion.sales.ordrep AS SELECT
  empnum
, last_name
, ordernum
, o.custnum
FROM
  trafodion.persnl.employee e
, trafodion.sales.orders o
, trafodion.sales.customer c
WHERE e.empnum = o.salesrep
  AND o.custnum = C.custnum
;

CREATE INDEX xcustnam ON customer
(
custname
) ;

CREATE VIEW trafodion.sales.custlist AS SELECT
  custnum
, custname
, street
, city
, state
, postcode
FROM trafodion.sales.customer
;

INSERT INTO trafodion.sales.orders VALUES
  ( 100210, DATE '2011-04-10', DATE '2011-04-10', 220, 1234 )
, ( 100250, DATE '2011-01-23', DATE '2011-06-15', 220, 7777 )
, ( 101220, DATE '2011-07-21', DATE '2011-12-15', 221, 5635 )
, ( 200300, DATE '2011-02-06', DATE '2011-07-01', 222,  926 )
, ( 200320, DATE '2011-02-17', DATE '2011-07-20', 223,   21 )
, ( 200490, DATE '2011-03-19', DATE '2011-11-01', 226,  123 )
, ( 300350, DATE '2011-03-03', DATE '2011-08-10', 231,  543 )
, ( 300380, DATE '2011-03-19', DATE '2011-08-20', 226,  123 )
, ( 400410, DATE '2011-03-27', DATE '2011-09-01', 227, 7654 )
, ( 500450, DATE '2011-04-20', DATE '2011-09-15', 220,  324 )
, ( 600480, DATE '2011-05-12', DATE '2011-10-10', 226, 3333 )
, ( 700510, DATE '2011-06-01', DATE '2011-10-20', 229,  143 )
, ( 800660, DATE '2011-10-09', DATE '2011-11-01', 568, 3210 )
;

UPDATE STATISTICS FOR TABLE trafodion.sales.orders ON EVERY COLUMN ;
