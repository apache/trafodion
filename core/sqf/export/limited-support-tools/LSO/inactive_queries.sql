-- set param ?filter 'INACTIVE_QUERIES=30';  -- 30 seconds
set param ?lsq  ' sqlSrc: ';

select current_timestamp "CURRENT_TIMESTAMP"   -- (1) Now
      ,cast(tokenstr('lastActivity:', variable_info)               -- (2) Last Activity
            as NUMERIC(18) ) LAST_ACTIVITY_SECS
      ,cast(tokenstr('Qid:', variable_info)               -- (3) QID
            as varchar(175) CHARACTER SET UTF8) QUERY_ID
      ,cast(substr(variable_info,             -- (5) SQL Source
             position(?lsq in variable_info) + char_length(?lsq),
             char_length(variable_info) - 
                        ( position(?lsq in variable_info) + char_length(?lsq) ))
            as char(256) CHARACTER SET UTF8) SOURCE_TEXT          
from table (statistics(NULL, ?filter)) 
order by 2 descending;
