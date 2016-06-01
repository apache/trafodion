/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
 */


package org.trafodion.wms.http;

import java.io.IOException;
import java.io.PrintWriter;
import java.net.BindException;
import java.net.InetSocketAddress;
import java.net.URL;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletRequestWrapper;
import javax.servlet.http.HttpServletResponse;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
//import org.apache.hadoop.conf.ConfServlet;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.CommonConfigurationKeys;
//import org.apache.hadoop.jmx.JMXJsonServlet;
import org.apache.hadoop.log.LogLevel;
//import org.apache.hadoop.security.Krb5AndCertsSslSocketConnector;
//import org.apache.hadoop.security.Krb5AndCertsSslSocketConnector.MODE;
//import org.apache.hadoop.security.UserGroupInformation;
//import org.trafodion.wms.security.authorize.AccessControlList;
import org.apache.hadoop.util.ReflectionUtils;
import org.mortbay.io.Buffer;
import org.mortbay.jetty.Connector;
import org.mortbay.jetty.Handler;
import org.mortbay.jetty.MimeTypes;
import org.mortbay.jetty.Server;
import org.mortbay.jetty.handler.ContextHandler;
import org.mortbay.jetty.handler.ContextHandlerCollection;
import org.mortbay.jetty.nio.SelectChannelConnector;
import org.mortbay.jetty.security.SslSocketConnector;
import org.mortbay.jetty.servlet.Context;
import org.mortbay.jetty.servlet.DefaultServlet;
import org.mortbay.jetty.servlet.FilterHolder;
import org.mortbay.jetty.servlet.FilterMapping;
import org.mortbay.jetty.servlet.ServletHandler;
import org.mortbay.jetty.servlet.ServletHolder;
import org.mortbay.jetty.webapp.WebAppContext;
import org.mortbay.thread.QueuedThreadPool;
import org.mortbay.util.MultiException;

import com.sun.jersey.spi.container.servlet.ServletContainer;

/**
 * Create a Jetty embedded server to answer http requests. The primary goal
 * is to serve up status information for the server.
 * There are three contexts:
 *   "/logs/" -> points to the log directory
 *   "/static/" -> points to common static files (src/webapps/static)
 *   "/" -> the jsp server code from (src/webapps/<name>)
 */
public class HttpServer implements FilterContainer {
  public static final Log LOG = LogFactory.getLog(HttpServer.class);

  // The ServletContext attribute where the daemon Configuration
  // gets stored.
  public static final String CONF_CONTEXT_ATTRIBUTE = "wms.conf";
  protected final Server webServer;
  protected final Connector listener;
  protected final WebAppContext webAppContext;
  protected final boolean findPort;
  protected final Map<Context, Boolean> defaultContexts =
      new HashMap<Context, Boolean>();
  protected final List<String> filterNames = new ArrayList<String>();
  private static final int MAX_RETRIES = 10;
  private final Configuration conf;

  private boolean listenerStartedExternally = false;

  public HttpServer(String name, String bindAddress, int port,
      boolean findPort, Configuration conf) throws IOException{
	 
    webServer = new Server();
    this.findPort = findPort;
    this.conf = conf;

    listenerStartedExternally = false;
    listener = createBaseListener(conf);
    listener.setHost(bindAddress);
    listener.setPort(port);
    
    webServer.addConnector(listener);

    webServer.setThreadPool(new QueuedThreadPool());

    final String appDir = getWebAppsPath();
    ContextHandlerCollection contexts = new ContextHandlerCollection();
    webServer.setHandler(contexts);
    
    webAppContext = new WebAppContext();
    webAppContext.setDisplayName("WepAppsContext");
    webAppContext.setContextPath("/");
    webAppContext.setResourceBase(appDir + "/" + name);
//  webAppContext.setWar(appDir + "/" + name);
    webAppContext.setWar("wms-webapps" + "/" + name);
    webAppContext.getServletContext().setAttribute(CONF_CONTEXT_ATTRIBUTE, conf);
    webServer.addHandler(webAppContext);

    addDefaultApps(contexts, appDir);
    
    addDefaultServlets();
  }

  /**
   * Create a required listener for the Jetty instance listening on the port
   * provided. This wrapper and all subclasses must create at least one
   * listener.
   */
  public Connector createBaseListener(Configuration conf)
      throws IOException {
    return HttpServer.createDefaultChannelConnector();
  }
  
