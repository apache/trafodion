/*
 * FileAppender.hh
 *
 * Copyright 2000, LifeLine Networks BV (www.lifeline.nl). All rights reserved.
 * Copyright 2000, Bastiaan Bakker. All rights reserved.
 *
 * (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef _LOG4CPP_FILEAPPENDER_HH
#define _LOG4CPP_FILEAPPENDER_HH

#include <log4cpp/Portability.hh>
#include <log4cpp/LayoutAppender.hh>
#include <string>
#include <stdarg.h>

namespace log4cpp {

    class LOG4CPP_EXPORT FileAppender : public LayoutAppender {
        public:

        /**
           Constructs a FileAppender.
           @param name the name of the Appender.
           @param fileName the name of the file to which the Appender has 
           to log.
           @param append whether the Appender has to truncate the file or
           just append to it if it already exists. Defaults to 'true'.
           @param mode file mode to open the logfile with. Defaults to 00644.
           @param addPid whether to append the process ID to the file name, 
           to allow multiple instances of the same process, each writing to 
           a separate log file.
        **/  
        FileAppender(const std::string& name, const std::string& fileName,
                     bool append = true, mode_t mode = 00644, bool addPid = false);

        /**
           Constructs a FileAppender to an already open file descriptor.
           @param name the name of the Appender.
           @param fd the file descriptor to which the Appender has to log.
        **/
        FileAppender(const std::string& name, int fd);
        virtual ~FileAppender();
        
        /**
           Reopens the logfile. 
           This can be useful for logfiles that are rotated externally,
           e.g. by logrotate. This method is a NOOP for FileAppenders that
           have been constructed with a file descriptor.           
           @returns true if the reopen succeeded.
        **/
        virtual bool reopen();

        /**
           Closes the logfile.
        **/
        virtual void close();

        /**
           Sets the append vs truncate flag.
           NB. currently the FileAppender opens the logfile in the 
           constructor. Therefore this method is too late to influence the 
           first file opening. We'll need something similar to log4j's
           activateOptions().
           @param append false to truncate, true to append
        **/
        virtual void setAppend(bool append);

        /**
           Gets the value of the 'append' option.
        **/
        virtual bool getAppend() const;

        /**
           Sets the file open mode.
        **/
        virtual void setMode(mode_t mode);

        /**
           Gets the file open mode.
        **/
        virtual mode_t getMode() const;

        protected:
        virtual void _append(const LoggingEvent& event);
        
	std::string& appendPid(const std::string fileName, bool addPid);

        const std::string _fileName;
        int _fd;
        int _flags;
        mode_t _mode;
    };
}

#endif // _LOG4CPP_FILEAPPENDER_HH
