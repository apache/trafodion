/*
 * PropertyConfigurator.cpp
 *
 * Copyright 2001, Glen Scott. All rights reserved.
 *
 * (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "PortabilityImpl.hh"
#include <log4cpp/PropertyConfigurator.hh>
#include "PropertyConfiguratorImpl.hh"

namespace log4cpp {

    void PropertyConfigurator::configure(const std::string& initFileName) throw (ConfigureFailure) {
        PropertyConfiguratorImpl configurator;

        configurator.doConfigure(initFileName);
    }

    void PropertyConfigurator::configure(const std::string& initFileName,
    	                                 const char* logsFolder) throw (ConfigureFailure) {
        PropertyConfiguratorImpl configurator;

        configurator.setLogsFolder(logsFolder);
        configurator.doConfigure(initFileName);
    }

    void PropertyConfigurator::configure(const std::string& initFileName,
    	                                 const char* logsFolder,
                                         const char* fileSuffix) throw (ConfigureFailure) {
        PropertyConfiguratorImpl configurator;

        configurator.setLogsFolder(logsFolder);
        if (fileSuffix)
           configurator.setFileSuffix(fileSuffix);
        configurator.doConfigure(initFileName);
    }

}

