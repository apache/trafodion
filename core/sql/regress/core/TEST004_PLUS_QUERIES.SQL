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

?SECTION  NOTIN_CQDOFF

CQD IGNORE_NOT_IN_ANSI_NULL_SEMANTICS 'OFF';

----------------------------------------------------------------------------
--NOT NULLABLE, NOT NULLABLE
-- all joins ==> a1=b1 with no cancel expression
prepare s from 
select *  from ta where a1 not in (select b1 from tb) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--NOT NULLABLE, NULLABLE with no NULL values
-- if hash join ==> a1=b2 with cancel expression
-- otherwise ==>NOT(a1<>b2) IS TRUE)
prepare s from 
select *  from ta where a1 not in (select b2 from tb) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--NOT NULLABLE, NULLABLE with NULL values
--NOT NULLABLE, NULLABLE with no NULL values
-- if hash join ==> a1=b4 with cancel expression
-- otherwise ==>NOT(a1<>b4) IS TRUE)

prepare s from 
select *  from ta where a1 not in (select b4 from tb) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--NULLABLE with NULL values, NOT NULLABLE
-- all joins ==> NOT((a2<>b1) IS TRUE)
prepare s from 
select *  from ta where a2 not in (select b1 from tb) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--NULLABLE with NULL values, NULLABLE with no NULL values
-- all joins ==> NOT((a2<>b2) IS TRUE)
prepare s from 
select *  from ta where a2 not in (select b2 from tb) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--NULLABLE with NULL values, NULLABLE with NULL values
-- all joins ==> NOT((a2<>b1) IS TRUE)
prepare s from 
select *  from ta where a4 not in (select b4 from tb) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--NULLABLE with NULL filtering, NULLABLE with NO NULL filtering
-- if hash join ==> a4=b4 with cancel expression
-- otherwise ==>NOT(a4<>b4) IS TRUE)
prepare s from 
select *  from ta where a4 not in (select b4 from tb) and a4 >0 order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;

----------------------------------------------------------------------------
--NULLABLE with NULL values, NULLABLE with NULL filtering
-- all joins ==> NOT((a4<>b4) IS TRUE)
prepare s from 
select *  from ta where a4 not in (select b4 from tb where b4>0) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--NULLABLE with  NULL filtering, NULLABLE with  NULL filtering
-- all joins ==> a4=b4 
prepare s from 
select *  from ta where a4 not in (select b4 from tb where b4>0) and a4>0 order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--NULLABLE with  NULL values, EMPTY NULLABLE with  NULL filtering
-- all joins ==> NOT((a4<>b4) IS TRUE)
prepare s from 
select *  from ta where a4 not in (select b4 from tb where b4<0) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--NULLABLE with NULL filtering, EMPTY NULLABLE with  NULL filtering
-- all joins ==> a4 = b4
prepare s from 
select *  from ta where a4 not in (select b4 from tb where b4<0) and a4>0 order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--Multi_column #1 
-- all joins ==> a1=b1 and not (((a2<>b2 ) or (a3<>b3) or a4<>b4)) is true) 
prepare s from 
select *  from ta where a1,a2,a3,a4 not in (select b1,b2,b3,b4 from tb where b2 > 2 and b3 >3 ) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--Multi_column #2
-- all joins ==> a1=b1 and a2=b2 and not (((a3<>b3) or a4<>b4)) is true)
prepare s from 
select *  from ta where a1,a2,a3,a4 not in (select b1,b2,b3,b4 from tb where b2 > 2 and b3 >3 ) and a2 > 0 order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--Multi_column #3
-- all joins ==>  not (((a2<>b2 ) or (a3<>b3) or a4<>b4)) is true)
prepare s from 
select *  from ta where a2,a3,a4 not in (select b2,b3,b4 from tb ) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
--Multi_column #4
-- all joins ==> a2=b2  and a3=b3 and a4=b4 
prepare s from 
select *  from ta where a2,a3,a4 not in (select b2,b3,b4 from tb 
where b2>0 and b3>0 and b4 > 0) and 
a2>0 and a3>0 and a4 >0 order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------

?SECTION NOTIN_PLUS

-- NOTIN_PLUS #1
-- (ta,tb) ==> not((a4<>b4) is true)
-- (ta,tc) ==> not((a4<>c4) is true)
prepare s from 
select *  
from ta 
where 
a4 not in (select b4 from tb) and 
a4 not in (select c4 from tc)
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;

----------------------------------------------------------------------------
-- NOTIN_PLUS #2
-- (ta,tb) ==> if hash a4=b4 with cancel expression otherwise not((a4<>b4) is true)
-- (ta,tc) ==> if hash a4=c4 with cancel expression otherwise not((a4<>c4) is true)
prepare s from 
select *  
from ta 
where 
a4 not in (select b4 from tb) and 
a4 not in (select c4 from tc) and 
a4>0
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;

