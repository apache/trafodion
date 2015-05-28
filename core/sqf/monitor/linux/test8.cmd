startup trace 2
exec shell test8.sub

echo Start Test5 - DOWN Node test
down 2
delay 1
ps
! should abort
event {DTM} 5
delay 5
! should commit
event {DTM} 5
delay 5

echo Exit TMs
event {DTM} 6
delay 1
ps
shutdown !

