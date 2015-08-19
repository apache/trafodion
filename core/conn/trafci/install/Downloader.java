// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

import java.io.BufferedReader;
import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.File;
import java.net.URLConnection;
import java.net.URL;
import java.net.MalformedURLException;
import java.net.BindException;
import java.net.ConnectException;
import java.net.UnknownHostException;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import javax.swing.JProgressBar;

public class Downloader implements Runnable  {
    private String installDirectory = "";
    
	private String targetURL = "";

	private String protocol = "http";
	private String proxyServer = "";
	private ArrayList<String> proxyServerList = null;
	private int proxyPort = 80;
	
	private String filename = "";
	private String error = "";
	private int progress = 0;
	
	private String winRegistryAutoProxy = "reg query \"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\" /v AutoConfigURL";
	private String winRegistryManualProxy = "reg query \"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\" /v ProxyServer";
	private String winRegString = "REG_SZ";
	private String osName;
	
	private JProgressBar pBar =null;
	
	private boolean asciiMode = false;
	
	public Downloader(){
        setOS();
    }
	
	public Downloader(String installDir){
	    setInstallDirectory(installDir);
        setOS();
    }
    
	public Downloader(String downloadURL, String targetFilename, String installDir){
		this.targetURL = downloadURL;
		this.filename = targetFilename;
		setInstallDirectory(installDir);
		setOS();
	}


	public void run(){
	    reset();
        download();
	}
	
	public boolean download(){
        
	    InputStream in =null;
        OutputStream out =null;
        
		try{
		    
		    this.error = "";
			
		    URL url;
			URLConnection conn;
			byte[] buffer;
			int bytesRead;
			long bytesWritten;
			int filesize;
			
			if (proxyServer == "")
				url = new URL(targetURL);
			else {
//				url = new URL(protocol, proxyServer, proxyPort, targetURL);
				url = new URL(targetURL);
				System.setProperty("http.proxyHost", proxyServer);
				System.setProperty("http.proxyPort", proxyPort + "");
			}
			
			
			conn = url.openConnection();
			
			out = new BufferedOutputStream(new FileOutputStream(filename));
			
			//conn.setRequestProperty("content-type", "binary/data");
						
			filesize = conn.getContentLength();
			progress = 0;
			
			in = conn.getInputStream();
			buffer = new byte[1024];
			
			bytesWritten = 0;
			
			while ((bytesRead = in.read(buffer))!= -1){
			    out.write(buffer, 0, bytesRead);
			    bytesWritten += bytesRead;
			    if(bytesWritten > (progress*filesize)/100.0){
			        increaseProgressBar(bytesWritten, filesize);
			    }
			}
			
			if(in != null)
			    in.close();
			if(out != null)
			    out.close();
			
			// great success
			return true;
			
		}catch(MalformedURLException mue){
			setError(DownloaderError.HTTP_INVALID_URL_ERR);
		}catch(BindException be){
		    if(proxyServer == "")
		        setError(DownloaderError.BIND_ERR);
		    else
		        setError(DownloaderError.PROXY_ERR);
		}catch(ConnectException ce){
		    setError(DownloaderError.CONNECTION_ERR);
		}catch(UnknownHostException ehe){
		    setError(DownloaderError.CONNECTION_ERR);
		}catch(FileNotFoundException fnfe){
		    if(fnfe.toString().indexOf(targetURL) != -1)
		        setError(DownloaderError.INVALID_REMOTE_FILE_ERR);
		    else
		        setError(DownloaderError.INVALID_LOCAL_FILE_ERR);
		}catch(SocketException se){
		    setError(DownloaderError.SOCKET_ERR);
		}catch(IOException ioex){
		    if(ioex.toString().indexOf("HTTP response code: 503") != -1)
		        setError(DownloaderError.SERVER_UNAVAILABLE_ERR);
		    else{
		        setError(DownloaderError.UNKNOWN_ERR);
		    }
		}
		
		try{
            if(in != null)
                in.close();
            if(out != null)
                out.close();
            
            /* try to remove the incomplete file */
            new File(filename).delete();
            
		}catch(Exception ex){
		    ;
		}
		
		return false;
	}
	
