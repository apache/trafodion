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
using System.ComponentModel;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using TenTec.Windows.iGridLib;


//Ported from NPA SQ_NPAT_R1.0 
//This control is used to output explain plan data in a presentable way. It contains a treeview,a grid
//and a richtextbox.  This control plugs into the queryinfo widget. 
//The tree contains plan operators in a tree view.  The grid displays explain plan items and the
//richtextbox show plan description ordered to be more readable. 
//[NOTE] This part is mainly GUI details. We want to maintain pretty much the same code as NPA. 
namespace Trafodion.Manager.DatabaseArea.NCC
{
    /// <summary>
    /// Query plan control
    /// </summary>
	public partial class NCCQueryPlanControl : UserControl
    {
        #region Fields

        private static String _SpacePadding = "";
		private EventHandler _onClickHandler = null;
		private TreeViewNodeSorterComparator _sortComparator = new TreeViewNodeSorterComparator();
		private static int _maxWidth = 1024;
		private ToolTip _toolTips;

		private NCCWorkbenchQueryData _wbqd = null;

		private Color [] _boundaryColors;
		private Color[]  _specialColors;

		private int _treeViewControlWidth = 0;

        private const String TRACE_SUB_AREA_NAME = "Query Plan";


		#region Indexing defines for retrieving images from the imageindex

		private const int UNSPECIFIED_ICON = 0;

		private const int USER_DEFINED_ROUNTINE_ICON = UNSPECIFIED_ICON;
		private const int DAM_SUBSET_ICON = 1;
		private const int DAM_UNIQUE_ICON = 2;
		private const int DATA_MINING_ICON = 3;
		private const int ESP_EXCHANGE_BLUE_ICON = 4;
		private const int ESP_EXCHANGE_GREEN_ICON = 5;
		private const int ESP_EXCHANGE_RED_ICON = 6;
		private const int PARTITION_ACCESS_ICON = 7;
		private const int SPLIT_TOP_ICON = 8;
		private const int GROUPBY_ICON = 9;
		private const int INSERT_ICON = 10;
		private const int JOIN_ICON = 11;
		private const int MATERIALIZE_ICON = UNSPECIFIED_ICON;
		private const int MERGE_UNION_ICON = 12;
		private const int ROOT_ICON = 13;
		private const int ROWSET_ICON = UNSPECIFIED_ICON;
		private const int SORT_ICON = 14;
		private const int STORED_FUNCTION_ICON = UNSPECIFIED_ICON;
		private const int TUPLE_ICON = 15;

		[DllImport("user32.dll")]
		extern static bool ShowScrollBar(IntPtr hWnd, int wBar, bool bShow);

		[DllImport("user32.dll")]
		extern static bool EnableScrollBar(IntPtr hWnd, uint wSBflags, uint wArrows);

		/*
		 * Scrollbar constants from WinUser.h header file.
		 */
		private const int ESB_ENABLE_BOTH = 0x0000;
		private const int ESB_DISABLE_BOTH = 0x0003;

		private const int SB_HORZ = 0;
		private const int SB_VERT = 1;
		private const int SB_CTL = 2;
		private const int SB_BOTH = 3;


		enum _operatorIconIndex
		{   //User define group
			CALL = USER_DEFINED_ROUNTINE_ICON,
			
			//DAM subset group
			FILE_SCAN = DAM_SUBSET_ICON,
			INDEX_SCAN = DAM_SUBSET_ICON,
			SUBSET_DELETE = DAM_SUBSET_ICON,
			SUBSET_UPDATE = DAM_SUBSET_ICON,
			
			//DAM Unique group
			CURSOR_DELETE = DAM_UNIQUE_ICON,
			CURSOR_UPDATE = DAM_UNIQUE_ICON,
			FILE_SCAN_UNIQUE = DAM_UNIQUE_ICON,
			INDEX_SCAN_UNIQUE = DAM_UNIQUE_ICON,
			UNIQUE_DELETE = DAM_UNIQUE_ICON,
			UNIQUE_UPDATE = DAM_UNIQUE_ICON,
			
			//Data Mining group
			SAMPLE = DATA_MINING_ICON,
			SEQUENCE = DATA_MINING_ICON,
			TRANSPOSE = DATA_MINING_ICON,
			
			//Exchange group
			ESP_EXCHANGE,
			PARTITION_ACCESS = PARTITION_ACCESS_ICON,
			SPLIT_TOP = SPLIT_TOP_ICON,
			
			//Groupby group
			HASH_GROUPBY = GROUPBY_ICON,
			HASH_PARTIAL_GROUPBY_LEAF = GROUPBY_ICON,
			HASH_PARTIAL_GROUPBY_ROOT = GROUPBY_ICON,
			SHORTCUT_SCALAR_AGRR = GROUPBY_ICON,
			SORT_GROUPBY = GROUPBY_ICON,
			SORT_PARTIAL_AGGR_LEAF = GROUPBY_ICON,
			SORT_PARTIAL_AGGR_ROOT = GROUPBY_ICON,
			SORT_PARTIAL_GROUPBY_LEAF = GROUPBY_ICON,
			SORT_PARTIAL_GROUPBY_ROOT = GROUPBY_ICON,
			SORT_SCALAR_AGGR = GROUPBY_ICON,
			
			//Insert group
			INSERT = INSERT_ICON,
			INSERT_VSBB = INSERT_ICON,
			
			//Join group
			HYBRID_HASH_ANTI_SEMI_JOIN = JOIN_ICON,
			HYBRID_HASH_JOIN = JOIN_ICON,
			HYBRID_HASH_SEMI_JOIN = JOIN_ICON,
			LEFT_HYBRID_HASH_JOIN = JOIN_ICON,
			LEFT_MERGE_JOIN = JOIN_ICON,
			LEFT_NESTED_JOIN = JOIN_ICON,
			LEFT_ORDERED_HASH_JOIN = JOIN_ICON,
			MERGE_ANTI_SEMI_JOIN = JOIN_ICON,
			MERGE_JOIN = JOIN_ICON,
			MERGE_SEMI_JOIN = JOIN_ICON,
			NESTED_ANTI_SEMI_JOIN = JOIN_ICON,
			NESTED_JOIN = JOIN_ICON,
			NESTED_SEMI_JOIN = JOIN_ICON,
			ORDERED_HASH_ANTI_SEMI_JOIN = JOIN_ICON,
			ORDERED_HASH_JOIN = JOIN_ICON,
			ORDERED_HASH_SEMI_JOIN = JOIN_ICON,
			TUPLE_FLOW = JOIN_ICON,
			
			//Materialize group
			MATERIALIZE = MATERIALIZE_ICON,
			
			//Merge Union group
			MERGE_UNION = MERGE_UNION_ICON,
			
			//Root group
			ROOT = ROOT_ICON,
			
			//Rowset group
			PACK = ROWSET_ICON,
			UNPACK = ROWSET_ICON,
			
			//Sort group
			SORT = SORT_ICON,
			
			//Explain group
			EXPLAIN = STORED_FUNCTION_ICON,
			
			//Tuple group
			EXPR = TUPLE_ICON,
			TUPLELIST = TUPLE_ICON,
			VALUES = TUPLE_ICON
		}
		#endregion

        /// <summary>
        /// Static members
        /// </summary>
        static NCCQueryPlanControl()
		{

			for (int idx = 0; idx < Screen.AllScreens.Length; idx++)
			{
				if (Screen.AllScreens[idx].Bounds.Width > _maxWidth)
					_maxWidth = Screen.AllScreens[idx].Bounds.Width;
			}

			char[] padArray = new char[_maxWidth];
			for (int i = 0; i < padArray.Length; i++)
				padArray[i] = ' ';

			_SpacePadding = new String(padArray);

        }

        #endregion Fields

        #region Constructor

        public NCCQueryPlanControl()
		{
			InitializeComponent();
			
			InitProcessBoundaryColors();
			setupQueryPlanIGridNumberFormatStyles();
            
            // Do now allow column filtering, which will mess up the display. 
            nccQueryPlanIGrid.AllowColumnFilter = false;
            nccQueryPlanIGrid.HelpTopic = HelpTopics.ExplainGridView;

			nccQueryPlanTreeView.TreeViewNodeSorter = null;
			nccQueryPlanTreeView.Height = Screen.PrimaryScreen.Bounds.Height + 2000;  // move past the screen size!!
			nccQueryPlanTreeView.ExpandAll();

			setTreeViewControlWidth(nccQueryPlanTreePanel.Width);

			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
			EnableScrollBar(nccQueryPlanTreePanel.Handle, SB_HORZ, ESB_DISABLE_BOTH);

			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_HORZ, true);
			EnableScrollBar(nccQueryPlanTreePanel.Handle, SB_HORZ, ESB_ENABLE_BOTH);

			_toolTips = new ToolTip();
			_toolTips.InitialDelay = 50;
			//helpProvider1.HelpNamespace = NCCRegistryKey.getInstallDirectory() + "\\" + 
			//							  helpProvider1.HelpNamespace;
        }

        #endregion Constructor

        private void setTreeViewControlWidth(int theWidth) {
			this._treeViewControlWidth = Math.Max(theWidth, nccQueryPlanTreePanel.Width);
			nccQueryPlanTreeView.Width = this._treeViewControlWidth;
			nccQueryPlanOperatorHeadingPanel.Width = this._treeViewControlWidth;

			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}


