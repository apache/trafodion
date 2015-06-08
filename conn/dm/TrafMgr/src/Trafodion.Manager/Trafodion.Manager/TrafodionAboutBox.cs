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
using System.Reflection;
using Trafodion.Manager.Framework.Controls;
using System.Diagnostics;
using System.Windows.Forms;

namespace Trafodion.Manager
{
    public partial class TrafodionAboutBox : TrafodionForm
    {
        public static List<Assembly> _loadedAssemblies = new List<Assembly>(); 


        public TrafodionAboutBox()
        {
            InitializeComponent();
            this.Text = "About";
            this.labelProductName.Text = AssemblyProduct;
            this.VProcLabel.Text = AssemblyDescription;
            this.labelCopyright.Text = AssemblyCopyright;
            this.labelCompanyName.Text = AssemblyCompany;
            //this.DetailsTextBox.Text = "Trafodion Database Manager is a client application used to manage the Trafodion database.";

            // Add the framework first.
            //this.LoadedAssemblieslistBox.Items.Add("Trafodion.Manager Framework" + "  " + AssemblyVersion);
 
            //foreach (Assembly assem in _loadedAssemblies)
               //this.LoadedAssemblieslistBox.Items.Add(getAssemblyTitle(assem) + "  " + getAssemblyVersion(assem));

            CenterToScreen();
        }

        #region Assembly Attribute Accessors

        public string getAssemblyTitle(Assembly assem)
        {

            object[] attributes = assem.GetCustomAttributes(typeof(AssemblyTitleAttribute), false);
            if (attributes.Length > 0)
            {
                AssemblyTitleAttribute titleAttribute = (AssemblyTitleAttribute)attributes[0];
                if (titleAttribute.Title != "")
                {
                    return titleAttribute.Title;
                }
            }
            return System.IO.Path.GetFileNameWithoutExtension(assem.CodeBase);

        }
       
        public string AssemblyTitle 
        {
            get
            {
                object[] attributes = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyTitleAttribute), false);
                if (attributes.Length > 0)
                {
                    AssemblyTitleAttribute titleAttribute = (AssemblyTitleAttribute)attributes[0];
                    if (titleAttribute.Title != "")
                    {
                        return titleAttribute.Title;
                    }
                }
                return System.IO.Path.GetFileNameWithoutExtension(Assembly.GetExecutingAssembly().CodeBase);
            }
        }

        public string getAssemblyVersion(Assembly assem)
        {

            {
                return assem.GetName().Version.ToString();
            }
        }

        public string AssemblyVersion
        {
            get
            {
                return Assembly.GetExecutingAssembly().GetName().Version.ToString();
            }
        }

        public string getAssemblyDescription (Assembly assem)
        {

            object[] attributes = assem.GetCustomAttributes(typeof(AssemblyDescriptionAttribute), false);
             if (attributes.Length == 0)
              {
                  return "";
              }
             return ((AssemblyDescriptionAttribute)attributes[0]).Description;
            
        }

        public string AssemblyDescription
        {
            get
            {
                object[] attributes = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyDescriptionAttribute), false);
                if (attributes.Length == 0)
                {
                    return "";
                }
                return ((AssemblyDescriptionAttribute)attributes[0]).Description;
            }
        }

        public string AssemblyProduct
        {
            get
            {
                object[] attributes = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyProductAttribute), false);
                if (attributes.Length == 0)
                {
                    return "";
                }
                return ((AssemblyProductAttribute)attributes[0]).Product;
            }
        }

        public string AssemblyCopyright
        {
            get
            {
                object[] attributes = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCopyrightAttribute), false);
                if (attributes.Length == 0)
                {
                    return "";
                }
                return ((AssemblyCopyrightAttribute)attributes[0]).Copyright;
            }
        }

        public string AssemblyCompany
        {
            get
            {
                object[] attributes = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCompanyAttribute), false);
                if (attributes.Length == 0)
                {
                    return "";
                }
                return ((AssemblyCompanyAttribute)attributes[0]).Company;
            }
        }

        /// <summary>
        /// ProductVersion: product version of the product
        /// </summary>
        public string ClientProductVersion
        {
            get
            {
                return Application.ProductVersion;
            }
        }

        #endregion

        private void okButton_Click(object sender, EventArgs e)
        {
            this.Hide();
        }


        private void LoadedAssemblieslistBox_SelectedIndexChanged_1(object sender, EventArgs e)
        {
            //int nSelectedIndex = this.LoadedAssemblieslistBox.SelectedIndex;
            //if (0 == nSelectedIndex)
            //{
                // Handle the framework description first.
               // this.DetailsTextBox.Text = "The TrafodionManager Framework.";
               // return;
            //}

           // this.DetailsTextBox.Text = getAssemblyDescription(_loadedAssemblies[nSelectedIndex-1]);

        }

        private void tableLayoutPanel_Paint(object sender, PaintEventArgs e)
        {

        }
    }
}
