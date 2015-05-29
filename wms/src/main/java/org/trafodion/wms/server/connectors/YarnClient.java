/*
package org.trafodion.wms.server.connectors;

import java.util.*;
import java.util.concurrent.ExecutorService;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.UnknownHostException;
import java.net.InetSocketAddress;

import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.codehaus.jackson.JsonGenerationException;
import org.codehaus.jackson.type.TypeReference;
import org.codehaus.jackson.JsonFactory;
import org.codehaus.jackson.JsonParser;
import org.codehaus.jackson.JsonToken;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.util.*;
import org.apache.hadoop.net.NetUtils;

import org.apache.hadoop.yarn.conf.YarnConfiguration;
import org.apache.hadoop.yarn.api.ClientRMProtocol;
import org.apache.hadoop.yarn.api.protocolrecords.KillApplicationRequest;
import org.apache.hadoop.yarn.api.records.ApplicationId;
import org.apache.hadoop.yarn.ipc.YarnRPC;
import org.apache.hadoop.yarn.util.Records;
import org.apache.hadoop.yarn.exceptions.YarnRemoteException;
import org.apache.hadoop.yarn.util.ConverterUtils;

import org.trafodion.wms.Constants;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.rpc.thrift.RpcHandler;

public class YarnClient {
	private static final Log LOG = LogFactory.getLog(YarnClient.class.getName());
	private Configuration conf = WmsConfiguration.create();
	private RpcHandler rpch;
	String[] args;

	public YarnClient(RpcHandler rpch){
		this.rpch = rpch;
	}

	public YarnClient(String[] args) {
		this.args = args;
	}

	class Application {

		private String id;
		private String user;
		private String name;
		private String queue;
		private String state;
		private String finalStatus;
		private Integer progress;
		private String trackingUI;
		private String trackingUrl;
		private String diagnostics;
		private String clusterId;
		private Long startedTime;
		private Long finishedTime;
		private Long elapsedTime;
		private String amContainerLogs;
		private String amHostHttpAddress;

		Application() throws java.io.IOException {
			id = "";
			user = "";
			name = "";
			queue = "";
			state = "";
			finalStatus = "";
			progress = 0;
			trackingUI = "";
			trackingUrl = "";
			diagnostics = "";
			clusterId = "";
			startedTime = 0L;
			finishedTime = 0L;
			elapsedTime = 0L;
			amContainerLogs = "";
			amHostHttpAddress = "";

		}

		void parse (JsonParser jp) throws java.io.IOException {
			String fieldname;
			while (jp.nextToken() != JsonToken.END_OBJECT) {
				fieldname = jp.getCurrentName();
				if ("id".equals(fieldname)){
					jp.nextToken();
					id = jp.getText();
				} else if ("user".equals(fieldname)){
					jp.nextToken();
					user = jp.getText();
				} else if ("name".equals(fieldname)){
					jp.nextToken();
					name = jp.getText();
				} else if ("queue".equals(fieldname)){
					jp.nextToken();
					queue = jp.getText();
				} else if ("state".equals(fieldname)){
					jp.nextToken();
					state = jp.getText();
				} else if ("finalStatus".equals(fieldname)){
					jp.nextToken();
					finalStatus = jp.getText();
				} else if ("progress".equals(fieldname)){
					jp.nextToken();
					progress = jp.getIntValue();
				} else if ("trackingUI".equals(fieldname)){
					jp.nextToken();
					trackingUI = jp.getText();
				} else if ("trackingUrl".equals(fieldname)){
					jp.nextToken();
					trackingUrl = jp.getText();
				} else if ("diagnostics".equals(fieldname)){
					jp.nextToken();
					diagnostics = jp.getText();
				} else if ("clusterId".equals(fieldname)){
					jp.nextToken();
					clusterId = jp.getText();
				} else if ("startedTime".equals(fieldname)){
					jp.nextToken();
					startedTime = jp.getLongValue();
				} else if ("finishedTime".equals(fieldname)){
					jp.nextToken();
					finishedTime = jp.getLongValue();
				} else if ("elapsedTime".equals(fieldname)){
					jp.nextToken();
					elapsedTime = jp.getLongValue();
				} else if ("amContainerLogs".equals(fieldname)){
					jp.nextToken();
					amContainerLogs = jp.getText();
				} else if ("amHostHttpAddress".equals(fieldname)){
					jp.nextToken();
					amHostHttpAddress = jp.getText();
				}
			}
		}
// getters
		String getId(){
			return id;
		}
		String getUser(){
			return user;
		}
		String getName(){
			return name;
		}
		String getQueue(){
			return queue;
		}
		String getState(){
			return state;
		}
		String getFinalStatus(){
			return finalStatus;
		}
		Integer getProgress(){
			return progress;
		}
		String getTrackingUI(){
			return trackingUI;
		}
		String getTrackingUrl(){
			return trackingUrl;
		}
		String getDiagnostics(){
			return diagnostics;
		}
		String getClusterId(){
			return clusterId;
		}
		Long getStartedTime(){
			return startedTime;
		}
		Long getFinishedTime(){
			return finishedTime;
		}
		Long getElapsedTime(){
			return elapsedTime;
		}
		String getAmContainerLogs(){
			return amContainerLogs;
		}
		String getAmHostHttpAddress(){
			return amHostHttpAddress;
		}
//setters
		void setId(String id){
			this.id=id;
		}
		void setUser(String user){
			this.user=user;
		}
		void setName(String name){
			this.name=name;
		}
		void setQueue(String queue){
			this.queue=queue;
		}
		void setState(String state){
			this.state=state;
		}
		void setFinalStatus(String finalStatus){
			this.finalStatus=finalStatus;
		}
		void setProgress(Integer progress){
			this.progress=progress;
		}
		void setTrackingUI(String trackingUI){
			this.trackingUI=trackingUI;
		}
		void setTrackingUrl(String trackingUrl){
			this.trackingUrl=trackingUrl;
		}
		void setDiagnostics(String diagnostics){
			this.diagnostics=diagnostics;
		}
		void setClusterId(String clusterId){
			this.clusterId=clusterId;
		}
		void setStartedTime(Long startedTime){
			this.startedTime=startedTime;
		}
		void setFinishedTime(Long finishedTime){
			this.finishedTime=finishedTime;
		}
		void setElapsedTime(Long elapsedTime){
			this.elapsedTime=elapsedTime;
		}
		void setAmContainerLogs(String amContainerLogs){
			this.amContainerLogs=amContainerLogs;
		}
		void setAmHostHttpAddress(String amHostHttpAddress){
			this.amHostHttpAddress=amHostHttpAddress;
		}

	};

	class ApplicationStore {
		private String workloadId;
		private long appStartTime;
		private Application app;
		private WorkloadRequest request = null;

		ApplicationStore(Application app) {
			this.workloadId = "";
			this.appStartTime = 0L;
			this.app = app;
		}
		String getWorkloadId(){
			return workloadId;
		}
		long getAppStartTime(){
			return appStartTime;
		}
		Application getApplication(){
			return app;
		}
		WorkloadRequest getRequest(){
			return request;
		}
		void setWorkloadId(String workloadId){
			this.workloadId = workloadId;
		}
		void setAppStartTime(long appStartTime){
			this.appStartTime = appStartTime;
		}
		void setApp(Application app){
			this.app = app;
		}
		void setRequest(WorkloadRequest request){
			this.request = request;
		}
	};

	class YarnClientExecute implements Callable<String> {

		@Override
		public String call() {

			HashMap< String, ApplicationStore> progressMap = new HashMap<String, ApplicationStore>();
			long globalCounter = 0;
			String msg = "";
			WorkloadResponse wresponse;
			String output;

			String yarnRestUrl = conf.get(Constants.YARN_REST_URL, Constants.DEFAULT_YARN_REST_URL);
			LOG.info("Yarn Rest Conection url is : " +  yarnRestUrl);

			try {
				Application app = new Application();

				while(true){
					DefaultHttpClient httpClient = new DefaultHttpClient();
					HttpGet getRequest = new HttpGet( yarnRestUrl);
					getRequest.addHeader("accept", "application/json");
					HttpResponse response = httpClient.execute(getRequest);

					if (response.getStatusLine().getStatusCode() != 200) {
						throw new RuntimeException("Failed : HTTP error code : " + response.getStatusLine().getStatusCode());
					}
					BufferedReader br = new BufferedReader(new InputStreamReader( (response.getEntity().getContent())));
					
					JsonFactory f = new JsonFactory();
					while ((output = br.readLine()) != null) {
						LOG.info("raw json=" + output);

						JsonParser jp = f.createJsonParser(output);
						while (jp.nextToken() == JsonToken.START_OBJECT) {

							long appEndTime = 0L;
							long duration = 0L;

							app.parse(jp);

							synchronized(progressMap){
								String state = app.getState();
								if ("FINISHED".equals(state) || "FAILED".equals(state) || "KILLED".equals(state)) {
									if (progressMap.containsKey(app.getId())){
										LOG.debug("YARN END : Application id : " + app.getId() + " state : " + app.getState());
										ApplicationStore as = progressMap.get(app.getId());
										appEndTime = System.currentTimeMillis();
										duration =  appEndTime - app.getStartedTime();
										as.getRequest().setOperation(OperationType.END);
										as.getRequest().setEndTimestamp(appEndTime);
										as.getRequest().setDuration(duration);
										as.getRequest().setJobState(app.getState());
										as.getRequest().setJobSubState(app.getFinalStatus());
										as.getRequest().setMapPct((int)(app.getProgress()));
										LOG.debug("End workload..." + as.getRequest());
										wresponse = rpch.send(as.getRequest());
										progressMap.remove(app.getId());
									}
								} else if (!progressMap.containsKey(app.getId())){
									appEndTime = System.currentTimeMillis();
									duration = appEndTime - app.getStartedTime();
									ApplicationStore as = new ApplicationStore(app);
									if (as != null ){
										LOG.debug("YARN BEGIN : Application id : " + app.getId() + " state : " + app.getState());
										as.setAppStartTime(app.getStartedTime());
										progressMap.put(app.getId(), as);
										as.request = WorkloadRequest.newBuilder()
										.setWorkloadId("")
										.setOperation(OperationType.BEGIN)
										.setJobId(app.getId())
										.setJobType(JobType.HADOOP)
										.setJobText(app.getName())
										.setJobState(app.getState())
										.setJobSubState(app.getFinalStatus())
										.setUserName(app.getName())
										.setStartTimestamp(app.getStartedTime())
										.setEndTimestamp(appEndTime)
										.setMapPct((int)(app.getProgress()))
										.setReducePct((int)(0.0))
										.setDuration(duration)
										.setParentId("")
										.setParentKey(app.getId())
										.build();
										LOG.debug("Begin workload..." + as.getRequest());
										wresponse = rpch.send(as.getRequest());
										switch(wresponse.getAction()){
										case REJECT:
										case CANCEL:
											LOG.debug("killJob..." + as.getRequest());
											killApplication(app.getId());
											break;
										default:
											break;
										}
									}
								} else{
									LOG.debug("YARN UPDATE : Application id : " + app.getId() + " state : " + app.getState());
									ApplicationStore as = progressMap.get(app.getId());
									appEndTime = System.currentTimeMillis();
									duration =  appEndTime - app.getStartedTime();
									as.getRequest().setOperation(OperationType.UPDATE);
									as.getRequest().setEndTimestamp(appEndTime);
									as.getRequest().setDuration(duration);
									as.getRequest().setJobState(app.getState());
									as.getRequest().setJobSubState(app.getFinalStatus());
									as.getRequest().setMapPct((int)(app.getProgress()));
									LOG.debug("Update workload..." + as.getRequest());
									wresponse = rpch.send(as.getRequest());
									switch(wresponse.getAction()){
									case REJECT:
									case CANCEL:
										LOG.debug("killJob..." + as.getRequest());
										killApplication(app.getId());
										break;
									default:
										break;
									}
								} //else
							} //synchronized
						}
						jp.close();
					}
					httpClient.getConnectionManager().shutdown();

					try {
						Thread.sleep(1000);
					} catch (Exception e) {
						throw new RuntimeException("Unable to sleep until next cycle" + e.getMessage());
					}
				}
			} catch (UnknownHostException e) {
				msg = "Unknown Host Exception : " + e.getMessage();
			} catch (ClientProtocolException e) {
				msg = "Client Protocal Exception : " + e.getMessage();
			} catch (IOException e) {
				msg = "IO Exception : " + e.getMessage();
			} catch (RuntimeException e) {
				msg = "Runtime Exception : " + e.getMessage();
			}
			LOG.error(msg);
			return msg;
		}
	}
    
    void killApplication(String applicationId){
            ApplicationId appId = ConverterUtils.toApplicationId(applicationId);
            LOG.info("Killing application " + applicationId);

            ClientRMProtocol applicationsManager;
       		String yarnRmAddress = conf.get(Constants.YARN_RM_ADDRESS, Constants.DEFAULT_YARN_RM_ADDRESS);
       		conf.set(Constants.YARN_RM_ADDRESS, yarnRmAddress);
            YarnConfiguration yarnConf = new YarnConfiguration(conf);
            yarnConf.set("yarn.resourcemanager.address", "sq151.houston.hp.com:8032");
            LOG.info(yarnConf.get("yarn.resourcemanager.address", ""));
            InetSocketAddress rmAddress = NetUtils.createSocketAddr(yarnConf.get( YarnConfiguration.RM_ADDRESS, YarnConfiguration.DEFAULT_RM_ADDRESS));
            LOG.info("Connecting to ResourceManager at " + rmAddress);
            YarnRPC rpc = YarnRPC.create(yarnConf);

            Configuration appsManagerServerConf = new Configuration(yarnConf);
            applicationsManager = ((ClientRMProtocol) rpc.getProxy(ClientRMProtocol.class, rmAddress, appsManagerServerConf));

            KillApplicationRequest killRequest = Records.newRecord(KillApplicationRequest.class);
            killRequest.setApplicationId(appId);
            try {
                    applicationsManager.forceKillApplication(killRequest);
            } catch (YarnRemoteException e){
                    LOG.error("YarnRemoteException : " + e);
            }
    }

	public static void main(String[] args) throws InterruptedException, ExecutionException {

		final ExecutorService exService = Executors.newSingleThreadExecutor();

		final Future<String> callFuture = exService.submit(new YarnClient(args).new YarnClientExecute());
// gets value of callable thread
		final String callval = callFuture.get();
		System.out.println("Callable:" + callval);
// checks for thread termination
		final boolean isTerminated = exService.isTerminated();
		System.out.println(isTerminated);
		// waits for termination for 30 seconds only
		exService.awaitTermination(30, TimeUnit.SECONDS);
		exService.shutdownNow();
	}
}
*/