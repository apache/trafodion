package org.trafodion.wms.rest;

/**
 * Common constants for org.trafodion.wms.rest
 */
public interface RestConstants {
  public static final String VERSION_STRING = "0.0.2";

  public static final int DEFAULT_MAX_AGE = 60 * 60 * 4;  // 4 hours

  public static final int DEFAULT_LISTEN_PORT = 8080;

  public static final String MIMETYPE_TEXT = "text/plain";
  public static final String MIMETYPE_HTML = "text/html";
  public static final String MIMETYPE_XML = "text/xml";
  public static final String MIMETYPE_BINARY = "application/octet-stream";
  public static final String MIMETYPE_JSON = "application/json";

  public static final String CRLF = "\r\n";
}