  // LimitedPrivate for creating secure datanodes
  public static Connector createDefaultChannelConnector() {
    SelectChannelConnector ret = new SelectChannelConnector();
    ret.setLowResourceMaxIdleTime(10000);
    ret.setAcceptQueueSize(128);
    ret.setResolveNames(false);
    ret.setUseDirectBuffers(false);
    return ret;   
  }
  
  /**
   * Add default apps.
   * @param appDir The application directory
   * @throws IOException
   */
  protected void addDefaultApps(ContextHandlerCollection parent,
    final String appDir) throws IOException {
    // set up the context for "/logs/" if "wms.log.dir" property is defined. 
    String logDir = System.getProperty("wms.log.dir");
    if (logDir != null) {
      Context logContext = new Context(parent, "/logs");
      logContext.setResourceBase(logDir);
      logContext.addServlet(DefaultServlet.class, "/");
      logContext.getInitParams().put(
            "org.mortbay.jetty.servlet.Default.aliases", "true");
      logContext.setDisplayName("logs");
      setContextAttributes(logContext);
      defaultContexts.put(logContext, true);
    }
    // set up the context for "/static/*"
    Context staticContext = new Context(parent, "/static");
    staticContext.setResourceBase(appDir + "/static");
    staticContext.addServlet(DefaultServlet.class, "/*");
    staticContext.setDisplayName("static");
    setContextAttributes(staticContext);
    defaultContexts.put(staticContext, true);
  }
  
  private void setContextAttributes(Context context) {
    context.getServletContext().setAttribute(CONF_CONTEXT_ATTRIBUTE, conf);
  }

  /**
   * Add default servlets.
   */
  protected void addDefaultServlets() {
    // set up default servlets
//     addServlet("stacks", "/stacks", StackServlet.class);
//     addServlet("logLevel", "/logLevel", LogLevel.Servlet.class);
//     addServlet("jmx", "/jmx", JMXJsonServlet.class);
//     addServlet("conf", "/conf", ConfServlet.class);
  }

  public void addContext(Context ctxt, boolean isFiltered)
      throws IOException {
    webServer.addHandler(ctxt);
    defaultContexts.put(ctxt, isFiltered);
  }

  /**
   * Add a context 
   * @param pathSpec The path spec for the context
   * @param dir The directory containing the context
   * @param isFiltered if true, the servlet is added to the filter path mapping 
   * @throws IOException
   */
  protected void addContext(String pathSpec, String dir, boolean isFiltered) throws IOException {
    if (0 == webServer.getHandlers().length) {
      throw new RuntimeException("Couldn't find handler");
    }
    WebAppContext webAppCtx = new WebAppContext();
    webAppCtx.setContextPath(pathSpec);
    webAppCtx.setWar(dir);
    addContext(webAppCtx, true);
  }

  /**
   * Set a value in the webapp context. These values are available to the jsp
   * pages as "application.getAttribute(name)".
   * @param name The name of the attribute
   * @param value The value of the attribute
   */
  public void setAttribute(String name, Object value) {
    setAttribute(webAppContext, name, value);
  }

  /**
   * Set a value in the webapp context. These values are available to the jsp
   * pages as "application.getAttribute(name)".
   * @param context Context to add attribute
   * @param name The name of the attribute
   * @param value The value of the attribute
   */
  public void setAttribute(Context context, String name, Object value) {
    context.setAttribute(name, value);
  }

  /** 
   * Add a Jersey resource package.
   * @param packageName The Java package name containing the Jersey resource.
   * @param pathSpec The path spec for the servlet
   */
  public void addJerseyResourcePackage(final String packageName,
      final String pathSpec) {
    LOG.info("addJerseyResourcePackage: packageName=" + packageName
        + ", pathSpec=" + pathSpec);
    final ServletHolder sh = new ServletHolder(ServletContainer.class);
    sh.setInitParameter("com.sun.jersey.config.property.resourceConfigClass",
        "com.sun.jersey.api.core.PackagesResourceConfig");
    sh.setInitParameter("com.sun.jersey.config.property.packages", packageName);
    webAppContext.addServlet(sh, pathSpec);
  }

  /**
   * Add a servlet in the server.
   * @param name The name of the servlet (can be passed as null)
   * @param pathSpec The path spec for the servlet
   * @param clazz The servlet class
   */
  public void addServlet(String name, String pathSpec,
      Class<? extends HttpServlet> clazz) {
    addInternalServlet(name, pathSpec, clazz, false, false);
    addFilterPathMapping(pathSpec, webAppContext);
  }

