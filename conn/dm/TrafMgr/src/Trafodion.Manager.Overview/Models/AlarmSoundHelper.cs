// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Collections;
using System.IO;
using System.Media;
using Trafodion.Manager.Framework;
using System.Runtime.InteropServices;
using System.Threading;

namespace Trafodion.Manager.OverviewArea.Models
{
    sealed class AlarmSoundHelper
    {
        private const string SYSTEM_SOUND_FOLDER = @"C:\Windows\Media";
        private const string ALARM_SOUND_PATTERN = "*.wav";
        private const string NO_ALARM_SOUND = "";
        private const string NO_ALARM_SOUND_TEXT = "(None)";

        private const string PERSISTENCE_ALARM_SOUND_FILE = "HealthStateAlarmSoundFile";
        private const string PERSISTENCE_ALARM_SOUND_FOLDER = "HealthStateAlarmSoundFOLDER";

        private static string _alarmSoundFile = string.Empty;

        private AlarmSoundHelper()
        {
        }

        static AlarmSoundHelper()
        {
            _alarmSoundFile = (string)Persistence.Get(PERSISTENCE_ALARM_SOUND_FILE);
            if (_alarmSoundFile == null)
            {
                Persistence.Put(PERSISTENCE_ALARM_SOUND_FILE, NO_ALARM_SOUND);
                _alarmSoundFile = NO_ALARM_SOUND;
            }
        }

        public static bool IsAlarmOn
        {
            get
            {
                return _alarmSoundFile.Trim().Length > 0;
            }
        }

        public static string AlarmSoundFile
        {
            get
            {
                return _alarmSoundFile;
            }
            set
            {
                if (value == null)
                {
                    throw new ArgumentNullException("The Alarm Sound File cannot be NULL!");
                }

                _alarmSoundFile = value;
                Persistence.Put(PERSISTENCE_ALARM_SOUND_FILE, value);
            }
        }

        public static string AlarmSoundFilter
        {
            get { return string.Format(Properties.Resources.AlarmSoundFilter, ALARM_SOUND_PATTERN); }
        }

        public static string AlarmSoundFolder
        {
            get
            {
                return GetAlarmSoundFolder(_alarmSoundFile);
            }
            set
            {
                if (value == null || value.Trim().Length == 0)
                {
                    throw new NullReferenceException("Alarm Sound Folder cannnot be empty!");
                }

                Persistence.Put(PERSISTENCE_ALARM_SOUND_FOLDER, value);
            }
        }

        public static bool Validate()
        {
            bool isValid = true;
            if (_alarmSoundFile != null && _alarmSoundFile.Trim().Length > 0
                && !File.Exists(_alarmSoundFile))
            {
                isValid = false;
                _alarmSoundFile = NO_ALARM_SOUND;
                Persistence.Put(PERSISTENCE_ALARM_SOUND_FILE, NO_ALARM_SOUND);
                Persistence.Put(PERSISTENCE_ALARM_SOUND_FOLDER, SYSTEM_SOUND_FOLDER);
            }
            return isValid;
        }

        public static List<DictionaryEntry> GetAlarmSounds(string alarmSoundFolder)
        {
            List<DictionaryEntry> systemSounds = new List<DictionaryEntry>();
            if (Directory.Exists(alarmSoundFolder))
            {
                string[] systemSoundsFiles = Directory.GetFiles(alarmSoundFolder, ALARM_SOUND_PATTERN);
                foreach (string systemSoundFile in systemSoundsFiles)
                {
                    systemSounds.Add(new DictionaryEntry(systemSoundFile, Path.GetFileName(systemSoundFile)));
                }
            }
            systemSounds.Insert(0, new DictionaryEntry(NO_ALARM_SOUND, NO_ALARM_SOUND_TEXT));
            return systemSounds;
        }

        public static string GetAlarmSoundFolder(string alarmSoundFile)
        {
            if (alarmSoundFile.Trim().Length > 0)
            {
                string alarmSoundFolder = Path.GetDirectoryName(alarmSoundFile);
                if (Directory.Exists(alarmSoundFolder))
                {
                    return alarmSoundFolder;
                }
            }
            else // In case of AlarmSoundFile is (None), get persisted alarm sound folder.
            {
                string lastAlarmSoundFolder = (string)Persistence.Get(PERSISTENCE_ALARM_SOUND_FOLDER);
                if (lastAlarmSoundFolder != null && lastAlarmSoundFolder.Trim().Length > 0)
                {
                    return lastAlarmSoundFolder;
                }
            }
            return SYSTEM_SOUND_FOLDER;
        }

        public static void Test(string alarmSoundFile)
        {
            Thread testSoundThread = new Thread(new ParameterizedThreadStart(TestSoundAsync));
            testSoundThread.Start(alarmSoundFile);
        }

        private static void TestSoundAsync(object objAlarmSoundFile)
        {
            string alarmSoundFile = (string)objAlarmSoundFile;
            Utilities.PlaySoundSync(alarmSoundFile);
        }

        public static void Alarm()
        {
            if (_alarmSoundFile == null || _alarmSoundFile.Trim().Length == 0)
            {
                throw new NullReferenceException("Alarm requires Alarm Sound File not be empty!");
            }

            if (!IsAlarmOn) return;
            Utilities.PlaySound(_alarmSoundFile);
        }
    }
}