		private void InitProcessBoundaryColors()
		{
			int alpha = 255;
			
			_boundaryColors = new Color[] { 
    				 Color.FromArgb(alpha, 177,225,225),  /*Turquise*/
    				 Color.FromArgb(alpha, 101,196,193),
    				 Color.FromArgb(alpha, 65,169,167),
    				 Color.FromArgb(alpha, 51,132,130),
    				 Color.FromArgb(alpha, 153,189,219), /*Ocean Blue*/
    				 Color.FromArgb(alpha, 107,160,203),  
    				 Color.FromArgb(alpha, 66,132,185),
    				 Color.FromArgb(alpha, 55,108,153),
    				 Color.FromArgb(alpha, 193,181,210), /*Light Magenta*/
    				 Color.FromArgb(alpha, 166,147,190),
    				 Color.FromArgb(alpha, 142,121,171),
    				 Color.FromArgb(alpha, 126,99,163),
    				 Color.FromArgb(alpha, 219,191,192), /*Soft Magenta/Tan*/ 
    				 Color.FromArgb(alpha, 201,158,158),
    				 Color.FromArgb(alpha, 185,134,135),
    				 Color.FromArgb(alpha, 164,98,100),
    				 Color.FromArgb(alpha, 189,211,184), /*Earth Green*/
    				 Color.FromArgb(alpha, 154,188,146),
    				 Color.FromArgb(alpha, 112,159,100),
    				 Color.FromArgb(alpha, 86,125,77),
    				 Color.FromArgb(alpha, 190,193,240), /*Light Blue*/
    				 Color.FromArgb(alpha, 154,160,232),
    				 Color.FromArgb(alpha, 118,125,222),
    				 Color.FromArgb(alpha, 78,88,214),
    				 Color.FromArgb(alpha, 250,211,219), /*Salmon*/
    				 Color.FromArgb(alpha, 243,148,167),
    				 Color.FromArgb(alpha, 236,83,114),
    				 Color.FromArgb(alpha, 181,30,91),
    				 Color.FromArgb(alpha, 226,169,169), /*Light Brown*/
    				 Color.FromArgb(alpha, 211,124,124),
    				 Color.FromArgb(alpha, 200,89,89),
    				 Color.FromArgb(alpha, 169,56,56),
    				 Color.FromArgb(alpha, 255,224,193), /*Soft Orange*/
    				 Color.FromArgb(alpha, 255,193,132),
    				 Color.FromArgb(alpha, 255,168,81),
    				 Color.FromArgb(alpha, 255,135,15),
    				 Color.FromArgb(alpha, 223,112,0),
    				 Color.FromArgb(alpha, 213,213,0),   /*Soft Lime*/
    				 Color.FromArgb(alpha, 176,176,0),
    				 Color.FromArgb(alpha, 147,147,0) };

			
			_specialColors = new Color[] { Color.FromArgb(alpha, 255, 255, 175),
										   Color.FromArgb(alpha, 235, 240, 248) };

		}


		private void setQueryPlanIGridColCellStyle(String colName, iGCellStyleDesign theStyle) {
			try 
            {
				this.nccQueryPlanIGrid.Cols[colName].CellStyle = theStyle;
                this.nccQueryPlanIGrid.Header.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
                this.nccQueryPlanIGrid.Header.ForeColor = Color.FromKnownColor(KnownColor.ControlText);
			} 
            catch (Exception) 
            { }

		}


		private void setupQueryPlanIGridNumberFormatStyles() {
			this.rowsOutCellStyleDesign.FormatString = NCCUtils.GetNumberFormatForCurrentLocale(0);
			this.iGCellStyleDesign1.FormatString = NCCUtils.GetLocaleNumberFormat(2, true);

			setQueryPlanIGridColCellStyle("Sequence",     this.rowsOutCellStyleDesign);
			setQueryPlanIGridColCellStyle("TotalCost",       this.iGCellStyleDesign1);
			setQueryPlanIGridColCellStyle("OperatorCost",    this.iGCellStyleDesign1);
			setQueryPlanIGridColCellStyle("RowsOut",      this.rowsOutCellStyleDesign);
			setQueryPlanIGridColCellStyle("LeftChild",    this.rowsOutCellStyleDesign);
			setQueryPlanIGridColCellStyle("RightChild",   this.rowsOutCellStyleDesign);
		}

			
		//This code is used to maintain tree highlight color when it looses focus.
		//It is turned off and on throughout the this code because it uses tons of cycles.
		private void nccQueryPlanTreeView_DrawNode(object sender, DrawTreeNodeEventArgs e)
		{

				
			Font f = e.Node.NodeFont != null ? e.Node.NodeFont : e.Node.TreeView.Font;
			Size sz = TextRenderer.MeasureText(e.Node.Text, f);
			Rectangle rc = new Rectangle(e.Bounds.X, e.Bounds.Y -1, _maxWidth + 500, e.Bounds.Height);

			Color fore = e.Node.ForeColor;
			if (fore == Color.Empty) 
				fore = e.Node.TreeView.ForeColor;
						
			if (e.Node == e.Node.TreeView.SelectedNode)
			{
			     fore = SystemColors.HighlightText;
			    //For testing.  Change color of text if in focus.
				//if ((e.State & TreeNodeStates.Focused) != 0) fore = Color.Yellow;
			}
			Color back = e.Node.BackColor;
			if (back == Color.Empty) 
				back = e.Node.TreeView.BackColor;
			
			if (e.Node == e.Node.TreeView.SelectedNode)
				back = SystemColors.Highlight;
			SolidBrush bbr = new SolidBrush(back);
			e.Graphics.FillRectangle(bbr, rc);
			TextRenderer.DrawText(e.Graphics, e.Node.Text, f, rc, fore, TextFormatFlags.VerticalCenter);
			bbr.Dispose();
			f.Dispose();
    	}

		public Color ABackColor
		{
			get { return nccQueryPlanTreeView.BackColor; }
			set { nccQueryPlanTreeView.BackColor = value; }
		}

		public EventHandler OnClickHandler
		{
			get { return _onClickHandler; }
			set { _onClickHandler = value; }
		}

		public void Clear()
		{
			nccQueryPlanIGrid.Rows.Clear();
			nccQueryPlanTreeView.Nodes.Clear();
			nccQueryExplainPlanRichTextBox.Clear();
			this._wbqd = null;
		}

		public bool PlanGridFocused
		{
			get { return nccQueryPlanIGrid.Focused; } 
		}

		public bool PlanSummaryFocused
		{
			get { return nccQueryExplainPlanRichTextBox.Focused; }
		}

		public void selectAllQueryPlanGrid()
		{
			nccQueryPlanIGrid.Focus();
			//Temporarily unsubscribe to query grid select event to prevent updates			
			nccQueryPlanIGrid.SelectionChanged -= new System.EventHandler(this.nccQueryPlanIGrid_SelectionChanged);
			
			nccQueryPlanIGrid.SelectionMode = iGSelectionMode.MultiExtended;

			if (0 < nccQueryPlanIGrid.Rows.Count) {
				foreach (iGRow row in nccQueryPlanIGrid.Rows) {
					foreach (iGCell cell in row.Cells)
						cell.Selected = true;
				}
			}

			nccQueryPlanIGrid.SelectionChanged += new System.EventHandler(this.nccQueryPlanIGrid_SelectionChanged);


		}

		public void selectAllPlanSummary()
		{
			nccQueryExplainPlanRichTextBox.Focus();
			nccQueryExplainPlanRichTextBox.SelectAll();

		}