	public void increaseProgressBar(long bytesWritten, int filesize){
	    progress += 1;
	    
	    if(asciiMode && (progress % 4 == 0))
	        System.out.print(".");
	    else if(pBar != null){
	        pBar.setValue(progress);
	    }
	    
	    if(bytesWritten > (progress*filesize)/100.0){
	        increaseProgressBar(bytesWritten, filesize);
        }
	    
	}
	
	/* ProxySelector is unavailable to Java 1.4, so if the user is running windows we have two options:
	 * 1. look in HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings\ProxyServer
	 *    and parse out the server:port
	 * 2. look in HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings\AutoConfigURL
	 *    then we can try to download the .pac file and locate a list of proxies
	 *    
	 *    For unix look in env variable http_proxy
	 */
	public ArrayList<String> findProxy(){
	    try{
	        
    	    if(osName.startsWith("window")){
    	        return findWindowsProxy();
    	    }else{
    	        return findNixProxy();
    	    }
	    
	    }catch(Exception ex){
	        //ex.printStackTrace();
	        System.out.println("An error has occurred while trying to detect a proxy server, please enter it manually.");
	    }
	    
	    return proxyServerList;
	}
	
	/* Try to determine *nix proxy */
	private ArrayList<String> findNixProxy() throws Exception{
        // First look in env variable http_proxy
        // ...consider using the autoproxy utility?
        Pattern p = Pattern.compile("([^0-9\\s]*):(\\d*)");
        Matcher m = p.matcher(System.getProperty("http_proxy"));
        String[] tmpArr = new String[2];
        proxyServerList = new ArrayList<String>();  // for consistency with Windows, in *nix we dont need a list
        if(m != null && m.find()){
            tmpArr = m.group().split(" ");
            tmpArr = tmpArr[1].split(":");
            proxyServerList.add(m.group(1)+":"+m.group(2));
            proxyServer = tmpArr[0];
            proxyPort = Integer.parseInt(tmpArr[1]);
        }
        
	    return proxyServerList;
	}
	
