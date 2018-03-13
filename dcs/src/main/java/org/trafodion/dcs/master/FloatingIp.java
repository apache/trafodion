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
package org.trafodion.dcs.master;

import java.io.IOException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.trafodion.dcs.script.ScriptManager;
import org.trafodion.dcs.script.ScriptContext;
import org.trafodion.dcs.Constants;

public class FloatingIp {
    private static final Log LOG = LogFactory.getLog(FloatingIp.class);
    private static FloatingIp instance = null;
    private DcsMaster master;
    private boolean isEnabled;

    public static FloatingIp getInstance(DcsMaster master) throws Exception {
        if (instance == null) {
            instance = new FloatingIp(master);
        }
        return instance;
    }

    private FloatingIp(DcsMaster master) throws Exception {
        try {
            this.master = master;
            isEnabled = master.getConfiguration().getBoolean(
                    Constants.DCS_MASTER_FLOATING_IP,
                    Constants.DEFAULT_DCS_MASTER_FLOATING_IP);
        } catch (Exception e) {
            e.printStackTrace();
            if (LOG.isErrorEnabled())
                LOG.error(e);
            throw e;
        }
    }

    public boolean isEnabled() {
        return isEnabled;
    }

    public synchronized int unbindScript() throws Exception {
        if (isEnabled)
            LOG.info("Floating IP is enabled");
        else {
            LOG.info("Floating IP is disabled");
            return 0;
        }

        ScriptContext scriptContext = new ScriptContext();
        scriptContext.setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);
        scriptContext.setStripStdOut(false);
        scriptContext.setStripStdErr(false);

        String command = master.getConfiguration().get(Constants.DCS_MASTER_FLOATING_IP_COMMAND_UNBIND,
                Constants.DEFAULT_DCS_MASTER_FLOATING_IP_COMMAND_UNBIND);

        scriptContext.setCommand(command);
        LOG.info("Unbind Floating IP [" + scriptContext.getCommand() + "]");
        ScriptManager.getInstance().runScript(scriptContext);// Blocking call

        StringBuilder sb = new StringBuilder();
        sb.append("exit code [" + scriptContext.getExitCode() + "]");
        if (!scriptContext.getStdOut().toString().isEmpty())
            sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
        if (!scriptContext.getStdErr().toString().isEmpty())
            sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
        if (LOG.isErrorEnabled())
            LOG.error(sb.toString());

        if (scriptContext.getExitCode() == 0)
            LOG.info("Unbind Floating IP successful, exit code [" + 0 + "]");
        else
            LOG.error("Unbind Floating IP failed, exit code [" + scriptContext.getExitCode() + "]");

        return scriptContext.getExitCode();
    }

    public synchronized int runScript() throws Exception {
        if (isEnabled)
            LOG.info("Floating IP is enabled");
        else {
            LOG.info("Floating IP is disabled");
            return 0;
        }

        ScriptContext scriptContext = new ScriptContext();
        scriptContext.setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);
        scriptContext.setStripStdOut(false);
        scriptContext.setStripStdErr(false);

        String externalInterface = master.getConfiguration().get(
                Constants.DCS_MASTER_FLOATING_IP_EXTERNAL_INTERFACE,
                Constants.DEFAULT_DCS_MASTER_FLOATING_IP_EXTERNAL_INTERFACE);
        if (externalInterface.equalsIgnoreCase("default")) {
            LOG.info("When floating IP feature is enabled the property "
                    + Constants.DCS_MASTER_FLOATING_IP_EXTERNAL_INTERFACE
                    + " must be specified in your dcs-site.xml");
            return 0;
        }

        String externalIpAddress = master.getConfiguration().get(
                Constants.DCS_MASTER_FLOATING_IP_EXTERNAL_IP_ADDRESS,
                Constants.DEFAULT_DCS_MASTER_FLOATING_IP_EXTERNAL_IP_ADDRESS);
        if (externalIpAddress.equalsIgnoreCase("default")) {
            LOG.info("When floating IP feature is enabled the property "
                    + Constants.DCS_MASTER_FLOATING_IP_EXTERNAL_IP_ADDRESS
                    + " must be specified in your dcs-site.xml");
            return 0;
        }

        int masterPort = master.getConfiguration().getInt(
                Constants.DCS_MASTER_PORT, Constants.DEFAULT_DCS_MASTER_PORT);

        String floatingIpCommand = master.getConfiguration().get(
                Constants.DCS_MASTER_FLOATING_IP_COMMAND,
                Constants.DEFAULT_DCS_MASTER_FLOATING_IP_COMMAND);

        String command = floatingIpCommand
                .replace("-i", "-i " + externalInterface + " ")
                .replace("-a", "-a " + externalIpAddress + " ")
                .replace("-p", "-p " + masterPort);
        scriptContext.setCommand(command);
        LOG.info("Floating IP [" + scriptContext.getCommand() + "]");
        ScriptManager.getInstance().runScript(scriptContext);// Blocking call

        StringBuilder sb = new StringBuilder();
        sb.append("exit code [" + scriptContext.getExitCode() + "]");
        if (!scriptContext.getStdOut().toString().isEmpty())
            sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
        if (!scriptContext.getStdErr().toString().isEmpty())
            sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
        if (LOG.isErrorEnabled())
            LOG.error(sb.toString());

        if (scriptContext.getExitCode() == 0)
            LOG.info("Floating IP successful");
        else
            LOG.error("Floating IP failed");

        return scriptContext.getExitCode();
    }
}