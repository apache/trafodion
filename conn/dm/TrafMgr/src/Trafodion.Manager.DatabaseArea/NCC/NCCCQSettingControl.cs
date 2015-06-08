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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Data.Odbc;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using TenTec.Windows.iGridLib;

//Ported from NPA SQ R1.0. 
//Control that manages Control Query Defaults, Control Query Shapes and Control Tables
namespace Trafodion.Manager.DatabaseArea.NCC
{
    /// <summary>
    /// NCCCQSettingControl
    /// </summary>
	public partial class NCCCQSettingControl : UserControl
    {

        #region Fields

        SortedList _cqdSortedList = new SortedList();
				
		private EventHandler _onClickHandler = null;

		const String ENTER_SHAPE = "Enter Query shape";
		const String ENTER_TABLE = "Enter {table | *} control-table-option";
        const String TRACE_SUB_AREA_NAME = "CQ Setting";

		String _shapeStr = ENTER_SHAPE;
		String _tableStr = ENTER_TABLE;
		
		ArrayList _resetControlStatements = new ArrayList();

        /// <summary>
        /// Delegate for row being added
        /// </summary>
		public delegate void rowAddedDelegate();

        /// <summary>
        /// Row added event
        /// </summary>
		public event rowAddedDelegate rowAdded;

        /// <summary>
        /// Delegate for row being deleted
        /// </summary>
        /// <param name="rra"></param>
		public delegate void rowDeletedDelegate(RemoveRowArgs rra);

        /// <summary>
        /// Row deleted event
        /// </summary>
		public event rowDeletedDelegate rowDeleted;

        /// <summary>
        /// Delegate for row being changed
        /// </summary>
		public delegate void rowChangedDelegate();

        /// <summary>
        /// Row changed event
        /// </summary>
		public event rowChangedDelegate rowChanged;

		private ReportControlStatement _cso;

        #endregion Fields

        #region Constructor

        /// <summary>
        /// Constructor
        /// </summary>
		public NCCCQSettingControl()
		{
			InitializeComponent();
            Setup(false);
        }

        #endregion Constructor

        #region Public Methods

        /// <summary>
        ///Handlers and functionality to help to gaurantee
        ///CQSettings widget gets focus when it's controls items
        ///are clicked
        /// </summary>
		public EventHandler OnClickHandler
		{
			get { return _onClickHandler; }
			set { _onClickHandler = value; }
		}


		
        /// <summary>
        /// Property: ControlStatementHolder
        /// </summary>
        public ReportControlStatement ControlStatementHolder
		{
			get { return _cso; }
			set { _cso = value; }
		}

        /// <summary>
        /// Property: CQSettingStats
        /// </summary>
        public string CQSettingStats
        {
            get { return _theCQSettingStats.Text; }
            set { _theCQSettingStats.Text = value; }
        }

        /// <summary>
        /// InitializeCQSettingsControl - set up the GUI based on the connection definition
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
		public void InitializeCQSettingsControl(ConnectionDefinition aConnectionDefinition)
		{
			_cqdSortedList.Clear();
            Setup((aConnectionDefinition != null) ? aConnectionDefinition.IsServicesUser : false);
            if (SetupCQDListForRole(aConnectionDefinition))
            {
                loadCQIGridValues(aConnectionDefinition);
            }
		}

        /// <summary>
        /// Clear the control contents
        /// </summary>
		public void Clear()
		{
			_resetControlStatements.Clear();
			cqSettingsGrid.Rows.Clear();
			cqSettingsGrid.Focus();
            _theDeleteStatementStripButton.Enabled = false;
            _theRemoveAllStripButton.Enabled = false;
		}

        /// <summary>
        /// Set up control list depends on the user privileges
        /// </summary>
        /// <param name="aPrivilegedUsers"></param>
        private void Setup(bool aPrivilegedUsers)
        {
            this.iGControlDropdownList.Items.Clear();
            if (aPrivilegedUsers)
            {
                this.iGControlDropdownList.Items.AddRange(
                    new TenTec.Windows.iGridLib.iGDropDownListItem[] {
                    new iGDropDownListItem("Control Query Default", "Control Query Default"), 
                    new iGDropDownListItem("Control Query Shape", "Control Query Shape"),
                    new iGDropDownListItem("Control Table", "Control Table")
                });
            }
            else
            {
                this.iGControlDropdownList.Items.AddRange(
                    new TenTec.Windows.iGridLib.iGDropDownListItem[] {
                    new iGDropDownListItem("Control Query Default", "Control Query Default")
                });
            }
        }