	private ArrayList<String> findWindowsProxy() throws Exception{
        int strPos;
        String url, result = runCommand(winRegistryManualProxy);
        String[] urlSplit = new String[2];
        if((strPos = result.indexOf(winRegString)) != -1){
            /* 1. Proxy server found in registry, parse ... REG_SZ <server>:<port> */
            url = result.substring(strPos + winRegString.length(), result.length()).trim();
            urlSplit = url.split(":");
            proxyServer = urlSplit[0];
            proxyPort = Integer.parseInt(urlSplit[1]);
            
            proxyServerList = new ArrayList<String>();
            proxyServerList.add(url);
        }else{
            result = runCommand(winRegistryAutoProxy);
            if((strPos = result.indexOf(winRegString)) != -1){
                /* 2. Auto proxy script found, contact url parse results for Proxy <server>:<port> */
                
                // temp pac file locatino
                targetURL = result.substring(strPos + winRegString.length(), result.length()).trim();
                String tmpDir = System.getProperty("java.io.tmpdir");
                String tmpFile = "ci_temp_proxy.pac";
                if(tmpDir != null){
                    if(!tmpDir.endsWith(File.separator))
                        tmpDir += File.separator;
                    
                    filename = tmpDir + tmpFile;
                }else{
                    /* store in C:\ as a last resort, only if tmpDir is null
                     * and if installDirectory wasn't set
                     */
                    if(installDirectory == "")
                        filename = "C:\\" + tmpFile;  
                    else    
                        filename = installDirectory + tmpFile;
                    
                }
                    
                proxyServer = ""; // DIRECT download
                if(download()){
                    String proxyFile = readFile(filename);
                    Pattern p = Pattern.compile("PROXY\\s([^0-9\\s]*):(\\d*)");
                    Matcher m = p.matcher(proxyFile);
                    String[] tmpArr = new String[2];
                    proxyServerList = new ArrayList<String>();
                    while(m != null && m.find()){
                        if(!proxyServerList.contains(m.group(1)+":"+m.group(2)))
                            proxyServerList.add(m.group(1)+":"+m.group(2));
                    }         
                    
                    /* default to the first proxy found */
                    if(proxyServerList.size()>0){
                        tmpArr = ((String)proxyServerList.get(0)).split(":");
                        proxyServer = tmpArr[0];
                        proxyPort = Integer.parseInt(tmpArr[1]);
                    }
                    
                    // remove temp pac file
                    File pacFile = new File(filename);
                    if(pacFile.exists())
                        pacFile.delete();
                }
            }
        } 
        
        return proxyServerList;
	}
	
	
	private String readFile(String path){
	    StringBuffer sb = new StringBuffer(1024);
	    
	    try{    	    
    	    BufferedReader buffer = new BufferedReader(new FileReader(path));
    	            
    	    char[] buf = new char[1024];
    	    while( buffer.read(buf) > -1){
    	        sb.append(String.valueOf(buf));   
    	    }
    
    	    buffer.close();
	    }catch(Exception ex){
	        ;
	    }
	    
	    return sb.toString();
	}

	
	private String runCommand(String command){
	    String result = "";
        try{
            String tmp;
            
            /* redirect stderr */
            PrintStream nps, stderr;
            stderr = System.err;
            if(osName.startsWith("window"))
                nps = new PrintStream(new FileOutputStream("NUL:"));
            else
                nps = new PrintStream(new FileOutputStream("/dev/null"));
            
            if(nps != null)
                System.setErr(nps);
         
            Process p = Runtime.getRuntime().exec(command);
            BufferedReader buffer = new BufferedReader(new InputStreamReader(p.getInputStream()));
            
            while ((tmp = buffer.readLine()) != null) {
                result += tmp;
            }
                            
            buffer.close();
        
            /* reset stderr */
            System.setErr(stderr);
            
        }catch(Exception ex){
            ;
        }    
        
        return result;
	}
	
	private void setOS(){
	      osName = System.getProperty("os.name");
	      if (osName != null)
	      {
	         osName = osName.trim().toLowerCase();
	      }
	      else
	      {
	         osName= "window";
	      }
	}
	
	private void reset(){
	    this.progress = 0;
	    this.error = "";
	}
	   
	/* Getter/Setter Methods ****************************************/
	public String getURL(){
		return targetURL;
	}
	
	public String getFilename(){
		return filename;
	}
	
	public String getProxyServer(){
	    return proxyServer;
	}
	
	public int getProxyPort(){
	    return proxyPort;
	}
	
	public int getProgressBar(){
	    return progress;
	}
	
	public String getError(){
	    return error;
	}
	
	public void setURL(String downloadURL){
		this.targetURL = downloadURL;
	}
	
	public void setFilename(String targetFilename){
		this.filename = targetFilename;
	}
	
	public void setProxy(String server, String port){
	    this.proxyServer = server;
	    try{
	        this.proxyPort = Integer.parseInt(port);
	    }catch(NumberFormatException nfe){
	        this.proxyPort = 80;
	    }
	}
	
	public void setError(String err){
	    this.error = err;
	}	
	
	public void setInstallDirectory(String installDir){
	    if(!installDir.endsWith(File.separator))
	        installDir += File.separator;
	    
	    this.installDirectory = installDir;
	}
	
	public void setAsciiMode(boolean aMode){
	    this.asciiMode = aMode;
	}
	
	public void setProgressBar(JProgressBar bar){
	    this.pBar = bar;
	}
	/*****************************************************************/
}

