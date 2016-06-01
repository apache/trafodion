import os import sys

## Modify this path
sys.path.append("C:\\Program Files (x86)\\Apache Software Foundation\\Trafodion Command Interface\\lib\\python")
import Session

# create a new session
sess = Session.Session()

# Connect to the database
x=sess. connect ("user1","password","16.123.456.78","23400")

# Execute sample queries

# execute takes the query string as argument
setSchema = "set schema TRAFODION.CI_SAMPLE"
selectTable = "select * from employee"
getStats = "get statistics"

#Contruct a list of SQL statements to be executed
queryList = [setSchema, selectTable, getStats] print "\n";

for query in queryList:
   print sess.execute (query)

# disconnect the session
sess.disconnect()
del sess
sess=None
