/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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
********************************************************************/

// TODO: i dislike all the casting due to the base class storing everything as strings
// -- make sure these properties are only called once in the actual driver as numeric values are converted for every call
// -- maybe we should use a different storage mechanism instead of relying on the base class implementation
// TODO: fill in descriptions for each of the properties
namespace Trafodion.Data
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Text;

    /// <summary>
    /// Provides a simple way to create and manage the contents of connection strings used by the TrafodionDBConnection class.
    /// </summary>
    public sealed class TrafodionDBConnectionStringBuilder : DbConnectionStringBuilder
    {
        private static Dictionary<string, string> _keywords;
        private static Dictionary<string, string> _defaults;

        /// <summary>
        /// Initializes static members of the TrafodionDBConnectionStringBuilder class.
        /// </summary>
        static TrafodionDBConnectionStringBuilder()
        {
            // define alternate keywords here
            _keywords = new Dictionary<string, string>();
            _keywords.Add("SERVER", "Server");
            _keywords.Add("ADDRESS", "Server");
            _keywords.Add("ADDR", "Server");
            _keywords.Add("NETWORK ADDRESS", "Server");
            _keywords.Add("HOST", "Server");

            _keywords.Add("DATASOURCE", "Datasource");

            _keywords.Add("CATALOG", "Catalog");
            _keywords.Add("INITIAL CATALOG", "Catalog");
            _keywords.Add("DATABASE", "Catalog");

            _keywords.Add("SCHEMA", "Schema");

            _keywords.Add("USER", "User");
            _keywords.Add("USER ID", "User");

            _keywords.Add("PASSWORD", "Password");
            _keywords.Add("PWD", "Password");

            _keywords.Add("ROLENAME", "Rolename");

            _keywords.Add("RETRYCOUNT", "RetryCount");

            _keywords.Add("RETRYTIME", "RetryTime");

            _keywords.Add("TCPKEEPALIVE", "TcpKeepAlive");

            _keywords.Add("IDLETIMEOUT", "IdleTimeout");

            _keywords.Add("LOGINTIMEOUT", "LoginTimeout");
            _keywords.Add("CONNECTION TIMEOUT", "LoginTimeout");
            _keywords.Add("COMMANDTIMEOUT", "CommandTimeout");

            _keywords.Add("FETCHBUFFERSIZE", "FetchBufferSize");
            _keywords.Add("FETCHROWCOUNT", "FetchRowCount");

            _keywords.Add("CPUTOUSE", "CpuToUse");

            _keywords.Add("APPLICATIONNAME", "ApplicationName");
            _keywords.Add("APPLICATION NAME", "ApplicationName");

            _keywords.Add("SQLSERVERMODE", "SqlServerMode");

            _keywords.Add("SESSIONNAME", "SessionName");

            _keywords.Add("COMPRESSION", "Compression");
            _keywords.Add("DOUBLEBUFFERING", "DoubleBuffering");
            _keywords.Add("DOUBLE BUFFERING", "DoubleBuffering");

            _keywords.Add("IGNORECANCEL", "IgnoreCancel");

            _keywords.Add("MAXPOOLSIZE", "MaxPoolSize");
            _keywords.Add("MAX POOL SIZE", "MaxPoolSize");

            // the base class is a <string, string> so we set the defaults as strings
            _defaults = new Dictionary<string, string>();
            _defaults.Add("Server", string.Empty);
            _defaults.Add("Datasource", string.Empty);
            _defaults.Add("Catalog", string.Empty);
            _defaults.Add("Schema", string.Empty);
            _defaults.Add("User", string.Empty);
            _defaults.Add("Password", string.Empty);
            _defaults.Add("Rolename", string.Empty);
            _defaults.Add("RetryCount", "12");
            _defaults.Add("RetryTime", "5000");
            _defaults.Add("TcpKeepAlive", "True");
            _defaults.Add("IdleTimeout", "-1");
            _defaults.Add("LoginTimeout", "300");
            _defaults.Add("CommandTimeout", "0");
            _defaults.Add("FetchBufferSize", "4096");
            _defaults.Add("FetchRowCount", "500");
            _defaults.Add("CpuToUse", "-1");
            _defaults.Add("ApplicationName", "TrafodionDB .NET Provider");
            _defaults.Add("SqlServerMode", "False");
            _defaults.Add("SessionName", string.Empty);
            _defaults.Add("Compression", "False");
            _defaults.Add("DoubleBuffering", "False");
            _defaults.Add("IgnoreCancel", "False");

            _defaults.Add("MaxPoolSize", "-1");
        }

        /// <summary>
        /// Initializes a new instance of the TrafodionDBConnectionStringBuilder class.
        /// </summary>
        public TrafodionDBConnectionStringBuilder()
            : this(string.Empty)
        {
        }

        /// <summary>
        /// Initializes a new instance of the TrafodionDBConnectionStringBuilder class. The provided connection string provides the data for the instance's internal connection information.
        /// </summary>
        /// <param name="connectionString">The basis for the object's internal connection information. Parsed into name/value pairs.</param>
        public TrafodionDBConnectionStringBuilder(string connectionString)
        {
            this.ResetDefaultValues();
            this.ConnectionString = connectionString;
        }

        [Category("General"), Description("")]
        public string Server
        {
            get { return base["Server"] as string; }
            set { base["Server"] = value; }
        }

        [Category("General"), Description("")]
        public string Datasource
        {
            get { return base["Datasource"] as string; }
            set { base["Datasource"] = value; }
        }

        [Category("General"), Description("")]
        public string Catalog
        {
            get { return base["Catalog"] as string; }
            set { base["Catalog"] = value; }
        }

        [Category("General"), Description("")]
        public string Schema
        {
            get { return base["Schema"] as string; }
            set { base["Schema"] = value; }
        }

        [Category("Credentials"), Description("")]
        public string User
        {
            get { return base["User"] as string; }
            set { base["User"] = value; }
        }

        [Category("Credentials")]
        [PasswordPropertyText(true)]
        [Description("")]
        public string Password
        {
            get { return base["Password"] as string; }
            set { base["Password"] = value; }
        }

        [Category("Credentials"), Description("")]
        public string Rolename
        {
            get { return base["Rolename"] as string; }
            set { base["Rolename"] = value; }
        }

        [Category("Connection"), Description("")]
        public short RetryCount
        {
            get { return Int16.Parse(base["RetryCount"] as string); }
            set { base["RetryCount"] = value; }
        }

        [Category("Connection"), Description("")]
        public int RetryTime
        {
            get { return Int32.Parse(base["RetryTime"] as string); }
            set { base["RetryTime"] = value; }
        }

        [Category("Connection"), Description("")]
        public bool TcpKeepAlive
        {
            get { return Boolean.Parse(base["TcpKeepAlive"] as string); }
            set { base["TcpKeepAlive"] = value; }
        }

        [Category("Connection"), Description("")]
        public int MaxPoolSize
        {
            get { return Int32.Parse(base["MaxPoolSize"] as string); }
            set { base["MaxPoolSize"] = value; }
        }

        [Category("Data"), Description("")]
        public bool Compression
        {
            get { return Boolean.Parse(base["Compression"] as string); }
            set { base["Compression"] = value; }
        }

        [Category("Data"), Description("")]
        public bool DoubleBuffering
        {
            get { return Boolean.Parse(base["DoubleBuffering"] as string); }
            set { base["DoubleBuffering"] = value; }
        }

        [Category("Data"), Description("")]
        public short FetchBufferSize
        {
            get { return Int16.Parse(base["FetchBufferSize"] as string); }
            set { base["FetchBufferSize"] = value; }
        }

        [Category("Data"), Description("")]
        public int FetchRowCount
        {
            get { return Int32.Parse(base["FetchRowCount"] as string); }
            set { base["FetchRowCount"] = value; }
        }

        [Category("Timeouts"), Description("")]
        public int IdleTimeout
        {
            get { return Int32.Parse(base["IdleTimeout"] as string); }
            set { base["IdleTimeout"] = value; }
        }

        [Category("Timeouts"), Description("")]
        public int LoginTimeout
        {
            get { return Int32.Parse(base["LoginTimeout"] as string); }
            set { base["LoginTimeout"] = value; }
        }

        [Category("Timeouts"), Description("")]
        public int CommandTimeout
        {
            get { return Int32.Parse(base["CommandTimeout"] as string); }
            set { base["CommandTimeout"] = value; }
        }

        [Category("Misc"), Description(""), Browsable(false)]
        public string ApplicationName
        {
            get { return base["ApplicationName"] as string; }
            set { base["ApplicationName"] = value; }
        }

        [Category("Misc"), Description("")]
        public bool IgnoreCancel
        {
            get { return Boolean.Parse(base["IgnoreCancel"] as string); }
            set { base["IgnoreCancel"] = value; }
        }

        [Category("Misc"), Description("")]
        public string SessionName
        {
            get { return base["SessionName"] as string; }
            set { base["SessionName"] = value; }
        }

        [Category("Misc"), Description(""), Browsable(false)]
        public short CpuToUse
        {
            get { return Int16.Parse(base["CpuToUse"] as string); }
            set { base["CpuToUse"] = value; }
        }

        [Category("Misc"), Description("")]
        public bool SqlServerMode
        {
            get { return Boolean.Parse(base["SqlServerMode"] as string); }
            set { base["SqlServerMode"] = value; }
        }

        /// <summary>
        /// Gets a value that indicates whether the TrafodionDBConnectionStringBuilder has a fixed size.
        /// </summary>
        public override bool IsFixedSize
        {
            get
            {
                return true;
            }
        }

        // note that this will not return a parsable connection string for ; and = literals
        // this is intended only for tracing/debugging purposes
        internal string TraceableConnectionString
        {
            get
            {
                StringBuilder sb = new StringBuilder(1000);
                foreach (KeyValuePair<string, object> kvp in this)
                {
                    sb.Append(kvp.Key);
                    sb.Append("=");
                    if (!kvp.Key.Equals("Password"))
                    {
                        sb.Append(kvp.Value);
                    }
                    else
                    {
                        // do not trace the password text, only the length in * characters
                        sb.Append('*', kvp.Value.ToString().Length);
                    }

                    sb.Append(";");
                }

                return sb.ToString();
            }
        }

        /// <summary>
        /// Gets or sets the value of the specified property.
        /// </summary>
        /// <param name="keyword">The keyword.</param>
        /// <returns>The value associated with the given keyword.</returns>
        public override object this[string keyword]
        {
            get
            {
                return base[MapKeyword(keyword)];
            }

            set
            {
                if (this.ContainsKey(keyword))
                {
                    base[MapKeyword(keyword)] = value;
                }
            }
        }

        /// <summary>
        /// Resets all properties to default values.
        /// </summary>
        public override void Clear()
        {
            base.Clear();
            this.ResetDefaultValues();
        }

        /// <summary>
        /// Determines whether the TrafodionDBConnectionStringBuilder contains a specific key.
        /// </summary>
        /// <param name="keyword">The key to locate in the TrafodionDBConnectionStringBuilder.</param>
        /// <returns>true if the TrafodionDBConnectionStringBuilder contains an element that has the specified key; otherwise, false.</returns>
        public override bool ContainsKey(string keyword)
        {
            keyword = keyword.ToUpper().Trim();
            if (_keywords.ContainsKey(keyword))
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Resets the entry with the specified key to its default.
        /// </summary>
        /// <param name="keyword">The keyword to reset.</param>
        /// <returns>true if the keyword was reset; otherwise, false.</returns>
        public override bool Remove(string keyword)
        {
            if (this.ContainsKey(keyword))
            {
                keyword = MapKeyword(keyword);
                base[keyword] = _defaults[keyword];

                return true;
            }

            return false;
        }

        /// <summary>
        /// Indicates whether the specified key exists in this TrafodionDBConnectionStringBuilder instance.
        /// </summary>
        /// <param name="keyword">The key to locate in the TrafodionDBConnectionStringBuilder.</param>
        /// <returns>true if the TrafodionDBConnectionStringBuilder contains an entry with the specified key; otherwise, false.</returns>
        public override bool ShouldSerialize(string keyword)
        {
            if (this.ContainsKey(keyword))
            {
                keyword = MapKeyword(keyword);
                return !base[keyword].Equals(_defaults[keyword]);
            }

            return false;
        }

        /// <summary>
        /// Retrieves a value corresponding to the supplied key.
        /// </summary>
        /// <param name="keyword">The key of the item to retrieve.</param>
        /// <param name="value">The value corresponding to the key.</param>
        /// <returns>true if the keyword was found, false otherwise</returns>
        public override bool TryGetValue(string keyword, out object value)
        {
            if (this.ContainsKey(keyword))
            {
                return base.TryGetValue(MapKeyword(keyword), out value);
            }

            value = null;
            return false;
        }

        private static string MapKeyword(string keyword)
        {
            return _keywords[keyword.ToUpper().Trim()];
        }

        private void ResetDefaultValues()
        {
            // reset the static defaults
            foreach (KeyValuePair<string, string> kvp in _defaults)
            {
                base[kvp.Key] = kvp.Value;
            }

            // reset the dynamic defaults
            string str = Process.GetCurrentProcess().MainModule.FileName;
            base["ApplicationName"] = str.Substring(str.LastIndexOf("\\") + 1);
        }
    }
}
