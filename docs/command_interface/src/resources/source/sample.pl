use lib 'C:\\Program Files (x86)\\Apache Software Foundation\\Trafodion Command Interface\\lib\\perl';
use Session;

# create a new session
$sess = Session->new();

# connect to the database
$sess->connect("user1","password","16.123.456.78","23400");

$retval=$sess->execute(" set schema TRAFODION.CI_SAMPLE ");
print $retval;

# Execute sample queries
$retval=$sess->execute("select * from employee"); print $retval;
$retval=$sess->execute("get statistics"); print $retval;

# disconnect from the database
print "\n\nSession 1: Disconnecting first session. \n\n";
$sess->disconnect();
