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
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// Wrapper class extending the HPDataGridView class. It has hooks to change the look and feel
    /// when the look and feel of the framework is changed.
    /// Base class for One GUI data grid views.  
    /// </summary>
    [ToolboxBitmapAttribute(typeof(DataGridView))]
    public class TrafodionDataGridView : DataGridView
    {

        private TrafodionLookAndFeelChangeHandler lookAndFeelChangeHandler = null;

        /// <summary>
        /// Header to print on every page if PDF selected
        /// </summary>
        private string theHeaderText = "";

        /// <summary>
        /// Save file dialog used by all instances of TrafodionDataGridView for duration of client session
        /// </summary>
        private SaveFileDialog theSaveFileDialog = null;

        private HelpProvider _helpProviderTrafodionDataGrid;

        /// <summary>
        /// Deliminated char for CSV file. Can be comma, TAB "\t" or just one-SPACE
        /// </summary>
        private string _deliminatedCharForCSVFile = ",";

        /// <summary>
        /// pattern match for multi-line TAB chars  etc. in a grid-cell
        /// </summary>
        private string _multiLinePatternToMatch = @"\r|\n|\t|\r\n";
        private Control _theCountControl;
        /// <summary>
        /// Constructor
        /// </summary>
        public TrafodionDataGridView()
        {
            //Changes the theme when the theme is changed for the framework and
            //also sets the default theme
            lookAndFeelChangeHandler = new TrafodionLookAndFeelChangeHandler(this);

            // Call standard initialization on ourselves.
            StandardInit();

            _helpProviderTrafodionDataGrid.HelpNamespace = TrafodionHelpProvider.TrafodionHelpFile;
            _helpProviderTrafodionDataGrid.SetHelpNavigator(this, System.Windows.Forms.HelpNavigator.TableOfContents);
            _helpProviderTrafodionDataGrid.SetShowHelp(this, true);

            if (HelpTopic != null)
            {
                _helpProviderTrafodionDataGrid.SetHelpKeyword(this, HelpTopic);
            }

            // enable ClipBoard including the table-Headers
            this.ClipboardCopyMode = DataGridViewClipboardCopyMode.EnableAlwaysIncludeHeaderText;

        }

        protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
        {
            switch (keyData)
            {
                case Keys.Control | Keys.C:
                case Keys.Control | Keys.Shift | Keys.C:
                    {
                        Clipboard.SetDataObject(this.GetClipboardContent());
                        break;
                    }
            } return base.ProcessCmdKey(ref msg, keyData);
        }
        /// <summary>
        /// Apply the standard initialization to ourslef
        /// </summary>
        public void StandardInit()
        {
            StandardInit(this);
            
             _helpProviderTrafodionDataGrid = new HelpProvider();
                                    
        }



        /// <summary>
        /// Apply the standard initialization to any DataGridView
        /// </summary>
        public static void StandardInit(TrafodionDataGridView aDataGridView)
        {
            // Set everything the way One GUI likes it
            aDataGridView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            aDataGridView.AutoSizeRowsMode = System.Windows.Forms.DataGridViewAutoSizeRowsMode.AllCells;
            aDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            aDataGridView.EditMode = System.Windows.Forms.DataGridViewEditMode.EditProgrammatically;
            aDataGridView.RowTemplate.ReadOnly = true;
            aDataGridView.RowHeadersVisible = false;
            aDataGridView.BackgroundColor = Color.WhiteSmoke;
            aDataGridView.AllowUserToAddRows = false;
            aDataGridView.AllowUserToOrderColumns = true;
            aDataGridView.AllowUserToResizeColumns = true;
            aDataGridView.AlternatingRowsDefaultCellStyle.BackColor = Color.WhiteSmoke;
            aDataGridView.GridColor = Color.LightGray;
            aDataGridView.BorderStyle = BorderStyle.None;
            aDataGridView.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
            aDataGridView.Font = new Font("Tahoma", 8.25F, FontStyle.Regular);
            aDataGridView.ColumnHeadersDefaultCellStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);

        }


        /// <summary>
        /// Initialize the File-dialog with appropriate dir  and file type
        /// </summary>
        private void FileDialogInit()
        {

            // Create the save file as dialog
            theSaveFileDialog = new SaveFileDialog();

            // start it  off with the same directory as it ended with the last time in the same session or previous.
            theSaveFileDialog.InitialDirectory = Utilities.FileDialogLocation();

            // We default to CSV Excel-appropriate format
            theSaveFileDialog.DefaultExt = "CSV";

            // Set the file dialog filter to the kinds of files we support
            theSaveFileDialog.Filter =
                  "Comma Separated Values Format file (*.CSV)|*.CSV"
                + "|HyperText Markup Language file (*.HTML)|*.HTML";

            // Future: 
            //  + "|Excel Spreadsheet file (*.XLS)|*.XLS";
            //+ "|Portable Document Format file (*.PDF)|*.PDF"

        }


        /// <summary>
        /// Export the grid to a file
        /// </summary>
        public void ExportToFile()
        {
            // create a Save-File-Dialog and position in appropriate dir
            FileDialogInit();

            // Present the dilaog to the user
            if (theSaveFileDialog.ShowDialog() == DialogResult.OK)
            {

                // Make comparisons easier
                string theUpperFileName = theSaveFileDialog.FileName.ToUpper();

                // Persist the last file-location in this or across user sessions
                Utilities.FileDialogLocation(theUpperFileName);

                 // Check to see what extension the filename ends with and pass in the chosen filename
                if (theUpperFileName.EndsWith(".CSV"))
                {

                    // CSV
                    ExportToCSVFile(theSaveFileDialog.FileName);

                }
                else if (theUpperFileName.EndsWith(".HTML"))
                {

                    // HTML
                    ExportToHtmlFile(theSaveFileDialog.FileName);
 
                }

                //if (theUpperFileName.EndsWith(".XLS"))
                //{

                //    // XLS
                //    ExportToExcelFile(theSaveFileDialog.FileName);

                //}
                //else if (theUpperFileName.EndsWith(".PDF"))
                //{

                //    // PDF
                //    ExportToPdfFile(theSaveFileDialog.FileName);

                //}

                else
                {

                    // Other: File type must be CSV or HTML
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.AllowedExportFileTypes, Properties.Resources.Error, MessageBoxButtons.OK); 

                }


            }
        }


        /// <summary>
        /// Save this data grid to an XLS file for Excel
        /// </summary>
        /// <param name="aFileName">The filename from the save file dialog</param>
        public void ExportToExcelFile(string aFileName)
        {
            //NOTE: There are many different ways this can be implemented.
            // usingExcelApplication(aFileName);
            // DataFromGridViewUsingDataSet();
        }


        /// <summary>
        /// Export the data grid to an Excel application's spreadsheet
        /// Saves data in html format and then invokes Excel.
        /// </summary>
        public void ExportToSpreadsheet()
        {
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                // Use system's feature to create/manage a unique temp file name
                string tempFileName = System.IO.Path.GetTempFileName();

                // add extension .xls to it for system to start the default spreadsheet-EXCEL app
                tempFileName += ".XLS";

                // 1. Save the data to an HTML file -suitable for Excel import
                ExportToHtmlFile(tempFileName);

                // 2. Launch Excel application on the above html file
                StartExcelSpreadsheetForHTMLFile(tempFileName);

                // 3. Leave the Deletion of this file to the user.
            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }

        }

        /// <summary>
        /// Export the full data from the grid to the system's Clipboard to enable paste
        /// e.g. to a word document.
        /// </summary>
        public void ExportToClipboard()
        {
            // call the internal helper method
            CopyDataToClipBoard();

        }


        /// <summary>
        /// Export the data grid to the browser- Internet explorer
        /// Saves data in html format and then invokes I.E.
        /// </summary>
        public void ExportToBrowser()
        {
            // Use system's feature to create/manage a unique temp file name
            string tempFileName = System.IO.Path.GetTempFileName();

            // add extension .html ot it for system to start the default browser.
            tempFileName += ".html";

            // 1. Save the data to a HTML file - suitable for IE import
            ExportToHtmlFile(tempFileName);

            // 2. Launch Internet Explorer Browser on the above html file
            StartBrowserForHTMLFile(tempFileName);

        }

        /// <summary>
        /// Save this data grid to a Comma Separated Values (.CSV) file for Excel import
        /// </summary>
        /// <param name="aFileName">The filename from the save file dialog</param>
        public void ExportToCSVFile(string aFileName)
        {
            // Open a file to write data from the grid
            StreamWriter theCSVStreamWriter = null;
            try
            {
                theCSVStreamWriter = File.CreateText(aFileName);

                // Write column-headers
                WriteColumnHeaderToCSVFile(theCSVStreamWriter);


                // Write ROWS: all  data from the rows
                WriteDataRowsToCSVFile(theCSVStreamWriter);
            }
            catch (Exception e)
            {
                //  "Error in  opening or writing to file: " + aFileName + "  Exception: " + e.Message);
                string message = string.Format(Properties.Resources.ErrorOpeningWritingToFile, aFileName, e.Message);
                MessageBox.Show(Utilities.GetForegroundControl(), message, Properties.Resources.Error, MessageBoxButtons.OK); 
            }
            finally
            {
                if (theCSVStreamWriter != null)
                {
                    try
                    {
                        theCSVStreamWriter.Close();
                    }
                    finally
                    {
                    }
                }
            } // end-try block
        }


        /// <summary>
        /// Write the Grid's column headers to the CSV file
        /// </summary>
        /// <param name="aCSVStreamWriter"></param>
        private void WriteColumnHeaderToCSVFile(StreamWriter aCSVStreamWriter)
        {
            int    columnIndex = 0;
            string columnText  = "";
            string rowText     = "";

            //  navigate thru column-headers, re-format the cells and write to the file
            foreach (DataGridViewColumn column in this.Columns)
            {
                columnText = "\"" + column.HeaderText + "\"";
                rowText += columnText;
                // Don't need separator/deliminator for last cell
                if (++columnIndex < this.Columns.Count)
                {
                    rowText += _deliminatedCharForCSVFile; // deliminator
                }
            }
            //Console.WriteLine(rowText);
            aCSVStreamWriter.WriteLine(rowText);
        }




        /// <summary>
        /// helper method to Write each row in CSV format to the file
        /// </summary>
        /// <param name="aCSVStreamWriter"></param>
        private void WriteDataRowsToCSVFile(StreamWriter aCSVStreamWriter)
        {
            string rowText    = "";
            string columnText = "";

            // navigate thru row-by-row, re-format the cells and write to the file
            foreach (DataGridViewRow row in this.Rows)
            {
                rowText = "";

                // navigate to each columns and make one-line per row to write to file
                for (int columnIndex = 0; columnIndex < this.Columns.Count; columnIndex++)
                {
                    DataGridViewCell cell = row.Cells[columnIndex];

                    string cellValue = "";

                    if (cell is DataGridViewImageCell)
                    {
                        cellValue = ((DataGridViewImageCell)cell).Description;
                    }
                    else
                    {

                        cellValue = "";
                        if (cell.Value != null)
                        {
                            //For DateTime use the long format so we preserve the microseconds
                            if (cell.Value is DateTime)
                                cellValue = Utilities.GetTrafodionSQLLongDateTime((DateTime)cell.Value, false);
                            else
                                cellValue = cell.Value.ToString();
                        }
                    }

                    if (string.IsNullOrEmpty(cellValue) || cellValue.StartsWith("\0"))
                        cellValue = "";

                    // Comments/Strings may span multiple lines
                    cellValue = ReplaceMultilineChars(cellValue);

                    // if cell has double-quotes as part of name e.g. "<FooBar>" is the name
                    // Put quotes around the value
                    columnText = "\"" + cellValue.Replace("\"", "\"\"")  + "\"";
                    rowText += columnText;
                    // Don't need separator/deliminator for last cell
                    if (columnIndex < row.Cells.Count-1 )
                    {
                        rowText += _deliminatedCharForCSVFile; // deliminator
                    }
                }
                aCSVStreamWriter.WriteLine(rowText);
            } // end-foreach on rows

        }

        /// <summary>
        /// Replace all newline, TAB chars from the input string with a space
        /// </summary>
        /// <param name="aFromString"></param>
        /// <returns></returns>
        private string ReplaceMultilineChars(string aFromString)
        {
            // Using  regular expressions 
            // this could be an in-line statement for efficiency. 
            // A separate method for readability and keep it modular to add new scenarios/chars
            return Regex.Replace(aFromString, _multiLinePatternToMatch, " ");
        }