  /**
   * Add an internal servlet in the server.
   * @param name The name of the servlet (can be passed as null)
   * @param pathSpec The path spec for the servlet
   * @param clazz The servlet class
   * @deprecated this is a temporary method
   */
  @Deprecated
  public void addInternalServlet(String name, String pathSpec,
      Class<? extends HttpServlet> clazz) {
    addInternalServlet(name, pathSpec, clazz, false, false);
  }

  /**
   * Add an internal servlet in the server, specifying whether or not to
   * protect with Kerberos authentication. 
   * Note: This method is to be used for adding servlets that facilitate
   * internal communication and not for user facing functionality. For
   * servlets added using this method, filters (except internal Kerberos
   * filters) are not enabled. 
   * 
   * @param name The name of the servlet (can be passed as null)
   * @param pathSpec The path spec for the servlet
   * @param clazz The servlet class
   * @param requireAuth Require Kerberos authenticate to access servlet
   * @param useKsslForAuth true to use KSSL for auth, false to use SPNEGO
   */
 
  public void addInternalServlet(String name, String pathSpec, 
		  Class<? extends HttpServlet> clazz, boolean requireAuth,
		  boolean useKsslForAuth) {
	  ServletHolder holder = new ServletHolder(clazz);
	  if (name != null) {
		  holder.setName(name);
	  }
	  webAppContext.addServlet(holder, pathSpec);
  }
 
  /** {@inheritDoc} */
  public void addFilter(String name, String classname,
      Map<String, String> parameters) {

    final String[] USER_FACING_URLS = { "*.html", "*.jsp" };
    defineFilter(webAppContext, name, classname, parameters, USER_FACING_URLS);
    LOG.info("Added filter " + name + " (class=" + classname
        + ") to context " + webAppContext.getDisplayName());
    final String[] ALL_URLS = { "/*" };
    for (Map.Entry<Context, Boolean> e : defaultContexts.entrySet()) {
      if (e.getValue()) {
        Context ctx = e.getKey();
        defineFilter(ctx, name, classname, parameters, ALL_URLS);
        LOG.info("Added filter " + name + " (class=" + classname
            + ") to context " + ctx.getDisplayName());
      }
    }
    filterNames.add(name);
  }
 
  /** {@inheritDoc} */
  public void addGlobalFilter(String name, String classname,
      Map<String, String> parameters) {
    final String[] ALL_URLS = { "/*" };
    defineFilter(webAppContext, name, classname, parameters, ALL_URLS);
    for (Context ctx : defaultContexts.keySet()) {
      defineFilter(ctx, name, classname, parameters, ALL_URLS);
    }
    LOG.info("Added global filter" + name + " (class=" + classname + ")");
  }
 
  /**
   * Define a filter for a context and set up default url mappings.
   */
  protected void defineFilter(Context ctx, String name,
      String classname, Map<String,String> parameters, String[] urls) {

    FilterHolder holder = new FilterHolder();
    holder.setName(name);
    holder.setClassName(classname);
    holder.setInitParameters(parameters);
    FilterMapping fmap = new FilterMapping();
    fmap.setPathSpecs(urls);
    fmap.setDispatches(Handler.ALL);
    fmap.setFilterName(name);
    ServletHandler handler = ctx.getServletHandler();
    handler.addFilter(holder, fmap);
  }

  /**
   * Add the path spec to the filter path mapping.
   * @param pathSpec The path spec
   * @param webAppCtx The WebApplicationContext to add to
   */
  protected void addFilterPathMapping(String pathSpec,
      Context webAppCtx) {
    ServletHandler handler = webAppCtx.getServletHandler();
    for(String name : filterNames) {
      FilterMapping fmap = new FilterMapping();
      fmap.setPathSpec(pathSpec);
      fmap.setFilterName(name);
      fmap.setDispatches(Handler.ALL);
      LOG.info("addFilterPathMapping:" + fmap);
      handler.addFilterMapping(fmap);
    }
  }
  
  /**
   * Get the value in the webapp context.
   * @param name The name of the attribute
   * @return The value of the attribute
   */
  public Object getAttribute(String name) {
    return webAppContext.getAttribute(name);
  }

  /**
   * Get the pathname to the webapps files.
   * @return the pathname as a URL
   * @throws IOException if 'webapps' directory cannot be found on CLASSPATH.
   */
  protected String getWebAppsPath() throws IOException {
    URL url = getClass().getClassLoader().getResource("wms-webapps");
    if (url == null) 
      throw new IOException("webapps not found in CLASSPATH"); 
    return url.toString();
  }

  /**
   * Get the port that the server is on
   * @return the port
   */
  public int getPort() {
    return webServer.getConnectors()[0].getLocalPort();
  }

