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
using System.Data;
using System.Text;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Threading;
using System.Data.Odbc;
using System.Collections;
using System.Windows.Forms;

namespace Trafodion.Manager.WorkloadArea.Model
{
    public class WMSOdbcAccess
    {
        #region Statics
        private static int m_errors = 0;
        private static WMSOdbcAccess m_odbcAccess = null;
        private static Form m_form = null;
        #endregion

        #region Properties
        public static int OdbcErrors
        {
            get { return m_errors; }
        }
        #endregion

        #region Constructors
        private WMSOdbcAccess()
        {
        }

        public static WMSOdbcAccess getNVOdbcAccess(Form form)
        {
            if (m_odbcAccess == null)
            {
                m_odbcAccess = new WMSOdbcAccess();
            }
            m_form = form;

            return m_odbcAccess;
        }
        #endregion

        public static DataTable getDataTableFromSQL(OdbcConnection connection, OdbcCommand command, string sqlStatement)
        {
            if (connection == null || command == null)
                return null;

            DataTable dt = null;
            try
            {
                dt = new DataTable();
                OdbcDataAdapter da = null;
                m_errors = 0;

                command.Connection = connection;
                command.CommandText = sqlStatement;
                da = new OdbcDataAdapter(command);
                da.FillError += new FillErrorEventHandler(da_FillError);

                da.Fill(dt);
            }
            catch (OdbcException ex)
            {
                Console.WriteLine("getDataTableFromSQL() OdbcException: " + ex.Message);
                throw ex;
            }

            return dt;
        }

        static void da_FillError(object sender, FillErrorEventArgs e)
        {
            e.Continue = true;
            m_errors++;
            MessageBox.Show(m_errors + ") da_FillError() " + e.Errors.Message, Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
            if (e.DataTable != null)
            {
                DataRow[] dr = e.DataTable.GetErrors();
                for (int i = 0; i < dr.Length; i++)
                {
                    MessageBox.Show(m_errors + ") da_FillError() dr[" + i + "]=" + dr[i].RowError + Environment.NewLine, Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

    }

}