#if NEED_TO_DUMP_GRID_DATA
        /// <summary>
        /// Internal helper method to dump the grid for debugging purposes
        /// </summary>
        /// to call ,ethod this e.g.  ToStringDataFromGridViewUsingCells();
        /// 
        private void ToStringDataFromGridViewUsingCells()
        {
            int numRows    = this.Rows.Count;
            int numColumns = this.Columns.Count;
            Console.WriteLine(" Rows: " + numRows + " Columns: " + numColumns );

            int rowIndex    = 0;
            int columnIndex = 0;
            foreach (DataGridViewRow row in this.Rows)
            {
                rowIndex++;
                columnIndex = 0;
                Console.Write("Row: " + rowIndex + "  Column-Values:  " );
                foreach (DataGridViewColumn col in this.Columns)
                {
                    DataGridViewCell cell = row.Cells[columnIndex++];
                    //Console.WriteLine("Row: " + rowIndex + " Col: " + columnIndex +  " Value: " + cell.Value);
                    Console.Write(cell.Value + "   " );
                }
                Console.WriteLine("");
            }
        }
#endif


#if FEATURE_LEVERAGE_DATGRID_DATASET

        /// <summary>
        /// If we decide to leverage DataSet. Grid need be bind with it for it to work well.
        /// can be used to dump in XML and other formats
        /// </summary>
        private void DataFromGridViewUsingDataSet()
        {
            // DataTable table = ((DataSet)this.DataSource).Tables[0];
            DataSet ds = (DataSet)this.DataSource;
            // Will crash now on this statement until DataSet is bound to the Grid
            DataTable table = ds.Tables[0]; // DataTableCollection

            int ColumnIndex = 0;
            foreach (DataColumn col in table.Columns)
            {
                ColumnIndex++;
                Console.WriteLine("ColumnIndex: " + ColumnIndex + " Value: " + col.ColumnName);
            }
            int rowIndex = 0;

            foreach (DataRow row in table.Rows)
            {
                rowIndex++;
                ColumnIndex = 0;
                foreach (DataColumn col in table.Columns)
                {
                    ColumnIndex++;
                    Console.WriteLine("rowIndex: " + rowIndex + " ColumnIndex: " + ColumnIndex + " Value: " + row.ItemArray[ColumnIndex - 1]);
                }
            }
        }

