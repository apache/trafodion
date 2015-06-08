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
using System.Text;
using System.Text.RegularExpressions;

namespace Trafodion.Manager.ConnectivityArea.Model
{
	/// <summary>
	/// Model for a valid NDCS identifier
	/// </summary>
	public class NDCSName
    {
        #region Public variables
        /// <summary>
        /// Minimum length for a NDCS object name
        /// </summary>
		public const int MinimumNameLength = 1;
        /// <summary>
        /// Maximum length for a NDCS object name
        /// </summary>
        public const int MaximumNameLength = 128;

        /// <summary>
        /// Special characters allowed in sql identifier
        /// </summary>
        public const string AllowedSpecialCharacters = "~-!#&_%'`()*+,-:;<=>?[]|${}\"";

        /// <summary>
        /// Array of allowed special characters
        /// </summary>
		static public char[] SpecialCharactersList = AllowedSpecialCharacters.ToCharArray();

        #endregion Public variables

        #region Private member variables

        string theName;

        #endregion Private member variables

        /// <summary>
        /// Constructs a ndcs identifier
        /// </summary>
        /// <param name="aName"></param>
        public NDCSName(string aName)
		{
			theName = aName;
		}
    
        /// <summary>
        /// Trims and upper cases the string
        /// </summary>
        /// <param name="aString"></param>
        /// <returns></returns>
		static public string TrimAndShiftUp(string aString)
		{
			string theString = aString.Trim();
        
			if ((theString.Length > 0) && theString.StartsWith("\""))
			{
				return theString.ToUpper();
			}
        
			return theString;
		}
    
        /// <summary>
        /// </summary>
        /// <returns></returns>
		public string TrimAndShiftUp()
		{
			return TrimAndShiftUp(theName);
		}
    
        public static string GetDelimitedName(string aName)
        {
            string delimitedName = "";
            if (String.IsNullOrEmpty(aName) || (aName.StartsWith("\"")) && (aName.EndsWith("\"")))
            {
                delimitedName = aName;
            }
            else
            {
                delimitedName = aName.Replace("\"", "\"\"");
                delimitedName = "\"" + delimitedName + "\"";

                //escape all single quotes with an additional single quote
                delimitedName = delimitedName.Replace("'", "''");
            }

            return delimitedName;
        }
        /// <summary>
        /// Validates the name and fills the error message holder with a valid error message if the validation fails.
        /// </summary>
        /// <param name="aName"></param>
        /// <param name="anErrorMessage"></param>
        /// <returns></returns>
		static public bool Validate(string aName, StringBuilder anErrorMessage)
		{
			// Sanitize the string no leading/trailing blank and get its length.
			string theSanitizedName = TrimAndShiftUp(aName);
			int theSanitizedNameLength = theSanitizedName.Length;
        
			anErrorMessage.Length = 0;
        
			// Validate the length
			if (theSanitizedNameLength < MinimumNameLength)
			{
				anErrorMessage.Append(Properties.Resources.NameCannotBeEmpty);
				return false;
			}
        
			// Validate the length
			if (theSanitizedNameLength > MaximumNameLength)
			{
				anErrorMessage.Append(string.Format(Properties.Resources.NameCannotExceedLength, MaximumNameLength));
				return false;
			}
        
			// Check for delimited form or not
			if (!theSanitizedName.StartsWith("\""))
			{
            
				// Validate the first character
				if (!Char.IsLetter(theSanitizedName[0]))
				{
					anErrorMessage.Append(Properties.Resources.NameShouldBeginWithLetter);
					return false;
				}
            
				// Validate the rest of the characters
				for (int theOffset = 1; theOffset < theSanitizedNameLength; theOffset++)
				{
					char theChar = theSanitizedName[theOffset];
					if (!Char.IsLetterOrDigit(theChar) && (theChar != '_'))
					{
						anErrorMessage.Append(Properties.Resources.NameCanContainCharacters);
						return false;
					}
				}
            
				// If we get here, it's a legal name
				return true;
            
			}
        
			// If we get here, it's a delimited name.  Internal double-quotes must be doubled.
			if (theSanitizedNameLength == 1)
			{
            
				// It's a lone double quote - not legal but no error message
				return false;
            
			}
        
			bool nonBlankFound = false;
			bool nonSpecialFound = false;
        
			// Check for embedded double quotes and illegal characters
			for (int theOffset = 1; theOffset < (theSanitizedNameLength - 1); theOffset++)
			{
				char theCurrentChar = theSanitizedName[theOffset];
            
				// First character after first " must not be \ or $
				if ((theOffset == 1) && ((theCurrentChar == '\\') || (theCurrentChar == '$')))
				{
                    anErrorMessage.Append(Properties.Resources.DelimitedNameStartingChar);
					return false;
				}
            
				// Check for non-special found
				if (Char.IsLetterOrDigit(theCurrentChar) || (theCurrentChar == '_') || (theCurrentChar == ' '))
				{
					nonSpecialFound = true;
				}
            
				// Check for non-blank found
				if (theCurrentChar != ' ')
				{
					nonBlankFound = true;
				}
            
				if (theCurrentChar == '\"')
				{
					theOffset++;
					if (theSanitizedName[theOffset] != '\"' || (theOffset == (theSanitizedNameLength - 1)))
					{
						anErrorMessage.Append(Properties.Resources.DoubledEmbeddedQuotes);
						return false;
					}
                
					continue;
                
				}
            
				if (!LegalCharacterInDelimitedIdentifier(theCurrentChar, anErrorMessage))
				{
					return false;
				}
            
			}
        
			// Must end with a double quote
			char theLastCharacter = theSanitizedName[theSanitizedNameLength - 1];
			if (theLastCharacter != '\"')
			{
				// Check the last character for legailty and show an error message if it isn't
				LegalCharacterInDelimitedIdentifier(theLastCharacter, anErrorMessage);
            
				// Regardless, the name isn't legal because there's no close double quote but
				// we don't badger the user with an error message for that.
				return false;
            
			}
        
			// Must not end with space(s) before the const double quote double quote
			if (theSanitizedNameLength > 2)
			{
				char theNextToLastCharacter = theSanitizedName[theSanitizedNameLength - 2];
				if (theNextToLastCharacter == ' ')
				{
					anErrorMessage.Append(Properties.Resources.NoTrailingSpaceinDelimitedName);
					return false;
				}
			}
        
			if (!nonSpecialFound)
			{
				anErrorMessage.Append(Properties.Resources.DelimitedNameNotAllSpecialChars);
				return false;
			}
        
			if (!nonBlankFound)
			{
				anErrorMessage.Append(Properties.Resources.NonBlankCharinDelimitedName);
				return false;
			}
        
			// If we get here, it's a legal delimited identifier
			return true;
        
		}
    
