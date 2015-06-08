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
using System.Text;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.MetricMiner
{
    public class LibraryManager
    {
        private static List<String> _theLibraryPaths = null;
        private static readonly string thePersistenceKey = "MetricMinerReportFolderPersistence";
        private static LibraryManager instance = new LibraryManager();
        private static string _theLastPath = null;


        public static LibraryManager Instance
        {
            get { return instance; }
        }

        private LibraryManager()
        {
            LoadPersistence();
            Persistence.PersistenceHandlers += new Persistence.PersistenceHandler(PersistencePersistenceHandlers);
        }


        private static void PersistencePersistenceHandlers(Dictionary<string, object> aDictionary, Persistence.PersistenceOperation aPersistenceOperation)
        {
            switch (aPersistenceOperation)
            {
                case Persistence.PersistenceOperation.Load:
                    {
                        LoadPersistence();
                        break;
                    }
                case Persistence.PersistenceOperation.Save:
                    {
                        aDictionary[thePersistenceKey] = _theLibraryPaths;
                        break;
                    }
            }
        }

        
        static void LoadPersistence()
        {
            _theLibraryPaths = Persistence.Get(thePersistenceKey) as List<String>;
            if (_theLibraryPaths == null)
            {
                _theLibraryPaths = new List<string>();
            }
        }

        public string LastUsedPath
        {
            get 
            {
                if (_theLibraryPaths.Count > 0)
                {
                    return _theLibraryPaths[_theLibraryPaths.Count - 1];
                }
                //return windows default direcotry if last used path is null or empty.
                return Environment.ExpandEnvironmentVariables("%HOMEDRIVE%%HOMEPATH%");
            }
        }


        public List<String> LibraryPaths
        {
            get { return _theLibraryPaths; }
            set 
            {
                if (value != null)
                {
                    _theLibraryPaths = value;
                }
            }
        }


        public void AddPath(string aPath)
        {
            if ((aPath != null) && (!_theLibraryPaths.Contains(aPath)))
            {
                _theLibraryPaths.Add(aPath);
            }
        }

        public void RemovePath(string aPath)
        {
            if ((aPath != null) && (_theLibraryPaths.Contains(aPath)))
            {
                _theLibraryPaths.Remove(aPath);
            }
        }
        public bool ContainsPath(string aPath)
        {
            return ((aPath != null) && (_theLibraryPaths.Contains(aPath)));
        }
    }
}