        public bool SetupCQDListForRole(ConnectionDefinition aConnectionDefinition)
        {
            if (aConnectionDefinition != null)
            {
                Connection conn = new Connection(aConnectionDefinition);
                try
                {
                    OdbcConnection odbcConnection = conn.OpenOdbcConnection;
                    try
                    {
                        //As of R2.5 SP1, there is a special CQD to enable display of both external and support CQDs.
                        string commandString = "SHOWCONTROL QUERY DEFAULT SHOWCONTROL_SUPPORT_ATTRS";
                        OdbcCommand command = new OdbcCommand(commandString, odbcConnection);
                        DataTable dt = Trafodion.Manager.Framework.Connections.OdbcAccess.executeQuery(command, 10);
                        command.Dispose();

                        //Process the rows to see if the special CQD has been set
                        for (int indexer = 0; indexer < dt.Rows.Count; indexer++)
                        {
                            String data = dt.Rows[indexer][0].ToString().Trim();
                            //Store the CQDs under Control Query Default title
                            if (data.StartsWith("SHOWCONTROL_SUPPORT_ATTRS"))
                            {
                                string[] tokens = data.Split(new char[] { ' ', '\t' });
                                if (tokens.Length > 1)
                                {
                                    //If the special CQD is already set, then no further test needs to be done.
                                    if (tokens[tokens.Length - 1].Equals("ON"))
                                        return true;
                                }
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        //If failure then do additional process below.
                    }

                    try
                    {
                        bool isAdminUser = aConnectionDefinition.isRoleDBAdmin || aConnectionDefinition.IsServicesUser;

                        //For admin users, turn ON the special CQD and all other users, turn off
                        string commanstring = string.Format("CQD SHOWCONTROL_SUPPORT_ATTRS '{0}'", isAdminUser ? "ON" : "OFF");

                        OdbcCommand command = new OdbcCommand(commanstring, odbcConnection);
                        Trafodion.Manager.Framework.Connections.OdbcAccess.executeQuery(command, 10);
                        return true;
                    }
                    catch (Exception ex)
                    {
                        return false;
                    }
                }
                finally
                {
                    if (conn != null)
                    {
                        conn.Close();
                    }
                }
            }
            else
            {
                return true;
            }
        }
        /// <summary>
        /// Make a control statement into a Control Reset statement
        /// </summary>
        /// <param name="rcs"></param>
        /// <returns></returns>
        private static StringBuilder MakeIntoResetStatement(ReportControlStatement rcs)
		{
			StringBuilder statement = new StringBuilder();
			try
			{
                if (rcs.CQType.Contains("Shape"))
                {
                    statement.Append(rcs.CQType);
					statement.Append(" OFF");
                }
                else if (rcs.CQType.Contains("Default"))
                {
                    if (!String.IsNullOrEmpty(rcs.Attribute))
                    {
                        statement.Append(rcs.CQType);
                        statement.Append("  " + rcs.Attribute + " RESET");
                    }
                }
                else if (rcs.CQType.Contains("Table"))
                {
                    if (!String.IsNullOrEmpty(rcs.Attribute))
                    {
                        statement.Append(rcs.CQType);
                        // The control table attribute is as following: 
                        //    {table | *} control-table-option
                        // so, the first token is the table itself
                        string[] attributes = rcs.Attribute.Split(' ');
                        statement.Append("  " + attributes[0].Trim() + " RESET");
                    }
                }
			}
			catch (Exception ex)
			{
				throw ex;
			}
			return statement;
		}

        /// <summary>
        /// SetControlStatementsToReset - convert all control settings to Control Reset statement
        /// </summary>
        /// <param name="aListControlStatements"></param>
        /// <param name="aResetControlStatements"></param>
		public static void SetControlStatementsToReset(List<ReportControlStatement> aListControlStatements, ArrayList aResetControlStatements)
		{
			StringBuilder statement = new StringBuilder();
			try
			{
                foreach (ReportControlStatement rcs in aListControlStatements)
				{
                    if (!rcs.Disable)
                    {
                        statement = MakeIntoResetStatement(rcs);
                        aResetControlStatements.Add(statement.ToString());
                    }
				}
			}
			catch(Exception ex) 
            {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Database,
                        TRACE_SUB_AREA_NAME,
                        "Unable to set the control statements to reset " + ex.Message);
                }
			}
		}
		