		/// <summary>
		/// Output the summary to the Operator Details window
		/// </summary>
		/// <param name="NCCWorkbenchQueryData object"></param>
		public void ShowSummaryForCurrentQuery(object qdArg)
		{
			nccQueryExplainPlanRichTextBox.Clear();  //My turn for this view

			NCCWorkbenchQueryData wbqd = null;

			try
			{
				wbqd = (NCCWorkbenchQueryData)qdArg;
			}
			catch (Exception)
			{
				return;
			}

			//Sometimes there is a glitch and the summary info does not get loaded. 
			//So just return so that there is no anoying error to user.
			if (wbqd.planSummaryInfo == null)
				return;


			Font nameFont = new Font("Arial", 8.25F, FontStyle.Regular);
            Font boldNameFont = new Font("Tahoma", 8.25F, FontStyle.Bold);
			Font valueFont = new Font("Trebuchet MS", 8.25F, FontStyle.Regular);
			Color regColor = Color.FromKnownColor(KnownColor.ControlText);
			Color nameColor = Color.FromKnownColor(KnownColor.ForestGreen);
            Color SectionColor = Color.FromKnownColor(KnownColor.ControlText);

            nccQueryExplainPlanRichTextBox.AppendText("Explain Plan Summary" + Environment.NewLine + Environment.NewLine);
			int scIndex = nccQueryExplainPlanRichTextBox.Text.IndexOf("Explain Plan Summary");
			if (-1 != scIndex)
			{
				colorizeExplainPlanDetails(boldNameFont, SectionColor, scIndex, "Explain Plan Summary".Length);
			}

			nccQueryExplainPlanRichTextBox.AppendText(" ESP EXCHANGES: ");
			nccQueryExplainPlanRichTextBox.AppendText(wbqd.planSummaryInfo.totalEspExchanges + Environment.NewLine);

			nccQueryExplainPlanRichTextBox.AppendText(" ESP PROCESSES: ");
			nccQueryExplainPlanRichTextBox.AppendText(wbqd.planSummaryInfo.totalChildProcesses + Environment.NewLine);

			nccQueryExplainPlanRichTextBox.AppendText(" HASH JOINS:    ");
			nccQueryExplainPlanRichTextBox.AppendText(wbqd.planSummaryInfo.totalHashJoins + Environment.NewLine);

			nccQueryExplainPlanRichTextBox.AppendText(" MERGE JOINS:   ");
			nccQueryExplainPlanRichTextBox.AppendText(wbqd.planSummaryInfo.totalMergeJoins + Environment.NewLine);

			nccQueryExplainPlanRichTextBox.AppendText(" NESTED JOINS:  ");
			nccQueryExplainPlanRichTextBox.AppendText(wbqd.planSummaryInfo.totalNestedJoins + Environment.NewLine);

			nccQueryExplainPlanRichTextBox.AppendText(" TOTAL JOINS:   ");
			nccQueryExplainPlanRichTextBox.AppendText(wbqd.planSummaryInfo.totalOverallJoins + Environment.NewLine);

			nccQueryExplainPlanRichTextBox.AppendText(" SCANS: ");
			nccQueryExplainPlanRichTextBox.AppendText(wbqd.planSummaryInfo.totalScans + Environment.NewLine);

			nccQueryExplainPlanRichTextBox.AppendText(" SORTS: ");
			nccQueryExplainPlanRichTextBox.AppendText(wbqd.planSummaryInfo.totalSorts + Environment.NewLine);

			nccQueryExplainPlanRichTextBox.AppendText(" UIDs: ");
			nccQueryExplainPlanRichTextBox.AppendText(wbqd.planSummaryInfo.totalIUDs + Environment.NewLine);

			if ((TrafodionContext.Instance.CurrentConnectionDefinition != null &&
                 TrafodionContext.Instance.CurrentConnectionDefinition.IsServicesUser) &&
                (null != wbqd.controlQueryShape) &&  
			    (0 < wbqd.controlQueryShape.Trim().Length) ) 
            {
				String cqShapeTitle = "Query Shape";

				nccQueryExplainPlanRichTextBox.AppendText(Environment.NewLine + Environment.NewLine);
                nccQueryExplainPlanRichTextBox.AppendText(cqShapeTitle + Environment.NewLine + Environment.NewLine);

				int sectionIdx = nccQueryExplainPlanRichTextBox.Text.IndexOf(cqShapeTitle);
				if (-1 != sectionIdx)
					colorizeExplainPlanDetails(boldNameFont, SectionColor, sectionIdx, cqShapeTitle.Length);

				nccQueryExplainPlanRichTextBox.AppendText(wbqd.controlQueryShape + Environment.NewLine);
			}


			String[] textContainer = nccQueryExplainPlanRichTextBox.Lines;


			//Colorize
			try
			{
				for (int i = 1; i < textContainer.Length; i++)
				{
					try
					{
						int colonIndex = textContainer[i].IndexOf(":");
						int index = nccQueryExplainPlanRichTextBox.Text.IndexOf(textContainer[i]);
						if (-1 != index)
						{
							colorizeExplainPlanDetails(nameFont, nameColor, index, colonIndex);

						}
					}
					catch (Exception)
					{
						continue;
					}
				}
			}
			catch (Exception)
			{
			}

		}

		private void copyIGridContentsToClipboard(iGrid igc, bool selectedCellsOnly)
		{
			if (null == igc)
				return;

            if (0 == igc.SelectedRows.Count)
			{
				if (0 == igc.Rows.Count)
					return;

				return;
			}

			StringBuilder headersSB = new StringBuilder();
			StringBuilder rowsSB = new StringBuilder();
			int nRows = igc.Rows.Count;
			int nCols = igc.Cols.Count;

			int i = 0;
			for (i = 0; i < nCols; i++) {
				if (0 < i)
					headersSB.Append('\t');

				headersSB.Append(igc.Cols[i].Text);
			}


			for (i = 0; i < igc.Rows.Count; i++)
			{
				if (selectedCellsOnly && (false == igc.Cells[i, 0].Selected))
					continue;

				for (int j = 0; j < nCols; j++)
				{
					if (0 < j)
						rowsSB.Append('\t');

					rowsSB.Append(igc.Cells[i, j].Text);
				}

				rowsSB.Append(Environment.NewLine);
				if (igc.RowTextVisible)
					rowsSB.Append(igc.Rows[i].RowTextCell.Value).Append(Environment.NewLine);

			}

			String copiedData = "";
			if (0 < rowsSB.Length)
				copiedData = headersSB.ToString() + Environment.NewLine + rowsSB.ToString();

			Clipboard.SetDataObject(copiedData, true);
		}

		public void CopyCurrentSelectionToClipboard()
		{
			if (nccQueryPlanIGrid.Focused)
				copyIGridContentsToClipboard(nccQueryPlanIGrid, true);
			else if (nccQueryExplainPlanRichTextBox.Focused)
				nccQueryExplainPlanRichTextBox.Copy();	
		}


		// kludge for sorting by levels and colorizing tree on option changes as well as 
		// on clicks to the headings of the Query Plan Tree View panel heading.
		public void ReloadPlanInformation() {
			if (null == this._wbqd)
				return;

			int num2search = -1;
			try {
				TreeNode tN = nccQueryPlanTreeView.SelectedNode;
				iGRow row = (iGRow)tN.Tag;
				num2search = (int) row.Cells["Sequence"].Value;
			} catch (Exception) {
				num2search = -1;
			}

			iGSortObject gridSortObj = nccQueryPlanIGrid.SortObject;
			if (null != gridSortObj)
				gridSortObj.Clear();

			LoadTreeGrid(this._wbqd);

			try {
				foreach (iGRow row in nccQueryPlanIGrid.Rows) {
					int seqNum = (int) row.Cells["Sequence"].Value;
					if (num2search == seqNum) {
						TreeNode tN = (TreeNode)row.Tag;
						nccQueryPlanTreeView.SelectedNode = tN;
					}
				}
			} catch (Exception) {
			}
		}

        /// <summary>
        /// This loads the tree and the grid given a Workbenchdata object
        /// </summary>
        /// <param name="qdArg"></param>
		public void LoadTreeGrid(Object qdArg)
		{
			NCCWorkbenchQueryData wbqd = null;

			Clear();

			try
			{
				wbqd = (NCCWorkbenchQueryData)qdArg;
				_wbqd = wbqd;
			}
			catch (Exception)
			{
				return;
			}


            if (!String.IsNullOrEmpty(wbqd.planErrorString))
			{
				nccQueryPlanIGrid.RowTextCol.CellStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;
				iGRow row = nccQueryPlanIGrid.Rows.Add();
                row.RowTextCell.Value = wbqd.planErrorString;
				row.Tag = null;
				nccQueryPlanIGrid.RowTextVisible = true;
				row.AutoHeight();
			}
			else
			{
				Hashtable tempht = new Hashtable(wbqd.planStepsHT);
				//This code outputs the plan data to the plan grid

				try {
					buildTreeGrid(tempht, 0, wbqd.rootPlan, null);

				}
				catch (Exception e) {
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCC::LoadTreeGrid(): Got an error in " + e.Message);
                    }

				}
			}