  /**
   * Set the min, max number of worker threads (simultaneous connections).
   */
  public void setThreads(int min, int max) {
    QueuedThreadPool pool = (QueuedThreadPool) webServer.getThreadPool() ;
    pool.setMinThreads(min);
    pool.setMaxThreads(max);
  }
 
  /**
   * Start the server. Does not wait for the server to start.
   */
  public void start() throws IOException {
    try {
      if(listenerStartedExternally) { // Expect that listener was started securely
        if(listener.getLocalPort() == -1) // ... and verify
          throw new Exception("Exepected webserver's listener to be started" +
          		"previously but wasn't");
        // And skip all the port rolling issues.
        webServer.start();
      } else {
        int port = 0;
        int oriPort = listener.getPort(); // The original requested port
        while (true) {
          try {
            port = webServer.getConnectors()[0].getLocalPort();
            LOG.info("Port returned by webServer.getConnectors()[0]." +
            		"getLocalPort() before open() is "+ port + 
            		". Opening the listener on " + oriPort);
            listener.open();
            port = listener.getLocalPort();
            LOG.info("listener.getLocalPort() returned " + listener.getLocalPort() + 
                  " webServer.getConnectors()[0].getLocalPort() returned " +
                  webServer.getConnectors()[0].getLocalPort());
            //Workaround to handle the problem reported in HADOOP-4744
            if (port < 0) {
              Thread.sleep(100);
              int numRetries = 1;
              while (port < 0) {
                LOG.warn("listener.getLocalPort returned " + port);
                if (numRetries++ > MAX_RETRIES) {
                  throw new Exception(" listener.getLocalPort is returning " +
                  		"less than 0 even after " +numRetries+" resets");
                }
                for (int i = 0; i < 2; i++) {
                  LOG.info("Retrying listener.getLocalPort()");
                  port = listener.getLocalPort();
                  if (port > 0) {
                    break;
                  }
                  Thread.sleep(200);
                }
                if (port > 0) {
                  break;
                }
                LOG.info("Bouncing the listener");
                listener.close();
                Thread.sleep(1000);
                listener.setPort(oriPort == 0 ? 0 : (oriPort += 1));
                listener.open();
                Thread.sleep(100);
                port = listener.getLocalPort();
              }
            } //Workaround end
            LOG.info("Jetty bound to port " + port);
            webServer.start();
            break;
          } catch (IOException ex) {
            // if this is a bind exception,
            // then try the next port number.
            if (ex instanceof BindException) {
              if (!findPort) {
                throw (BindException) ex;
              }
            } else {
              LOG.info("HttpServer.start() threw a non Bind IOException"); 
              throw ex;
           }
          } catch (MultiException ex) {
            LOG.info("HttpServer.start() threw a MultiException"); 
            throw ex;
          }
          listener.setPort((oriPort += 1));
        }
      }
      // Make sure there is no handler failures.
      Handler[] handlers = webServer.getHandlers();
      for (int i = 0; i < handlers.length; i++) {
        if (handlers[i].isFailed()) {
          throw new IOException(
              "Problem in starting http server. Server handlers failed");
        }
      }
      
      // Make sure there are no errors initializing the context.
      Throwable unavailableException = webAppContext.getUnavailableException();
      if (unavailableException != null) {
        // Have to stop the webserver, or else its non-daemon threads
        // will hang forever.
        webServer.stop();
        throw new IOException("Unable to initialize WebAppContext",
            unavailableException);
      }
    } catch (IOException e) {
      throw e;
    } catch (Exception e) {
      throw new IOException("Problem starting http server", e);
    }
  }

  /**
   * stop the server
   */
  public void stop() throws Exception {
    listener.close();
    webServer.stop();
  }

  public void join() throws InterruptedException {
    webServer.join();
  }

