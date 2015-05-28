-- set param ?filter 'QUERIES_IN_SQL=30';  -- 30 seconds
set param ?lsq    ' sqlSrc: ';

select current_timestamp "CURRENT_TIMESTAMP"    -- (1) Now
       , cast(tokenstr('blockedInSQL:', variable_info)             -- (2) Time in SQL
             as NUMERIC(18)) TIME_IN_SECONDS
       , cast(tokenstr('Qid:', variable_info)            -- (3) QID
             as varchar(175) CHARACTER SET UTF8) QUERY_ID
       , cast(tokenstr('State:', variable_info)           -- (4) State
             as char(30)) EXECUTE_STATE 
       , cast(substr(variable_info,            -- (5) SQL Source
             position(?lsq in variable_info) + char_length(?lsq),
             char_length(variable_info) - 
                        ( position(?lsq in variable_info) + char_length(?lsq) ))
             as char(256) CHARACTER SET UTF8) SOURCE_TEXT
from table (statistics(NULL, ?filter))
order by 2 descending;
