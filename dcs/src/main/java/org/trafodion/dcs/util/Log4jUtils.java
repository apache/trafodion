/**********************************************************************
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**********************************************************************/
package org.trafodion.dcs.util;

import java.util.*;
import java.net.URL;
import java.net.URLClassLoader;
import java.io.*;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

//import org.apache.commons.logging.Log;
//import org.apache.commons.logging.LogFactory;

import org.apache.log4j.*;
import org.apache.log4j.config.PropertyPrinter;

import org.apache.hadoop.conf.Configuration;

import org.trafodion.dcs.util.DcsConfiguration;
import org.trafodion.dcs.Constants;

public final class Log4jUtils {
    public void dumpPath() {
        System.out.println("#path");
        System.out.println(System.getProperty("java.class.path"));
        System.out.println("#End of path");
    }
    
    public void dumpClasspath() {
        System.out.println("#classpath");
        System.out.println(System.getProperty("java.class.path"));
        ClassLoader cl = this.getClass().getClassLoader();
        URL[] urls = ((URLClassLoader)cl).getURLs();
        for(URL url: urls) {
            System.out.println(url.getFile());
        }
        System.out.println("#End of classpath");
    }
    
    public void resetLog4j() {
         //LogManager.resetConfiguration();
        Logger.getRootLogger().getLoggerRepository().resetConfiguration();
        ClassLoader cl = this.getClass().getClassLoader();
        URL log4jprops = cl.getResource("log4j.properties");
        System.out.println("log4jprops=" + log4jprops);
        if (log4jprops != null) {
            PropertyConfigurator.configure(log4jprops);
        }
    }
    
    public void dumpLog4j(String text) {
        System.out.println(text);
        System.out.println("#log4j Loggers");
        List<String> stringListOfLoggers = new ArrayList<String>();
        stringListOfLoggers.add(LogManager.getRootLogger().getName());
        Enumeration<?> loggers = LogManager.getLoggerRepository().getCurrentLoggers();
        while(loggers.hasMoreElements()) {
            org.apache.log4j.Logger logger = (org.apache.log4j.Logger)loggers.nextElement();
            String nameAndLevel = logger.getName() + "=" + logger.getLevel();
            stringListOfLoggers.add(nameAndLevel);
            for (Enumeration appenders=logger.getAllAppenders(); appenders.hasMoreElements(); )  {
                Appender appender = (Appender) appenders.nextElement();
                String name = appender.getName();
                stringListOfLoggers.add("appender=" + name);
            }
        }
        System.out.println(stringListOfLoggers);
        System.out.println("#End of Loggers");
        printLog4jProps(text);
    }
    
    public void printLog4jProps(String text) {
        System.out.println(text);
        System.out.println("#log4j Config");
        PrintWriter pw = new PrintWriter(System.out);
        PropertyPrinter pp = new PropertyPrinter(pw);
        pp.print(pw);
        System.out.println("#End of Config");
    }
}