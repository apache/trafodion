if [ "$1" = "QID" ]; then
   QID=`grep "QID is" QIDOUT | awk '{print $3}'`
   echo "SET SESSION DEFAULT PARENT_QID '$QID';" > PQIDOUT  
elif [ "$1" = "EXPLAIN" ]; then
   QID=`grep "QID is" QIDOUT | awk '{print $3}'`
   $scriptsdir/tools/runmxci.ksh <<EOF
log LOGRTS;
explain options 'f' for qid $QID ;
log;
EOF
else
   QID=`grep "QID is" QIDOUT | awk '{print $3}'`
   $scriptsdir/tools/runmxci.ksh <<EOF
log LOGRTS;
SET SESSION DEFAULT STATISTICS_VIEW_TYPE 'DEFAULT' ;
display statistics for QID $QID $1;
get statistics for QID $QID $1;
log;
EOF
fi
