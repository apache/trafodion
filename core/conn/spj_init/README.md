##How to build and initialize default SPJs
###Build
```  
cd $MY_SQROOT/../conn/spj_init    
make clean && make all  
```
###Initialization
```  
cds  
./init_spj.sql  
```  
###SPJ Example
```
DEFAULT_SPJ.HELP(INOUT COMMANDNAME VARCHAR)  
E.g.   
trafci>set param ?p1 help;  
trafci>call DEFAULT_SPJ.HELP(?p1)  
PUT - Upload a JAR. SHOWDDL PRODURE DEFAULT_SPJ.PUT for more info.  
LS - List JARs. SHOWDDL PRODURE DEFAULT_SPJ.LS for more info.  
LSALL - List all JARs. SHOWDDL PRODURE DEFAULT_SPJ.LSALL for more info.  
RM - Remove a JAR. SHOWDDL PRODURE DEFAULT_SPJ.RM for more info.  
RMREX - Remove JARs by a perticular pattern. SHOWDDL PRODURE DEFAULT_SPJ.RMREX for more info.  
GETFILE - Upload a JAR. SHOWDDL PRODURE DEFAULT_SPJ.GETFILE for more info.  
```
