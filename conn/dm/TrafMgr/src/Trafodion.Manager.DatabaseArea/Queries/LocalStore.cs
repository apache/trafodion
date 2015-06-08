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
using System.Collections;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;

namespace Trafodion.Manager.DatabaseArea.Queries
{
    
    class LocalStore
    {
        private static LocalStore _instance = null;
        private string  FileName = "c:\\localDB.bin";
        private const string TEMP_FILE = ".temp";
        private const string BACKUP_FILE = ".back";
        private Hashtable objectDB = new Hashtable();


        private LocalStore() 
        {
            loadObjectFromFile(FileName);
        }
        static LocalStore()
        {
            _instance = new LocalStore();
        }

        internal static LocalStore Instance
        {
            get { return _instance; }
        }

        //Retuns the object as is stored in the DB
        public Object getFromDb(String key)
        {
            return objectDB[key];
        }//getFromDb

        //Saves the object to the Db using the Key
        public  void saveToDb(String key, object value)
        {   

            //Reload the content. That will ensure we are saving only the object 
            //with the key passed
            loadObjectFromFile(FileName);

            if (objectDB == null)
            {
                objectDB = new Hashtable();            
            }
            
            if (objectDB[key] != null)
            {
                objectDB.Remove(key);
            }
            objectDB.Add(key, value);
            saveToFile(objectDB);        
        }//saveToDb

        private void saveToFile(object toSave)
        {
            FileStream    fos = null;
            //Write the object to the temp file
            try
            {
                //Remove the temp file first
                if (File.Exists(FileName + TEMP_FILE))
                {
                    File.Delete(FileName + TEMP_FILE);
                }
                //Now create it with the object
                fos = File.Create(FileName + TEMP_FILE);
                BinaryFormatter bf = new BinaryFormatter();
                bf.Serialize(fos, toSave);
            }
            catch(Exception ex)
            {
                throw ex;
            }
            finally
            {
                try
                {
                    if (fos != null) fos.Close();
                }
                catch(Exception ex)
                {
                    //do nothing
                }
            }

            //Rename the original file to backup
            try
            {
                //delete the backup file if it exists
                if (File.Exists(FileName + BACKUP_FILE))
                {
                    File.Delete(FileName + BACKUP_FILE);
                }

                if (File.Exists(FileName))
                {
                    File.Move(FileName, FileName + BACKUP_FILE);
                }
            }
            catch(Exception ex)
            {
                throw ex;
            }

            //Rename the temp to original
            try
            {
                File.Move(FileName + TEMP_FILE, FileName);
            }
            catch(Exception ex)
            {
                throw ex;
            }
        }

        //Loads the objectDB from the serialized object in the file
        private void loadObjectFromFile(String FileName)
        {
            FileStream fis = null;
            try
            {
                fis = File.OpenRead(FileName);
                BinaryFormatter bf = new BinaryFormatter();
                objectDB = (Hashtable)bf.Deserialize(fis);
            }
            catch(FileNotFoundException ex)
            {
                //This is ok
                return;
            }
            catch(Exception ex)
            {
                throw ex;
            }
            finally
            {
                try
                {
                    if (fis != null) fis.Close();
                }
                catch (Exception ex)
                {
                    //do nothing
                }
            }

        }//loadObjectFromFile

    }
}
