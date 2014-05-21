/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1997-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-Java-*-
 ******************************************************************************
 *
 * File:         LmClassLoader.java
 * Description:  Language Manager's Java Class Loader
 *
 * Created:      07/01/1999
 * Language:     Java
 *
 *
 ******************************************************************************
 */
package org.trafodion.sql.udr;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.IOException;

import java.util.StringTokenizer;
import java.util.Vector;

import java.util.jar.JarFile;
import java.util.jar.JarEntry;

import java.net.URL;
import java.net.URLConnection;

import java.security.AccessController;
import java.security.CodeSource;
import java.security.PrivilegedAction;
import java.security.PrivilegedActionException;
import java.security.PrivilegedExceptionAction;
import java.security.SecureClassLoader;

/**
 * LmClassLoader is an extension of SecureClassLoader.
 * This class is responsible for loading the classes from its search path.
 * Search Path includes external path(the directory specified in 'external path'
 * clause of 'create procedure' statement) and CLASSPATH env variable. This class
 * always checks external path first before checking CLASSPATH.
 *
 * CLASSPATH can be removed from the search path by calling removeCpURLs() 
 * and added by addCpURLs(). This facility is provided to have the main class
 * (the one specified in 'external name' clause of 'create procedure') loaded
 * only from 'external path'. Typically, applications call removeCpURLs() before
 * loading main class and call addCpURLs() after loading the class.
 *
 **/
public class LmClassLoader extends SecureClassLoader
{
  private int debug_; 	      // Debug flag.
  private String path_;       // The external path managed by the loader.
  private int size_;          // Total size in bytes for loaded classes.
  private boolean chkCpURLs_; // flag to check CLASSPATH URLs.
  private URL urlCS_;         // The URL (used to get the CodeSource)
			      // corresponding to the loaded class. 
  private final static boolean DEBUG = false; // static DEBUG

  static private Vector cpURLs_ = null;    // vector of URLs used by all instances
				           // of this Class. Created from CLASSPATH

  /**
   * Creates a new instance for given external path.
   * And adds external path to the search path.
   * @param extPath    external path this instance is responsible for
   * @param debug      debug flag
   *
   **/
  public LmClassLoader(String extPath, int debug)
  {
    super();
    debug_ = debug;
    if (DEBUG)
      System.out.println("LmClassLoader::LmClassLoader extPath=" + extPath);
    path_ = extPath;
    size_ = 0;
    chkCpURLs_ = true;
  }

  /**
   * Sets the classpath.
   * This classpath is seen by all the LmClassLoader instances.
   * @param  classPath  CLASSPATH to set
   *
   **/
  public static void setClassPath(String classPath)
  {
    cpURLs_ = createURLVector(classPath);
  }

  /**
   * Creates a vector of URLs from given path.
   * Here URL refers to a String type (not java.net.URL type) representing dir or
   * jar file. 'path' is a set of elements(dir or jar file names) separated by
   * ':' or ';' depending on the platform.
   *
   * @param   path set of directories or jar file names separated by ':' or ';'
   * @return  a Vector of String type(dirs and jars)
   *
   **/
  private static Vector createURLVector(String path)
  {
    Vector v = new Vector();
    if (path == null)
    {
      return v;
    }

    StringTokenizer st = new StringTokenizer(path, File.pathSeparator);

    while (st.hasMoreTokens())
    {
      String url = st.nextToken();
      v.addElement(url);
    }

    return v;
  }

  /**
   * Accessor method to get the size of all the loaded classes by this
   * ClassLoader.
   * @return size of all the loaded classes.
   *
   **/
  public int size()
  {
    return size_;
  }

  /**
   * Adds CLASSPATH URLs to the search path of this ClassLoader instance.
   *
   **/
  public void addCpURLs()
  {
    chkCpURLs_ = true;
    return;
  }

  /**
   * Removes CLASSPATH URLs from the search list of this ClassLoader instance.
   * CLASSPATH won't be checked for ClassLoading until addCpURLs() is called.
   *
   **/
  public void removeCpURLs()
  {
    chkCpURLs_ = false;
    return;
  }

  /**
   * Finds the class in the search path of this ClassLoader instance.
   * Search path includes external path and CLASSPATH(if it is added to
   * the search path) in that order.
   * This method overrides the one in baseclass.
   * @param  name class name to be loaded
   * @return the resulting Class object
   *
   **/

