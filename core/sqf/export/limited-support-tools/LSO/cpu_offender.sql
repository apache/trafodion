-- set param ?filter 'CPU_OFFENDER=-1' ;
select current_timestamp "CURRENT_TIMESTAMP"  -- (1) Now
       ,count(*) no_of_processes              -- (2) Number of processed in QID
       ,sum(cast(tokenstr('diffCpuTime:', variable_info)        -- (3) Sum of CPU TIME in QID
             as NUMERIC(18) )) DIFF_CPU_TIME
       ,cast(tokenstr('Qid:', variable_info)             -- (4) QID
         as varchar(175) CHARACTER SET UTF8) QUERY_ID 
from table (statistics(NULL,?filter))
group by 4
order by 3 descending;
