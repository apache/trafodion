#!/bin/sh
echo "===Starting environment in trace mode"
./xshell -c "'startup trace 2'" 2>/dev/null &
sleep 5
echo "===Test1-Client/Server, priority, open, close, processinfo"
./xshell -a test1.sub
echo "===attach test"
mpirun -ha -spawn -np 1 ./attach
echo "===Restarting environment"
./xshell -c shutdown
echo "===Test2-Client/Server Multi pass"
./xshell -c "'startup trace 0'" 2>/dev/null &
sleep 5
./xshell -a test2.sub
echo "===Test3-NonStop Process Pairs"
./xshell -a test3.sub
echo "===Test4-Configuration tests"
./xshell -a test4.sub
echo "===Test5-Get TM seq#"
./xshell -a test5.sub
echo "===Test6-Process Death notices"
./xshell -a test6.sub
echo "===Test7-Load-balance large number of processes"
./xshell -a test7.sub
echo "===Test8-DTM Sync"
./xshell -a test8.sub
echo "===Exit DTMs"
./xshell -c "'event {DTM} 6'"
./xshell -c ps
./xshell -c "'shutdown !'"