		/// <summary>
		/// 
		/// </summary>
		/// <param name="listControlStatements"></param>
        public void SetCQGridValues(List<ReportControlStatement> listControlStatements)
		{
			cqSettingsGrid.Rows.Clear();

            if (listControlStatements != null)
            {
                foreach (ReportControlStatement rcs in listControlStatements)
                {
                    iGRow row = cqSettingsGrid.Rows.Add();
                    row.Cells[0].Value = rcs.Disable;
                    row.Cells[1].Value = rcs.CQType;
                    row.Cells[2].Value = rcs.Attribute;
                    row.Cells[3].Value = rcs.Value;

                    if (row.Cells[1].Value.ToString().Contains("Shape") || row.Cells[1].Value.ToString().Contains("Table"))
                    {
                        //Change to not show CQD dropdown
                        row.Cells[2].Style = iGCellStyleText;
                        row.Cells[3].Enabled = iGBool.False;
                    }

                    if (row.Cells[1].Value.ToString().Contains("Default"))
                    {
                        row.Cells[2].Style = iGCellStyleCQDCombo;
                        row.Cells[3].Enabled = iGBool.True;
                        if (!InCQDDropDown(rcs.Attribute))
                            storeParsedCQinList(rcs.Attribute);
                    }

                }
            }

            populateCQDDropdownList();
            UpdateToolStrip();
		}

        /// <summary>
        /// Convert grid items to objects for later storage and retrieval.
        /// Return null if there is no item added.
        /// </summary>
        /// <returns></returns>
        public List<ReportControlStatement> GetCQGridValues()
		{
				
            List<ReportControlStatement> tempList = new List<ReportControlStatement>();
			
			foreach (iGRow row in cqSettingsGrid.Rows)
			{

				ReportControlStatement cs = new ReportControlStatement();
				cs.Disable = (bool)row.Cells[0].Value;
				cs.CQType = row.Cells[1].Value as string;
				cs.Attribute = row.Cells[2].Value as string;
				cs.Value = row.Cells[3].Value as string;
                tempList.Add(cs);
			}

            return (tempList.Count > 0) ? tempList : null;
		}

        /// <summary>
        /// Convert the items in the wbqd.ControlStatements arraylist to realb commands for SQL.
        /// </summary>
        /// <param name="listControlStatements"></param>
        /// <returns></returns>
        public static ArrayList PrepareControlStatements(List<ReportControlStatement> listControlStatements)
		{
			ArrayList statements = new ArrayList();

			try
			{
			    foreach (ReportControlStatement rcs in listControlStatements)
				{
					StringBuilder statement = new StringBuilder();

					if (rcs.CQType.Contains("Shape") && rcs.Disable == false)
					{
                        statement.Append(rcs.CQType + "  " + rcs.Attribute);
						statements.Add(statement.ToString());
					}

                    if (rcs.CQType.Contains("Table") && rcs.Disable == false)
					{
                        statement.Append(rcs.CQType + "  " + rcs.Attribute);
						statements.Add(statement.ToString());
					}

                    if (rcs.CQType.Contains("Default") && rcs.Disable == false)
					{
                        statement.Append(rcs.CQType + " " + rcs.Attribute + " '" + rcs.Value + "'");
						statements.Add(statement.ToString());
					}
				}
			}
			catch (Exception ex)
			{
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Database,
                        TRACE_SUB_AREA_NAME,
                        "Unable to prepare the control statements correctly" + ex.Message);
                }
				return null;
			}

