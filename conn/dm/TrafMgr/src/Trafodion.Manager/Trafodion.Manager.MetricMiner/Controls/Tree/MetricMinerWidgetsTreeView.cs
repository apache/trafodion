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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.MetricMiner.Controls.Tree
{
    /// <summary>
    /// Treeview to display the files on Trafodion platform
    /// </summary>
    public class MetricMinerWidgetsTreeView : NavigationTreeView, IDisposable
    {
        public const string MM_REPORT_ROOT_ICON = "ReportRootFolder";
        public const string MM_ADHOC_REPORT_ICON = "AdhocReport";
        public const string MM_LIBRARY_FOLDER_ICON = "LibraryFolder";
        public const string MM_REPORT_ICON = "Report";
        public const string MM_CHART_REPORT_ICON = "ChartReport";
        public const string MM_Linked_REPORT_ICON = "LinkedReport";

        #region Private member variables
        private static readonly string thePersistenceKey = "MetricMinerReportFolderPersistence";
        MetricMinerRootNode _theRootNode = null;
        public delegate void OnTreePopulate(MetricMinerWidgetsTreeView treeView);
        private OnTreePopulate _theTreePopulateImpl;

        #endregion Private member variables

        public MetricMinerWidgetsTreeView() : base()
        {

            this.theImageList.Images.Add(MM_REPORT_ROOT_ICON, global::Trafodion.Manager.Properties.Resources.ReportRootFolderIcon);
            this.theImageList.Images.Add(MM_ADHOC_REPORT_ICON, global::Trafodion.Manager.Properties.Resources.AdhocReportIcon);
            this.theImageList.Images.Add(MM_LIBRARY_FOLDER_ICON, global::Trafodion.Manager.Properties.Resources.LibraryFolderIcon);
            this.theImageList.Images.Add(MM_REPORT_ICON, global::Trafodion.Manager.Properties.Resources.ReportIcon);
            this.theImageList.Images.Add(MM_CHART_REPORT_ICON, global::Trafodion.Manager.Properties.Resources.ChartReportIcon);
            this.theImageList.Images.Add(MM_Linked_REPORT_ICON, global::Trafodion.Manager.Properties.Resources.LinkedReportIcon);
        }


        public void Populate()
        {
            _theRootNode = new MetricMinerRootNode();
            this.Nodes.Add(_theRootNode);

            _theRootNode.ImageKey = MetricMinerWidgetsTreeView.MM_REPORT_ROOT_ICON;
            _theRootNode.SelectedImageKey = MetricMinerWidgetsTreeView.MM_REPORT_ROOT_ICON;

            //invoke the delegate to notify that the tree has been populated 
            if (_theTreePopulateImpl != null)
            {
                _theTreePopulateImpl(this);
            }
            
        }

        /// <summary>
        /// Given a universal widget config, finds the node in the children nodes
        /// </summary>
        /// <param name="config"></param>
        /// <returns></returns>
        public TreeNode getNodeForConfig(UniversalWidgetConfig config)
        {
            if (_theRootNode != null)
            {
                foreach(TreeNode treeNode in _theRootNode.Nodes)
                {
                    if (treeNode is MetricMinerReportFolder)
                    {
                        MetricMinerReportFolder folderNode = (MetricMinerReportFolder)treeNode;
                        TreeNode destNode = folderNode.getNodeForConfig(config);
                        if (destNode != null)
                        {
                            return destNode;
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
            }
            return null;
        }

        #region Public properties
        public MetricMinerRootNode RootNode
        {
            get { return _theRootNode; }
            set { _theRootNode = value; }
        }

        public OnTreePopulate TreePopulateImpl
        {
            get { return _theTreePopulateImpl; }
            set { _theTreePopulateImpl = value; }
        }

        #endregion Public properties

        #region Public methods
        #endregion Public methods

        #region IDisposable Members

        void Dispose()
        {
            base.Dispose(true);
        }

        #endregion
    }
}