#endif


#if  FEATURE_USE_EXCEL_INTEROP

        // If we decide to inter-operate with Excel- move it at the top
        //using Microsoft.Office.Interop.Excel;

        /// <summary>
        /// Leverage Excel interoperability --Save this data grid to an XLS file for Excel
        /// Note that: this approach is same in-principle as the 3rd party software from  ConmpleIT tried earlier.
        /// Excel adn HP DM  need to work as coopearting apps via COM interface
        /// </summary>
        /// <param name="aFileName">The filename from the save file dialog</param>
        private void usingExcelApplication(string aFileName)
        {

            // TODO: After installing the Excel.dll 

            Excel.Application excel = new Excel.Application();
            Excel.Workbook workbook = excel.Application.Workbooks.Add(true);
            DataTable table = ((DataSet)this.DataSource).Tables[0];
         
            int ColumnIndex = 0;
            foreach (DataColumn col in table.Columns)
            {
                ColumnIndex++;
                excel.Cells[1, ColumnIndex] = col.ColumnName;
            }
            int rowIndex = 0;
         
            foreach (DataRow row in table.Rows)
            {
                rowIndex++;
                ColumnIndex = 0;
                foreach (DataColumn col in table.Columns)
                {
                    ColumnIndex++;
                    excel.Cells[rowIndex + 1, ColumnIndex] = row.ItemArray[ColumnIndex-1];
                }
            }
         
            excel.Visible = true;
            Excel.Worksheet worksheet = (Excel.Worksheet)excel.ActiveSheet;
            worksheet.Activate();
            workbook.SaveAs(aFileName, Type.Missing, Type.Missing, Type.Missing, Type.Missing, Type.Missing, Excel.XlSaveAsAccessMode.xlNoChange, Type.Missing, Type.Missing, Type.Missing, Type.Missing, Type.Missing);   
         
        }