----------------------------------------------------------------------------
-- NOTIN_PLUS #3
-- (ta,tb) ==>  a4=b4 
-- (ta,tc) ==> if hash a4=c4 with cancel expression otherwise not((a4<>c4) is true)
prepare s from 
select *  
from ta 
where 
a4 not in (select b4 from tb where b4 >0 ) and 
a4 not in (select c4 from tc) and
a4>0
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
-- NOTIN_PLUS #4
-- (ta,tb) ==> a4=b4 
-- (ta,tc) ==> a4=c4 
prepare s from 
select *  
from ta 
where 
a4 not in (select b4 from tb where b4 >0) and 
a4 not in (select c4 from tc where c4 >0) and
a4>0
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
-- NOTIN_PLUS #5
-- (ta,tb) ==> not((a4<>b4) is true)
-- (ta,tc) ==> not((a4<>c4) is true)
prepare s from 
select *  
from ta 
where 
a4 not in (select b4 from tb where b4 >0) and 
a4 not in (select c4 from tc where c4 >0)
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
-- NOTIN_PLUS #6
prepare s from 
select *  
from ta 
where (select max(b4) from tb) not in (select c4 from tc where c4 >0)
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
-- NOTIN_PLUS #6-1
prepare s from 
select *  
from ta 
where (select max(b4) from tb) not in (select max(c4) from tc where c4 >0)
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
-- NOTIN_PLUS #7
prepare s from 
select *  
from ta 
where (select max(b4) from tb where ta.a4=tb.b4) not in (select c4 from tc where c4>0)
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
-- NOTIN_PLUS #8
prepare s from 
select *  
from ta 
where a4 not in (select b4 from tb where b4  not in (select c4 from tc where c4>0))
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
-- NOTIN_PLUS #8-1
prepare s from 
select *  
from ta 
where a4 not in (select b4 from tb where b4  not in (select c4 from tc where c4>0))
and a4 >0
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
-- NOTIN_PLUS #9
prepare s from 
select *  
from ta 
where a4 not in (select c4 from tc where c4  not in (select b4 from tb))
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
-- NOTIN_PLUS #10
prepare s from 
select *  
from ta 
where a4+1 not in (select c4+1 from tc)
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
-- NOTIN_PLUS #10
prepare s from 
select *  
from ta 
where (select a4,sum(a1) from ta where a4 >0 group by a4) not in (select c4, sum(c1) from tc where c4 >0 group by c4)
order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------

?SECTION  NOTIN_CQDON_SB
CQD IGNORE_NOT_IN_ANSI_NULL_SEMANTICS 'ON';
prepare s from 
select *  from ta where  a4 not in (select b4 from tb) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
prepare s from 
select *  from ta where a1,a2,a3,a4 not in (select b1,b2,b3,b4 from tb) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;

----------------------------------------------------------------------------

?SECTION  NOTIN_CQDON_REF
CQD IGNORE_NOT_IN_ANSI_NULL_SEMANTICS 'ON';
prepare s from 
select *  from ta where  not exists (select 1 from tb where a4=b4) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
----------------------------------------------------------------------------
prepare s from 
select *  from ta where  not exists (select 1 from tb where a1,a2,a3,a4=b1,b2,b3,b4 ) order by a1;

execute s;  
explain options 'f'  s;

execute explainASJ;
?SECTION   setup

-----------------NULLABLE INNER COLUMNS 
-----------------(NULLABLE OUTER ALSO) 

drop  table ta;
drop  table tb;
drop  table tc;

create table ta ( a1 int NOT NULL, a2 int,a3 int , a4 int , primary key (a1))
  location $data2
  hash2 partition
  (
    add location $data3
  );

create table tb ( b1 int NOT NULL, b2 int, b3 int, b4 int, primary key (b1) )
  location $data2
  hash2 partition
  (
    add location $data3
  );

create table tc ( c1 int NOT NULL, c2 int, c3 int, c4 int, primary key (c1) )
  location $data2
  hash2 partition
  (
    add location $data3
  );
  
insert into tb(b1) values (1),(2),(3),(4),(5),(6),(7),(8),(9);
insert into tb(b1) select b1+(select max(b1)+1 from tb) from tb;
insert into tb(b1) select b1+(select max(b1)+1 from tb) from tb;
insert into tb(b1) select b1+(select max(b1)+1 from tb) from tb;
insert into tb(b1) select b1+(select max(b1)+1 from tb) from tb;
update tb set b2=b1, b3=b1 , b4 =b1;

update tb set b2 = 20000,b3 = 20000,b4 = 20000 where b1 < 50;
update tb set b2 = 20001,b3 = 20001,b4 = 20001 where b1 between  51 and 100;

insert into tc select b1+100, b2 +100, b3 + 100, b4 + 100 from tb where b1 >(select max(b1)-5 from tb)
and b1 < (select max(b1)+5 from tb);
insert into tc select (select max(c1)+1 from tc),(select max(c2)+1 from tc),(select max(c3)+1 from tc),null from (values(1)) t;

insert into ta select * from tb where b1 > (select min(b1)+2 from tb);
insert into ta select * from tc where c1 > (select min(c1)+1 from tc) and c1 < (select max(c1)-2 from tc);

insert into ta select x,x,x,x from (values(100000 ),(100001),(100002)) t(x);
insert into ta select (select max(a1)+1 from ta), null, null,null from (values(1)) t;
 
insert into tb select (select max(b1)+1 from tb),(select max(b2)+1 from tb),(select max(b3)+1 from tb),null from (values(1)) t;
insert into tb select (select max(b1)+1 from tb),(select max(b2)+1 from tb),(select max(b3)+1 from tb),null from (values(1)) t;

update statistics for table ta on every column;
update statistics for table tb on  every column;
update statistics for table tc on  every column;

select count(*),  count(*)-count(a1),count(*)-count(a2),count(*)-count(a3),count(*)-count(a4) from ta;
select count(*), count(*)-count(b1),count(*)-count(b2),count(*)-count(b3),count(*)-count(b4) from tb;
select count(*), count(*)-count(c1),count(*)-count(c2),count(*)-count(c3),count(*)-count(c4) from tc;

----------------------------------------------------------------------------
----------------------------------------------------------------------------

-- serial -- non parallel
-- NULLS coming for inner side

control query shape cut;
select count(*) from ta;
select count(*) from tb;

select count(*) from tb where b4 is null;
