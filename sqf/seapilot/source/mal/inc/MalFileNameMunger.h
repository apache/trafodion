// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef _MAL_FILE_NAME_MUNGER_
#define _MAL_FILE_NAME_MUNGER_

#include <string>

namespace Trinity {

    //! The class MalFileNameMunger provides file name massaging services.
    class MalFileNameMunger
    {
    public:

        //! Constructor
        //! \param [in] fileNameTemplate gives a string from which a file name
        //! will be built by adding the optionalParameter and a process identifier
        //! \param [in] sequenceNumber if >= 0 denotes a sequence number that
        //! will be appended to the file name; if < 0 no sequence number is appended
        //! \param [in] addPid if true the pid will be appended to the file name
        //! \param [in] optionalParameter if non-empty is a string that will be
        //! placed inside the munged file name
        MalFileNameMunger(const std::string & fileNameTemplate,
                          int32_t sequenceNumber,
                          bool addPid,
                          const std::string & optionalParameter);

        //! Destructor
        ~MalFileNameMunger(void);

        //! returns the munged file name, with optional parameter and pid added
        //! \return the munged file name
        std::string getMungedFileName(void);

        //! returns the directory part of a file name
        //! \return the directory part of a file name
        std::string getDirectory(void);

        //! returns a prefix of the munged file name, lacking directory and lacking
        //! sequence number and suffix
        //! \return a prefix of a file name
        std::string getFileNamePrefix(void);

    private:

        //! Too hard to debug in ctor due to lack of symbols; gcc/gdb quirk; so guts
        //! of constructor are here.
        void munge(const std::string & fileNameTemplate,
                          int32_t sequenceNumber,
                          bool addPid,
                          const std::string & optionalParameter);

        //! the munged file name to return
        std::string mungedFileName_;

        //! the directory part
        std::string directory_;

        //! the prefix part (no directory, no sequence number, no suffix)
        std::string prefix_;
    };

}

#endif  // _MAL_FILE_NAME_MUNGER_
