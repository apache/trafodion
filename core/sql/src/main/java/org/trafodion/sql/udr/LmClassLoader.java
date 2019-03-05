/**********************************************************************
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
import java.util.StringTokenizer;
import java.util.Vector;

import java.util.jar.JarFile;
import java.util.jar.JarEntry;

import java.net.URL;
import java.net.URLConnection;
import java.net.URLClassLoader;

import java.security.AccessController;
import java.security.CodeSource;
import java.security.PrivilegedActionException;
import java.security.PrivilegedExceptionAction;

/**
 * LmClassLoader is an extension of URLClassLoader which in turn is
 * derived from SecureClassLoader.  This class is responsible for
 * loading the classes from its search path.  Since Java class loaders
 * try the parent class loader first, the search path of this class
 * loader tries the following, in sequence:
 * - the CLASSPATH as defined in the environment variable
 * - the name of the container for the UDR
 * - jar files in $TRAF_VAR/udr/public/external_libs, in alphabetical
 *   order
 *
 **/
public class LmClassLoader extends ClassLoader
{
  private final static boolean DEBUG = false; // static DEBUG

    private ChildURLClassLoader childClassLoader;

    private static class FindClassClassLoader extends ClassLoader {
        public FindClassClassLoader(ClassLoader parent) {
            super(parent);
        }

        @Override
        public Class<?> findClass(String name) throws ClassNotFoundException {
            return super.findClass(name);
        }
    }

    private static class ChildURLClassLoader extends URLClassLoader {
        private FindClassClassLoader realParent;

        
        public ChildURLClassLoader(URL[] urls, FindClassClassLoader realParent) {
            super(urls, null);

            this.realParent = realParent;
        }

        @Override
        public Class<?> findClass(String name) throws ClassNotFoundException {
            try {
                Class<?> loaded = super.findLoadedClass(name);
                if (loaded != null)
                    return loaded;
                return super.findClass(name);
            } catch (ClassNotFoundException e) {
                return realParent.loadClass(name);
            }
        }
    }

  /**
   * Creates a new instance for a given set of URLs.
   * And adds external path to the search path.
   * @param extPath    external path this instance is responsible for
   * @param debug      debug flag
   *
   **/
  public LmClassLoader(URL[] urls)
  {
      //super(urls);
      super(Thread.currentThread().getContextClassLoader());
      childClassLoader = new ChildURLClassLoader(urls, new FindClassClassLoader(this.getParent()));
  }

  String getContainerPath()
  {
      // The first URL points to the container. Get the path of that URL
      return childClassLoader.getURLs()[0].getPath();
  }

  /**
   * Finds the class in the path specified in the LmClassLoader constructor
   * without looking in other files. This does not actually load the class.
   * @param  name of the class name to be loaded
   * @return the size of the path file
   * @throws  ClassNotFoundException
   *
   **/

  long verifyClassIsInFile(String resourceName)
    throws ClassNotFoundException
  {
    // Change the package qualified name to directory structure name
    final String dname = resourceName.replace('.', '/') + ".class";

    try
    {
        File f =
          AccessController.doPrivileged(new PrivilegedExceptionAction<File>()
              {
                  public File run() throws Exception
                  {
                      return findResourceInternal(dname);
                  }
              });

      if (f != null)
          return f.length();
      else 
      {
        throw new ClassNotFoundException(resourceName);
      }
    } catch (PrivilegedActionException e)
        {
          throw new ClassNotFoundException(resourceName, e.getException());
        }
      catch (ClassNotFoundException e)
	{
	  throw e;
	}
      catch (Exception e)
        {
	  throw new ClassNotFoundException(resourceName, e);
        }
  }

  /**
   * Finds the resource with the given name.
   * This method only checks the given container, not the CLASSPATH or
   * other jars.
   * @param  name  the resource name
   * @return a File object for the file containing the resource or null if not found
   **/
  public File findResourceInternal(String name) throws Exception
  {
    String path = getContainerPath();
    File f = new File(path);

    if (DEBUG)
      System.out.println("LmClassLoader::findResourceInternal trying " + path);

    if (f.isFile())
      {
        JarFile jf = new JarFile(path);
        JarEntry je = jf.getJarEntry(name);
        jf.close();
        if (je != null)
          {
            if (DEBUG)
              System.out.println("LmClassLoader::findResourceInternal returning jar:file:" + path + "!/" + name);
            return f;
          }
      }

    if (DEBUG)
        System.out.println("LmClassLoader::findResourceInternal did not find resource " + name + "in container " + path);
    return null;
  }
    @Override
    protected synchronized Class<?> loadClass(String name, boolean resolve) throws ClassNotFoundException {
        try {
            Class<?> result = childClassLoader.findClass(name);
            if (resolve)
                resolveClass(result);
            return result;
        } catch (ClassNotFoundException e) {
            return super.loadClass(name, resolve);
        }
    }
}