  protected Class findClass(String name)
    throws ClassNotFoundException
  {
    // Change the package qualified name to directory structure name
    final String dname = name.replace('.', '/') + ".class";

    try
    {
      byte[] b = (byte[]) 
      AccessController.doPrivileged(new PrivilegedExceptionAction()
      {
	 public Object run() throws Exception 
	 {
	   return loadClassData(dname);
	 }
      });

      if (b != null) 
      {
        size_ += b.length;

        // Get the CodeSource for the url. urlCS_  was set in
        // loadClassData. urlCS_ and will be deallocated before returning.
        CodeSource cs = new CodeSource(urlCS_, (java.security.cert.Certificate[])null);

        Class c = defineClass(name, b, 0, b.length, cs);
        urlCS_ = null;
        return c;
      } 
      else 
      {
        throw new ClassNotFoundException(name);
      }
    } catch (PrivilegedActionException e)
        {
          throw new ClassNotFoundException(name, e.getException());
        }
      catch (ClassNotFoundException e)
	{
	  throw e;
	}
      catch (Exception e)
        {
	  throw new ClassNotFoundException(name, e);
        }
  }

  /**
   * Reads the bytes of the given class from external path or CLASSPATH(if included
   * in search path).
   * @param  name class name to read
   * @return bytes of the class,
   *         null if the class is not found
   **/
  private byte[] loadClassData(String name)
  throws Exception
  {
    URL u = findResourceInternal(name, chkCpURLs_);

    if (u == null)
      return null;

    urlCS_ = u;

    return readFromURLSource(u);
  }

  /**
   * Reads bytes from the given URL. 
   * @param   url URL to read from
   * @return  bytes of the file
   *          null if the protocol is not 'file' or 'jar'
   **/
  private byte[] readFromURLSource(URL url)
    throws Exception
  {
    BufferedInputStream bis = null;

    try
    {
      String prot = url.getProtocol();
      if (!prot.equals("file") && !prot.equals("jar"))
        return null;

      URLConnection connection = url.openConnection();
      connection.setDefaultUseCaches(false); // Do not use caches

      bis = new BufferedInputStream(connection.getInputStream());
      int size = connection.getContentLength();

      return readFromStream(bis, size);
    } catch(Exception e)
        {
          throw e;
        }
      finally
        {
          try
          {
            bis.close();
          } catch (Exception e)
              { }
        }
  }

  /**
   * Reads specified number of bytes from the given stream.
   * Caller needs to open and close the  stream.
   * @param bis  Buffered input stream to read from
   * @param size number of bytes to read
   * @return byte array
   *
   **/
  private byte[] readFromStream(BufferedInputStream bis, int size)
  throws Exception
  {
    byte buf[] = new byte[size];
    int bytesRead = 0, offset = 0;

    while ((size - offset) > 0)
    {
      bytesRead = bis.read(buf, offset, size - offset);
      if (bytesRead == -1)
        break;
      offset += bytesRead;
    }

    return buf;
  }

  /**
   * Finds the resource with the given name.
   * Checks search path (external path and CLASSPATH) for the resource.
   * This method overrides the one in baseclass.
   * @param  name  the resource name
   * @return a URL for reading the resource,
   *         or null if the resource could not be found
   **/
  protected URL findResource(String name)
  {
    return findResourceInternal(name, true);
  }

  /**
   * Finds the resource with the given name.
   * Checks search path (external path and CLASSPATH) for the resource.
   * @param  name  the resource name
   * @return a URL for reading the resource,
   *         or null if the resource could not be found
   **/
  private URL findResourceInternal(String name, boolean searchClassPath)
  {
    int searchSize = 0;
    if (searchClassPath == true && cpURLs_ != null)
      searchSize = cpURLs_.size();

    for (int i=-1; i<searchSize; i++)
    {
      String element;

      if (i == -1)
        element = path_;
      else
        element = (String)cpURLs_.elementAt(i);

      try
      {
        if (DEBUG)
          System.out.println("LmClassLoader::findResourceInternal trying " + element);
        File f = new File(element);
        if (f.isFile())
        {
          JarFile jf = new JarFile(element);
          JarEntry je = jf.getJarEntry(name);
          if (je != null)
          {
            if (DEBUG)
              System.out.println("LmClassLoader::findResourceInternal returning jar:file:" + element + "!/" + name);
            return (new URL("jar:file:" + element + "!/" + name));
          }
        }
        else
        {
          String filename = element + "/" + name;
          if (DEBUG)
            System.out.println("LmClassLoader::findResourceInternal trying " + filename);
          File file = new File(filename);
          if (file.exists() == true)
          {
            if (DEBUG)
              System.out.println("LmClassLoader::findResourceInternal returning file:" + filename);
            return (new URL("file:" + filename));
          }
        }
      } catch (Exception e) { }

    }

    return null;
  }
}