  /**
   * A very simple servlet to serve up a text representation of the current
   * stack traces. It both returns the stacks to the caller and logs them.
   * Currently the stack traces are done sequentially rather than exactly the
   * same data.
   */
  public static class StackServlet extends HttpServlet {
    private static final long serialVersionUID = -6284183679759467039L;

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response)
      throws ServletException, IOException {

      // Do the authorization
//      if (!HttpServer.isInstrumentationAccessAllowed(getServletContext(),
//          request, response)) {
//        return;
//      }

      PrintWriter out = new PrintWriter
                    (HtmlQuoting.quoteOutputStream(response.getOutputStream()));
//      ReflectionUtils.printThreadInfo(out, "");
      out.close();
//      ReflectionUtils.logThreadInfo(LOG, "jsp requested", 1);      
    }
  }
  
  /**
   * A Servlet input filter that quotes all HTML active characters in the
   * parameter names and values. The goal is to quote the characters to make
   * all of the servlets resistant to cross-site scripting attacks.
   */
  public static class QuotingInputFilter implements Filter {
    private FilterConfig config;

    public static class RequestQuoter extends HttpServletRequestWrapper {
      private final HttpServletRequest rawRequest;
      public RequestQuoter(HttpServletRequest rawRequest) {
        super(rawRequest);
        this.rawRequest = rawRequest;
      }
      
      /**
       * Return the set of parameter names, quoting each name.
       */
      @SuppressWarnings("unchecked")
      @Override
      public Enumeration<String> getParameterNames() {
        return new Enumeration<String>() {
          private Enumeration<String> rawIterator = 
            rawRequest.getParameterNames();
          @Override
          public boolean hasMoreElements() {
            return rawIterator.hasMoreElements();
          }

          @Override
          public String nextElement() {
            return HtmlQuoting.quoteHtmlChars(rawIterator.nextElement());
          }
        };
      }
      
      /**
       * Unquote the name and quote the value.
       */
      @Override
      public String getParameter(String name) {
        return HtmlQuoting.quoteHtmlChars(rawRequest.getParameter
                                     (HtmlQuoting.unquoteHtmlChars(name)));
      }
      
      @Override
      public String[] getParameterValues(String name) {
        String unquoteName = HtmlQuoting.unquoteHtmlChars(name);
        String[] unquoteValue = rawRequest.getParameterValues(unquoteName);
        String[] result = new String[unquoteValue.length];
        for(int i=0; i < result.length; ++i) {
          result[i] = HtmlQuoting.quoteHtmlChars(unquoteValue[i]);
        }
        return result;
      }

      @SuppressWarnings("unchecked")
      @Override
      public Map<String, String[]> getParameterMap() {
        Map<String, String[]> result = new HashMap<String,String[]>();
        Map<String, String[]> raw = rawRequest.getParameterMap();
        for (Map.Entry<String,String[]> item: raw.entrySet()) {
          String[] rawValue = item.getValue();
          String[] cookedValue = new String[rawValue.length];
          for(int i=0; i< rawValue.length; ++i) {
            cookedValue[i] = HtmlQuoting.quoteHtmlChars(rawValue[i]);
          }
          result.put(HtmlQuoting.quoteHtmlChars(item.getKey()), cookedValue);
        }
        return result;
      }
      
      /**
       * Quote the url so that users specifying the HOST HTTP header
       * can't inject attacks.
       */
      @Override
      public StringBuffer getRequestURL(){
        String url = rawRequest.getRequestURL().toString();
        return new StringBuffer(HtmlQuoting.quoteHtmlChars(url));
      }
      
      /**
       * Quote the server name so that users specifying the HOST HTTP header
       * can't inject attacks.
       */
      @Override
      public String getServerName() {
        return HtmlQuoting.quoteHtmlChars(rawRequest.getServerName());
      }
    }

    @Override
    public void init(FilterConfig config) throws ServletException {
      this.config = config;
    }

    @Override
    public void destroy() {
    }

    @Override
    public void doFilter(ServletRequest request, 
                         ServletResponse response,
                         FilterChain chain
                         ) throws IOException, ServletException {
      HttpServletRequestWrapper quoted = 
        new RequestQuoter((HttpServletRequest) request);
      HttpServletResponse httpResponse = (HttpServletResponse) response;

      String mime = inferMimeType(request);
      if (mime == null || mime.equals("text/html")) {
        // no extension or HTML with unspecified encoding, we want to
        // force HTML with utf-8 encoding
        // This is to avoid the following security issue:
        // http://openmya.hacker.jp/hasegawa/security/utf7cs.html
        httpResponse.setContentType("text/html; charset=utf-8");
      }
      chain.doFilter(quoted, httpResponse);
    }

    /**
     * Infer the mime type for the response based on the extension of the request
     * URI. Returns null if unknown.
     */
    private String inferMimeType(ServletRequest request) {
      String path = ((HttpServletRequest)request).getRequestURI();
      ContextHandler.SContext sContext = (ContextHandler.SContext)config.getServletContext();
      MimeTypes mimes = sContext.getContextHandler().getMimeTypes();
      Buffer mimeBuffer = mimes.getMimeByExtension(path);
      return (mimeBuffer == null) ? null : mimeBuffer.toString();
    }
  }
}
