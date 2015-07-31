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

drop table customer_ddl;
create external table customer_ddl
(
    c_customer_sk             int,
    c_customer_id             string,
    c_current_cdemo_sk        int,
    c_current_hdemo_sk        int,
    c_current_addr_sk         int,
    c_first_shipto_date_sk    int,
    c_first_sales_date_sk     int,
    c_salutation              string,
    c_first_name              string,
    c_last_name               string,
    c_preferred_cust_flag     string,
    c_birth_day               int,
    c_birth_month             int,
    c_birth_year              int,
    c_birth_country           string,
    c_login                   string,
    c_email_address           string,
    c_last_review_date        string
)
row format delimited fields terminated by '|'
location '/user/hive/exttables/customer_ddl';

drop table customer_temp;
create external table customer_temp
(
    c_customer_sk             int,
    c_customer_id             string,
    c_current_cdemo_sk        int,
    c_current_hdemo_sk        int,
    c_current_addr_sk         int,
    c_first_shipto_date_sk    int,
    c_first_sales_date_sk     int,
    c_salutation              string,
    c_first_name              string,
    c_last_name               string,
    c_preferred_cust_flag     string,
    c_birth_day               int,
    c_birth_month             int,
    c_birth_year              int,
    c_birth_country           string,
    c_login                   string,
    c_email_address           string,
    c_last_review_date        string
)
row format delimited fields terminated by '|'
location '/user/hive/exttables/customer_temp';

drop table customer_bp;
create table customer_bp
(
    c_customer_sk             int,
    c_customer_id             string,
    c_current_cdemo_sk        int,
    c_current_hdemo_sk        int,
    c_current_addr_sk         int,
    c_first_shipto_date_sk    int,
    c_first_sales_date_sk     int,
    c_salutation              string,
    c_first_name              string,
    c_last_name               string,
    --c_preferred_cust_flag     string, -- partitioning key
    c_birth_day               int,
    c_birth_month             int,
    c_birth_year              int,
    c_birth_country           string,
    c_login                   string,
    c_email_address           string,
    c_last_review_date        string
)
partitioned by (c_preferred_cust_flag string)
clustered by (c_customer_sk) sorted by (c_customer_sk) into 4 buckets
row format delimited fields terminated by '|';

drop table tbl_utf8;
create external table tbl_utf8
(
    id           int,
    chapter      string,
    english      string,
    translator   string
)
row format delimited fields terminated by '|'
location '/user/hive/exttables/tbl_utf8';

drop table tbl_utf8_temp;
create table tbl_utf8_temp
(
    id           int,
    chapter      string,
    english      string,
    translator   string
)
row format delimited fields terminated by '|';

drop table tbl_utf8p;
create table tbl_utf8p
(
    id           int,
    chapter      string,
    english      string
)
partitioned by (translator string)
row format delimited fields terminated by '|';

drop table tbl_type;
create external table tbl_type
(
     tint        tinyint,
     sm          smallint,
     i           int,
     big         bigint,
     str         string,
     f           float,
     d           double,
     t           timestamp
)
row format delimited fields terminated by '|'
location '/user/hive/exttables/tbl_type';

drop table tbl_type_temp;
create table tbl_type_temp
(
     tint        tinyint,
     sm          smallint,
     i           int,
     big         bigint,
     str         string,
     f           float,
     d           double,
     t           timestamp
)
row format delimited fields terminated by '|';