			return statements;
        }

        #endregion Public Methods

        #region Private methods

        /// <summary>
        /// setAsActiveControl
        /// </summary>
        /// <param name="eArgs"></param>
        private void setAsActiveControl(EventArgs eArgs)
        {
            if (null != _onClickHandler)
                this._onClickHandler(this, eArgs);
        }

        /// <summary>
        /// Take all the CQDs stored in the sorted list and add them to the CQD dropdown
        /// </summary>
		private void populateCQDDropdownList()
		{
			iGCQDDropdownList.Items.Clear();
			
			foreach (DictionaryEntry element in _cqdSortedList)
			{
				iGCQDDropdownList.Items.Add((String)element.Key);	
			}
		}

        /// <summary>
        /// 
        /// </summary>
        /// <param name="cq"></param>
		private void storeParsedCQinList(String cq)
		{
			if (cq == "")
				return;

			String[] cqdStr;
			String cqdVal;

			cq = cq.Trim();
			cqdStr = cq.Split(new char[] { ' ', '\t' });
			if (!InCQDDropDown(cq))
			{
				cqdVal = cq.Substring(cqdStr[0].Length, cq.Length - cqdStr[0].Length);
				
				if(!_cqdSortedList.Contains(cqdStr[0].ToString()))
					_cqdSortedList.Add(cqdStr[0], cqdVal.Trim());
			}
		}

        /// <summary>
        /// 
        /// </summary>
        /// <param name="anOdbcConnection"></param>
        /// <returns></returns>
        private bool loadCQIGridValues(ConnectionDefinition aConnectionDefinition)
        {
			DataTable dt;
            Connection conn = null;

            try
            {
                if (aConnectionDefinition != null)
                {
                    conn = new Connection(aConnectionDefinition);
                    OdbcCommand command = new OdbcCommand("SHOWCONTROL ALL", conn.OpenOdbcConnection);
                    dt = Trafodion.Manager.Framework.Connections.OdbcAccess.executeQuery(command, 2000);
                    command.Dispose();
                    int indexer = 0;

                    for (indexer = 0; indexer < dt.Rows.Count; indexer++)
                    {
                        String data = dt.Rows[indexer][0].ToString().Trim();
                        //Store the CQDs under Control Query Default title
                        if (data.StartsWith("CONTROL QUERY DEFAULT") ||
                            data.StartsWith("No CONTROL QUERY DEFAULT"))
                        {
                            for (int c = indexer + 1; indexer < dt.Rows.Count; c++)
                            {
                                data = dt.Rows[c][0].ToString();
                                //Increment to current Defaults title then break
                                if (data.StartsWith("Current DEFAULTS"))
                                {
                                    indexer++;
                                    break;
                                }
                                storeParsedCQinList(data);
                                indexer++;
                            }
                            break;
                        }
                    }
                    //Now store the rest of the CQDs 
                    for (int i = indexer + 1; i < dt.Rows.Count; i++)
                    {
                        String data = dt.Rows[i][0].ToString();
                        storeParsedCQinList(data);

                    }
                }

                //Load all the CQDs into the dropdown 
                populateCQDDropdownList();
                return true;

            }
            catch (Exception ex)
            {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Database,
                        TRACE_SUB_AREA_NAME,
                        "Unable to retrieve the Control Query Defaults" + ex.Message);
                }
                return false;
            }
            finally
            {
                if (conn != null)
                {
                    conn.Close();
                }
            }

		}

        /// <summary>
        /// addRowCQGridBtn_Click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void addRowCQGridBtn_Click(object sender, EventArgs e)
		{
			cqSettingsGrid.RowTextVisible = false;

			iGRow newRow = cqSettingsGrid.Rows.Add();

			newRow.Cells[0].Value = false;

			if (0 < iGCQDDropdownList.Items.Count) 
            {
				newRow.Cells[2].Value = iGCQDDropdownList.Items[0].Value.ToString();
				newRow.Cells[3].Value = _cqdSortedList[iGCQDDropdownList.Items[0].Value.ToString()];
			}
									
			newRow.Cells[1].Value = iGControlDropdownList.Items[0].Value.ToString();

			setAsActiveControl(e);
			rowAdded();
            UpdateToolStrip();
		}

        /// <summary>
        /// removeCQRowBtn_Click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void removeCQRowBtn_Click(object sender, EventArgs e)
		{
			try 
            {
				cqSettingsGrid.Focus();
				cqSettingsGrid.RowTextVisible = false;

				if (0 < cqSettingsGrid.SelectedRows.Count) 
                {
                    int count = cqSettingsGrid.SelectedRows.Count;

					for (int i = 0; count > i; i++) {
                        int selectionIndex = cqSettingsGrid.SelectedRows[0].Index;
						cqSettingsGrid.Rows.RemoveAt(selectionIndex);

						RemoveRowArgs rra = new RemoveRowArgs(selectionIndex);

						rowDeleted(rra);
					}
				}
			} 
            catch (Exception) 
            { } 
            finally 
            {
				_resetControlStatements.Clear();
				setAsActiveControl(e);
                //_theDeleteStatementStripButton.Enabled = false;
                UpdateToolStrip();
				this.Refresh();
			}
		}

        /// <summary>
        /// clearCQgridbtn_Click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void clearCQgridbtn_Click(object sender, EventArgs e)
		{
			cqSettingsGrid.RowTextVisible = false;
			Clear();
			setAsActiveControl(e);
			rowChanged();
            //_theDeleteStatementStripButton.Enabled = false;
            UpdateToolStrip();
		}

        /// <summary>
        /// Determines if a value is in the CQ dropdown list
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
		private bool InCQDDropDown(String value)
		{
			if (value == "")
				return false;

			for (int i = 0; iGCQDDropdownList.Items.Count > i; i++ )
			{
				if (iGCQDDropdownList.Items[i].Value.ToString() == value)
					return true;
			}

			return false;

		}

        /// <summary>
        /// Used to support extra Control Query Defaults typed in manually
        /// </summary>
        /// <param name="index"></param>
		private void addNewCQ(int index)
		{
			try 
            {
				String cell2Val = cqSettingsGrid.Rows[index].Cells[2].Value.ToString();

				if (!InCQDDropDown(cell2Val) && !cell2Val.Equals(ENTER_SHAPE)
					&& !cell2Val.Equals(ENTER_TABLE))
				{
					if (!_cqdSortedList.ContainsKey(cell2Val)) 
                    {
						String cqdValue = "";
						try 
                        { 
							cqdValue = cqSettingsGrid.Rows[index].Cells[3].Value.ToString();
						} catch (Exception) {
							cqdValue = "";
						}

						_cqdSortedList.Add(cell2Val, cqdValue);
						populateCQDDropdownList();
						cqSettingsGrid.Rows[index].Cells[2].Value = cell2Val;
					}			
				}
			}
			catch (Exception ex) {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Database,
                        TRACE_SUB_AREA_NAME,
                        "addNewCQ(): caught an exception '" + ex.Message + "'. ");
                }

			}
		}

        /// <summary>
        /// 
        /// </summary>
        private void UpdateToolStrip()
        {
            string stats = "";
            if (cqSettingsGrid.Rows.Count > 0)
            {
                _theRemoveAllStripButton.Enabled = true;
                _theDeleteStatementStripButton.Enabled = (cqSettingsGrid.SelectedRows.Count > 0);

                if (cqSettingsGrid.Rows.Count > 1)
                {
                    stats = String.Format("Total Control Statements: {0}", cqSettingsGrid.Rows.Count);
                }
                else
                {
                    stats = String.Format("Total Control Statement: {0}", cqSettingsGrid.Rows.Count);
                }
            }
            else
            {
                _theDeleteStatementStripButton.Enabled = false;
                _theRemoveAllStripButton.Enabled = false;
            }

            CQSettingStats = stats;
        }

        /// <summary>
        /// iGControlDropdownList_SelectedItemChanged
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void iGControlDropdownList_SelectedItemChanged(object sender, TenTec.Windows.iGridLib.iGSelectedItemChangedEventArgs e)
		{
			setAsActiveControl(e);

			try
			{
                cqSettingsGrid.BeginUpdate();
				if (e.SelectedItem != null)
				{
                    int index = cqSettingsGrid.SelectedRows[0].Index;
		
					if (e.SelectedItem.Value.ToString().Contains("Shape"))
					{
						//Change to not show CQD dropdown
                        if (null != cqSettingsGrid.Rows[index].Cells[2].Style)
                        {
                            cqSettingsGrid.Rows[index].Cells[2].Style.DropDownControl = null;
                        }
                        cqSettingsGrid.Rows[index].Cells[2].Style = iGCellStyleText;
                        cqSettingsGrid.Rows[index].Cells[2].Value = _shapeStr;
						cqSettingsGrid.Rows[index].Cells[3].Enabled = iGBool.False;
						cqSettingsGrid.Rows[index].Cells[3].Value = "N/A";

						return;
					}
					if (e.SelectedItem.Value.ToString().Contains("Table"))
					{
						//Change to not show CQD dropdown
                        if (null != cqSettingsGrid.Rows[index].Cells[2].Style)
                        {
                            cqSettingsGrid.Rows[index].Cells[2].Style.DropDownControl = null;
                        }
                        cqSettingsGrid.Rows[index].Cells[2].Style = iGCellStyleText;
                        cqSettingsGrid.Rows[index].Cells[2].Value = _tableStr;
						cqSettingsGrid.Rows[index].Cells[3].Enabled = iGBool.False;
						cqSettingsGrid.Rows[index].Cells[3].Value = "N/A";
						return;
					}
										
					if (e.SelectedItem.Value.ToString().Contains("Default"))
					{
                        cqSettingsGrid.Rows[index].Cells[2].Style = iGCellStyleCQDCombo;
                        cqSettingsGrid.Rows[index].Cells[2].Style.DropDownControl = this.iGCQDDropdownList;
                        cqSettingsGrid.Rows[index].Cells[2].Value = (this.iGCQDDropdownList.Items.Count > 0) ? this.iGCQDDropdownList.Items[0].Value as string: "";
						cqSettingsGrid.Rows[index].Cells[3].Value = _cqdSortedList[iGCQDDropdownList.Items[0].Value.ToString()];
						cqSettingsGrid.Rows[index].Cells[3].Enabled = iGBool.True;
						
                        //Save the CQD into the dropdown if it is new
						addNewCQ(index);
					}
				}
			}
			catch (Exception ex) 
            {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Database,
                        TRACE_SUB_AREA_NAME,
                        "Got an error in Type dropdown change() : error = " + ex.Message);
                }
			}
			finally
			{
				rowChanged();
                cqSettingsGrid.EndUpdate();
			}
		}

        /// <summary>
        /// iGCQDDropdownList_SelectedItemChanged
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void iGCQDDropdownList_SelectedItemChanged(object sender, TenTec.Windows.iGridLib.iGSelectedItemChangedEventArgs e)
		{
			setAsActiveControl(e);

			if (e.SelectedItem != null)
			{
				String value = e.SelectedItem.Value.ToString();
				try
				{
					if (_cqdSortedList.ContainsKey(value))
					{
                        int index = cqSettingsGrid.SelectedRows[0].Index;

						cqSettingsGrid.Rows[index].Cells[3].Value = _cqdSortedList[value];
						cqSettingsGrid.Rows[index].Cells[2].Value = value;
					}
				}
				catch (Exception ex) {
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(
                            TraceOptions.TraceOption.DEBUG,
                            TraceOptions.TraceArea.Database,
                            TRACE_SUB_AREA_NAME,
                            "Got an error in CQD Dropdown change() : error = " + ex.Message);
                    }
				}
				finally
				{
					rowChanged();
				}
			}

		}

        /// <summary>
        /// cqSettingsGrid_SelectionChanged
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void cqSettingsGrid_SelectionChanged(object sender, EventArgs e)
		{
			setAsActiveControl(e);
            _theDeleteStatementStripButton.Enabled = true;
		}
	
		//For testing
		private void reloadCQDs_Click(object sender, EventArgs e)
		{
			setAsActiveControl(e);
		}

		/// <summary>
        /// cqSettingsGrid_AfterCommitEdit
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void cqSettingsGrid_AfterCommitEdit(object sender, iGAfterCommitEditEventArgs e) {
			try {
				int rowIdx = e.RowIndex;
				addNewCQ(rowIdx);
			} 
            catch (Exception) 
            { }
			
			rowChanged();
		}

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void cqSettingsGrid_Click(object sender, EventArgs e) 
        {
			setAsActiveControl(e);
		}

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void cqSettingsGrid_CellClick(object sender, iGCellClickEventArgs e)
		{
			setAsActiveControl(e);
		}

        /// <summary>
        /// copyContentsToClipboard
        /// </summary>
		public void copyContentsToClipboard() {
            if (0 >= cqSettingsGrid.SelectedRows.Count)
				return;

            int i = cqSettingsGrid.SelectedRows[0].Index;

			try {
				String value = cqSettingsGrid.CurCell.Value.ToString();
				Clipboard.SetText(value);

			} catch(Exception) 
            { }
		}

        /// <summary>
        /// cutPasteContentsToClipboard
        /// </summary>
        /// <param name="copyTextToClipboard"></param>
        /// <param name="valueToSet"></param>
		private void cutPasteContentsToClipboard(bool copyTextToClipboard, String valueToSet) {
            if (0 >= cqSettingsGrid.SelectedRows.Count)
				return;

            int i = cqSettingsGrid.SelectedRows[0].Index;
			int curColIndex = cqSettingsGrid.CurCell.ColIndex;

			try {
				if (copyTextToClipboard) {
					String value = cqSettingsGrid.CurCell.Value.ToString();
					Clipboard.SetText(value);
				}

				// this.Focus();
				if (2 == curColIndex)
					cqSettingsGrid.Rows[i].Cells[2].Value = valueToSet;
				else if ((3 == curColIndex) &&
						 cqSettingsGrid.Rows[i].Cells[1].Value.ToString().StartsWith("Control Query Default"))
					cqSettingsGrid.Rows[i].Cells[3].Value = valueToSet;

			} catch (Exception e) {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Database,
                        TRACE_SUB_AREA_NAME,
                        "cutPasteContentsToClipboard(): Got an error '" + e.Message + "'. ");
                }
			} 
            finally 
            { }

		}

        /// <summary>
        /// To select all rows
        /// </summary>
		private void selectAllRows() {
			try {
				cqSettingsGrid.PerformAction(iGActions.SelectAllRows);
			} 
            catch (Exception) 
            { }

		}

        /// <summary>
        /// ProcessCmdKey
        /// </summary>
        /// <param name="msg"></param>
        /// <param name="keyData"></param>
        /// <returns></returns>
		protected override bool ProcessCmdKey(ref Message msg, Keys keyData) {
			bool processFurther = true; 

			switch (msg.Msg) {
				case 0x100:
				case 0x104:
					switch (keyData) {
						case Keys.Control | Keys.C:
						case Keys.Control | Keys.Shift | Keys.C:
							copyContentsToClipboard();
							processFurther = false;
							break;

						case Keys.Control | Keys.X:
						case Keys.Control | Keys.Shift | Keys.X:
							cutPasteContentsToClipboard(true, "");
							processFurther = false;
							break;

						case Keys.Control | Keys.V:
						case Keys.Control | Keys.Shift | Keys.V:
							cutPasteContentsToClipboard(false, Clipboard.GetText());
							processFurther = false;
							break;

						case Keys.Control | Keys.A:
						case Keys.Control | Keys.Shift | Keys.A:
							selectAllRows();
							processFurther = false;
							break;

						default:  break;
					}
					break;

				default: break;
			}

			if (!processFurther)
				return false;

			return base.ProcessCmdKey(ref msg, keyData);
		}

        private void _theHelpStripButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ControlSettings);
        }

        private void _theApplyStripButton_Click(object sender, EventArgs e)
        {

        }

        #endregion Private Methods
    }

    #region RemoveRowArgs Class

    /// <summary>
    /// remove row attributes
    /// </summary>
	public class RemoveRowArgs : System.EventArgs
	{

		private int deletedIndex;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="i"></param>
		public RemoveRowArgs(int i)
		{
			this.deletedIndex = i;
		}

        /// <summary>
        /// return the deleted index
        /// </summary>
        /// <returns></returns>
		public int DeletedIndex()
		{
			return deletedIndex;
		}
    }

    #endregion  RemoveRowArgs Class

}