        /// <summary>
        /// </summary>
        /// <param name="anErrorMessage"></param>
        /// <returns></returns>
		public bool Validate(StringBuilder anErrorMessage)
		{
			return Validate(theName, anErrorMessage);
		}
    
        /// <summary>
        /// Identifies if the character is a legal character that is allowed in a delimited name
        /// </summary>
        /// <param name="aCharacter"></param>
        /// <param name="anErrorMessage"></param>
        /// <returns></returns>
		static private bool LegalCharacterInDelimitedIdentifier(char aCharacter, StringBuilder anErrorMessage)
		{
        
			// Check for letter, digit, or blank.  Check for blank here so it's not in the specials array
			// where it would disappear in the error message.
			if (Char.IsLetterOrDigit(aCharacter) || (aCharacter == ' '))
			{
				return true;
			}
        
			// Check for legal special characters
            for (int theSpecialsOffset = 0; (theSpecialsOffset < SpecialCharactersList.Length); theSpecialsOffset++)
			{
				if (aCharacter == SpecialCharactersList[theSpecialsOffset])
				{
					return true;
				}
			}
        
			anErrorMessage.Append(string.Format(Properties.Resources.ValidSpecialCharacters, AllowedSpecialCharacters));
        
			return false;
        
		}
    
        /// <summary>
        /// Enclose the name in single quotes
        /// </summary>
        /// <param name="aLiteralString"></param>
        /// <returns></returns>
		public static string EncloseInSingleQuotes(string aLiteralString)
		{
			return "'" + aLiteralString.Replace("'", "''") + "'";
		}

        public static string EscapeSingleQuotes(string aLiteralString)
        {
            return aLiteralString.Replace("'", "''");
        }

        public static string EscapeDoubleQuotes(string aLiteralString)
        {
            return aLiteralString.Replace("\"", "\"\"");
        }
        
        /// <summary>
        /// Returns a Identifier's external form.  
		/// </summary>
		/// <param name="anInternalName"></param>
		/// <returns></returns>
 		public static string ExternalForm(string anInternalName)
		{
			string theName = anInternalName.Trim();
        
			// May be empty?
			if (theName.Length == 0)
			{
				return "";
			}

			// If it contains specials, it needs to be delimited.
			if (Regex.IsMatch(theName, "^[a-zA-Z0-9_-]+$"))
			{

				// No specials, it's itself
				return theName;

			}

			// It has specials; delimit it.
            return GetDelimitedName(theName);

		}
    
		/// <summary>
        /// Returns a Identifiers internal form.  
		/// </summary>
		/// <param name="anExternalName"></param>
		/// <returns></returns>
 		public static string InternalForm(string anExternalName)
		{
			int theLength = anExternalName.Length;

			if ((theLength > 1) && (anExternalName.StartsWith("\"")) && (anExternalName.EndsWith("\"")))
			{
                string internalName = anExternalName.Substring(1, theLength - 2);

                if (Regex.IsMatch(internalName, "^[a-zA-Z0-9_-]+$"))
                    return internalName;
			}
            return anExternalName;
        }
    
        /// <summary>
        /// Checks if internal and external names are same
        /// </summary>
        /// <param name="anIneternalOrExternalName"></param>
        /// <returns></returns>
		static public bool ExternalInternalFormSame(string anIneternalOrExternalName)
		{
            string internalFormName = InternalForm(anIneternalOrExternalName);
            string externalFormName = ExternalForm(internalFormName);
			return (externalFormName.Equals(internalFormName));
		}

        /// <summary>
        /// Displays the identifier name
        /// </summary>
        /// <returns></returns>
		override public string ToString()
		{
			return theName;
		}
    
        /// <summary>
        /// Checks if the identifier name is equal to the passed in name
        /// </summary>
        /// <param name="aName"></param>
        /// <returns></returns>
		public bool Equals(string aName)
		{
            return theName.Equals(aName);
		}
	}
}
