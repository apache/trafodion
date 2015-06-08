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
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Collections;
using System.Drawing.Drawing2D;
using System.Threading;
using System.Timers;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionSystemStatusUserControl : UserControl
    {
        public ArrayList ErrorArray = new ArrayList();
        private bool ErrorTableInvalidated = true;
        private SystemAspect SelectedAspect = SystemAspect.General;
        private bool ShowErrorTable = false;

        private Color ROLL_OVER_COL = Color.FromArgb(241, 241, 241);
        private Color ROLL_OUT_COL = Color.FromKnownColor(KnownColor.Control);
        private Color CLICK_COL = Color.FromArgb(230, 230, 230);//FromKnownColor(KnownColor.ControlDarkDark);
        private Color UNCLICK_COL = Color.FromKnownColor(KnownColor.Control);

        private bool onTop = true;
        private int OldSplitterDistance = 0;
        private int AdjustedSplitterDistance = 0;
        private int OldHeight = 0;
        private int AdjustedHeight = 0;

        private int ConnState = 0;
        private int TmfState = 0;
        private int DiskState = 0;

        private ArrayList panelList = new ArrayList();

        private const int ROW_HEIGHT = 35;
        private const int TABLE_CONST = 35;
        private bool firstTime = true;

        private Dictionary<string, TrafodionStatusLightUserControl> _statusLights = new Dictionary<string, TrafodionStatusLightUserControl>();
        private Hashtable _lightOrder = new Hashtable();



        private SystemMonitorStatusLightPopup ConPopup = new SystemMonitorStatusLightPopup("Connectivity Information");
        private SystemMonitorStatusLightPopup TranPopup = new SystemMonitorStatusLightPopup("Transaction Information");
        private SystemMonitorStatusLightPopup DiskPopup = new SystemMonitorStatusLightPopup("Disk Information");

        private delegate void UpdateDetailsDelegate(ArrayList anArrayList);

        private TrafodionStatusLightUserControl.ChangingHandler LightClickHandler = null;
        public Dictionary<string, TrafodionStatusLightUserControl> StatusLightsHash
        {
            get { return _statusLights; }
        }


        public TrafodionSystemStatusUserControl()
        {
            InitializeComponent();
            AddStatusLight("Connectivity", "Connectivity");
            AddStatusLight("Disks", "Disks");
            AddStatusLight("Transactions", "Transactions");
            AddStatusLight("Alerts", "Alerts");

        }

        public TrafodionSystemStatusUserControl(bool aConnectivityLight, bool aTransactionLight, bool aDiskLight, bool aAlertLight)
        {
            InitializeComponent();
            SetupLights(aConnectivityLight, aTransactionLight, aDiskLight, aAlertLight);
        }


        public void SetupLights(bool aConnectivityLight, bool aTransactionLight, bool aDiskLight, bool aAlertLight)
        {
            ClearAllLights();
            //ConPopup.MouseCli
            if (aConnectivityLight)
            {
                AddStatusLight("Connectivity", "Connectivity");
            }
            if (aTransactionLight)
            {
                AddStatusLight("Transactions", "Transactions");
            }
            if (aDiskLight)
            {
                AddStatusLight("Disks", "Disks");

            }
            //Add the "Alert" light no matter what
            AddStatusLight("Alerts", "Alerts", aAlertLight);
        }

        void DiskClickHandler(object sender, EventArgs args)
        {
            //Popup the display
        }

        private void ClearAllLights()
        {
            this.statusLightTable_tableLayoutPanel.Controls.Clear();
            this.statusLightTable_tableLayoutPanel.ColumnStyles.Clear();

            this.statusLightTable_tableLayoutPanel.ColumnCount = 1;
            this.statusLightTable_tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
        }

        public void InitializeDiddleLights(bool Connectivity, bool Transactions, bool Disks)
        {
            this.statusLightTable_tableLayoutPanel.ColumnStyles[0].Width = 0;
            this.statusLightTable_tableLayoutPanel.ColumnStyles[1].Width = 0;
            this.statusLightTable_tableLayoutPanel.ColumnStyles[2].Width = 0;

            if (Connectivity)
            {
                this.statusLightTable_tableLayoutPanel.ColumnStyles[0].Width = 100;
            }
            if (Transactions)
            {
                this.statusLightTable_tableLayoutPanel.ColumnStyles[1].Width = 100;
            }
            if (Disks)
            {
                this.statusLightTable_tableLayoutPanel.ColumnStyles[2].Width = 100;
            }

            UpdateLights(3, 3, 3);

            //ErrorTableInvalidated = true;
            //this.UpdateErrorTable();
        }

        public bool OnTop
        {
            get { return onTop; }
            set { onTop = value; }
        }

        public void UpdateLights(Hashtable systemSummaryHash)
        {
            if (systemSummaryHash == null)
                return;


            string ConnText = "";
            string TransactionText = "";
            string DiskText = "";

            if (systemSummaryHash.ContainsKey("NDCS"))
            {
                Hashtable ConnectivitySummary = (Hashtable)systemSummaryHash["NDCS"];
                //if ((int)ConnectivitySummary["state"] > ConnState)
                ConnState = (int)ConnectivitySummary["state"];
            }

            if (systemSummaryHash.ContainsKey("TMFS"))
            {
                Hashtable TransactionsSummary = (Hashtable)systemSummaryHash["TMFS"];
                //if ((int)TransactionsSummary["state"] > TmfState)
                TmfState = (int)TransactionsSummary["state"];
            }

            if (systemSummaryHash.ContainsKey("DISK"))
            {
                Hashtable DiskSummary = (Hashtable)systemSummaryHash["DISK"];
                //if ((int)DiskSummary["state"] > DiskState)
                DiskState = (int)DiskSummary["state"];
            }

            UpdateLights(ConnState, TmfState, DiskState);
        }

        public void UpdateDetails(ArrayList aErrorArray)
        {
            try
            {
                this.BeginInvoke(new UpdateDetailsDelegate(UpdateDetailsArray), new object[] { aErrorArray });
            }
            catch (Exception ex)
            {
                //Do nothing
            }
        }

        public void UpdateDetailsArray(ArrayList aErrorArray)
        {
            //ConPopup.ErrorGrid.Rows.Clear();
            //TranPopup.ErrorGrid.Rows.Clear();
            //DiskPopup.ErrorGrid.Rows.Clear();

            bool conCleared = false;
            bool tranCleared = false;
            bool diskCleared = false;

            SystemMonitorStatusLightPopup PopupToEdit = null;

            foreach (ErrorTableData etd in aErrorArray)
            {
                if (etd.ErrorType == SystemAspect.General)
                {
                    continue;
                }
                else if (etd.ErrorType == SystemAspect.Connectivity)
                {
                    if (!conCleared)
                    {
                        ConPopup.ErrorGrid.Rows.Clear();
                        conCleared = true;
                    }

                    PopupToEdit = ConPopup;
                }
                else if (etd.ErrorType == SystemAspect.Transactions)
                {

                    if (!tranCleared)
                    {
                        TranPopup.ErrorGrid.Rows.Clear();
                        tranCleared = true;
                    }

                    PopupToEdit = TranPopup;

                }
                else if(etd.ErrorType == SystemAspect.Disk)
                {
                    if (!diskCleared)
                    {
                        DiskPopup.ErrorGrid.Rows.Clear();
                        diskCleared = true;
                    }

                    PopupToEdit = DiskPopup;
                }

                if (null != PopupToEdit)
                {
                    try
                    {
                        iGRow newRow = PopupToEdit.ErrorGrid.Rows.Add();
                        newRow.Cells["DateTime"].Value = etd.Timestamp;
                        newRow.Cells["SegNum"].Value = etd.SegmentNumber;
                        newRow.Cells["Entity"].Value = etd.ErrorType;
                        newRow.Cells["Details"].Value = etd.ErrorString;
                    }
                    catch (Exception ex)
                    {
                        //Logger.OutputErrorLog(ex.Message);
                    }
                }
            }
        }


        public void UpdateLights(int aConnectivity, int aTransactions, int aHardware)
        {

            if (this._statusLights.ContainsKey("Connectivity"))
                ((TrafodionStatusLightUserControl)this._statusLights["Connectivity"]).SetState(aConnectivity);

            if (this._statusLights.ContainsKey("Transactions"))
                ((TrafodionStatusLightUserControl)this._statusLights["Transactions"]).SetState(aTransactions);

            if (this._statusLights.ContainsKey("Disks"))
                ((TrafodionStatusLightUserControl)this._statusLights["Disks"]).SetState(aHardware);

        }

        public void UpdateConnectivity(int aConnectivity)
        {
            ConnState = aConnectivity;
            ((TrafodionStatusLightUserControl)this._statusLights["Connectivity"]).SetState(aConnectivity);
        }

        public void UpdateTransactions(int aTransactions)
        {
            TmfState = aTransactions;
            ((TrafodionStatusLightUserControl)this._statusLights["Transactions"]).SetState(aTransactions);
        }

        public void UpdateHardware(int aHardware)
        {
            DiskState = aHardware;
            ((TrafodionStatusLightUserControl)this._statusLights["Disks"]).SetState(aHardware);
        }

        public bool UpdateStatus(object aSystemKey, int aStatus)
        {
            try
            {
                if (this._statusLights.ContainsKey(aSystemKey.ToString()))
                {
                    TrafodionStatusLightUserControl controlToSet = this._statusLights[aSystemKey.ToString()] as TrafodionStatusLightUserControl;
                    controlToSet.SetState(aStatus);
                    return true;
                }
                return false;
            }
            catch (Exception e)
            {
                return false;
            }
        }


        public void RemoveStatusLight(string aKey)
        {
            //int columnIndex = _lightOrder[aKey] as Int32;
            TrafodionStatusLightUserControl lightToRemove = _statusLights[aKey] as TrafodionStatusLightUserControl;
            lightToRemove.MouseClickLight -= this.LightClickHandler;
            this.statusLightTable_tableLayoutPanel.Controls.Remove(lightToRemove);
            _statusLights.Remove(aKey);
            _lightOrder.Remove(aKey);
            this.statusLightTable_tableLayoutPanel.ColumnCount--;

        }

        public TrafodionStatusLightUserControl AddStatusLight(string aKey, string aLabel) { return AddStatusLight(aKey, aLabel, true);}
        public TrafodionStatusLightUserControl AddStatusLight(string aKey, string aLabel, bool aVisible)
        {

            TrafodionStatusLightUserControl lightToConfigure = null;

            //Check to make sure it doesn't already exist:
            if (_statusLights.ContainsKey(aKey))
            {
                lightToConfigure = _statusLights[aKey] as TrafodionStatusLightUserControl;
            } else
            {
                //Let there be light!
                lightToConfigure = new TrafodionStatusLightUserControl(aLabel, aKey);               
                _statusLights.Add(aKey, lightToConfigure);
                lightToConfigure.Anchor = AnchorStyles.None;
                lightToConfigure.Dock = DockStyle.Fill;
                lightToConfigure.MouseClickLight += this.LightClickHandler;
            }

            //If the light should be shown
            if (aVisible)
            {
                //Make the light visible
                int columnIndex = this.statusLightTable_tableLayoutPanel.ColumnCount;

                if (this.statusLightTable_tableLayoutPanel.Controls.Count <= 0)
                {
                    columnIndex = 0;
                }
                else
                {
                    this.statusLightTable_tableLayoutPanel.ColumnCount++;
                    this.statusLightTable_tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
                }

                if (_lightOrder.ContainsKey(aKey))
                {
                    _lightOrder[aKey] = columnIndex;
                }
                else
                {
                    _lightOrder.Add(aKey, columnIndex);
                }

                this.statusLightTable_tableLayoutPanel.Controls.Add(lightToConfigure, columnIndex, 0);
            }

            return lightToConfigure;
        }

        void MouseClickLight(object sender, EventArgs args)
        {
            TrafodionStatusLightUserControl clickedLight = sender as TrafodionStatusLightUserControl;
            if (clickedLight.Key.Equals("Connectivity"))
            {
                this.ConPopup.Show();
                this.ConPopup.BringToFront();
            }
            else if (clickedLight.Key.Equals("Transactions"))
            {
                this.TranPopup.Show();
                this.TranPopup.BringToFront();
            }
            else if (clickedLight.Key.Equals("Disks"))
            {
                this.DiskPopup.Show();
                this.DiskPopup.BringToFront();
            }
        }

        /*
        public TrafodionAdvancedStatusLightUserControl AddStatusLightAdvanced(string aKey, string aLabel, string aComboTitle)
        {
            int columnIndex = this.statusLightTable_tableLayoutPanel.ColumnCount;
            TrafodionAdvancedStatusLightUserControl newLight = new TrafodionAdvancedStatusLightUserControl(0, aLabel, aComboTitle);

            if (columnIndex == 1 && this._statusLights.Count == 0)
            {
                columnIndex = 0;
            }
            else
            {
                this.statusLightTable_tableLayoutPanel.ColumnCount++;
                this.statusLightTable_tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            }
            
            _statusLights.Add(aKey, newLight);
            _lightOrder.Add(aKey, columnIndex);

            this.statusLightTable_tableLayoutPanel.Controls.Add(newLight, columnIndex, 0);
            
            return newLight;
        }*/

        /// <summary>
        /// Cleanup before dispose
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            ClearAllLights();
            foreach (KeyValuePair<string, TrafodionStatusLightUserControl> kvp in this._statusLights)
            {
                kvp.Value.MouseClickLight -= this.LightClickHandler;
                kvp.Value.Dispose();
            }
            this.LightClickHandler = null;
            this.DiskPopup.EnableCloseHandler = false;
            this.DiskPopup.Close();
            this.TranPopup.EnableCloseHandler = false;
            this.TranPopup.Close();
            this.ConPopup.EnableCloseHandler = false;
            this.ConPopup.Close();
        }
    }
}
