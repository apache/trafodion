/*
package org.trafodion.wms.server.connectors;

import java.io.IOException;
import java.util.*;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.mapred.JobClient;
import org.apache.hadoop.mapred.JobConf;
import org.apache.hadoop.mapred.JobStatus;
import org.apache.hadoop.mapred.RunningJob;
import org.apache.hadoop.mapred.JobID;
import org.apache.hadoop.util.*;    

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.thrift.generated.*;
import org.trafodion.wms.rpc.thrift.RpcHandler;

public class JTClient implements Runnable {  
	private static final Log LOG = LogFactory.getLog(JTClient.class.getName());
	private Configuration conf = WmsConfiguration.create();
	private RpcHandler rpch;
	private Thread thrd;
	
	public JTClient(RpcHandler rpch){
		this.rpch = rpch;
		thrd = new Thread(this);
		thrd.setDaemon(true);
		thrd.start();
	}
	
	public void run() {
		
		class JobStore {
			private String workloadId;
			private long globalCounter;
			private long jobStartTime;
			private JobID jobID;
			private WorkloadRequest request = null;

			JobStore(JobID jobID) {
				this.workloadId = "";
				this.globalCounter = 0L;
				this.jobStartTime = 0L;
				this.jobID = jobID;
			}
		}
		
		HashMap< String, JobStore> progressMap = new HashMap<String, JobStore>();

		JobClient client = null;
		long globalCounter = 0;

//		conf.set("mapred.job.tracker", "sq084.houston.hp.com:9077");
//		long jobTimeoutMins = conf.getLong("wms.job.timeout.min", 0);
//		LOG.info("Job timeout is " + jobTimeoutMins + " minutes");

		String jobTracker = conf.get("hadoop.mapred.job.tracker", "sq084.houston.hp.com");
		String jobTrackerPort = conf.get("hadoop.mapred.job.tracker.port", "9077");
		jobTracker = jobTracker + ":" + jobTrackerPort;
		LOG.info("Conf JobTracker is: " +  jobTracker);
		conf.set("mapred.job.tracker", jobTracker);

		while (true) {
			try {
				client = new JobClient(new JobConf(conf));
				LOG.info("JobClient started");
				break;
			} catch (IOException ioe) {
				LOG.error("Ignoring exception JobClient " + ioe);
			}
			try {
				Thread.sleep(3000);
			} catch (Exception e) {
				LOG.info("Unable to sleep until next JobClient " + e);
			}
		}

		while (true){
			JobStatus[] jobStatuses = null; 
			RunningJob runningJob = null;
			
			try {
				jobStatuses = client.jobsToComplete();
			} catch (Exception e) {
				LOG.debug("Ignoring exception jobStatuses " + e);
			}
			
			if (jobStatuses == null || jobStatuses.length <= 0) {
				LOG.debug("There are no active jobs");
				try {
					Thread.sleep(3000);
					continue;
				} catch (Exception e) {
					LOG.info("Unable to sleep until next JobClient " + e);
				}
			}
			
			LOG.debug("There are " + + jobStatuses.length + " active jobs" );
			globalCounter++;

			for (JobStatus jobStatus : jobStatuses) {
				try {	
					long jobEndTime = 0L;
					long duration = 0L;
					
					String jobState;
					int runState = jobStatus.getRunState();
					
					runningJob = client.getJob(jobStatus.getJobID());
					//LOG.info("JobID=" + runningJob + ",JobState=" + runState);
        			TaskReport[] mapReports = client.getMapTaskReports(jobStatus.getJobID());
        			for (TaskReport r : mapReports) {
            				if (lastTaskEndTime < r.getFinishTime()) {
                				lastTaskEndTime = r.getFinishTime();
            				}			
        			}	

        			TaskReport[] reduceReports = client.getReduceTaskReports(jobStatus.getJobID());
        			for (TaskReport r : reduceReports) {
            				if (lastTaskEndTime < r.getFinishTime()) {
                				lastTaskEndTime = r.getFinishTime();
            				}
        			}

					synchronized(progressMap){
						if (!progressMap.containsKey(jobStatus.getJobID().toString())){
							jobEndTime = System.currentTimeMillis();
							duration = jobEndTime - jobStatus.getStartTime();
							JobStore job = new JobStore(jobStatus.getJobID());
							if (job != null ){
								job.globalCounter = globalCounter;
								job.jobStartTime = jobStatus.getStartTime();
								progressMap.put(jobStatus.getJobID().toString(), job);
								LOG.debug("HADOOP BEGIN: " + jobStatus.getJobID().toString());
								job.request = WorkloadRequest.newBuilder()
								.setWorkloadId("") 
								.setOperation(OperationType.BEGIN) 
								.setJobId(jobStatus.getJobID().toString())
								.setJobType(JobType.HADOOP)
								.setJobText(runningJob.getJobName())
								.setJobState(JobStatus.getJobRunState(jobStatus.getRunState()))
								.setJobSubState("BEGIN")
								.setUserName(jobStatus.getUsername()) 
								.setStartTimestamp(jobStatus.getStartTime()) 
								.setEndTimestamp(jobEndTime)
								.setMapPct((int)(100.0 * jobStatus.mapProgress()))
								.setReducePct((int)(100.0 * jobStatus.reduceProgress()))
								.setDuration(duration)
								.setParentId("")
								.setParentKey(jobStatus.getJobID().toString())
								.build();
								LOG.debug("Begin workload..." + job.request);
								WorkloadResponse response = rpch.send(job.request);
								switch(response.getAction()){
								case REJECT:
								case CANCEL:
									LOG.debug("killJob..." + job.request);
									runningJob.killJob();
									break;
								default:
									break;
								}
							}	
						} else {
							JobStore job = progressMap.get(jobStatus.getJobID().toString());
							LOG.debug("HADOOP UPDATE: " + jobStatus.getJobID().toString());
							job.globalCounter = globalCounter;
							jobEndTime = System.currentTimeMillis();
							duration =  jobEndTime - jobStatus.getStartTime();
							job.request.setOperation(OperationType.UPDATE);
							job.request.setStartTimestamp(jobStatus.getStartTime());
							job.request.setEndTimestamp(jobEndTime);
							job.request.setDuration(duration);
							job.request.setUserName(jobStatus.getUsername());
							job.request.setJobId(jobStatus.getJobID().toString());
							job.request.setJobText(runningJob.getJobName());
							job.request.setJobState(JobStatus.getJobRunState(jobStatus.getRunState()));
							job.request.setJobSubState("UPDATE");
							job.request.setMapPct((int)(100.0 * jobStatus.mapProgress()));
							job.request.setReducePct((int)(100.0 * jobStatus.reduceProgress()));
							LOG.debug("Update workload..." + job.request);
							WorkloadResponse response = rpch.send(job.request);
							switch(response.getAction()){
							case REJECT:
							case CANCEL:
								LOG.debug("killJob..." + job.request);
								runningJob.killJob();
								break;
							default:
								break;
							}
						} //else
					} //synchronized
					client.getSetupTaskReports(jobStatus.getJobID());
					client.getCleanupTaskReports(jobStatus.getJobID());
					//
				} catch (Exception e){
					LOG.error(e);
					break;
				}
			} // for
			
			synchronized(progressMap){
				if (!progressMap.isEmpty()){

					for (Iterator<String> i = progressMap.keySet().iterator(); i.hasNext(); )   
					{   
						String key = i.next();   
						JobStore job = progressMap.get(key);
						JobID jobID = job.jobID;
						if (job.globalCounter != globalCounter){
							long jobStartTime = job.jobStartTime;
							long jobEndTime =  System.currentTimeMillis();
							long duration = jobEndTime - jobStartTime;
							try {
								LOG.debug("HADOOP END: " + jobID);
								runningJob = client.getJob(jobID);
								//String subState = JobStatus.getJobRunState(runningJob.getJobState());
								job.request.setOperation(OperationType.END);
								job.request.setJobId(key);
								job.request.setJobText(runningJob.getJobName());
								job.request.setJobState(JobStatus.getJobRunState(runningJob.getJobState()));
								job.request.setJobSubState(job.request.getJobState());
								//job.request.setJobSubState(subState);
								job.request.setStartTimestamp(jobStartTime);
								job.request.setEndTimestamp(jobEndTime);
								job.request.setDuration(duration);
								job.request.setMapPct((int)(100.0 * runningJob.mapProgress()));
								job.request.setReducePct((int)(100.0 * runningJob.reduceProgress()));
								LOG.debug("End workload..." + job.request);
								rpch.send(job.request);
								i.remove();
							} catch (IOException ioe) {
								LOG.error("Ignoring exception "+ ioe);
							} //catch
						}//if
					}//for
				}//if
			} //synchronized

			try {
				Thread.sleep(1000);
			} catch (Exception e) {
				LOG.error("Unable to sleep until next cycle" + e);
				break;
			}
		}
	}    
}
*/