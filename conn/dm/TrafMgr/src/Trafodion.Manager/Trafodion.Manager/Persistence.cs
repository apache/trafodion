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
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Windows.Forms;
using System.ComponentModel;
using Trafodion.Manager.Framework.Controls;
using System.Xml;
using System.Xml.Serialization;
using Trafodion.Manager.Framework.Favorites;
using System.Data;

namespace Trafodion.Manager.Framework
{
    static public class Persistence
    {
        static Dictionary<string, object> _thePersistenceDictionary = null;

        static public void Put(string aKey, object anObject)
        {
            if (_thePersistenceDictionary == null)
            {
                throw new PersistenceNotLoadedException();
            }
            _thePersistenceDictionary[aKey] = anObject;
        }

        static public object Get(string aKey)
        {
            if (_thePersistenceDictionary == null)
            {
                throw new PersistenceNotLoadedException();
            }

            return _thePersistenceDictionary.ContainsKey(aKey) ? _thePersistenceDictionary[aKey] : null;
        }

        static Persistence()
        {
            // Create the Open and Save File dialogs.  We create and re-use them so that they always
            // have the same directory staring point as when the user last used them is the session.
            // called only once per session! Therefore, InitialDirectory setting need to move to Open/Save methods 
            // Otherwise, code does not really persist last filename, always goes to HomeDirectory.
            {
                theOpenFileDialog.InitialDirectory = HomeDirectory;
                theOpenFileDialog.CheckFileExists = true;
                theOpenFileDialog.DefaultExt = theDefaultPersistenceFilenameExtension;
                theOpenFileDialog.Filter = "Trafodion Database Manager saved state file (*." + theDefaultPersistenceFilenameExtension + ") | *." + theDefaultPersistenceFilenameExtension;
                theOpenFileDialog.Title = Properties.Resources.MenuOpen;

                theSaveFileDialog.InitialDirectory = HomeDirectory;
                theSaveFileDialog.DefaultExt = theOpenFileDialog.DefaultExt;
                theSaveFileDialog.Filter = theOpenFileDialog.Filter;
            }

        }

        /// <summary>
        /// Save all persisted state to the default file
        /// </summary>
        static public void SaveAllToDefault()
        {
            // Persist all interested components
            Persistence.DoSaveFile(Path.Combine(Persistence.HomeDirectory, Persistence.DefaultPersistenceFilename));
        }

        /// <summary>
        /// Restore all persisted state from the default file
        /// </summary>
        static public void RestoreAllFromDefault()
        {
            // Restore persisted state
            if (File.Exists(Path.Combine(Persistence.HomeDirectory, Persistence.DefaultPersistenceFilename)))
            {
                // if the current default exist, use it
                Persistence.DoOpenFile(Path.Combine(Persistence.HomeDirectory, Persistence.DefaultPersistenceFilename), true, true);
            }
            else if (File.Exists(Path.Combine(Persistence.HomeDirectory, Properties.Resources.DefaultPersistenceFile + "." + Properties.Resources.DefaultPersistenceFileExtensionPriorVersion)))
            {
                // else check for the prior version, if it exists, use it
                Persistence.DoOpenFile(Path.Combine(Persistence.HomeDirectory, Properties.Resources.DefaultPersistenceFile + "." + Properties.Resources.DefaultPersistenceFileExtensionPriorVersion), true, true);
                DoSaveFile(Path.Combine(Persistence.HomeDirectory, Persistence.DefaultPersistenceFilename));
            }
            else
            {
                // if none could be found, use the crrent default to cause the creation
                Persistence.DoOpenFile(Path.Combine(Persistence.HomeDirectory, Persistence.DefaultPersistenceFilename), true, true);
            }

        }

        // The application's home directory
        static private string theHomeDirectory = null;

