! NonStop Process pair testing.
!
startup trace 0
exec {nowait,name $SERV0,nid 0}nsserver
delay 5
exec {nowait,name $CLI,nid 0}nsclient
ps
delay 15
! *** We need to kill $CLI because it currently is locking up
!kill $CLI
!kill 2,2
delay 15
ps
!kill $SERV0
exit
