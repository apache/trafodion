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
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// Represents a Control Table for managing DataSource.
    /// </summary>
    public class ControlledTable
    {
        #region member variables

        private const String COMMAND = "CONTROL TABLE ";
        private const String CONTROL = "CONTROL";
        private const String TABLE = "TABLE";
        private const String MDAM = "MDAM";
        private const String PRIORITY = "PRIORITY";
        private const String IF_LOCKED = "IF_LOCKED";
        private const String TABLELOCK = "TABLELOCK";
        private const String TIMEOUT = "TIMEOUT";
        private const String RESET = "RESET";
        private const String SIMILARITY_CHECK = "SIMILARITY_CHECK";
        private const int MIN_PARAMETERS = 4;
        
        public const String ENABLE = "Enable";
        public const String ON = "On";
        public const String OFF = "Off";
        public const String RETURN = "Return";
        public const String WAIT = "Wait";
        public const String WILLNOTWAIT = "Will Not Wait";
        public const String NOTIMEOUT = "No Timeout";
        
        public const int MIN_PRIORITY = 1;
        public const int MAX_PRIORITY = 9;
        public const int UNDEFINED_PRIORITY = MIN_PRIORITY - 100;
        
        public const int MIN_TIMEOUT = -1;
        public const int MAX_TIMEOUT = 2147483519;
        public const int UNDEFINED_TIMEOUT = MIN_TIMEOUT - 100;
        public const int WILLNOTWAIT_TIMEOUT = 0;
        public const int NO_TIMEOUT = -1;

        
        private String _theName = "";
        private String _theMDAM = "";
        private int _thePriority = UNDEFINED_PRIORITY;
        private String _theIfLocked = "";
        private String _theTableLock = "";
        private double _theTimeout = UNDEFINED_TIMEOUT;
        private String _theSimCheck = "";


        #endregion member variables

        #region Properties

        public String Name
        {
            get
            {
                return _theName;
            }

            set
            {
                _theName = value;
            }
        }
        public String Mdam
        {
            get { return GetMDAM(); }
            set { SetMDAM(value); }
        }
        public int Priority
        {
            get { return GetPriority(); }
            set { SetPriority(value); }
        }
        public String IfLocked
        {
            get { return GetIfLocked(); }
            set {SetIfLocked(value); }
        }
        public String TableLock
        {
            get { return GetTableLock(); }
            set { SetTableLock(value); }
        }
        public double Timeout
        {
            get { return GetTimeout(); }
            set { SetTimeout(value); }
        }
        public String SimilarityCheck
        {
            get { return GetSimilarityCheck(); }
            set { SetSimilarityCheck(value); }
        }

        #endregion Properties

        #region Constructors
        /// <summary>
        /// Creates a new instance of ControlledTable
        /// </summary>
        /// <param name="aControl">The name of the table, view or index name to which this
        /// control applies.</param>
        public ControlledTable(String aControl)
        {
            this.ParseControlTable(aControl);
        }

        /// <summary>
        /// Constructor with all parameters.
        /// </summary>
        /// <param name="name"></param>
        /// <param name="ifLocked"></param>
        /// <param name="MDAM"></param>
        /// <param name="priority"></param>
        /// <param name="simCheck"></param>
        /// <param name="tableLock"></param>
        /// <param name="timeout"></param>
        public ControlledTable(String name, String ifLocked, String MDAM,
            String priority, String simCheck, String tableLock, String timeout)
        {
            Name = name;
            IfLocked = ifLocked;
            Mdam = MDAM;
            SetPriority(priority);
            SimilarityCheck = simCheck;
            TableLock = tableLock;
            SetTimeout(timeout);
        }

        public ControlledTable(String name, String ifLocked, String MDAM,
             int priority, String simCheck, String tableLock, int timeout)
        {
            Name = name;
            IfLocked = ifLocked;
            Mdam = MDAM;
            Priority = priority;
            SimilarityCheck = simCheck;
            TableLock = tableLock;
            Timeout = timeout;
        }
        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Given a control statment returns whether this is a control table. 
        /// </summary>
        /// <param name="aControl"></param>
        /// <returns></returns>
        public static bool IsControlTable(string aControl)
        {
            Regex r = new Regex("\\s*CONTROL\\s*TABLE\\s*.*");
            if ((aControl != null) && (r.IsMatch(aControl)))
            {
                return true;
            }
            return false;
        }

        /// <summary>
        /// Returns a control statement for a given control table.
        /// </summary>
        /// <param name="control"></param>
        /// <returns></returns>
        private static List<string> GenerateControlStatement(ControlledTable control)
        {
            List<string> stmts = new List<string>();
            
            // If there are no control statements, return an empty list.
            if (!control.resultsInAnyStatements())
                return stmts;
            
            String stmtPrefix = COMMAND + NDCSName.EscapeDoubleQuotes(control.Name) + " ";
            
            // MDAM
            String strTemp = control.GetMDAM();
            if (strTemp.Length > 0)
                stmts.Add(stmtPrefix + MDAM + " ''" + strTemp + "''");
            
            // PRIORITY
            strTemp = control.GetPriorityString();
            if (strTemp.Length > 0)
                stmts.Add(stmtPrefix  + PRIORITY + " ''" + strTemp + "''");
            
            // IF_LOCKED
            strTemp = control.GetIfLocked();
            if (strTemp.Length > 0)
                stmts.Add(stmtPrefix + IF_LOCKED + " ''" + strTemp + "''");
            
            // TABLELOCK
            strTemp = control.GetTableLock();
            if (strTemp.Length > 0)
                stmts.Add(stmtPrefix + TABLELOCK + " ''" + strTemp + "''");
            
            // TIMEOUT
            int timeOut = control.GetTimeout();
            if (timeOut != UNDEFINED_TIMEOUT)
                stmts.Add(stmtPrefix + TIMEOUT + " ''" + timeOut + "''");
            
            // SIMILARITY_CHECK
            strTemp = control.GetSimilarityCheck();
            if (strTemp.Length > 0)
                stmts.Add(stmtPrefix + SIMILARITY_CHECK + " ''" + strTemp + "''");

            return stmts;
        }

        /// <summary>
        /// Returns an array of Control statements for the given array of control tables. 
        /// </summary>
        /// <param name="controls"></param>
        /// <returns></returns>
        static public String[] GenerateControlStmts(ControlledTable[] controls)
        {
            List<string> stmts = new List<string>();
            for (int i = 0; i < controls.Length; i++)
            {
                stmts.AddRange( GenerateControlStatement(controls[i]));
            }

            return stmts.ToArray<string>();
        }

        /// <summary>
        /// Returns whether this control table will result in a control statement. 
        /// </summary>
        /// <returns></returns>
        public bool resultsInAnyStatements()
        {
            return (_theMDAM.Length > 0 || _thePriority != UNDEFINED_PRIORITY ||
                _theIfLocked.Length > 0 || _theTableLock.Length > 0 ||
                _theTimeout != UNDEFINED_TIMEOUT || _theSimCheck.Length > 0);
        }

        /// <summary>
        /// Merges the passed ControlledTable object to the current object
        /// </summary>
        /// <param name="aControlledTable"></param>
        public void Merge(ControlledTable aControlledTable)
        {
            if ((aControlledTable != null) && (this.Equals(aControlledTable)))
            {
                if (!aControlledTable.Mdam.Equals("")) Mdam = aControlledTable.Mdam;
                if (!aControlledTable.IfLocked.Equals("")) IfLocked = aControlledTable.IfLocked;
                if (!aControlledTable.TableLock.Equals("")) TableLock = aControlledTable.TableLock;
                if (!aControlledTable.SimilarityCheck.Equals("")) SimilarityCheck = aControlledTable.SimilarityCheck;
                if (aControlledTable.Priority != UNDEFINED_PRIORITY) Priority = aControlledTable.Priority;
                if (aControlledTable.Timeout != UNDEFINED_TIMEOUT) Timeout = aControlledTable.Timeout;
            }
        }

        /// <summary>
        /// Overrides the default Equals method
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public override bool Equals(object obj)
        {
            ControlledTable controlledTable = obj as ControlledTable;
            if (controlledTable != null)
            {
                return NDCSObject.AreStringsEqual(Name, controlledTable.Name);
            }
            return false;
        }

        /// <summary>
        /// Compares each field to check if anything has changed
        /// </summary>
        /// <param name="aControlledTable"></param>
        /// <returns></returns>
        public bool DeepCompare(ControlledTable aControlledTable)
        {
            if (aControlledTable != null)
            {
                return (NDCSObject.AreStringsEqual(Name, aControlledTable.Name)
                    && NDCSObject.AreStringsEqual(Mdam, aControlledTable.Mdam)
                    && NDCSObject.AreStringsEqual(IfLocked, aControlledTable.IfLocked)
                    && NDCSObject.AreStringsEqual(TableLock, aControlledTable.TableLock)
                    && NDCSObject.AreStringsEqual(SimilarityCheck, aControlledTable.SimilarityCheck)
                    && (Priority == aControlledTable.Priority)
                    && (Timeout == aControlledTable.Timeout));
            }
            return false;
        }

        /// <summary>
        /// Returns a string representing this control table. 
        /// </summary>
        /// <returns></returns>
        public override String ToString()
        {
            // Format: ControlledTable[<name>, MDAM=<value>, PRIORITY=<value>, IF_LOCKED=<value>, TABLELOCK=<value>, TIMEOUT=<value>, SIMILARITY_CHECK=<value>]
            String priorityValue;
            String temp = GetPriorityString();
            if (temp.Length == 0)
                priorityValue = "UNDEFINED_PRIORITY";
            else
                priorityValue = temp;
            
            String timeoutValue;
            temp = GetTimeoutString();
            if (temp.Length == 0)
                timeoutValue = "UNDEFINED_TIMEOUT";
            else
                timeoutValue = temp;
            
            return "ControlledTable["+Name+", "+IF_LOCKED+"="+GetIfLocked()+", "+MDAM+"="+GetMDAM()+ ", "+PRIORITY+ "="+priorityValue+", "+SIMILARITY_CHECK+"="+GetSimilarityCheck()+", "+TABLELOCK+"="+GetTableLock()+", "+TIMEOUT+"="+timeoutValue+"]";
        }

        #endregion Public methods

        #region Private methods
        /// <summary>
        ///  Parses a control statement and returns a ControlledTable object
        ///  Statement Syntax:
        ///       CONTROL TABLE {table | *} control-table-option
        ///  Tokens: 0      1        2              3
        /// 
        ///  control-table-option is:
        ///     MDAM {'ENABLE' | 'ON' | 'OFF'}
        ///   | PRIORITY 'priority-value'
        ///   | IF_LOCKED {'RETURN' | 'WAIT'}
        ///   | TABLELOCK {'ENABLE' | 'ON' | 'OFF'}
        ///   | TIMEOUT 'timeout-value'
        ///   | SIMILARITY_CHECK {'ON' | 'OFF'}
        ///   | RESET (this parameter is ignored)
        /// 
        /// </summary>
        /// <param name="aControl"></param>
        private void ParseControlTable(string aControl)
        {
            if (!IsControlTable(aControl))
            {
                return;
            }

            String[] tokens = aControl.Trim().Split(new char[] { ' ' }, 3);

           // Extract the name from the parameters.
            string parameters = tokens[2];
            char[] paramCharArry = parameters.Trim().ToCharArray();

            int index = 0;
            bool inDelimitedName = false;
            
            while (index < paramCharArry.Length &&
                (inDelimitedName || ! char.IsWhiteSpace(paramCharArry[index])))
            {
                if (paramCharArry[index++] == '"')
                    inDelimitedName = !inDelimitedName;
            }
            
            // Did we exit the loop prematurely?
            if (inDelimitedName || index >= paramCharArry.Length)
            {
                return;
            }
            
            // Get the name.
            String name = parameters.Substring(0, index);
            
            // Remove all leading whitespaces from the remaining parameters list.
            while (index < paramCharArry.Length && char.IsWhiteSpace(paramCharArry[index]))
                index++;
            
            String paramList = parameters.Substring(index);
            
            // Ignore the RESET option since its use does not make sense for the
            // scope of the ControlledTable object's planned use.
            Regex r = new Regex("RESET\\s*");
            if (r.IsMatch(paramList))
            {
                return;
            }
            
            // Validate that the params list.
            Regex validParamlist = new Regex(".*\\s*'.*'");
            if (!validParamlist.IsMatch(paramList))
            {
                return;
            }
            
            // Tokenize the parameters. Since a space and a "'" (singlequote)
            // are adjacent, the second token will be blank.
            Regex parselist = new Regex("\\s|'");
            tokens =  parselist.Split(paramList);
            String option = tokens[0];
            String value = tokens[2];


            this._theName = name;
            // Set the MDAM parameter.
            if (option.Equals(MDAM, StringComparison.InvariantCultureIgnoreCase))
            {
                this.SetMDAM(value);
            }
            // Set the PRIORITY parameter.
            else if (option.Equals(PRIORITY, StringComparison.InvariantCultureIgnoreCase))
            {
                try
                {
                    this.SetPriority(int.Parse(value));
                }
                catch (Exception e)
                {
                    throw e;
                }
            }

            // Set the IF_LOCKED parameter.
            else if (option.Equals(IF_LOCKED, StringComparison.InvariantCultureIgnoreCase))
                this.SetIfLocked(value);

            // Set the TABLELOCK parameter.
            else if (option.Equals(TABLELOCK, StringComparison.InvariantCultureIgnoreCase))
                this.SetTableLock(value);

            // Set the TIMEOUT parameter.
            else if (option.Equals(TIMEOUT, StringComparison.InvariantCultureIgnoreCase))
            {
                try
                {
                    this.SetTimeout(int.Parse(value));
                }
                catch (Exception e)
                {
                    throw e;
                }
            }

            // Set the SIMILARITY_CHECK parameter.
            else if (option.Equals(SIMILARITY_CHECK, StringComparison.InvariantCultureIgnoreCase))
                this.SetSimilarityCheck(value);

            // Unknown parameter.
            else
            {
                throw new Exception(String.Format(Properties.Resources.UnknownParameterInControlTableException, aControl));
            }

        }


        private void SetMDAM(String MDAM)
        {
            if (MDAM == null || MDAM.Length == 0)
            {
                _theMDAM = "";
                return;
            }

            if (MDAM.Equals(ENABLE, StringComparison.InvariantCultureIgnoreCase))
                _theMDAM = ENABLE;
            else if (MDAM.Equals(ON, StringComparison.InvariantCultureIgnoreCase))
                _theMDAM = ON;
            else if (MDAM.Equals(OFF, StringComparison.InvariantCultureIgnoreCase))
                _theMDAM = OFF;
            else
                throw new Exception(String.Format(Properties.Resources.InvalidMDAMValueException, _theMDAM));
        }

        private String GetMDAM()
        {
            return _theMDAM;
        }

        private void SetPriority(int value)
        {
            if ((value < MIN_PRIORITY || value > MAX_PRIORITY) && value != UNDEFINED_PRIORITY)
                throw new Exception(String.Format(Properties.Resources.InvalidPriorityException, value));

            _thePriority = value;
        }

        private void SetPriority(String value)
        {
            if (value == null || value.Length == 0)
            {
                _thePriority = UNDEFINED_PRIORITY;
                return;
            }

            SetPriority(int.Parse(value));
        }

        private String GetPriorityString()
        {
            if (_thePriority == UNDEFINED_PRIORITY)
                return "";

            return _thePriority + "";
        }

        private int GetPriority()
        {
            return _thePriority;
        }

        private void SetIfLocked(String ifLocked)
        {
            if (ifLocked == null || ifLocked.Length == 0)
            {
                _theIfLocked = "";
                return;
            }

            if (ifLocked.Equals(RETURN, StringComparison.InvariantCultureIgnoreCase))
                _theIfLocked = RETURN;
            else if (ifLocked.Equals(WAIT, StringComparison.InvariantCultureIgnoreCase))
                _theIfLocked = WAIT;
            else
                throw new Exception(String.Format(Properties.Resources.InvalidIfLockedException, _theIfLocked));

        }

        private String GetIfLocked()
        {
            return _theIfLocked;
        }

        private void SetTableLock(String tablelock)
        {
            if (tablelock == null || tablelock.Length == 0)
            {
                _theTableLock = "";
                return;
            }

            if (tablelock.Equals(ENABLE, StringComparison.InvariantCultureIgnoreCase))
                _theTableLock = ENABLE;
            else if (tablelock.Equals(ON, StringComparison.InvariantCultureIgnoreCase))
                _theTableLock = ON;
            else if (tablelock.Equals(OFF, StringComparison.InvariantCultureIgnoreCase))
                _theTableLock = OFF;
            else
                throw new Exception(String.Format(Properties.Resources.InvalidTablelockException, _theTableLock));
        }

        private String GetTableLock()
        {
            return _theTableLock;
        }

        private void SetTimeout(double timeout)
        {
            if ((timeout < MIN_TIMEOUT || timeout > MAX_TIMEOUT) && timeout != UNDEFINED_TIMEOUT)
                throw new Exception(String.Format(Properties.Resources.InvalidTimeoutException, timeout));

            _theTimeout = timeout;
        }

        private void SetTimeout(String timeout)
        {
            if (timeout == null || timeout.Length == 0)
            {
                _theTimeout = UNDEFINED_TIMEOUT;
                return;
            }

            if (timeout.Equals(WILLNOTWAIT))
            {
                _theTimeout = WILLNOTWAIT_TIMEOUT;
                return;
            }

            if (timeout.Equals(NOTIMEOUT))
            {
                _theTimeout = NO_TIMEOUT;
                return;
            }

            SetTimeout(Double.Parse(timeout));
        }

        private String GetTimeoutString()
        {
            if (_theTimeout == UNDEFINED_TIMEOUT)
                return "";
            else if (_theTimeout == WILLNOTWAIT_TIMEOUT)
                return WILLNOTWAIT;
            else if (_theTimeout == NO_TIMEOUT)
                return NOTIMEOUT;

            return _theTimeout + "";
        }

        private int GetTimeout()
        {
            // Convert the double into an int.
            return (int)_theTimeout;
        }

        private void SetSimilarityCheck(String simCheck)
        {
            if (simCheck == null || simCheck.Length == 0)
            {
                _theSimCheck = "";
                return;
            }

            if (simCheck.Equals(ON, StringComparison.InvariantCultureIgnoreCase))
                _theSimCheck = ON;
            else if (simCheck.Equals(OFF, StringComparison.InvariantCultureIgnoreCase))
                _theSimCheck = OFF;
            else
                throw new Exception(String.Format(Properties.Resources.InvalidSimilarityCheckException, _theSimCheck));
        }

        private String GetSimilarityCheck()
        {
            return _theSimCheck;
        }
        
        #endregion Private methods
    }
}
