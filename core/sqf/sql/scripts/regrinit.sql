-- initializes environment needed for sql dev regressions
upsert into TRAFODION."_MD_".DEFAULTS
     values
     ('SCHEMA ', 'TRAFODION.SCH ', 'inserted during seabase regressions run', 0);

initialize authorization;

register user sql_user1 as sql_user1;
register user sql_user2 as sql_user2;
register user sql_user3 as sql_user3;
register user sql_user4 as sql_user4;
register user sql_user5 as sql_user5;
register user sql_user6 as sql_user6;
register user sql_user7 as sql_user7;
register user sql_user8 as sql_user8;
register user sql_user9 as sql_user9;
register user sql_user10 as sql_user10;

