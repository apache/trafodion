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

namespace Trafodion.Data.Installation
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Configuration;
    using System.Configuration.Install;
    using System.IO;
    using System.Reflection;
    using System.Windows.Forms;
    using System.Xml;
    using Microsoft.Win32;

    [RunInstaller(true)]
    public class InstallHelper : System.Configuration.Install.Installer
    {
        private const string Invariant = "Trafodion.Data";
        private static readonly string FactoryType = typeof(TrafodionDbFactory).AssemblyQualifiedName;

        private static readonly Assembly HPDataTrafodionDb = Assembly.GetAssembly(typeof(Trafodion.Data.TrafodionDbFactory));
        private static readonly Assembly HPDataTrafodionDbVisualStudio = Assembly.GetAssembly(typeof(Trafodion.Data.VisualStudio.AssemblyReference));

        private static readonly string[] MachingConfigs = 
        {
            @"Framework\v2.0.50727\CONFIG\machine.config",
            @"Framework64\v2.0.50727\CONFIG\machine.config",
            @"Framework\v4.0.30319\Config\machine.config",
            @"Framework64\v4.0.30319\Config\machine.config"
        };
        private static readonly string[] VersionList = 
        {
            @"9.0",
            @"10.0"
        };
        private static string targetpath;

        public InstallHelper()
        {
        }

        public override void Install(IDictionary savedState)
        {
            base.Install(savedState);
            targetpath=Context.Parameters[@"targetdir"];
            savedState[@"targetdir"] = targetpath;
        }

        public override void Uninstall(IDictionary savedState)
        {
            base.Uninstall(savedState);

            try
            {
                UninstallMachingConfig();
                UninstallReferenceDialog();
                UninstallDDEX();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }
        
        public override void Commit(IDictionary savedState)
        {
            base.Commit(savedState);

            try
            {
                //remove any entries from the machine.config to make sure we dont get duplicate entries
                UninstallMachingConfig();

                InstallMachineConfig();
                InstallReferenceDialog(savedState);
                InstallDDEX(savedState);
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }

        public override void Rollback(IDictionary savedState)
        {
            base.Rollback(savedState);
        }

        private void InstallMachineConfig()
        {
            try
            {
                string name = ((AssemblyTitleAttribute)HPDataTrafodionDb.GetCustomAttributes(
                   typeof(AssemblyTitleAttribute), false)[0]).Title;
                string description = ((AssemblyDescriptionAttribute)HPDataTrafodionDb.GetCustomAttributes(
                    typeof(AssemblyDescriptionAttribute), false)[0]).Description;

                //load machine.config
                string[] paths = GetMachineConfigs();

                for (int i = 0; i < paths.Length; i++)
                {
                    XmlDocument doc = new XmlDocument();
                    doc.Load(paths[i]);

                    //add factory entry
                    XmlElement root = doc.SelectSingleNode("/configuration/system.data/DbProviderFactories") as XmlElement;
                    //create the new entry
                    XmlElement e = doc.CreateElement("add");
                    XmlAttribute a = doc.CreateAttribute("name");
                    a.Value = name;
                    e.Attributes.Append(a);

                    a = doc.CreateAttribute("invariant");
                    a.Value = Invariant;
                    e.Attributes.Append(a);

                    a = doc.CreateAttribute("description");
                    a.Value = description;
                    e.Attributes.Append(a);

                    a = doc.CreateAttribute("type");
                    a.Value = String.Concat(FactoryType);
                    e.Attributes.Append(a);

                    root.AppendChild(e);

                    //add section entry
                    root = doc.SelectSingleNode("/configuration/configSections") as XmlElement;
                    e = doc.CreateElement("section");
                    a = doc.CreateAttribute("name");
                    a.Value = "Trafodion.Data";
                    e.Attributes.Append(a);

                    a = doc.CreateAttribute("type");
                    a.Value = "System.Data.Common.DbProviderConfigurationHandler, System.Data, Version=2.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";
                    e.Attributes.Append(a);

                    root.AppendChild(e);

                    doc.Save(paths[i]);
                }
            }
            catch (Exception e)
            {
                throw new InstallException("An error occured updating machine.confg", e);
            }
        }

        private void UninstallMachingConfig()
        {
            try
            {
                //load machine.config
                string[] paths = GetMachineConfigs();
                for (int i = 0; i < paths.Length; i++)
                {
                    string factoryPath = String.Concat("/configuration/system.data/DbProviderFactories/add[@invariant='", Invariant, "']");
                    string sectionPath = String.Concat("/configuration/configSections/section[@name='Trafodion.Data']");

                    XmlDocument doc = new XmlDocument();
                    doc.Load(paths[i]);
                    XmlElement e = doc.SelectSingleNode(factoryPath) as XmlElement;
                    if (e != null)
                    {
                        e.ParentNode.RemoveChild(e);
                    }

                    e = doc.SelectSingleNode(sectionPath) as XmlElement;
                    if (e != null)
                    {
                        e.ParentNode.RemoveChild(e);
                    }

                    doc.Save(paths[i]);
                }
            }
            catch (Exception e)
            {
                throw new InstallException("An error occured updating maching.config", e);
            }
        }

        private void InstallReferenceDialog(IDictionary savedState)
        {
            string keyPath = @"SOFTWARE\Microsoft\.NETFramework\AssemblyFolders\Trafodion ADO.NET 3.0 Provider";
            RegistryKey key = Registry.LocalMachine.CreateSubKey(keyPath);
            key.SetValue(null, Path.GetDirectoryName(HPDataTrafodionDb.Location));

            keyPath = @"SOFTWARE\Microsoft\.NETFramework\v2.0.50727\AssemblyFoldersEx\Trafodion ADO.NET 3.0 Provider";
            key = Registry.LocalMachine.CreateSubKey(keyPath);
            key.SetValue(null, Path.GetDirectoryName(HPDataTrafodionDb.Location));

            keyPath = @"SOFTWARE\Microsoft\.NETFramework\v4.0.30319\AssemblyFoldersEx\Trafodion ADO.NET 3.0 Provider";
            key = Registry.LocalMachine.CreateSubKey(keyPath);
            key.SetValue(null, Path.GetDirectoryName(HPDataTrafodionDb.Location));
        }

        private void UninstallReferenceDialog()
        {
            string keyPath = @"SOFTWARE\Microsoft\.NETFramework\AssemblyFolders\Trafodion ADO.NET 3.0 Provider";
            RegistryKey key = Registry.LocalMachine.OpenSubKey(keyPath);
            if (key != null)
            {
                key.Close();
                Registry.LocalMachine.DeleteSubKeyTree(keyPath);
            }

            keyPath = @"SOFTWARE\Microsoft\.NETFramework\v2.0.50727\AssemblyFoldersEx\Trafodion ADO.NET 3.0 Provider";
             key = Registry.LocalMachine.OpenSubKey(keyPath);
            if (key != null)
            {
                key.Close();
                Registry.LocalMachine.DeleteSubKeyTree(keyPath);
            }

            keyPath = @"SOFTWARE\Microsoft\.NETFramework\v4.0.30319\AssemblyFoldersEx\Trafodion ADO.NET 3.0 Provider";
            key = Registry.LocalMachine.OpenSubKey(keyPath);
            if (key != null)
            {
                key.Close();
                Registry.LocalMachine.DeleteSubKeyTree(keyPath);
            }

        }
        
        private void InstallDDEX(IDictionary savedState)
        {

            foreach (string version in VersionList)
            {
                if (Registry.LocalMachine.OpenSubKey(@"Software\Microsoft\VisualStudio\"+version) == null)
                {
                    return;
                }

                //TODO: FIX THESE
                RegistryKey root = Registry.LocalMachine;
                RegistryKey key;
                

                //***Datasource***
                string keyPath = String.Format(@"Software\Microsoft\VisualStudio\{0}\DataSources\{1}",
                    version, GuidList.TrafodionDbDatasource);

                key = root.CreateSubKey(keyPath);
                key.SetValue(null, "Trafodion Database");
                key.SetValue("DefaultProvider", GuidList.TrafodionDbProvider);

                key = key.CreateSubKey("SupportingProviders").CreateSubKey(GuidList.TrafodionDbProvider);
                key.SetValue("Description", "Provider_Description, Trafodion.Data.VisualStudio.Properties.Resources");
                key.SetValue("DisplayName", "DataSource_DisplayName, Trafodion.Data.VisualStudio.Properties.Resources");

                //***Data Provider***
                keyPath = String.Format(@"Software\Microsoft\VisualStudio\{0}\DataProviders\{1}",
                    version, GuidList.TrafodionDbProvider);

                key = root.CreateSubKey(keyPath);
                key.SetValue(null, ".NET Framework Data Provider for Trafodion Database");
                key.SetValue("Assembly", HPDataTrafodionDb.FullName);
                key.SetValue("AssociatedSource", GuidList.TrafodionDbDatasource);
                key.SetValue("Description", "Provider_Description, Trafodion.Data.VisualStudio.Properties.Resources");
                key.SetValue("DisplayName", "Provider_DisplayName, Trafodion.Data.VisualStudio.Properties.Resources");
                //key.SetValue("Codebase", savedState[@"targetdir"].ToString()+ @"Trafodion.Data.dll");
                key.SetValue("FactoryService", GuidList.TrafodionDbProviderObjectFactory);
                key.SetValue("InvariantName", "Trafodion.Data");
                key.SetValue("ShortDisplayName", "Provider_ShortDisplayName, Trafodion.Data.VisualStudio.Properties.Resources");
                key.SetValue("Technology", "{77AB9A9D-78B9-4ba7-91AC-873F5338F1D2}");

                key = key.CreateSubKey("SupportedObjects");

                key.CreateSubKey("DataConnectionPromptDialog");
                key.CreateSubKey("DataConnectionProperties");
                key.CreateSubKey("DataConnectionSupport");
                key.CreateSubKey("DataConnectionUIControl");
                key.CreateSubKey("DataSourceInformation");
                key.CreateSubKey("DataObjectSupport");
                key.CreateSubKey("DataViewSupport");

                //Service
                keyPath = String.Format(@"Software\Microsoft\VisualStudio\{0}\Services\{1}",
                    version, GuidList.TrafodionDbProviderObjectFactory);

                key = root.CreateSubKey(keyPath);
                key.SetValue(null, GuidList.TrafodionDbDataPackage);
                key.SetValue("Name", "TrafodionDbProviderObjectFactory");

                //Package
                keyPath = String.Format(@"Software\Microsoft\VisualStudio\{0}\Packages\{1}",
                    version, GuidList.TrafodionDbDataPackage);

                key = root.CreateSubKey(keyPath);
                key.SetValue(null, @"Trafodion ADO.NET 3.0 Package");
                key.SetValue("Assembly", HPDataTrafodionDbVisualStudio.FullName);
                key.SetValue("Class", "Trafodion.Data.VisualStudio.TrafodionDbDataPackage");
                //key.SetValue("Codebase", savedState[@"targetdir"].ToString()+ @"Trafodion.Data.VisualStudio.dll");
                key.SetValue("CompanyName", "Hewlett-Packard");
                key.SetValue("ID", 100, RegistryValueKind.DWord);
                key.SetValue("InProcServer32", Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "mscoree.dll"));
                key.SetValue("MinEdition", "Standard");
                key.SetValue("ProductName", "Trafodion ADO.NET 3.0 Package");
                key.SetValue("ProductVersion", "3.0.0.0");
                //key.SetValue("ThreadingModel", "Both");

                // Installed products
                keyPath = String.Format(@"Software\Microsoft\VisualStudio\{0}\InstalledProducts\Trafodion ADO.NET 3.0 Package",
                    version);
                key = root.CreateSubKey(keyPath);
                key.SetValue(null, "#101"); //title
                key.SetValue("Package", GuidList.TrafodionDbDataPackage);
                key.SetValue("ProductDetails", "#102");
                key.SetValue("LogoID", "#200");
                key.SetValue("PID", "#103");
            }
        }

        private void UninstallDDEX()
        {
            foreach (string version in VersionList)
            {
                //TODO: FIX THESE
                RegistryKey rootKey = Registry.LocalMachine;

                //Datasource
                string keyPath = String.Format(@"Software\Microsoft\VisualStudio\{0}\DataSources\{1}",
                    version, GuidList.TrafodionDbDatasource);
                RegistryKey key = rootKey.OpenSubKey(keyPath);
                if (key != null)
                {
                    key.Close();
                    rootKey.DeleteSubKeyTree(keyPath);
                }

                //Data Provider
                keyPath = String.Format(@"Software\Microsoft\VisualStudio\{0}\DataProviders\{1}",
                    version, GuidList.TrafodionDbProvider);
                key = rootKey.OpenSubKey(keyPath);
                if (key != null)
                {
                    key.Close();
                    rootKey.DeleteSubKeyTree(keyPath);
                }

                //Service
                keyPath = String.Format(@"Software\Microsoft\VisualStudio\{0}\Services\{1}",
                    version, GuidList.TrafodionDbProviderObjectFactory);
                key = rootKey.OpenSubKey(keyPath);
                if (key != null)
                {
                    key.Close();
                    rootKey.DeleteSubKeyTree(keyPath);
                }

                //Package
                keyPath = String.Format(@"Software\Microsoft\VisualStudio\{0}\Packages\{1}",
                    version, GuidList.TrafodionDbDataPackage);
                key = rootKey.OpenSubKey(keyPath);
                if (key != null)
                {
                    key.Close();
                    rootKey.DeleteSubKeyTree(keyPath);
                }

                //Installed Products
                keyPath = String.Format(@"Software\Microsoft\VisualStudio\{0}\InstalledProducts\Trafodion ADO.NET 3.0 Package",
                    version);
                key = rootKey.OpenSubKey(keyPath);
                if (key != null)
                {
                    key.Close();
                    rootKey.DeleteSubKeyTree(keyPath);
                }
            }
        }

        private static string[] GetMachineConfigs()
        {
            // find the current machine.config and move up 3 directories to get the root of all 
            // the installed frameworks
            DirectoryInfo currentConfig = new DirectoryInfo(Path.GetDirectoryName(ConfigurationManager.OpenMachineConfiguration().FilePath));
            string root = currentConfig.Parent.Parent.Parent.FullName;

            List<string> paths = new List<string>(4);
            string s;

            // check all the possible release configurations
            foreach (string config in MachingConfigs)
            {
                s = Path.Combine(root, config);
                if (File.Exists(s))
                {
                    paths.Add(s);
                }
            }

            return paths.ToArray();
        }
    }
}