			//Unsubscribe to some events when tree is loading.  This speeds up performance because it reduces unnessary 
			//function calls that these events do.  I also prevents tree to gain focus which the events cause to happen.
 			nccQueryPlanTreeView.AfterExpand -= new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterExpand);
			nccQueryPlanIGrid.SelectionChanged -= new System.EventHandler(this.nccQueryPlanIGrid_SelectionChanged);
			nccQueryPlanTreeView.ExpandAll();
			//default select the first row in  grid... Tree is already zero selected at this point
			//makes it look better
			//nccQueryPlanIGrid.SetCurRow(0);
			syncUpTreeViewAndIGridScrollbars();

			setTreeViewControlWidth(calculatePlanTreeWidth());

			nccQueryPlanTreeView.AfterExpand += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterExpand);
			nccQueryPlanIGrid.SelectionChanged += new System.EventHandler(this.nccQueryPlanIGrid_SelectionChanged);
			ShowSummaryForCurrentQuery(wbqd);

            //to enable the SQL Whiteboard options
            if (DatabaseAreaOptions.GetOptions().EnableSortExplainGirdByLevels)
				nccQueryPlanIGrid.SortByLevels = true;
			else
				nccQueryPlanIGrid.SortByLevels = false;

            if (DatabaseAreaOptions.GetOptions().EnableExplainColorProcessBoundaries)
            {
				try 
                {
					ColorizeTree();
				} 
                catch (Exception exc) 
                {
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCC::Warning on colorizing explain plan tree. Details = " + exc.Message);
                    }
				}
			}
		}
		
        /// <summary>
        /// Take the plan values and put them into row cells
        /// </summary>
        /// <param name="row"></param>
        /// <param name="qd"></param>
		private void SetPlanValuesToRow(iGRow row, NCCWorkbenchQueryData.QueryPlanData qd)
		{

			row.Cells["Operator"].Value = qd.theOperator;
			row.Cells["OperatorName"].Value = qd.theOperator;
			row.Cells["TotalCost"].Value = getDoubleValue(qd.totalCost);
			row.Cells["OperatorCost"].Value = getDoubleValue(qd.operatorCost);
			row.Cells["RowsOut"].Value = getDoubleValue(qd.cardinality);
			row.Cells["TableName"].Value = qd.tableName;
			row.Cells["DetailCost"].Value = qd.detailCost;
			row.Cells["Description"].Value = qd.description;
			row.Cells["Sequence"].Value = getIntValue(qd.sequenceNumber);
			row.Cells["LeftChild"].Value = qd.leftChildSeqNum;
			row.Cells["RightChild"].Value = qd.rightChildSeqNum;
		}
				
        /// <summary>
        /// Given an operator name, return the index to access the icon in the imagelist1
        /// </summary>
        /// <param name="operatorName"></param>
        /// <returns></returns>
		private int GetImageIcon(String operatorName)
		{

			_operatorIconIndex index;
			try
			{
				index = (_operatorIconIndex)Enum.Parse(typeof(_operatorIconIndex), operatorName, true);
			}
			catch (ArgumentException)
			{
				return UNSPECIFIED_ICON;
			}
			catch (Exception)
			{
				return UNSPECIFIED_ICON;
			}

			return (int)index;

		}


		/// <summary>
		/// Retrive the description value of a given description
		/// </summary>
		/// <param name="Description string"></param>
		/// <returns>String</returns>
		private String GetOperatorDescriptionItem(String des, String desText)
		{

			desText += "terminator:";
			try
			{
				if (!desText.Contains(des))
					return "";

				string regex = @des + @"\s*(?<Output>[A-Za-z_0-9\(\)\$\.\=\s]+)\s.+?:";
				RegexOptions options = RegexOptions.Singleline;
				String input = @desText;
				MatchCollection matches = Regex.Matches(input, regex, options);
				return matches[0].Groups["Output"].Value;
				
			}
			catch(Exception ex) {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCC::Exception during parsing :" + ex.Message);
                }

				return "";
			}
		
		}

		/// <summary>
		/// Retrieves more data describing the operator like the acronym and better tooltip
		/// It is used to help display descriptive items next to operators in the treeview
		/// </summary>
		/// <param name="des"></param>
		/// <param name="destext"></param>
		/// <param name="acronym"></param>
		/// <param name="toolTip"></param>
		private void GetOperatorInfo(NCCWorkbenchQueryData.QueryPlanData qd, out String acronym, out String toolTip)
		{

			acronym = "";
			toolTip = "    ";
			String child_procs = "";
			String parent_procs = "";
			String proc_desc = "";
			

			//This is put in to mimic what VQP does.  These use to be called top and bottom_degree_parallelism. 
			child_procs = GetOperatorDescriptionItem("child_processes:", qd.description); 
			parent_procs = GetOperatorDescriptionItem("parent_processes:", qd.description);
			if(parent_procs.Length > 0 && child_procs.Length > 0)
				proc_desc = child_procs + "->" + parent_procs; 


			if (qd.theOperator.Contains("HYBRID_HASH_JOIN"))
			{
				acronym = GetOperatorDescriptionItem("parallel_join_type:", qd.description);
				if (acronym == "1")
				{
					acronym = "  (MP)";
					toolTip = "  Matched Partition";
					return;
				}
				else if (acronym == "2")
				{
					acronym = "  (NMP)";
					toolTip = "  Non Matching Partition";
					return;
				}
				
			}

			if (qd.theOperator.Contains("ESP_EXCHANGE"))
			{		
				acronym = GetOperatorDescriptionItem("parent_partitioning_function:", qd.description);
				if (acronym.Contains("broadcast"))
				{
					acronym = "  (BR) " + proc_desc;
					toolTip = " Broadcast Partitioned";
					return;
				}
				else if (acronym.Contains("hash2") & !acronym.Contains("randomNum"))  
				{
					toolTip = " Hash Partitioned" ;
					acronym = "  (HP) " + proc_desc;
					
					return;
				}
				else if (acronym.Contains("range"))
				{
					toolTip = "  Range Partitioned";
					acronym = "  (RP) " + proc_desc;

					return;
				}
				else if (acronym.Contains("randomNum"))
				{
					toolTip = "  Round-Robin Partitioned";
					acronym = "  (RR) " + proc_desc;
					return;
				}

				acronym = "  " + proc_desc;
				return;
			}

			if (qd.theOperator.Contains("SPLIT_TOP"))
			{
				acronym = "  " + proc_desc;
			}
		}

        /// <summary>
        /// Colorizes the process boundaries in the operator tree
        /// </summary>
		private void ColorizeTree()
		{
			if (_wbqd == null)
				return;

			//int result;
			//int espCount = Int32.Parse(_wbqd.planSummaryInfo.totalEspExchanges);
			//int colorIncrement = Math.DivRem(256,espCount,out result);

			String descriptionDetails = "";
			String operatorName = "";
			
			Hashtable colorHt = new Hashtable();

			// int alpha = 50;
			// int red = 0;
			// int green = 255;
			// int blue = 0;
			
			// colorHt.Add("yellow", Color.FromArgb(alpha, 255,235,0));
			colorHt.Add("DAM", _specialColors[0]);
			colorHt.Add("MASTER", _specialColors[1]);

			//Store the color values for each fragment id to mark the process boundaries.
			int colorIndex = 0;
			foreach (iGRow iRow in nccQueryPlanIGrid.Rows)
			{

				descriptionDetails = (String)iRow.Cells["Description"].Value;
				String id = GetOperatorDescriptionItem("fragment_id:", descriptionDetails);
				String type = GetOperatorDescriptionItem("fragment_type:", descriptionDetails).Trim().ToLower();
				operatorName = (String)iRow.Cells["Operator"].Value;
				//if (operatorName != "ROOT" && !operatorName.Contains("FILE") && !operatorName.Contains("PARTITION") && !operatorName.Contains("INSERT"))
				//When the fragment type is an esp then store a color
				if(type == "esp")
				{
					//Color boundaryColor = Color.FromArgb(alpha, red, green, blue);
					//green -= 20;

					if (!colorHt.Contains(id) && id != "")
					{
						colorHt.Add(id, _boundaryColors[colorIndex]);
						colorIndex++;
					}

					
				}

				try
				{
					int test = _boundaryColors.GetLength(0);
				}
				catch(Exception ex) {
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCCQueryPlanControl::ColorizeTree():" + ex.Message);
                    }
				}

				////We ran out of colors, reset to the beginning
				if (colorIndex >= _boundaryColors.GetLength(0))
				    colorIndex = 0;

			}
			
			//Now colorize tree
			ProcessBoundaryColors(nccQueryPlanTreeView.Nodes, colorHt);

		}


		private void ChangeDataRowBackColor(iGRow row, Color c) {
			try {
				int len = row.Cells.Count;
				for (int idx = 0; idx < len; idx++)
					row.Cells[idx].BackColor = c;
			} catch (Exception) {
			}
		}

		private void ProcessBoundaryColors(TreeNodeCollection nodes, Hashtable colorHt)
		{

				foreach (TreeNode tn in nodes)
				{

					iGRow row = (iGRow)tn.Tag;
					String opStr = tn.Text.Trim();
					//Use the data from the grid to get the fragment id;
					String descriptionDetails = (String)row.Cells["Description"].Value;
					String id = GetOperatorDescriptionItem("fragment_id:", descriptionDetails);
					String type = GetOperatorDescriptionItem("fragment_type:", descriptionDetails).Trim().ToLower();
					
					//if (opStr.Contains("FILE") || opStr.Contains("PARTITION") || opStr.Contains("INSERT")) 
					String colorID = null;

					if (type.Equals("dam", StringComparison.CurrentCultureIgnoreCase)  || 
					    type.Equals("dp2", StringComparison.CurrentCultureIgnoreCase) ) 
						colorID = "DAM";
					else if (type.Equals("master", StringComparison.CurrentCultureIgnoreCase) )
						colorID = "MASTER";
					else if (type.Equals("esp", StringComparison.CurrentCultureIgnoreCase) ) { // (opStr != "ROOT") {
						if (colorHt.Contains(id))
							colorID = id;
					}

					if (null != colorID) {
						Color bgColor = (Color)colorHt[colorID];
						ChangeDataRowBackColor(row, bgColor);
						tn.BackColor = bgColor;
					}

					ProcessBoundaryColors(tn.Nodes, colorHt);
				}
			



		}


		//BuildTreeGrid used to recursivly populate the tree and grid with explain output
		private void buildTreeGrid(Hashtable planStepsHT, int treeDepth, NCCWorkbenchQueryData.QueryPlanData qd, TreeNode tn)
		{
			if (null == qd)
				return;


			iGRow row = nccQueryPlanIGrid.Rows.Add();
			row.Level = treeDepth;
			SetPlanValuesToRow(row, qd);
			
			//Build acronym strings that get appended to operator strings for
			//easy identifying operator characteristics
			String acroStr = "";
			String toolTipStr = "    ";
			
			


			int iconIndex;
			//Refresh plan Treeview and add Root node
			if (tn == null)
			{
				nccQueryPlanTreeView.Nodes.Clear();
				iconIndex = GetImageIcon(qd.theOperator.Trim().ToUpper());
				tn = new TreeNode(qd.theOperator.Trim() + acroStr + _SpacePadding, iconIndex, iconIndex);
				tn.ToolTipText = "  " + qd.theOperator.Trim() + toolTipStr;
				nccQueryPlanTreeView.Nodes.Add(tn);
			}

			

			tn.Tag = row;  // Store the row as a tag on the treenode for faster retrieval.
			row.Tag = tn;  // Store the treenode as a tag on the row for faster retreival. 

			// Do this to prevent recursion.
			int seqNum = getIntValue(qd.sequenceNumber);
			planStepsHT.Remove(seqNum);

			int lc = getIntValue(qd.leftChildSeqNum);
			int rc = getIntValue(qd.rightChildSeqNum);

			if (0 < rc)
			{
				NCCWorkbenchQueryData.QueryPlanData childQDH = (NCCWorkbenchQueryData.QueryPlanData)planStepsHT[rc];
				if (null != childQDH)
				{

					//Do we need an acronym?
					GetOperatorInfo(childQDH, out acroStr, out toolTipStr);

					String operatorName = childQDH.theOperator.Trim();

					iconIndex = GetImageIcon(operatorName.ToUpper());
					TreeNode ch = new TreeNode(operatorName + acroStr + _SpacePadding, iconIndex, iconIndex);
					ch.ToolTipText = "  " + operatorName + toolTipStr;
					tn.Nodes.Add(ch);
					buildTreeGrid(planStepsHT, treeDepth + 1, childQDH, ch);
					row.TreeButton = iGTreeButtonState.Visible; //  Absent;
				}
			}

			if (0 < lc)
			{
				NCCWorkbenchQueryData.QueryPlanData childQDH = (NCCWorkbenchQueryData.QueryPlanData)planStepsHT[lc];
				if (null != childQDH)
				{
					//Do we need an acronym?
					GetOperatorInfo(childQDH, out acroStr, out toolTipStr);

					String operatorName = childQDH.theOperator.Trim();

					iconIndex = GetImageIcon(operatorName.ToUpper());
					TreeNode ch = new TreeNode(operatorName + acroStr + _SpacePadding, iconIndex, iconIndex);
					ch.ToolTipText = "  " + operatorName  + toolTipStr;
					tn.Nodes.Add(ch);

					buildTreeGrid(planStepsHT, treeDepth + 1, childQDH, ch);
					row.TreeButton = iGTreeButtonState.Visible;  // Absent;
				}
			}

			
			//Set top node to be selected so Operator Details gets displayed
			//nccQueryPlanTreeView.SelectedNode = nccQueryPlanTreeView.Nodes[0];
			//nccQueryPlanTreeView.Focus();
		}

		private static double getDoubleValue(String s)
		{
			double dbl = 0.000;
			try {
				dbl = Double.Parse(s.Trim(), NCCUtils.GetNumberFormatInfoForParsing() );

			}
			catch (Exception)
			{
				dbl = 0.00;
			}

			return dbl;
		}

		private static int getIntValue(String s)
		{
			int val = -1;

			if ((null == s) || (0 >= s.Trim().Length))
				return -1;

			try
			{
				val = Int32.Parse(s.Trim());

			}
			catch (Exception)
			{
				val = -1;
			}

			return val;
		}


		/// <summary>
		/// Causes a tree node to become selected based on the grids selection index
		/// </summary>
		private void syncTreeSelection()
		{
			try
			{
                iGSelectedRowsCollection selectedRows = nccQueryPlanIGrid.SelectedRows;
                if (null != selectedRows && 0 < selectedRows.Count)
				{
					//Temporarily unsubscribe to tree select event to prevent looping between grid and tree selects
					nccQueryPlanTreeView.AfterSelect -= new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
					nccQueryPlanTreeView.DrawNode -= nccQueryPlanTreeView_DrawNode;

				    nccQueryPlanTreeView.Select();
                    TreeNode tNode = (TreeNode)selectedRows[0].Tag;
					nccQueryPlanTreeView.SelectedNode = tNode;
					nccQueryPlanIGrid.Select();  //Need to make sure that grid stays selected for copy abilities
				}

				syncUpTreeViewAndIGridScrollbars();
			}
			catch (Exception)
			{
			}
			finally
			{
                if (null != nccQueryPlanIGrid.SelectedRows && 0 < nccQueryPlanIGrid.SelectedRows.Count)
                {
                    try
                    {
                        //Resubscribe to tree select events
                        nccQueryPlanTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
                    }
                    catch (Exception)
                    { }
                }
			}

		}
		
		private void colorizeExplainPlanDetails(Font font, Color color, int idx, int len)
		{
			try
			{
				this.nccQueryExplainPlanRichTextBox.Select(idx, len);
				this.nccQueryExplainPlanRichTextBox.SelectionColor = color;
				this.nccQueryExplainPlanRichTextBox.SelectionFont = font;

			}
			catch (Exception)
			{
			}

		}
		
		//showExplainPlanDetailsForSelectedStep parses description and displays output to
		//queryplan details output area. 
		private void showExplainPlanDetailsForSelectedStep()
		{
			//if (0 >= this.nccQueryPlanIGrid.SelectedCells.Count)
            if (0 >= this.nccQueryPlanIGrid.SelectedRows.Count)
				return;

			Font nameFont = new Font("Arial", 8.25F, FontStyle.Regular);
            Font boldNameFont = new Font("Tahoma", 8.25F, FontStyle.Bold);
			Font valueFont = new Font("Trebuchet MS", 8.25F, FontStyle.Regular);
			Color nameColor = Color.FromKnownColor(KnownColor.ForestGreen); //Color for the cost and description strings
			Color regColor = Color.FromKnownColor(KnownColor.ControlText); //Color for cost and desc values
			Color labelColor = Color.FromKnownColor(KnownColor.MidnightBlue); //Color for "Cost" and "Description" titles
            Color SectionColor = Color.FromKnownColor(KnownColor.ControlText);  //Operator Details Title

			try
			{

				nccQueryExplainPlanRichTextBox.Clear();

				nccQueryExplainPlanRichTextBox.AppendText("Operator Details" + Environment.NewLine + Environment.NewLine);
				int scIndex = nccQueryExplainPlanRichTextBox.Text.IndexOf("Operator Details");
				if (-1 != scIndex)
				{
					colorizeExplainPlanDetails(boldNameFont, SectionColor, scIndex, "Operator Details".Length);
				}

				//int rowIndex = this.nccQueryPlanIGrid.SelectedCells[0].RowIndex;
                int rowIndex = this.nccQueryPlanIGrid.SelectedRows[0].Index;
				String costDetails = (String)nccQueryPlanIGrid.Rows[rowIndex].Cells["DetailCost"].Value;
				String descriptionDetails = (String)nccQueryPlanIGrid.Rows[rowIndex].Cells["Description"].Value;

				if ((null != costDetails) && (0 < costDetails.Trim().Length))
				{


					//Add a title for Cost Details  
					nccQueryExplainPlanRichTextBox.AppendText("Costs:" + Environment.NewLine);
					int cdIndex = nccQueryExplainPlanRichTextBox.Text.IndexOf("Costs");
					if (-1 != cdIndex)
					{
						colorizeExplainPlanDetails(nameFont, labelColor, cdIndex, "Costs:".Length);
					}
					try
					{
						String nameValueRE = @"\b(\w+\s*:=?\s*[0-9\.eE\-\+]*)";
						Regex re = new Regex(nameValueRE);
						String[] nvPairs = re.Split(costDetails);
						foreach (String s in nvPairs)
						{
							if (0 < s.Trim().Length)
							{
								// In R2.4 Explain adds a := to the DETAIL_COST information -- go figure why!! :^(
								// So we just replace it with a single ":", so it looks like the R2.3 one!!
								String costMetricAndValue = s.Replace(":=", ":");
								nccQueryExplainPlanRichTextBox.AppendText("  " + costMetricAndValue + Environment.NewLine);

								Color textColor = nameColor;
								int size = costMetricAndValue.IndexOf(":");
								if (-1 == size)
								{
									size = costMetricAndValue.Length;
									textColor = Color.MidnightBlue;
								}

								int rtbIndex = nccQueryExplainPlanRichTextBox.Text.IndexOf(costMetricAndValue);
								if (-1 != rtbIndex)
								{
									colorizeExplainPlanDetails(nameFont, textColor, rtbIndex, size);
									colorizeExplainPlanDetails(valueFont, regColor, rtbIndex + size,
															   costMetricAndValue.Length - size);
								}
							}
						}

					}
					catch (Exception)
					{
						nccQueryExplainPlanRichTextBox.AppendText(costDetails + Environment.NewLine);
						int rtbIndex = nccQueryExplainPlanRichTextBox.Text.IndexOf(costDetails);
						if (-1 != rtbIndex)
							colorizeExplainPlanDetails(nameFont, Color.MidnightBlue, rtbIndex,
														costDetails.Length);

					}

				}

				if ((null == descriptionDetails) || (0 >= descriptionDetails.Trim().Length))
					return;



				//Add a title for Description Details 
				nccQueryExplainPlanRichTextBox.AppendText(Environment.NewLine + " Description:" + Environment.NewLine);
				int ddIndex = nccQueryExplainPlanRichTextBox.Text.IndexOf(" Description:");
				if (-1 != ddIndex)
				{
					colorizeExplainPlanDetails(nameFont, labelColor, ddIndex, " Description:".Length);
				}

				try
				{
					//Original RE
					//String nameValueRE = @"\b([A-Za-z0-9_\(\)]+\s*:\s*(.*?))";

					//A varation that skips \SYS0101: system name strings that are in the value of the description. DS
					//String nameValueRE = @"((?![\\A-Z0-9]+:)\b[A-Za-z0-9_\(\)]+\s*:\s*(.*?))";  

					//Another varation that skips \SYS0101: system name strings that are in the value of the description. DS
					String nameValueRE = @"(\b[A-Za-z0-9_\(\)]+\s*:([^\\A-Z0-9a-z:]))";  
					
					Regex re = new Regex(nameValueRE);
					String[] nvPairs = re.Split(descriptionDetails);

					int count = nvPairs.Length;
					int idx = 0;
					while (nvPairs.Length > idx)
					{
						String nameStr = nvPairs[idx++].Trim();
						if (0 >= nameStr.Length)
							continue;

						String valueStr = "";
						bool found = false;
                        bool first_round = true;
						while (!found && (nvPairs.Length > idx))
						{
                            // Only assume one space between name and value pair. 
							valueStr = nvPairs[idx++].Trim();
                            if (0 < valueStr.Length || !first_round)
                            {
                                found = true;
                                break;
                            }
                            else
                            {
                                first_round = false;
                            }
						}

						nccQueryExplainPlanRichTextBox.AppendText("  " + nameStr);
						int rtbIndex = nccQueryExplainPlanRichTextBox.Find(nameStr, RichTextBoxFinds.Reverse);
                        if (-1 != rtbIndex)
                        {
                            colorizeExplainPlanDetails(nameFont, nameColor, rtbIndex,
                                                       nameStr.Length);

                            nccQueryExplainPlanRichTextBox.AppendText("  " + valueStr + Environment.NewLine);
                            //rtbIndex = nccQueryExplainPlanRichTextBox.Find(valueStr, RichTextBoxFinds.Reverse);
                            rtbIndex = (valueStr.Length > 0) ? rtbIndex + nameStr.Length + 2 : -1;
                            if (-1 != rtbIndex)
                                colorizeExplainPlanDetails(valueFont, regColor, rtbIndex,
                                                           valueStr.Length);
                        }
					}

				}
				catch (Exception)
				{
					nccQueryExplainPlanRichTextBox.AppendText(descriptionDetails + Environment.NewLine);
					int rtbIndex = nccQueryExplainPlanRichTextBox.Find(descriptionDetails,
																	   RichTextBoxFinds.Reverse);
					if (-1 != rtbIndex)
						colorizeExplainPlanDetails(nameFont, Color.MidnightBlue, rtbIndex,
												   descriptionDetails.Length);
				}

			}
			catch (Exception e)
			{
				String msg = "Show Plan Details error:  " + e.Message;
				nccQueryExplainPlanRichTextBox.AppendText(Environment.NewLine + Environment.NewLine +
														  msg + Environment.NewLine);
				int rtbIndex = nccQueryExplainPlanRichTextBox.Find(msg, RichTextBoxFinds.Reverse);
				if (-1 != rtbIndex)
					colorizeExplainPlanDetails(valueFont, Color.Red, rtbIndex, msg.Length);

                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Database,
                        TRACE_SUB_AREA_NAME,
                        "NCCQueryPlanControl:: Show Plan Details exception = " + e.Message);
                }