        /// <summary>
        /// Readonly property giving the applciation's home directory
        /// </summary>
        static public string HomeDirectory
        {
            get
            {

                // Check to see if we already know it
                if (theHomeDirectory == null || theHomeDirectory.Trim().Equals(""))
                {
 
                    // We don't know it yet.  Try the environment variables.
                    String userHome = Environment.GetEnvironmentVariable("HOME");
                    String appData = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
                    if (! String.IsNullOrEmpty(userHome))
                    {
                        try
                        {
                            theHomeDirectory = CreateDefaultDirectoryIfNeeded(userHome);
                            //if the userHome folder doesnot exist, theHomeDirectory will be null
                            //so try to create the home directory under app data
                            if (theHomeDirectory == null)
                            {
                                theHomeDirectory = CreateDefaultDirectoryIfNeeded(appData);
                            }
                        }
                        catch (Exception ex)
                        {
                            //We could have got an exception while creating the TrafodionManager folder under the 
                            //User folder. That case we will try to create the folder under app data.
                            //If there is an error creating the folder under appData then an unhandled
                            //exception will be thrown that will eventually be caught by TrafodionManager
                            theHomeDirectory = CreateDefaultDirectoryIfNeeded(appData);
                        }
                    }
                    else if (! String.IsNullOrEmpty(appData))
                    {
                        theHomeDirectory = CreateDefaultDirectoryIfNeeded(appData);
                    }

                    //if theHomeDirectory is null, that would mean that both userHome and appData doesnot exist
                    if (theHomeDirectory == null)
                    {
                        throw new Exception(String.Format("TrafodionManager persistence folder {0} could not be created under {1} or {2}", TheDefaultPersistenceFolder, userHome, appData));
                    }
                }
                return theHomeDirectory;
            }
        }


        static string CreateDefaultDirectoryIfNeeded(String theHomeDirectory)
        {
            if (theHomeDirectory != null)
            { 
                String path = Path.Combine(theHomeDirectory, TheDefaultPersistenceFolder);
                if (Directory.Exists(path))
                {
                    return path;
                }
                else
                {
                    Directory.CreateDirectory(path);
                    return path;
                }
            }
            return null;
        }

        /// <summary>
        /// Read from the most recently named persistence file
        /// </summary>
        static public void DoOpenFile()
        {
            if (theCurrentFileName != null)
            {
                // Seting FileName alone does not work. Need to set InitialDirectory as well.
                theOpenFileDialog.InitialDirectory = Path.GetDirectoryName(theCurrentFileName);
                theOpenFileDialog.FileName = theCurrentFileName;
                theOpenFileDialog.Title = Properties.Resources.MenuOpen;
            }
            if (theOpenFileDialog.ShowDialog() == DialogResult.OK)
            {
                DoOpenFile(theOpenFileDialog.FileName);
            }
        }

        /// <summary>
        /// Save to a persistence file defaulting to the most recently named persistence file
        /// </summary>
        static public void DoSaveFileAs()
        {
            if (theCurrentFileName != null)
            {
                // Seting FileName alone does not work. Need to set InitialDirectory as well. 
                theSaveFileDialog.InitialDirectory = Path.GetDirectoryName(theCurrentFileName);
                theSaveFileDialog.FileName = theCurrentFileName;
                theSaveFileDialog.Title = Properties.Resources.MenuSaveAs;
            }
            if (theSaveFileDialog.ShowDialog() == DialogResult.OK)
            {
                DoSaveFile(theSaveFileDialog.FileName);
            }
        }

        /// <summary>
        /// Read from a specific persistence file
        /// </summary>
        /// <param name="aFileName">The name of the file</param>
        static public void DoOpenFile(string aFileName)
        {
            DoOpenFile(aFileName, false, false);
        }

