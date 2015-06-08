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
using System.ComponentModel;
using System.Globalization;
using System.Reflection;

namespace Trafodion.Manager.WorkloadArea.Model
{
	public class WorkloadEnumConverter : System.ComponentModel.EnumConverter {

		public WorkloadEnumConverter(Type type) : base(type) {
			//  Console.WriteLine("NCCEnumConverter Constructor : : Type = " + type.ToString());
		}

		public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
			//  Console.WriteLine("NCCEnumConverter CanConvertFrom : : Type = " + sourceType.ToString());
			if (isConversionSupported(sourceType))
				return true;

			return base.CanConvertFrom(context, sourceType);
		}


		public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
#if DEBUG
			Console.WriteLine("NCCEnumConverter CanConvertTo : : Type = " + destinationType.ToString());
#endif
			if (isConversionSupported(destinationType))
				return true;

			return base.CanConvertTo(context, destinationType);
		}

	


		public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
#if DEBUG
			 Console.WriteLine("NCCEnumConverter ConvertFrom : : val = " + value);
#endif

			if (value == null)
				return null;

			Type valType = value.GetType();

			if (base.EnumType == valType)
				return getDescriptiveAnnotation(value);
			else if (typeof(String) == valType)
				return convertDescriptionToEnum(value as String);

			return base.ConvertFrom(context, culture, value);
		}


		public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture,
										 object value, Type destinationType) {
#if DEBUG
			Console.WriteLine("NCCEnumConverter ConvertTo : : val = " + value + ", destType = " + destinationType.ToString() );
#endif

			if (value == null)
				return null;

			if ((typeof(String) == destinationType) && isConversionSupported(value.GetType()))
				return getDescriptiveAnnotation(value);

			return base.ConvertTo(context, culture, value, destinationType);
		}



		private bool isConversionSupported(Type theOtherType) {
			Type[] supportedTypes = { base.EnumType, typeof(String) };

			foreach (Type usableType in supportedTypes)
				if (usableType == theOtherType)
					return true;


			return false;
		}


		private String getDescriptiveAnnotation(Object value) {
			FieldInfo theFieldInfo = base.EnumType.GetField(value.ToString() );
			if (null == theFieldInfo)
				return value.ToString();


			Object[] attributeArray = theFieldInfo.GetCustomAttributes(typeof(DescriptionAttribute), false);
			if (0 < attributeArray.Length) {
				try {
					DescriptionAttribute da = (DescriptionAttribute) attributeArray[attributeArray.Length - 1];
					return da.Description;

				} catch (Exception) {
				}
			}

			return value.ToString();

		}



		private Object convertDescriptionToEnum(String descriptionValue) {
			if (null == descriptionValue)
				return	null;
			
			FieldInfo[] fields = base.EnumType.GetFields();
			foreach (FieldInfo fi in fields) {
				String  fieldName = fi.Name;
				Object[] attributeArray = fi.GetCustomAttributes(typeof(DescriptionAttribute), false);
				if (0 < attributeArray.Length) {
					try {
						DescriptionAttribute da = (DescriptionAttribute)attributeArray[attributeArray.Length - 1];
						if (da.Description.Equals(descriptionValue) )
							return fi.GetValue(fieldName);

					} catch (Exception) {
					}
				}

				// Check if the description is the name. 
				if (descriptionValue.Equals(fi.Name))
					return fi.GetValue(fieldName);

			}


			return  descriptionValue;
		}


	}
}
