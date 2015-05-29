from java.lang import System
from org.trafodion.wms.thrift.generated import Action 
from org.trafodion.wms.thrift.generated import Operation

def begin():
    response.getData().setAction(Action.ACTION_CONTINUE)
#    print 'Thresholds before [%s]' % thresholds.toString()
#    thresholds.setMaxMemUsage(5)
#    print 'Thresholds after [%s]' % thresholds.toString()
 

def update():
    return

#    jobType = request.getJobType()
#    if jobType != JobType.HIVE  :
#        return

#    elapsed = System.currentTimeMillis() - request.getData().getBeginTimestamp()
#    if elapsed > 60000L :
#        print 'Canceling...elapsed time [%s] milliseconds' % elapsed
#        response.getData().setAction(Action.ACTION_CANCEL)
        
def end():
    return

def start():
    operation = request.getData().getOperation().toString()

    if operation == "OPERATION_BEGIN" :
        begin()
    elif operation == "OPERATION_UPDATE" :
        update()
    elif operation == "OPERATION_END" :
        end()
    else :
        response.getData().setAction(Action.ACTION_CONTINUE)

#######################################################################
#                main portion of script starts here
#######################################################################
print 'Rcvd request[%s]' % request
start()
print 'Send response:[%s]' % response