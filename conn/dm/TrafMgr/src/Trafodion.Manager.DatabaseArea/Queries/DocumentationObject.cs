//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
//

using System;
using System.Collections.Generic;
using System.Collections;
using System.Text;

namespace Trafodion.Manager.DatabaseArea.Queries
{
    public class DocumentationObject
    {
        public const string QUERY_TITLE          = "QUERYTITLE:";
        public const string QUERY_FILENAME       = "QUERYFILENAME:";
        public const string QUERY_REVNUM         = "QUERYREVNUM:";
        public const string REPOS_REVNUM         = "REPOSREVNUM:";
        public const string APPLICATION          = "APPLICATION:";
        public const string QUERY_STYLE_REVNUM   = "QUERYSTYLEREVNUM:";
        public const string QUERY_SHORT_DESC     = "QUERYSHORTDESC:";
        public const string QUERY_PARAM_DESC     = "QUERYPARAMDESC:";
        public const string QUERY_LONG_DESC      = "QUERYLONGDESC:";
        public const string FORMAT_BARS          = "FORMATBARS:";

        private static string[] tokens = { QUERY_TITLE, QUERY_FILENAME, QUERY_REVNUM, REPOS_REVNUM, APPLICATION, QUERY_STYLE_REVNUM, FORMAT_BARS, QUERY_SHORT_DESC, QUERY_PARAM_DESC, QUERY_LONG_DESC };

        public static string[] DOCUMENTATION_TOKENS
        {
            get { return tokens; }
        }


        /** Creates a new instance of DocumentationObject */
        private string queryTitle;
        private string queryFileName;

        public string QueryFileName
        {
            get { return queryFileName; }
            set { queryFileName = value; }
        }
        private string queryRevNum;

        public string QueryRevNum
        {
            get { return queryRevNum; }
            set { queryRevNum = value; }
        }
        private string reposRevNum;

        public string ReposRevNum
        {
            get { return reposRevNum; }
            set { reposRevNum = value; }
        }
        private string application;

        public string Application
        {
            get { return application; }
            set { application = value; }
        }
        private string queryStyleRevNum;

        public string QueryStyleRevNum
        {
            get { return queryStyleRevNum; }
            set { queryStyleRevNum = value; }
        }
        private string shortDescription;

        public string ShortDescription
        {
            get { return shortDescription; }
            set { shortDescription = value; }
        }
        private string longDescription;

        public string LongDescription
        {
            get { return longDescription; }
            set { longDescription = value; }
        }
        private Hashtable parameters;

        public Hashtable Parameters
        {
            get { return parameters; }
            set { parameters = value; }
        }
        
        public DocumentationObject() {
        }

        public string QueryTitle
        {
            get { return queryTitle; }
            set { queryTitle = value; }
        }

    }
}
