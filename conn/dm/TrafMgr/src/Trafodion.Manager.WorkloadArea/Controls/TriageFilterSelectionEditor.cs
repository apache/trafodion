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
using System.Text;
using System.Windows.Forms;
using System.Drawing;
using System.Drawing.Design;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
	public class TriageFilterSelectionEditor : UITypeEditor
	{
		public override UITypeEditorEditStyle GetEditStyle(System.ComponentModel.ITypeDescriptorContext context)
		{
			//return base.GetEditStyle(context);
			return UITypeEditorEditStyle.Modal;
		}

		public override object EditValue(System.ComponentModel.ITypeDescriptorContext context, IServiceProvider provider, object value)
		{
			//return base.EditValue(context, provider, value);
			string title = "";
			try {
				title = context.PropertyDescriptor.Name;

			} catch (Exception exc) {
                Trafodion.Manager.Framework.Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG,
                    TraceOptions.TraceArea.Monitoring,
                    TriageHelper.TRACE_SUB_AREA_NAME,
                    "Missing property description name using default. Details = " + exc.Message);

			}

			TriageFilterInfo fi = (TriageFilterInfo)context.Instance;
            TriageHelper triageHelper = fi.TriageHelper;
            TriageFilterSelectionDialog fs = new TriageFilterSelectionDialog(title, triageHelper, value);
            DialogResult result = fs.ShowDialog();
            if (result == DialogResult.OK)
            {
                value = fs.SelectedList;
            }

			return value;
		}

		public override bool GetPaintValueSupported(System.ComponentModel.ITypeDescriptorContext context)
		{
			//return base.GetPaintValueSupported(context);
			return false;
		}
	}
}
