//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Data;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Xml.Serialization;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// Persistence object is used to wrap the real object to be persisted.
    /// </summary>
    [Serializable]
    public class PersistenceObject
    {
        #region Fields

        bool _useXMLSerialization = false;
        string _name = null;
        string _type = null;
        object _value = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: UseXMLSerialization - instructs to use xml serialization format else use the bindary format
        /// </summary>
        public bool UseXMLSerialization
        {
            get { return _useXMLSerialization; }
            set { _useXMLSerialization = value; }
        }

        /// <summary>
        /// Property: Name - the name of the object to be persisted
        /// </summary>
        public string Name
        {
            get { return _name; }
            set { _name = value; }
        }

        /// <summary>
        /// Property: Type - the type of the object to be persisted
        /// </summary>
        public string Type
        {
            get { return _type; }
            set { _type = value; }
        }

        /// <summary>
        /// Property: Value - the persisted object in string format
        /// </summary>
        public object Value
        {
            get { return _value; }
            set { _value = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Default constructor 
        /// </summary>
        public PersistenceObject()
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="useXML"></param>
        /// <param name="name"></param>
        /// <param name="type"></param>
        /// <param name="value"></param>
        public PersistenceObject(bool useXML, string name, string type, object value)
        {
            _useXMLSerialization = useXML;
            _name = name;
            _type = type;
            _value = value;
        }

        #endregion Constructors
    }

    /// <summary>
    /// Persistence Store is used to prepare objects to be persisted to file. 
    /// </summary>
    [Serializable]
    public class PersistenceStore
    {
        #region Fields

        private PersistenceObject[] _cDataStrings = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: CDataStrings - the array of Persistence Objects
        /// </summary>
        public PersistenceObject[] CDataStrings
        {
            get { return _cDataStrings; }
            set { _cDataStrings = value; }
        }

        #endregion Properties

        #region Constructor
        /// <summary>
        /// Default Constructor
        /// </summary>
        public PersistenceStore()
        {
            _cDataStrings = null;
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// Prepare the persistence store before serialize it to the file. 
        /// </summary>
        /// <param name="dictionary"></param>
        public Exception PreSeriealize(Dictionary<string, object> dictionary)
        {
            int idx = 0;
            _cDataStrings = new PersistenceObject[dictionary.Count];
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Persistence Name");
            errorTable.Columns.Add("Error Text");

            foreach (string key in dictionary.Keys)
            {
                bool useXML = true;
                try
                {
                    _cDataStrings[idx] = new PersistenceObject();
                    if (dictionary[key] is IUseXMLSerializable)
                    {
                        _cDataStrings[idx].UseXMLSerialization = true;
                        _cDataStrings[idx].Name = key;
                        _cDataStrings[idx].Type = dictionary[key].GetType().ToString();
                        try
                        {
                            _cDataStrings[idx].Value = SerializeToXMLString(dictionary[key]);
                        }
                        catch (Exception ex)
                        {
                            useXML = false;
                        }
                    }
                    else
                    {
                        useXML = false;
                    }

                    if (!useXML)
                    {
                        _cDataStrings[idx].UseXMLSerialization = false;
                        _cDataStrings[idx].Name = key;
                        _cDataStrings[idx].Type = dictionary[key].GetType().ToString();
                        _cDataStrings[idx].Value = SerializeToBase64String(dictionary[key]);
                    }
                    idx++; // will advance to next location only when every thing is OK. Else, this slot will be re-filled.
                }
                catch (Exception ex)
                {
                    // Report error message to log file
                    errorTable.Rows.Add(new object[] { key, ex.Message });
                }
            }

            if (errorTable.Rows.Count > 0)
            {
                string summaryMessage = String.Format("Warning: Failed to save one or more persistence(s); however, the rest of persistences have been saved.");
                return new PersistenceNotCompletedLoadedException(summaryMessage, errorTable);
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Deserialize the Persitence Store to the dictonary object
        /// </summary>
        /// <returns></returns>
        public Dictionary<string, object> Deserialize(out Exception anException)
        {
            Dictionary<string, object> dictionary = new Dictionary<string, object>();
            int count = CDataStrings.Length;
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Persistence Name");
            errorTable.Columns.Add("Error Text");

            for (int i = 0; i < count; i++)
            {
                try
                {
                    if (CDataStrings[i] != null)
                    {                        
                        bool useXML = CDataStrings[i].UseXMLSerialization;
                        if (useXML)
                        {
                            Type type = Type.GetType((string)CDataStrings[i].Type);
                            XmlSerializer serializerObj = new XmlSerializer(type);
                            MemoryStream m = new MemoryStream();
                            StreamWriter writer = new StreamWriter(m);
                            //XmlTextWriter writer = new XmlTextWriter(m, Encoding.ASCII);
                            writer.WriteLine(CDataStrings[i].Value as string);
                            writer.Flush();
                            m.Seek(0, SeekOrigin.Begin);
                            Object obj = serializerObj.Deserialize(m);
                            dictionary.Add(CDataStrings[i].Name, obj);
                        }
                        else
                        {
                            Type type = Type.GetType((string)CDataStrings[i].Type);
                            Object obj = DeserializeFromBase64String((string)CDataStrings[i].Value);
                            dictionary.Add(CDataStrings[i].Name, obj);
                        }
                    }                    
                }
                catch (Exception ex)
                {
                    // Report error message to log file
                    if (CDataStrings[i] != null)
                    {
                        errorTable.Rows.Add(new object[] { CDataStrings[i].Name, ex.Message });
                    }
                    else
                    {
                        errorTable.Rows.Add(new object[] { "Unsupported type", ex.Message });
                    }
                }
            }

            if (errorTable.Rows.Count > 0)
            {
                string summaryMessage = String.Format("Warning: Failed to load one or more persistence(s); TrafodionManager will continue.");
                anException = new PersistenceNotCompletedLoadedException(summaryMessage, errorTable);
            }
            else
            {
                anException = null;
            }

            return dictionary;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Utility to serialize an object to binary format and encode the binary with Base64
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        private string SerializeToBase64String(object obj)
        {
            if (obj != null)
            {
                MemoryStream theMemoryStream = new MemoryStream();
                BinaryFormatter theBinaryFormatter = new BinaryFormatter();
                theBinaryFormatter.Serialize(theMemoryStream, obj);
                theMemoryStream.Position = 0;
                byte[] bytes = theMemoryStream.ToArray();
                string content = System.Convert.ToBase64String(bytes);
                return content;
            }
            return "";
        }

        /// <summary>
        /// Utility to serialize an object to it's XML serializaiton format
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        private string SerializeToXMLString(object obj)
        {
            if (obj != null && obj is IUseXMLSerializable)
            {
                MemoryStream theMemoryStream = new MemoryStream();
                StreamWriter writer = new StreamWriter(theMemoryStream);
                XmlSerializer serializerObj = new XmlSerializer(obj.GetType());
                serializerObj.Serialize(writer, obj);
                theMemoryStream.Seek(0, SeekOrigin.Begin);
                StreamReader reader = new StreamReader(theMemoryStream);
                return reader.ReadToEnd();
            }

            return "";
        }

        /// <summary>
        /// Utility to deserialize an object from a Base64 encoded string 
        /// </summary>
        /// <param name="content"></param>
        /// <returns></returns>
        private Object DeserializeFromBase64String(string content)
        {
            MemoryStream theMemoryStream = new MemoryStream();
            BinaryFormatter theBinaryFormatter = new BinaryFormatter();
            BinaryWriter theWriter = new BinaryWriter(theMemoryStream);
            byte[] buffer = System.Convert.FromBase64String(content);
            theMemoryStream.Write(buffer, 0, buffer.Length);
            theMemoryStream.Position = 0;

            Object obj = theBinaryFormatter.Deserialize(theMemoryStream);
            return obj;
        }

        #endregion Private methods
    }
}
