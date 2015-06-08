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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Favorites;

namespace Trafodion.Manager.Framework.Navigation
{
    public partial class NavigationUserControl : UserControl
    {
        public virtual NavigationTreeConnectionFolderFactory TheConnectionFolderFactory 
        {
            get { return new NavigationTreeConnectionFolderFactory(); }
        }

        public NavigationUserControl()
        {
            InitializeComponent();

            TrafodionToolStripRenderer toolStripRenderer = new TrafodionToolStripRenderer(new CustomProfessionalColors(), false);
            this.theFavoritesLabelStrip.Renderer = toolStripRenderer;
            this.theNavigationLabelStrip.Renderer = toolStripRenderer;

            Persistence.PersistenceHandlers += new Persistence.PersistenceHandler(TrafodionMain_Persistence);

            theFilterButton.Hide();
        }

        void LinkNavigationAndFavorites()
        {
            // Weld the navigation tree and the favorites tree together
            TheNavigationTreeView.FavoritesTreeView = TheFavoritesTreeView;
            TheFavoritesTreeView.NavigationTreeView = TheNavigationTreeView;
        }

        public FavoritesTreeView TheFavoritesTreeView
        {
            get { return this.theFavoritesTreeViewUserControl.FavoritesTreeView; }
            set 
            { 
                this.theFavoritesTreeViewUserControl.FavoritesTreeView = value;
                if (value != null)
                {
                    LinkNavigationAndFavorites();
                }
            }
        }

        public NavigationTreeView TheNavigationTreeView
        {
            get { return this.theNavigationTreeViewUserControl.NavigationTreeView; }
            set 
            { 
                this.theNavigationTreeViewUserControl.NavigationTreeView = value;
                if (value != null)
                {
                    LinkNavigationAndFavorites();
                    TheNavigationTreeView.NavigationTreeConnectionFolderFactory = TheConnectionFolderFactory;
                    TheNavigationTreeView.SetAndPopulateRootFolders();
                }
            }
        }

        public virtual string ThePersistenceKey
        {
            get { return "NavigationTreePersistence"; }
        }

        void TrafodionMain_Persistence(Dictionary<string, object> aDictionary, Persistence.PersistenceOperation aPersistenceOperation)
        {
            switch (aPersistenceOperation)
            {
                case Persistence.PersistenceOperation.Load:
                    {
                        if (!aDictionary.ContainsKey(ThePersistenceKey))
                        {
                            return;
                        }
                        FavoritesTreeViewReflection theFavoritesTreeViewReflection = aDictionary[ThePersistenceKey] as FavoritesTreeViewReflection;
                        if (theFavoritesTreeViewReflection != null)
                        {
                            FavoritesTreeView theNewFavoritesTreeView;
                            try
                            {
                                theNewFavoritesTreeView = theFavoritesTreeViewReflection.MakeFavoritesTreeView(TheNavigationTreeView);
                            }
                            catch (Exception e)
                            {
                                MessageBox.Show(Utilities.GetForegroundControl(), e.Message, Properties.Resources.Error, MessageBoxButtons.OK);
                                return;
                            }

                            theFavoritesTreeViewUserControl.Controls.Remove(TheFavoritesTreeView);

                            TheFavoritesTreeView = theNewFavoritesTreeView;

                            TheNavigationTreeView.FavoritesTreeView = theNewFavoritesTreeView;

                            theNewFavoritesTreeView.Dock = DockStyle.Fill;
                            theFavoritesTreeViewUserControl.Controls.Add(theNewFavoritesTreeView);
                            theNewFavoritesTreeView.Nodes[0].Expand();
                        }
                        break;
                    }
                case Persistence.PersistenceOperation.Save:
                    {
                        FavoritesTreeViewReflection theFavoritesTreeViewReflection = new FavoritesTreeViewReflection(TheFavoritesTreeView);
                        aDictionary[ThePersistenceKey] = theFavoritesTreeViewReflection;
                        break;
                    }
            }
        }

        public void LoadFavoritesTreeViewReflection(FavoritesTreeViewReflection aFavoritesTreeViewReflection)
        {
            FavoritesTreeView theNewFavoritesTreeView;
            try
            {
                theNewFavoritesTreeView = aFavoritesTreeViewReflection.MakeFavoritesTreeView(TheNavigationTreeView);
            }
            catch (Exception e)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), e.Message, Properties.Resources.Error, MessageBoxButtons.OK);
                return;
            }

            theFavoritesTreeViewUserControl.Controls.Remove(TheFavoritesTreeView);

            TheFavoritesTreeView = theNewFavoritesTreeView;

            TheNavigationTreeView.FavoritesTreeView = theNewFavoritesTreeView;

            theNewFavoritesTreeView.Dock = DockStyle.Fill;
            theFavoritesTreeViewUserControl.Controls.Add(theNewFavoritesTreeView);
        }
    }

    /// <summary>
    /// Custom colors to get the XP look and style for the Area buttons
    /// </summary>
    public class CustomProfessionalColors : TrafodionColorTable
    {
        public override Color ToolStripGradientBegin
        {
            get
            {
                return base.ToolStripPanelGradientBegin;
            }
        }
        public override Color ToolStripGradientMiddle
        {
            get
            {
                return base.ToolStripPanelGradientEnd;
            }
        }
        public override Color ToolStripGradientEnd
        {
            get
            {
                return base.ToolStripPanelGradientEnd;
            }
        }
    }

}
