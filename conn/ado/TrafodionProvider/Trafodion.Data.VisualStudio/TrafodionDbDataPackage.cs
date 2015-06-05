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

using System;
using System.ComponentModel.Design;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Shell;

namespace Trafodion.Data.VisualStudio
{
    [ProvideLoadKey("Standard", "3.0.0.0", "Trafodion ADO.NET 3.0 Package", "Hewlett-Packard", 100)]
    [Guid("F60A05D1-9D3B-42F3-B7E3-3D4924399FAB")]
    [DefaultRegistryRoot(@"Software\Microsoft\VisualStudio\9.0")]
    [PackageRegistration(UseManagedResourcesOnly = true)]
    [ProvideService(typeof(TrafodionDbProviderObjectFactory))]
    [ProvideAutoLoad("{f1536ef8-92ec-443c-9ed7-fdadf150da82}")]
    [InstalledProductRegistration(false, "#101", "#102", "#103", IconResourceID = 200)]
    public sealed class TrafodionDbDataPackage : Package
    {
        protected override void Initialize()
        {
            base.Initialize();

            ((IServiceContainer)this).AddService(typeof(TrafodionDbProviderObjectFactory),
                    new ServiceCreatorCallback(this.Callback), true);
        }

        public object Callback(IServiceContainer sc, Type t)
        {
            return (t == typeof(TrafodionDbProviderObjectFactory))?new TrafodionDbProviderObjectFactory():null;
        }
    }
}
