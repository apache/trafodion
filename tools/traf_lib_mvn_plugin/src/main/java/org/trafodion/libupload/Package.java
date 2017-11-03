/**
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
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * @@@ END COPYRIGHT @@@
 */
package org.trafodion.libupload;

import org.apache.maven.plugin.AbstractMojo;
import org.apache.maven.plugin.MojoExecutionException;
import org.apache.maven.plugin.MojoFailureException;
import org.apache.maven.plugins.annotations.LifecyclePhase;
import org.apache.maven.plugins.annotations.Mojo;
import org.apache.maven.plugins.annotations.Parameter;

import java.io.File;
import java.io.FileInputStream;
import java.sql.CallableStatement;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;

@Mojo(name = "package", defaultPhase = LifecyclePhase.PACKAGE)
public class Package extends AbstractMojo {

    @Parameter(property = "driveClass", defaultValue = "org.trafodion.jdbc.t4.T4Driver")
    private String className;
    @Parameter(property = "password")
    private String password;
    @Parameter(property = "userName")
    private String userName;
    @Parameter(property = "url")
    private String url;
    @Parameter(property = "uploadJarSql", defaultValue = "call \"_LIBMGR_\".put(?,?,?,?)")
    private String uploadJarSql;
    @Parameter(property = "libName")
    private String libName;

    @Parameter(property = "createLibSql", defaultValue = "call \"_LIBMGR_\".addLib(?,?,?,?)")
    private String createLibSql;
    @Parameter(property = "project.artifactId")
    private String artifactId;
    @Parameter(property = "project.version")
    private String version;
    @Parameter(property = "basedir")
    private String basedir;
    @Parameter(property = "skipCreateLib", defaultValue = "true")
    private Boolean skipCreateLib;

    public void execute() throws MojoExecutionException, MojoFailureException {
        Connection conn = null;
        CallableStatement uploadJarStatement = null;
        CallableStatement createLibSqlStatement = null;
        try {
            Class.forName(this.className);
            conn = DriverManager.getConnection(this.url, this.userName, this.password);
            getLog().info(String.format("Connected with %s, %s, %s", this.className, this.url, this.userName));
            uploadJarStatement = conn.prepareCall(uploadJarSql);
            getLog().info(String.format("Prepared %s", uploadJarSql));
            String fileName = this.artifactId + "-" + this.version + ".jar";
            String jarPath = this.basedir + File.separator + "target" + File.separator +
                    fileName;
            FileInputStream jar = new FileInputStream(jarPath);
            byte[] buffer = new byte[102400];
            int n = -1;
            int append = 1;
            while ((n = jar.read(buffer)) > 0) {
                uploadJarStatement.setString(1, new String(buffer, 0, n));
                uploadJarStatement.setString(2, fileName);
                uploadJarStatement.setInt(3, append);
                uploadJarStatement.setInt(4, 0);
                if (append == 1) {
                    append = 0;
                }
                uploadJarStatement.execute();
            }
            getLog().info(String.format("The Jar has been uploaded with path %s.", jarPath));
            if (!this.skipCreateLib) {
                createLibSqlStatement = conn.prepareCall(createLibSql);
                getLog().info(String.format("Prepared %s.", createLibSql));
                getLog().info(String.format("libName: %s, filename: %s", this.libName, fileName));
                createLibSqlStatement.setString(1, this.libName);
                createLibSqlStatement.setString(2, fileName);
                createLibSqlStatement.setString(3, "hostname");
                createLibSqlStatement.setString(4, "local file");
                createLibSqlStatement.execute();
                getLog().info(String.format("Library %s has been created.", this.libName));
            }
        } catch (Exception e) {
            getLog().error(e.getMessage(), e);
            throw new MojoFailureException(e.getMessage());
        } finally {
            if (uploadJarStatement != null) {
                try {
                    uploadJarStatement.close();
                } catch (SQLException e) {
                    getLog().warn(e.getMessage(), e);
                }
            }
            if (createLibSqlStatement != null) {
                try {
                    createLibSqlStatement.close();
                } catch (SQLException e) {
                    getLog().warn(e.getMessage(), e);
                }
            }
            if (conn != null) {
                try {
                    conn.close();
                } catch (SQLException e) {
                    getLog().warn(e.getMessage(), e);
                }
            }
        }
    }

    public String getClassName() {
        return className;
    }

    public void setClassName(String className) {
        this.className = className;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public String getUserName() {
        return userName;
    }

    public void setUserName(String userName) {
        this.userName = userName;
    }

    public String getUrl() {
        return url;
    }

    public void setUrl(String url) {
        this.url = url;
    }

    public Boolean getSkipCreateLib() {
        return skipCreateLib;
    }

    public void setSkipCreateLib(Boolean skipCreateLib) {
        this.skipCreateLib = skipCreateLib;
    }
}