        /// <summary>
        /// Read from a given perisistence file
        /// </summary>
        /// <param name="aFileName">The name of the file</param>
        /// <param name="throwException">True to throw exception or false to display message box on error</param>
        /// <param name="bang">True to suppress yes/no message box and assume yes if overwriting an existing file</param>
        static public void DoOpenFile(string aFileName, bool throwException, bool bang)
        {
            FileStream theFileStream = null;
            FileStream theFileStreamBak = null;
            Exception pnclException = null;
            try
            {
                Action<int> actionLoadPersistence = delegate(int placeholder)
                {
                    //Checking file with ".config.xml" as the extension.
                    if (File.Exists(aFileName))
                    {
                        //Check if user wants to overwrite the persistence
                        if (!bang && (MessageBox.Show("\n" + Properties.Resources.Persistence_Load_Overwrite + "\n",
                                                  Properties.Resources.ConfirmTitle,
                                                  MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.No))
                        {
                            return;
                        }

                        //Fire the preload event before replacing the in-memory dictionary. This event allows the subscribers to do any cleanup.
                        FirePersistence(null, PersistenceOperation.PreLoad);

                        theFileStream = File.Open(aFileName, FileMode.Open, FileAccess.Read, FileShare.Read);

                        // Check for the prior version extion (".config") first
                        if (aFileName.EndsWith("." + Properties.Resources.DefaultPersistenceFileExtensionPriorVersion))
                        {
                            BinaryFormatter theBinaryFormatter = new BinaryFormatter();

                            // Read the persistence dictionary from the file
                            _thePersistenceDictionary = (Dictionary<string, object>)theBinaryFormatter.Deserialize(theFileStream);
                        }
                        else
                        {
                            try
                            {
                                //Deserialize the persistence file using XML style
                                XmlSerializer serializerObj = new XmlSerializer(typeof(PersistenceStore));
                                PersistenceStore obj = (PersistenceStore)serializerObj.Deserialize(theFileStream);
                                _thePersistenceDictionary = obj.Deserialize(out pnclException);
                                Utilities.HAS_ERROR_ON_LOADING_CONFIG_FILE = false;
                            }
                            catch(Exception ex)
                            {
                                Utilities.HAS_ERROR_ON_LOADING_CONFIG_FILE = true;
                                string fileName=aFileName+".bak";
                                if (File.Exists(fileName))
                                {
                                    theFileStreamBak = File.Open(fileName, FileMode.Open, FileAccess.Read, FileShare.Read);
                                    //Deserialize the persistence file using XML style
                                    XmlSerializer serializerObj = new XmlSerializer(typeof(PersistenceStore));
                                    PersistenceStore obj = (PersistenceStore)serializerObj.Deserialize(theFileStreamBak);
                                    _thePersistenceDictionary = obj.Deserialize(out pnclException);
                                    Utilities.HAS_ERROR_ON_LOADING_CONFIG_FILE = false;
                                    Logger.OutputErrorLog(string.Format("Exception occurs while loading file, recovering to the bak persistence XML file.{0} --->successfully done!",fileName));
                                }
                                else
                                {
                                    throw ex;
                                }
                            }
                        }


                        // Tell all interested components to get their info from their entries in the dictionary
                        FirePersistence(_thePersistenceDictionary, PersistenceOperation.Load);
                    }
                };

                // Use global lock to avoid error when multiple TrafodionManager clients read/write one same persistence file
                Utilities.RunInGlobalLock(actionLoadPersistence, 0);

                theCurrentFileName = aFileName;
            }
            catch (Exception e)
            {
                _thePersistenceDictionary = new Dictionary<string, object>();
                try
                {
                    if (theFileStream != null)
                    {
                        try
                        {
                            theFileStream.Close();
                        }
                        catch (Exception ex)
                        {
                        }
                    }

                    Action<int> actionBackupPersistenceFile = delegate(int placeholder)
                    {
                        //IF persistence file exists already, save a back up before writing new info
                        if (File.Exists(aFileName))
                        {

                            if (File.Exists(aFileName + ".loadfail"))
                                File.Delete(aFileName + ".loadfail");

                            //backup current persistence file
                            File.Move(aFileName, aFileName + ".loadfail");
                        }
                    };

                    Utilities.RunInGlobalLock(actionBackupPersistenceFile, 0);
                }
                catch (Exception ex)
                {

                }
                //string message = string.Format(Properties.Resources.PersistenceLoadFailure, aFileName, aFileName + ".loadfail", Logger.DefaultErrorLogFilename);
                string message = string.Format(Properties.Resources.PersistenceLoadFailure, aFileName, aFileName + ".loadfail", Logger.ErrorLog);
                Logger.OutputErrorLog("Error loading persistence file : " + e.Message);
                Logger.OutputErrorLog(e.StackTrace);

                if (throwException)
                {  
                    throw new PersistenceNotLoadedException(message);
                }
                else
                {
                    MessageBox.Show(message, Properties.Resources.Error, MessageBoxButtons.OK); 
                }
            }
            finally
            {
                if (theFileStream != null)
                {
                    try
                    {
                        theFileStream.Close();
                    }
                    finally
                    {
                    }
                }

                if (theFileStreamBak != null)
                {
                    try
                    {
                        theFileStreamBak.Close();
                    }
                    finally
                    {
                    }
                }

                if (pnclException != null)
                {
                    if (throwException)
                    {
                        throw pnclException;
                    }
                    else
                    {
                        PersistenceNotCompletedLoadedException pex = (PersistenceNotCompletedLoadedException)pnclException;
                        TrafodionMultipleMessageDialog md = new TrafodionMultipleMessageDialog(pex.Message, pex.ErrorTable, System.Drawing.SystemIcons.Error);
                        md.BringToFront();
                        md.ShowDialog();
                    }
                }

                if (_thePersistenceDictionary == null)
                {
                    _thePersistenceDictionary = new Dictionary<string, object>();
                }
            }
        }

        /// <summary>
        /// Saves to a specific persistence file
        /// </summary>
        /// <param name="aFileName">The name of the file</param>
        static public void DoSaveFile(string aFileName)
        {
            //aFileName += ".xml";
            Dictionary<string, object> theNewEntries = new Dictionary<string, object>();

            // Tell all interested components to add/replace their entries to the dictionary
            FirePersistence(theNewEntries, PersistenceOperation.Save);

            // We do not want to lose any entries for componenets which were not here this time
            foreach (string theKey in theNewEntries.Keys)
            {

                // Check to see if there is a previous value for this key
                if (_thePersistenceDictionary.ContainsKey(theKey))
                {

                    // Yes, remove it
                    _thePersistenceDictionary.Remove(theKey);

                }

                // Insert this key, knowning that there's no old version
                _thePersistenceDictionary[theKey] = theNewEntries[theKey];

            }

            // Write the dictionary and its entries out to the file
            FileStream theFileStream = null;
            TextWriter writer = null;
            try
            {
                //find if any of the values are not serializable
                List<string> nonSerializableKeys = new List<string>();
                foreach (string key in _thePersistenceDictionary.Keys)
                {
                    if (_thePersistenceDictionary[key] != null && _thePersistenceDictionary[key].GetType() != null && _thePersistenceDictionary[key].GetType().IsSerializable)
                        continue;

                    nonSerializableKeys.Add(key);
                }
                //If there are any non serializable objects, remove them from the dictionary
                foreach (string key in nonSerializableKeys)
                {
                    _thePersistenceDictionary.Remove(key);
                }
                PersistenceNotCompletedLoadedException pex = null;
                Action<int> actionSavePersistenceFile = delegate(int placeholder)
                    {
                        //IF persistence file exists already, save a back up before writing new info
                        if (!Utilities.HAS_ERROR_ON_LOADING_CONFIG_FILE //only if there is no error on load config file that could backup the config
                            && File.Exists(aFileName))
                        {                            
                            //If backup file already exists, delete it
                            if (File.Exists(aFileName + ".bak"))
                            {
                                File.Delete(aFileName + ".bak");
                            }
                            //backup current persistence file
                            File.Move(aFileName, aFileName + ".bak");
                        }

                        PersistenceStore obj = new PersistenceStore();
                        pex = (PersistenceNotCompletedLoadedException)obj.PreSeriealize(_thePersistenceDictionary);

                        writer = new StreamWriter(aFileName);
                        XmlSerializer serializerObj = new XmlSerializer(typeof(PersistenceStore));
                        serializerObj.Serialize(writer, obj);
                    };

                Utilities.RunInGlobalLock(actionSavePersistenceFile, 0);
                    
                theCurrentFileName = aFileName;
                if (pex != null)
                {
                    TrafodionMultipleMessageDialog md = new TrafodionMultipleMessageDialog(pex.Message, pex.ErrorTable, System.Drawing.SystemIcons.Warning);
                    md.BringToFront();
                    md.ShowDialog();
                }
            }
            catch (Exception e)
            {
                //Logger.OutputErrorLog("Error saving persistence : " + e.StackTrace);
                MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.PersistenceSaveFailure + "\n\n" + e.Message, Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                try
                {
                    if (writer != null)
                    {
                        writer.Close();
                    }

                    Action<int> actionRestorePersistenceFile = delegate(int placeholder)
                    {
                        //Restore the backup
                        File.Delete(aFileName);
                        File.Move(aFileName + ".bak", aFileName);
                    };

                    Utilities.RunInGlobalLock(actionRestorePersistenceFile, 0);
                }
                catch (Exception ex) { }
            }
            finally
            {
                if (writer != null)
                {
                    try
                    {
                        writer.Close();
                    }
                    finally
                    { }
                }
            }
        }

        // The name of the most recently used persistence file
        static private string theCurrentFileName = null;

        public static string CurrentFileName
        {
            get { return Persistence.theCurrentFileName; }
            set { Persistence.theCurrentFileName = value; }
        }

        // The open file and save file dialogs
        static private OpenFileDialog theOpenFileDialog = new OpenFileDialog();
        static private SaveFileDialog theSaveFileDialog = new SaveFileDialog();

        /// <summary>
        /// Persistence is either loaded or saved
        /// </summary>
        public enum PersistenceOperation
        {
            PreLoad, //Preload event id
            Load,   // Load persistence info from a file
            Save    // Save persistence info to a file
        }

        /// <summary>
        /// A persistence event handler
        /// </summary>
        /// <param name="aDictionary">The persistence dictionary</param>
        /// <param name="aPersistenceOperation">Load or Save</param>
        public delegate void PersistenceHandler(Dictionary<string, object> aDictionary, PersistenceOperation aPersistenceOperation);

        // List of all components having persistence
        static private EventHandlerList theEventHandlers = new EventHandlerList();

        private static readonly string thePersistenceEventKey = "Persistence";

        /// <summary>
        /// The list of all components having persistence
        /// </summary>
        static public event PersistenceHandler PersistenceHandlers
        {
            add { theEventHandlers.AddHandler(thePersistenceEventKey, value); }
            remove { theEventHandlers.RemoveHandler(thePersistenceEventKey, value); }
        }

        /// <summary>
        /// Call to tell all interested components that they are to load or save their persisted info
        /// </summary>
        /// <param name="aDictionary">The persistence dictionary</param>
        /// <param name="aPersistenceOperation">Wheterh to load or save</param>
        static private void FirePersistence(Dictionary<string, object> aDictionary, PersistenceOperation aPersistenceOperation)
        {
            PersistenceHandler thePersistenceHandlers = (PersistenceHandler)theEventHandlers[thePersistenceEventKey];

            if (thePersistenceHandlers != null)
            {
                thePersistenceHandlers(aDictionary, aPersistenceOperation);
            }
        }

        /// <summary>
        /// The filename for the default persistence file
        /// </summary>
        private static readonly string theDefaultPersistenceFilename =
            Trafodion.Manager.Properties.Resources.DefaultPersistenceFile;

        public static string DefaultPersistenceFilename
        {
            get { return Persistence.theDefaultPersistenceFilename + "." + Persistence.theDefaultPersistenceFilenameExtension; }
        }


        /// <summary>
        /// The filename extension for the default persistence file
        /// </summary>
        private static readonly string theDefaultPersistenceFilenameExtension = Trafodion.Manager.Properties.Resources.DefaultPersistenceFileExtension;

        public static string DefaultPersistenceFilenameExtension
        {
            get { return Persistence.theDefaultPersistenceFilenameExtension; }
        } 
        
        /// <summary>
        /// The default persistence folder. This is the folder name under User folder or AppData
        /// </summary>
        private static readonly string theDefaultPersistenceFolder = Trafodion.Manager.Properties.Resources.DefaultPersistenceFolder;

        public static string TheDefaultPersistenceFolder
        {
            get { return Persistence.theDefaultPersistenceFolder; }
        } 



    }

    public class PersistenceNotLoadedException : ApplicationException
    {
        public PersistenceNotLoadedException()
            : base("Error: Persisted information has not been reloaded")
        {

        }
        public PersistenceNotLoadedException(string aMessage)
            : base(aMessage)
        {

        }
    }

    public class PersistenceNotCompletedLoadedException : Exception
    {
        private DataTable _errorTable = null;
        private string _message = null;

        public DataTable ErrorTable
        {
            get { return _errorTable; }
            set { _errorTable = value; }
        }

        public string Message
        {
            get { return _message; }
            set { _message = value; }
        }

        public PersistenceNotCompletedLoadedException(string aMessage, DataTable anErrorTable)
        : base(aMessage)
        {
            _errorTable = anErrorTable;
            _message = aMessage;
        }
    }

}