#endif



        /// <summary>
        /// Save the this data grid to a PDF file
        /// </summary>
        /// <param name="aFileName">The filename from the save file dialog</param>
        public void ExportToPdfFile(string aFileName)
        {
            // TODO - Export to PDF
        }



        /// <summary>
        /// Save the this data grid to an HTML file
        /// </summary>
        /// <param name="aFileName">The filename from the save file dialog</param>
        public void ExportToHtmlFile(string aFileName)
        {
            // Open a file to write data from the grid
            StreamWriter theHTMLStreamWriter = null;
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                theHTMLStreamWriter = File.CreateText(aFileName);

                // 1: html -BEGIN html tag
                string rowText = "<html>";
                theHTMLStreamWriter.WriteLine(rowText);

                rowText = "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">";
                theHTMLStreamWriter.WriteLine(rowText);

                // 2: html -BEGIN  table tag
                rowText = "<table border=\"1\">";
                theHTMLStreamWriter.WriteLine(rowText);

                // 3: Write Grid-Header
                WriteColumnHeaderToHTMLFile(theHTMLStreamWriter);
                
                // 4: Write column-data for each  Row 
                WriteDataRowsToHTMLFile(theHTMLStreamWriter);

                // 5: html -END   table tag
                rowText = "</table>";

                // 6: html -END html tag
                rowText += "\n</html>";
                theHTMLStreamWriter.WriteLine(rowText);

            }
            catch (Exception e)
            {
                //  "Error in  opening or writing to file: " + aFileName + "  Exception: " + e.Message);
                string message = string.Format(Properties.Resources.ErrorOpeningWritingToFile, aFileName, e.Message);
                MessageBox.Show(Utilities.GetForegroundControl(), message, Properties.Resources.Error, MessageBoxButtons.OK);
            }
            finally
            {
                if (theHTMLStreamWriter != null)
                {
                    try
                    {
                        theHTMLStreamWriter.Close();
                    }
                    finally
                    {
                    }
                }

                Cursor.Current = Cursors.Default;

            } // end-try block
        } // ExportToHtmlFile


        /// <summary>
        /// internal  helper method to - Write Grid-Header to HTML file
        /// </summary>
        /// <param name="aHTMLStreamWriter"></param>
        private void  WriteColumnHeaderToHTMLFile(StreamWriter aHTMLStreamWriter)
        {

            // 1: begin of - Header Row 
            // <tr>  
            string rowText = "<tr>";
            aHTMLStreamWriter.WriteLine(rowText);

            // 2: Write column-headers
            // navigate thru column-headers, re-format the cells and write to the file
            // <tr>  <th>col_1</<th>  <th>col_2</<th> </tr>
            foreach (DataGridViewColumn column in this.Columns)
            {
                rowText = "<th>" + column.HeaderText + "</th>";
                aHTMLStreamWriter.WriteLine(rowText);
            }

            // 3: end of - Header row
            // </tr>  
            rowText = "</tr>";
            aHTMLStreamWriter.WriteLine(rowText);

        }



        /// <summary>
        /// internal helper method to Write each cell for each  Row 
        /// </summary>
        /// <param name="aHTMLStreamWriter"></param>
        private void WriteDataRowsToHTMLFile(StreamWriter aHTMLStreamWriter)
        {
            // 1: Write column-data for each  Row 
            // <td valign=\"top\">  column_value </td>
            // navigate thru row-by-row, the cells and write to the file
            foreach (DataGridViewRow row in this.Rows)
            {
                string rowText = "";

                // 1 : begin of - DATA Row 
                // <tr>  
                rowText = "<tr>";
                aHTMLStreamWriter.WriteLine(rowText);

                // navigate to each columns and make one-line per row to write to file
                for (int columnIndex = 0; columnIndex < this.Columns.Count; columnIndex++)
                {
                    // 2 : begin of - DATA Row 
                    DataGridViewCell cell = row.Cells[columnIndex];

                    //string theCellValue = cell.Value;
                    if (cell.Value == null)
                    {
                        rowText = "<td valign=\"top\">" + "" + "</td>";
                    }
                    else if (cell.Value is DateTime)
                    {
                        //For DateTime use the long format so we preserve the microseconds
                        rowText = "<td valign=\"top\">" + Utilities.GetTrafodionSQLLongDateTime((DateTime)cell.Value, false) +"</td>";
                    }
                    else if (cell.Value is long)
                    {
                        rowText = "<td valign=\"top\" style='mso-number-format:\"@\";'>" + FormatToHTMLString(cell.Value.ToString()) + "</td>";
                    }
                    else if (cell is DataGridViewImageCell)
                    {
                        rowText = "<td valign=\"top\">" + FormatToHTMLString(((DataGridViewImageCell)cell).Description) + "</td>";
                    }
                    else 
                    {
                        rowText = "<td valign=\"top\">" + FormatToHTMLString(cell.Value.ToString()) + "</td>";
                    }
                    aHTMLStreamWriter.WriteLine(rowText);
                }

                // 3: end of -  DATA row
                // </tr>  
                rowText = "</tr>";
                aHTMLStreamWriter.WriteLine(rowText);
            } // end-foreach on rows

        }



        /// <summary>
        /// Covert the given value from the grid-cell to HTML appropriate value
        /// </summary>
        /// <param name="aCellValue"></param>
        /// <returns>HTML string for the given string</returns>
        // e.g. <lame> tp &lt;lame&gt;  to make it HTML readable
        private string FormatToHTMLString(string aCellValue)
        {
            string htmlString = "";
            if (aCellValue.StartsWith("\0"))
                aCellValue = "";

            for (int index = 0; index < aCellValue.Length; index++)
            {
                char theChar = aCellValue[index];

                switch (theChar)
                {
                    case '<':
                    {
                        htmlString += "&lt;";
                        break;
                    }
                    case '>':
                    {
                        htmlString += "&gt;";
                        break;
                    }
                    default:
                    {
                        htmlString += theChar;
                        break;
                    }
                }
            } // for

            //Console.WriteLine("htmlString: "  + htmlString);
            return htmlString;
        }


        /// <summary>
        /// Invoke Browser e.g. default Internet Explorer with the HTML file 
        /// </summary>
        /// <param name="aHTMLFileName"></param>
        private void StartBrowserForHTMLFile(string aHTMLFileName)
        {
            // this method will be invoked from a button e.g. "Data to Browser..."  e.g. 
            // Invoke Internet Explorer by default
            // If in future: Someone may want other browser, we can query system default browser from registry.
            //
            //System.Diagnostics.Process ieProcess = new System.Diagnostics.Process();
            //ieProcess.EnableRaisingEvents = false;
            //ieProcess.StartInfo.FileName = aHTMLFileName;
            //ieProcess.Start();

            // If it contains spaces, delimit it
            if (aHTMLFileName.Contains(" "))
            {
                BrowserUtilities.LaunchURL("\"" + aHTMLFileName + "\"");
            }
            else
            {
                BrowserUtilities.LaunchURL(aHTMLFileName);
            }

            // In case, we decide that we should wait. Don't think so!
            //ieProcess.WaitForExit();

        }



        /// <summary>
        /// Start Excel-Spreadsheet displaying the data from the grid that was saved in .html format
        /// </summary>
        /// <param name="aHTMLFileName"></param>
        private void StartExcelSpreadsheetForHTMLFile(string aHTMLFileName)
        {
            // this method will be invoked from a button e.g. "Data to Spreadsheet..."
            try
            {
                System.Diagnostics.Process excelProcess = new System.Diagnostics.Process();
                excelProcess.EnableRaisingEvents = false;

                // Approach #1: Start Excel with file-name as argument
                //excelProcess.StartInfo.FileName = "excel";
                // Spaces are not liked by Excel in the file name e.g. 
                // C:\Documents and Settings\sharmay\h1.html  should be in double-Quotes
                //excelProcess.StartInfo.Arguments = "\"" + aHTMLFileName + "\"";

                // Approach 2: Execute the file with proper extension and let system invoke
                // EXcel by associating the extension -XLS with Excel
                excelProcess.StartInfo.FileName = aHTMLFileName;

                excelProcess.Start();
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error Launching Spreadsheet: " + ex.ToString());
            }
                

        }


        /// <summary>
        /// Copy the data from the datagridview to the system's  Clipboard.
        /// </summary>
        /// Future: This method may be invoked  via standard CTRL+C, if DGV handles the keyboard
        private void CopyDataToClipBoard()
        {
            // This method will be invoked from a button e.g. "Data to Clipboard..." 
            // Future: If we would like to enable copy of only selected rows or the full table
            // if (this.GetCellCount(DataGridViewElementStates.Selected) > 0)

            try
            {
                this.SelectAll();
                // Add the full selection to the clipboard.
                Clipboard.SetDataObject(this.GetClipboardContent());
                this.ClearSelection();
            }
            catch (System.Runtime.InteropServices.ExternalException)
            {
                // Exception: The Clipboard could not be accessed. 
                MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.ClipboardExceptionMessage, Properties.Resources.Error, MessageBoxButtons.OK); 

            }
        }


        /// <summary>
        /// Clear all rows and columns from the grid
        /// </summary>
        public void Clear()
        {
            Columns.Clear();
            Rows.Clear();
        }

        /// <summary>
        /// Call to get a control panel for the grid.  The control
        /// is wired to the grid and requires nothing from the caller other than a place to "mount" it.
        /// </summary>
        /// <returns>The control</returns>
        public Control GetButtonControl()
        {
            return new DataGridViewButtonsUserControl(this);
        }

        /// <summary>
        /// Creates a panel with a count of the number of entries in the grid and docks it in the parent.
        /// </summary>
        /// <param name="aFormat">A text format string</param>
        /// <param name="aDockStyle">Where to dock the panel</param>
        /// <returns>True if the panel was created and docked</returns>
        public bool AddCountControlToParent(string aFormat, DockStyle aDockStyle)
        {
            try
            {
                TrafodionPanel thePanel = new TrafodionPanel();

                thePanel.Height = 20;

                _theCountControl = GetCountControl(aFormat);
                _theCountControl.Dock = DockStyle.Fill;
                thePanel.Controls.Add(_theCountControl);

                thePanel.Dock = aDockStyle;
                Parent.Controls.Add(thePanel);

                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// <summary>
        /// The format of the count message can be updated
        /// </summary>
        /// <param name="aMessage"></param>
        public void SetCountMessage(String aMessage)
        {
            if (_theCountControl != null)
            {
                ((DataGridViewCountUserControl)_theCountControl).TheFormat = aMessage;
            }
        }
        /// <summary>
        /// Creates a control panel for the grid and docks it in the parent.
        /// </summary>
        /// <param name="aDockStyle">Where to dock the panel</param>
        /// <returns>True if the panel was created and docked</returns>
        public bool AddButtonControlToParent(DockStyle aDockStyle)
        {
            try
            {
                TrafodionPanel thePanel = new TrafodionPanel();

                thePanel.Height = 40;

                Control theButtonControl = GetButtonControl();
                theButtonControl.Dock = aDockStyle;
                thePanel.Controls.Add(theButtonControl);

                thePanel.Dock = aDockStyle;
                Parent.Controls.Add(thePanel);

                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// <summary>
        /// Sorts the datagrid using the selected column
        /// </summary>
        public void Sort()
        {
            DataGridViewColumn sortColumn = SortedColumn;
            if (SortOrder == SortOrder.None || sortColumn == null)
            {
                // No column select as the sort column. Do nothing.
                return;
            }

            if (SortOrder == SortOrder.Ascending)
                Sort(sortColumn, ListSortDirection.Ascending);
            else if (SortOrder == SortOrder.Descending)
                Sort(sortColumn, ListSortDirection.Descending);
        }

        /// <summary>
        /// Called when the "Export" button on the control panel is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void theExportToFileButton_Click(object sender, EventArgs e)
        {
            ExportToFile();
        }

        /// <summary>
        /// Creates a panel with a count of the number of entries in the grid.
        /// </summary>
        /// <param name="aFormat">A text format string</param>
        /// <returns>The control</returns>
        public Control GetCountControl(string aFormat)
        {
            return new DataGridViewCountUserControl(this, aFormat);
        }

        /// <summary>
        /// proeprty for header to print on every page if PDF selected
        /// </summary>
        public string TheHeaderText
        {
            get { return theHeaderText; }
            set { theHeaderText = value; }
        }

        /// <summary>
        /// Returns the helpprovider assigned to TrafodionDataGridView
        /// This allows the derived class to change help properties if needed.  
        /// </summary>
        public HelpProvider HelpProviderTrafodionDataGrid
        {
            get { return _helpProviderTrafodionDataGrid; }
        
        }
        
        /// <summary>
        /// Help topic
        /// </summary>
        virtual public String HelpTopic 
        {
            get { return null; }
        
        }

 
        
    }

}