#if DEBUG
				MessageBox.Show("***DEBUG:  Show Plan Details exception = " + e.Message);
#endif

			}
            
            // Reset the selection index 
            colorizeExplainPlanDetails(nameFont, nameColor, 0, 0);
		}

		private void syncUpTreeViewAndIGridScrollbars()
		{
			VScrollProperties sbProps = nccQueryPlanTreePanel.VerticalScroll;
			HScrollProperties hsbProps = nccQueryPlanTreePanel.HorizontalScroll;

			iGScrollBar gridScrollbar = nccQueryPlanIGrid.VScrollBar;

			nccQueryPlanTreeView.BeginUpdate();

			try {
				this.nccQueryPlanIGrid.VScrollBarValueChanged -= this.nccQueryPlanIGrid_VScrollBarValueChanged;

				nccQueryPlanTreeView.Height = nccQueryPlanIGrid.Height + gridScrollbar.Maximum +
											  nccQueryPlanOperatorHeadingPanel.Height + 100;

				sbProps.Minimum = gridScrollbar.Minimum;
				/*
				sbProps.Maximum = (nccQueryPlanIGrid.Rows.Count * nccQueryPlanIGrid.DefaultRow.Height) + 
								  nccQueryPlanOperatorHeadingPanel.Height;
				 * */
				sbProps.Maximum = gridScrollbar.Maximum + 1;

				sbProps.SmallChange = gridScrollbar.SmallChange;
				sbProps.LargeChange = gridScrollbar.LargeChange;
				sbProps.Value = gridScrollbar.Value;

				nccQueryPlanTreePanel.Visible = false;
				nccQueryPlanTreePanel.AutoScrollPosition = new Point(hsbProps.Value, gridScrollbar.Value);
				nccQueryPlanTreePanel.Visible = true;

				//This is another way to do it.
				//nccQueryPlanTreePanel.VerticalScroll.Value = gridScrollbar.Value;

			} catch (Exception) {
			} finally {
				this.nccQueryPlanIGrid.VScrollBarValueChanged += this.nccQueryPlanIGrid_VScrollBarValueChanged;
			}


			nccQueryPlanTreeView.EndUpdate();

			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}
		
		private void sortSubTree(TreeNode aNode, int depth)
		{
			List<TreeNode> nodeList = new List<TreeNode>();
			foreach (TreeNode childNode in aNode.Nodes) { nodeList.Add(childNode); }

			nodeList.Sort(_sortComparator);

			aNode.Nodes.Clear();

#if DEBUG
			String prefix = "";
			for (int j = 0; j < depth; j++) prefix = prefix + " ";

			// Console.WriteLine(prefix + " +Node=" + aNode.Text.Trim() + "[" + ((iGRow)(aNode.Tag)).Index + "]");
#endif

			for (int i = 0; i < nodeList.Count; i++)
			{
				sortSubTree(nodeList[i], depth + 1);
				aNode.Nodes.Add(nodeList[i]);
			}
		}

		private void sortTreeViewNodesToMatchGridSortOrder()
		{		 
			
			List<TreeNode> rootNodeList = new List<TreeNode>();
			foreach (TreeNode kidNode in nccQueryPlanTreeView.Nodes) { rootNodeList.Add(kidNode); }

			rootNodeList.Sort(_sortComparator);

			nccQueryPlanTreeView.Nodes.Clear();
			for (int idx = 0; idx < rootNodeList.Count; idx++)
			{
				sortSubTree(rootNodeList[idx], 0);
				nccQueryPlanTreeView.Nodes.Add(rootNodeList[idx]);
			}

		}
		
		private void nccQueryPlanIGrid_AfterContentsSorted(object sender, EventArgs e) 
        {

            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.Database,
                                   TRACE_SUB_AREA_NAME,
                                   "NCCQueryPlanControl:: In AfterSortted Time = " + DateTime.Now.Ticks.ToString());
            }

			setAsActiveControl(e);

			try
			{
				TreeNode selectedNode = nccQueryPlanTreeView.SelectedNode;

				for (int idx = 0; idx < nccQueryPlanIGrid.Rows.Count; idx++)
				{
					try
					{
						// On a sort, IGrid adds a new row and throws away the old one. Our pointers are way off. 
						// So set the TreeNode's index to match the currently displayed iGRow.
						TreeNode tnode = ((TreeNode)(nccQueryPlanIGrid.Rows[idx].Tag));
						tnode.Tag = nccQueryPlanIGrid.Rows[idx];
					}
					catch (Exception)
					{
					}

#if DEBUG_TREE_SORT
					if (doTracing)
						NCCTraceManager.OutputToLog("Index #" + idx + "  [" + nccQueryPlanIGrid.Rows[idx].Index +
													"] :  " + nccQueryPlanIGrid.Rows[idx].Cells["Operator"].Value +
													",  seq = " + nccQueryPlanIGrid.Rows[idx].Cells["Sequence"].Value +
													", total cost = " + nccQueryPlanIGrid.Rows[idx].Cells["TotalCost"].Value);
#endif

				}

                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCCQueryPlanControl:: Before sortTree Time = " + DateTime.Now.Ticks.ToString());
                }

				//Turn off drawmode.  It's only needed when tree is out of focus
				nccQueryPlanTreeView.DrawNode -= nccQueryPlanTreeView_DrawNode;
				nccQueryPlanTreeView.DrawMode = TreeViewDrawMode.Normal;

				nccQueryPlanTreeView.BeginUpdate();

				if (nccQueryPlanIGrid.SortByLevels)
					sortTreeViewNodesToMatchGridSortOrder();

                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCCQueryPlanControl:: After sortTree Time = " + DateTime.Now.Ticks.ToString());
                }

				iGRow gRow = (iGRow) selectedNode.Tag;

				nccQueryPlanIGrid.SetCurRow(gRow.Index);

                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCCQueryPlanControl:: After setCurRow Time = " + DateTime.Now.Ticks.ToString());
                }

				syncTreeSelection();

				nccQueryPlanTreeView.EndUpdate();

				ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);

				// nccQueryPlanTreeView.DrawMode = TreeViewDrawMode.OwnerDrawText;
				// nccQueryPlanTreeView.DrawNode += nccQueryPlanTreeView_DrawNode;

                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCCQueryPlanControl:: After syncTreeSelection Time = " + DateTime.Now.Ticks.ToString() + "\n" +
                                       "                      Ticks per second = " + TimeSpan.FromSeconds(1).Ticks.ToString());
                }

			}
			catch (Exception)
			{
			}
		}
		
		private void nccQueryPlanIGrid_SelectionChanged(object sender, EventArgs e)
		{
			if (0 >= nccQueryPlanIGrid.SelectedRows.Count) {
				nccQueryPlanTreeView.SelectedNode = null;
				ShowSummaryForCurrentQuery(this._wbqd);
				return;
			}

			setAsActiveControl(e);
			syncTreeSelection();
			showExplainPlanDetailsForSelectedStep();
		}
		
		private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
		{
			setAsActiveControl(e);

			try {
				//Temporarily unsubscribe to query grid select event to prevent looping between grid and tree selects			
				nccQueryPlanIGrid.SelectionChanged -= new System.EventHandler(this.nccQueryPlanIGrid_SelectionChanged);

				nccQueryPlanIGrid.SelectionMode = iGSelectionMode.One;

				iGRow row = (iGRow)e.Node.Tag;
				if (0 <= row.Index) 
                {
					nccQueryPlanIGrid.SetCurRow(row.Index);
				}

				syncUpTreeViewAndIGridScrollbars();

			} catch (Exception) {
			} finally {
				nccQueryPlanIGrid.SelectionChanged += new System.EventHandler(this.nccQueryPlanIGrid_SelectionChanged);
				nccQueryPlanIGrid.SelectionMode = iGSelectionMode.MultiExtended;
			}
				
			showExplainPlanDetailsForSelectedStep();
		}
		
		private void treeView1_AfterExpand(object sender, TreeViewEventArgs e)
		{
			setAsActiveControl(e);

			try
			{
				iGRow row = (iGRow)e.Node.Tag;
				row.Expanded = true;
				syncUpTreeViewAndIGridScrollbars();
			}
			catch (Exception exc) {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCCQueryPlanControl:: error in treeView1_AfterExpand exc = " + exc.Message);
                }
			}

		}

		private void treeView1_AfterCollapse(object sender, TreeViewEventArgs e)
		{
			setAsActiveControl(e);

			try
			{
				iGRow row = (iGRow)e.Node.Tag;
				row.Expanded = false;
				syncUpTreeViewAndIGridScrollbars();
			}
			catch (Exception exc) {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCCQueryPlanControl::error in treeView1_AfterCollapse exc = " + exc.Message);
                }
			}
		}
		
		private void nccQueryPlanIGrid_ColHdrClick(object sender, iGColHdrClickEventArgs e)
		{
			if (false == nccQueryPlanIGrid.SortByLevels)
				nccQueryPlanIGrid.SelectionChanged -= new System.EventHandler(this.nccQueryPlanIGrid_SelectionChanged);

			setAsActiveControl(e);
			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}

		private void nccQueryPlanIGrid_VScrollBarValueChanged(object sender, EventArgs e)
		{
			//Turn off drawmode.  It's only needed when tree is out of focus
			try {
				nccQueryPlanTreeView.DrawNode -= nccQueryPlanTreeView_DrawNode;
				nccQueryPlanTreeView.DrawMode = TreeViewDrawMode.Normal;
				syncUpTreeViewAndIGridScrollbars();

			} finally {
				nccQueryPlanTreeView.DrawMode = TreeViewDrawMode.OwnerDrawText;
				nccQueryPlanTreeView.DrawNode += nccQueryPlanTreeView_DrawNode;
			}

		}

		private void nccQueryPlanIGrid_AfterRowStateChanged_1(object sender, iGAfterRowStateChangedEventArgs e)
		{
			iGRow row = nccQueryPlanIGrid.Rows[e.RowIndex];
			if (null != row)
			{
				TreeNode tNode = (TreeNode)(row.Tag);
				if (row.Expanded)
					tNode.Expand();
				else
					tNode.Collapse();
			}

			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}
		
		private void NCCQueryPlanControl_Resize(object sender, EventArgs e) {
			setTreeViewControlWidth(calculatePlanTreeWidth() );
			syncUpTreeViewAndIGridScrollbars();
		}

		private void nccQueryPlanTreeGridSplitContainer_SplitterMoved(object sender, SplitterEventArgs e) {
			setTreeViewControlWidth(calculatePlanTreeWidth() );
		}
		
		private void setAsActiveControl(EventArgs eArgs)
		{
			if (null != _onClickHandler)
				this._onClickHandler(this, eArgs);
		}

		private void nccQueryPlanTreeView_Leave(object sender, EventArgs e)
		{
			//Turn on Drawmode when tree loses focus.  Drawmode handler redraws the 
			//highlight. If it is turned on full time, the tree gets very slugish when 
			//scrolling, sorting and selecting
			nccQueryPlanTreeView.DrawMode = TreeViewDrawMode.OwnerDrawText;
			nccQueryPlanTreeView.DrawNode += nccQueryPlanTreeView_DrawNode;
			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}

		private void nccQueryPlanTreeView_Enter(object sender, EventArgs e) {
			setAsActiveControl(e);
			nccQueryPlanTreeView.DrawNode -= nccQueryPlanTreeView_DrawNode;
			nccQueryPlanTreeView.DrawMode = TreeViewDrawMode.Normal;
			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}

		
		//Used for output richtext control to have tooltips on individual text lines.
		private void nccQueryExplainPlanRichTextBox_MouseMove(object sender, MouseEventArgs e)
		{
			if ((null == nccQueryExplainPlanRichTextBox.Text) || 
				(0 >= nccQueryExplainPlanRichTextBox.Text.Trim().Length) )
				return;

			Point ploc = new Point(e.X, e.Y);
			_toolTips.InitialDelay = 50;

			try
			{
				int i = nccQueryExplainPlanRichTextBox.GetCharIndexFromPosition(ploc);

				if (i < 0)
					return;
				
				char c = nccQueryExplainPlanRichTextBox.GetCharFromPosition(ploc);

				int lineNumber = nccQueryExplainPlanRichTextBox.GetLineFromCharIndex(i);
				if (lineNumber >= nccQueryExplainPlanRichTextBox.Lines.Length)
					return;

				string[] textLines = nccQueryExplainPlanRichTextBox.Lines;

				String theText = textLines[lineNumber];
								
				if (theText.CompareTo((String)_toolTips.Tag) == 0 || theText.Contains("Costs:") || theText.Contains("Description:"))
					return;
				else
					_toolTips.Tag = theText;

				_toolTips.Hide(nccQueryExplainPlanRichTextBox);

				if (theText.Contains("UIDs"))
					_toolTips.Show("Total Updates, Inserts, or Deletes.", nccQueryExplainPlanRichTextBox);
				else if (theText.Contains("TOTAL JOINS"))
					_toolTips.Show("Total Joins counted", nccQueryExplainPlanRichTextBox);
				else if (theText.Contains("ESP PROCESSES"))
					_toolTips.Show("Total child processes", nccQueryExplainPlanRichTextBox);
				else if (theText.Contains("Explain Plan Summary"))
					_toolTips.Show("This view displays a summary of the plan" + Environment.NewLine +
								   "including operator and process totals.", nccQueryExplainPlanRichTextBox);
				else if (theText.Contains("Operator Details"))
					_toolTips.Show("This view displays the operator description items.", nccQueryExplainPlanRichTextBox);

				else if (theText.Contains("CPU_TIME"))
					_toolTips.Show("Relative estimate of the processor cost needed to "
									+ Environment.NewLine + "execute the instructions of this operator and all children.", nccQueryExplainPlanRichTextBox);
				else if (theText.Contains("IO_TIME"))
					_toolTips.Show("Relative estimate of the I/O cost (depends on the number of seeks and amount of data transferred) "
									+ Environment.NewLine +
									"needed to perform the I/O for this operator and all children.", nccQueryExplainPlanRichTextBox);
				else if (theText.Contains("MSG_TIME"))
					_toolTips.Show("Relative estimate of the messaging cost (depends on the number of local/remote messages and amount of data sent) "
									+ Environment.NewLine +
									"for this operator and all children.", nccQueryExplainPlanRichTextBox);
				else if (theText.Contains("IDLETIME"))
					_toolTips.Show("Relative estimate of the idle cost (depends on the amount of expected wait for an event) "
									+ Environment.NewLine +
								   "for this operator and all children. Events are opening a table, starting an ESP process, and so on."
									, nccQueryExplainPlanRichTextBox);
				else if (theText.Contains("PROBES"))
					_toolTips.Show("Estimated number of requests for execution of this operator. " +
						Environment.NewLine +
						"The value is 1 except for the right child operator (inner scan) of a nested-loop join."
						,nccQueryExplainPlanRichTextBox);
				//Default tool tip disabled for now				
				//else _toolTips.Show(theText, nccQueryExplainPlanRichTextBox);			
			}
			catch (Exception)
			{
			   //Who cares
			}
						
		}

		private void nccQueryPlanIGrid_ColHdrMouseEnter(object sender, iGColHdrMouseEnterLeaveEventArgs e)
		{
			if (e.ColIndex < 0)
				return;

			
			String colStr = nccQueryPlanIGrid.Cols[e.ColIndex].Text.ToString();
			
			_toolTips.InitialDelay = 1000;

			_toolTips.Hide(nccQueryPlanIGrid);

			try
			{
				if (colStr.Contains("Sequence"))
					_toolTips.Show("Sequence number provided for the operator in the query plan.", nccQueryPlanIGrid);
				else if (colStr.Contains("Total Cost"))
					_toolTips.Show("Estimated cost associated with execution of the current operator and all children.", nccQueryPlanIGrid);
				else if (colStr.Contains("Operator Cost"))
					_toolTips.Show("Estimated cost associated with executing the current operator.", nccQueryPlanIGrid);
				else if (colStr.Contains("Rows Out"))
					_toolTips.Show("Estimated number of rows returned by the plan. Cardinality and Rows Out are the same.", nccQueryPlanIGrid);
				else if (colStr.Contains("Table Name"))
					_toolTips.Show("Name provided for the base table.", nccQueryPlanIGrid);
			}
			catch (Exception)
			{
				//Who cares
			}

		}
		#region
		//Context menu handlers
		private void contextCopy_Click(object sender, EventArgs e)
		{
			if (contextMenuStrip1.SourceControl == nccQueryPlanIGrid)
			{
				nccQueryExplainPlanRichTextBox.Focus();
				copyIGridContentsToClipboard(nccQueryPlanIGrid, true);
			}

			if (contextMenuStrip1.SourceControl == nccQueryExplainPlanRichTextBox)
			{
				nccQueryExplainPlanRichTextBox.Focus();
				nccQueryExplainPlanRichTextBox.Copy();
			}

		}

		private void contextSelectAll_Click(object sender, EventArgs e)
		{
			if (contextMenuStrip1.SourceControl == nccQueryExplainPlanRichTextBox)
			{
				//Make sure it's focused
				nccQueryExplainPlanRichTextBox.Focus();
				nccQueryExplainPlanRichTextBox.SelectAll();
			}
			if (contextMenuStrip1.SourceControl == nccQueryPlanIGrid)
			{
				nccQueryPlanIGrid.Focus();
				selectAllQueryPlanGrid();
			}
		}

		private void contexthelp_Click(object sender, EventArgs e)
		{
			if (contextMenuStrip1.SourceControl == nccQueryPlanIGrid)
			{
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ExplainGridView);
			}
            else if (contextMenuStrip1.SourceControl == nccQueryExplainPlanRichTextBox)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.PlanSummaryView);
            }
            else if (contextMenuStrip1.SourceControl == nccQueryPlanTreeView)
			{
				try
				{
					String [] tnTxt = nccQueryPlanTreeView.SelectedNode.Text.ToLower().Split();
									
					//Get the operator of the current node to map context sensitive help
					//Launch appropriate context sensitive help
                    TrafodionHelpProvider.Instance.ShowHelpTopic(TrafodionHelpProvider.SQQueryGuideFile, "i1004209.html" + "#" + tnTxt[0]);
				}
				catch (Exception)
				{
                    TrafodionHelpProvider.Instance.ShowHelpTopic(TrafodionHelpProvider.SQQueryGuideFile, "i1004209.html");
				}
			}
		}


		private void contextMenuStrip1_Opening(object sender, CancelEventArgs e)
		{
            this.contexthelp.Visible = true;

			if (contextMenuStrip1.SourceControl == nccQueryPlanIGrid)
			{
                this.contextSelectAll.Visible = false;
                this.contextCopy.Visible = false;
              
                //this.contextSelectAll.Visible = true;
                //this.contextCopy.Visible = true;
				

                ////if (nccQueryPlanIGrid.SelectedCells.Count > 0)
                //if (nccQueryPlanIGrid.SelectedRows.Count > 0)
                //    contextCopy.Enabled = true;
                //else
                //    contextCopy.Enabled = false;

                //if (nccQueryPlanIGrid.Rows.Count > 0)
                //    contextSelectAll.Enabled = true;
                //else
                //    contextSelectAll.Enabled = false;
				
			}

			if (contextMenuStrip1.SourceControl == nccQueryExplainPlanRichTextBox)
			{
				this.contextSelectAll.Visible = true;
				this.contextCopy.Visible = true;

				this.contextCopy.Enabled = (0 < nccQueryExplainPlanRichTextBox.SelectionLength);
				this.contextSelectAll.Enabled = (0 < nccQueryExplainPlanRichTextBox.Text.Length);
			}

			if (contextMenuStrip1.SourceControl == nccQueryPlanTreeView)
			{
                this.contextSelectAll.Visible = false;
                this.contextCopy.Visible = false;
                this.contexthelp.Visible = true;
			}

		}
		#endregion

		private void nccQueryPlanIGrid_CellClick(object sender, iGCellClickEventArgs e)
		{
			setAsActiveControl(e);
		}


		class TreeViewNodeSorterComparator : IComparer<TreeNode>
		{
			public int Compare(TreeNode nodeA, TreeNode nodeB)
			{
				int retCode = 0;

				try
				{
					iGRow rowA = (iGRow)nodeA.Tag;
					iGRow rowB = (iGRow)nodeB.Tag;

					if (rowA.Index == rowB.Index)
						retCode = 0;
					else
						retCode = (rowA.Index > rowB.Index) ? 1 : -1;

				}
				catch (Exception e) {
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCCQueryPlanControl::Comparator error = " + e.Message);
                    }
					retCode = 0;
				}

				return retCode;
			}

		}

		private void nccQueryExplainPlanRichTextBox_Click(object sender, EventArgs e)
		{
			setAsActiveControl(e);
		}

		private void nccQueryPlanOperatorNameLabel_Click(object sender, EventArgs e) {
			setAsActiveControl(e);
            nccQueryPlanTreeView.AfterSelect -= new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
            try
            {
                // Reset the selection
                nccQueryPlanTreeView.SelectedNode = null;
                ReloadPlanInformation();
            }
            finally
            {
                nccQueryPlanTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
            }

			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}

		private void nccQueryPlanOperatorHeadingPanel_Click(object sender, EventArgs e) {
			setAsActiveControl(e);
            nccQueryPlanTreeView.AfterSelect -= new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
			ReloadPlanInformation();
            nccQueryPlanTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}

		private void nccQueryPlanTreeView_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
		{
			if (e.Button == MouseButtons.Right) 
				nccQueryPlanTreeView.SelectedNode = e.Node;

			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}

		private void nccQueryPlanIGrid_Click(object sender, EventArgs e) {
			setAsActiveControl(e);
			// syncUpTreeViewAndIGridScrollbars();
		}

		private void nccQueryPlanTreeView_Click(object sender, EventArgs e) {
			setAsActiveControl(e);
			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
			syncUpTreeViewAndIGridScrollbars();
		}

		private void nccQueryPlanTreePanel_Click(object sender, EventArgs e) {
			setAsActiveControl(e);
			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}

		private void nccQueryPlanTreeGridSplitContainer_Click(object sender, EventArgs e) {
			setAsActiveControl(e);
			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}

		private void nccQueryPlanDetailsPanelSplitContainer_Click(object sender, EventArgs e) {
			setAsActiveControl(e);
			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}

		private void nccQueryPlanTreePanel_Resize(object sender, EventArgs e) {
			if (nccQueryPlanTreeView.Width != this._treeViewControlWidth)
				setTreeViewControlWidth(calculatePlanTreeWidth() );

			ShowScrollBar(nccQueryPlanTreePanel.Handle, SB_VERT, false);
		}


		private int getTreeViewNodeTextPixelWidth(TreeNode aNode) {
			return (aNode.Level * nccQueryPlanTreeView.Indent) + 50 +
					(int) (nccQueryPlanTreeView.Font.SizeInPoints * aNode.Text.Trim().Length);

		}

		private TreeNode compareTreeViewNodes(TreeNode currentLowestNode, 
											  TreeNode theChallenger) {
			try {
				if (null == currentLowestNode)
					return theChallenger;

				int challengerWidth = getTreeViewNodeTextPixelWidth(theChallenger);

				if (getTreeViewNodeTextPixelWidth(currentLowestNode) < challengerWidth)
					return theChallenger;

			} catch(Exception) {
			}

			// Nope, then nodeA is the lower level node.
			return currentLowestNode;
		}


		private TreeNode findLowestTreeViewNode(TreeNode currentLowestNode, TreeNodeCollection theNodes) {
			TreeNode myLowestLevelNode = currentLowestNode;


			foreach (TreeNode aNode in theNodes) {
				if (aNode.IsExpanded) {
					myLowestLevelNode = findLowestTreeViewNode(myLowestLevelNode, aNode.Nodes);
					myLowestLevelNode = compareTreeViewNodes(myLowestLevelNode, aNode);
				}
			}

			return myLowestLevelNode;
		}



		private int calculatePlanTreeWidth() {
			int theWidth = nccQueryPlanTreePanel.Width;

			try {
				TreeNode deepestNode = findLowestTreeViewNode(nccQueryPlanTreeView.TopNode,
															  nccQueryPlanTreeView.Nodes);

				if (null == deepestNode)
					return theWidth;

				int value = getTreeViewNodeTextPixelWidth(deepestNode);
				theWidth = Math.Max(theWidth, value);

			} catch (Exception) {
			}

			return theWidth;
		}

		private void nccQueryPlanDetailsPanelSplitContainer_SplitterMoved(object sender, SplitterEventArgs e) {
			setTreeViewControlWidth(calculatePlanTreeWidth());
		}

	}//NCCQueryPlanControl

}//Namespace
