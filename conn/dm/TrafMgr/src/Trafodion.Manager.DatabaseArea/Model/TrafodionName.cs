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
using System.Data.Odbc;
using System.Text;
using System.Text.RegularExpressions;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// Model for a valid Trafodion identifier
	/// </summary>
	public class TrafodionName
    {
        #region Public variables
        /// <summary>
        /// Minimum length for a sql name
        /// </summary>
		public const int MinimumNameLength = 1;
        /// <summary>
        /// Maximum length for a sql name
        /// </summary>
        public const int MaximumNameLength = 128;

        /// <summary>
        /// Minimum length for a sql heading
        /// </summary>
        public const int MinimumHeadingLength = 1;
        /// <summary>
        /// Maximum length for a sql heading
        /// </summary>
		public const int theMaximumHeadingLength = 128;

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

        private static Hashtable theObjectTypeNameHashTable = null;
        string theName;
        string theCanonicalForm;

        #endregion Private member variables

        /// <summary>
        /// Constructs a sql identifier
        /// </summary>
        /// <param name="aName"></param>
		public TrafodionName(string aName)
		{
			theName = aName;
			theCanonicalForm = CanonicalForm(aName);
		}
    
        /// <summary>
        /// Identifies if the name is that of a system catalog
        /// </summary>
        /// <returns></returns>
		public bool IsASystemCatalogName()
		{
			return IsASystemCatalogName(theName);
		}
    
        /// <summary>
        /// Identifies if the passed name is a system catalog name
        /// </summary>
        /// <param name="aCatalogName"></param>
        /// <returns></returns>
		static public bool IsASystemCatalogName(string aCatalogName)
		{
            return aCatalogName.ToUpper().StartsWith("TRAFODION");
		}

        /// <summary>
        /// Identifies if the passed name is a system schema name
        /// </summary>
        /// <param name="aSchemaName"></param>
        /// <returns></returns>
        static public bool IsASystemSchemaName(string aSchemaName)
        {
             string UCaseSchemaName = aSchemaName.ToUpper();
             return UCaseSchemaName.StartsWith("TRAFODION_") || IsAnInternalSchemaName(aSchemaName);
        }

        /// <summary>
        /// Identifies if the passed name is an internal schema name that should be visible to users
        /// </summary>
        /// <param name="aSchemaName"></param>
        /// <returns></returns>
        static public bool IsAnInternalSchemaName(string aSchemaName)
        {
            string UCaseSchemaName = aSchemaName.ToUpper();
            return UCaseSchemaName.StartsWith("\"@") || UCaseSchemaName.StartsWith("@") || UCaseSchemaName.StartsWith("VOLATILE_SCHEMA");
        }

        /// <summary>
        /// Identifies if the passed name is an system table name
        /// </summary>
        /// <param name="aTableName"></param>
        /// <returns></returns>
        static public bool IsASystemTableName(string aTableName)
        {
            return aTableName.ToUpper().StartsWith("\"@") || aTableName.ToUpper().StartsWith("@");
        }

        /// <summary>
        /// To check whether an object is a metadata object with its fully qualified name
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aFullyQualifiedTableName"></param>
        /// <returns></returns>
        static public bool IsMetadataObject(Connection aConnection, string aFullyQualifiedTableName)
        {
            string[] names = aFullyQualifiedTableName.Split('.');
            if (names.Length < 3)
            {
                return false;
            }

            string cat_name = names[0].ToUpper();
            string sch_name = names[1].ToUpper();
            string obj_name = names[2].ToUpper();
            OdbcDataReader theReader = null;

            try
            {
                theReader = Queries.ExecuteInfoObjectSecurityClass(aConnection, cat_name, sch_name, obj_name);
                while (theReader.Read())
                {
                    string SecurityClass = theReader.GetString(3).Trim().ToUpper();

                    switch (SecurityClass)
                    {
                        case "UM": // User metadata objects
                        case "SM": // System metadata objects
                        case "MU": // MV user metadata objects
                            return true;
  
                        default:
                            return false;
                    }
                }
            }
            catch (Exception ex)
            {
                //string message = ex.Message;
                //discard any exception
            }
            finally
            {
                // Donot close the connection, the caller is responsible for the connection
                // Only clean up the reader.
                if (theReader != null)
                {
                    theReader.Close();
                }
            }

            return false;
        }
    
        /// <summary>
        /// Identifies if the name is name of a system schema 
        /// </summary>
        /// <returns></returns>
		public bool IsASystemSchemaName()
		{
			return IsASystemSchemaName(theName);
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
        /// Trims and upper cases the name
        /// </summary>
        /// <returns></returns>
		public string TrimAndShiftUp()
		{
			return TrimAndShiftUp(theName);
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
            
				// Make sure it's not a reserved word
				if (ReservedWords.IsReserved(theSanitizedName))
				{
					anErrorMessage.Append(Properties.Resources.NameNoReserverdWord);
					return false;
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
        /// Validate the current name
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
        /// Validate single quoted string
        /// </summary>
        /// <param name="aStringName"></param>
        /// <param name="aString"></param>
        /// <param name="anErrorMessage"></param>
        /// <returns></returns>
		static public bool ValidateSingleQuotedString(string aStringName, string aString, StringBuilder anErrorMessage)
		{
             
			// Sanitize the string no leading/trailing blank and get its length.
			string theSanitizedString = aString.Trim();
			int theSanitizedStringLength = theSanitizedString.Length;
        
			anErrorMessage.Length = 0;
        
			// Empty is okay
			if (theSanitizedStringLength == 0)
			{
				return true;
			}
        
			// Else must start with a single quote
			if (!theSanitizedString.StartsWith("\'"))
			{
				anErrorMessage.Append(string.Format(Properties.Resources.EmbedStringInQuotes, aStringName));
				return false;
			}
        
			// If we get here, it's a single quoted name.  Internal single quotes must be doubled.
			if (theSanitizedStringLength == 1)
			{
            
				// It's a lone single quote - not legal but no error message
				return false;
            
			}
        
			// Check for embedded single quotes
			for (int theOffset = 1; theOffset < (theSanitizedStringLength - 1); theOffset++)
			{
				char theCurrentChar = theSanitizedString[theOffset];
            
				if (theCurrentChar == '\'')
				{
					theOffset++;
					if (theSanitizedString[theOffset] != '\'' || (theOffset == (theSanitizedStringLength - 1)))
					{
						anErrorMessage.Append(string.Format(Properties.Resources.EmbedStringInQuotes, aStringName));
						return false;
					}
                
					continue;
                
				}
            
			}
        
			// Must end with a single quote
			if (!theSanitizedString.EndsWith("\'"))
			{
            
				// Regardless, the name isn't legal because there's no close single quote
				anErrorMessage.Append(string.Format(Properties.Resources.EmbedStringInQuotes, aStringName));
				return false;
            
			}
        
			// If we get here, it's a legal single quoted string
			return true;
        
		}
    
        /// <summary>
        /// Validate a sql column heading
        /// </summary>
        /// <param name="aHeading"></param>
        /// <param name="anErrorMessage"></param>
        /// <returns></returns>
		static public bool ValidateTrafodionColumnHeading(string aHeading, StringBuilder anErrorMessage)
		{
        
			// Sanitize the string no leading/trailing blank and get its length.
			string theSanitizedHeading = aHeading.Trim();
			int theSanitizedHeadingLength = theSanitizedHeading.Length;
        
			anErrorMessage.Length = 0;
        
			// Validate the length
			if (theSanitizedHeadingLength < MinimumHeadingLength)
			{
				anErrorMessage.Append("The heading must not be empty.");
				return false;
			}
        
			// Check for single quotes
			bool quoteNeeded = false;
			int theHeadingLength = 0;
			for (int theOffset = 0; theOffset < (theSanitizedHeadingLength); theOffset++)
			{
				char theCurrentChar = theSanitizedHeading[theOffset];
            
				if (quoteNeeded)
				{
					if (theCurrentChar == '\'')
					{
						quoteNeeded = false;
						theHeadingLength++;
					}
					else
					{
						anErrorMessage.Append("Single quotes in heading must be doubled.");
						return false;
					}
				}
				else
				{
					if (theCurrentChar == '\'')
					{
						quoteNeeded = true;
					}
					else
					{
						theHeadingLength++;
					}
				}
            
			}
			if (quoteNeeded)
			{
				anErrorMessage.Append("Single quotes in heading must be doubled.");
				return false;
			}
        
			// Validate the length
			if (theHeadingLength > theMaximumHeadingLength)
			{
				anErrorMessage.Append("The heading may not exceed " + theMaximumHeadingLength + " characters.");
				return false;
			}
        
			// If we get here, it's a legal delimited identifier
			return true;
        
		}
    
        /// <summary>
        /// Break an ansi name into name parts
        /// </summary>
        /// <param name="anAnsiName"></param>
        /// <returns></returns>
		public static string[] CrackAnsiName(string anAnsiName)
		{
			int theAnsiLength = anAnsiName.Length;
			bool inQuotes = false;
			int theBeginOffset = 0;
        
			int theResultPart = 0;
        
			string[] theResult = new string[3];
        
				for (int theCurrentOffset = 0; theCurrentOffset < theAnsiLength; theCurrentOffset++)
				{
					char theCharacter = anAnsiName[theCurrentOffset];
            
					switch (theCharacter)
					{
						case '"':
						{
							inQuotes = !inQuotes;
							break;
						}
						case '.':
						{
							if (!inQuotes)
							{
								theResult[theResultPart] = anAnsiName.Substring(theBeginOffset, theCurrentOffset);
								theResultPart++;
								theBeginOffset = theCurrentOffset + 1;
							}
							break;
						}
						default:
						{
							break;
						}
					}
            
				}
        
			theResult[theResultPart] = anAnsiName.Substring(theBeginOffset, theAnsiLength);
			theResultPart++;
        
			return theResult;
        
		}
    
		/// <summary>
        /// Checks to see if a string is real ansi name (3 part name) and if not uses the default values 
        /// that are passed in to fill in the catalog and schema names returns the 3 part name as an array
		/// </summary>
		/// <param name="anAnsiName"></param>
		/// <param name="defaultCatalog"></param>
		/// <param name="defaultSchema"></param>
		/// <returns></returns>
		public static string[] CrackAnsiName(string anAnsiName, string defaultCatalog, string defaultSchema)
		{
			string[] names = CrackAnsiName(anAnsiName);
			string[] theResult = new string[3];
        
			if(names[1] == null) 
			{
				//name is only 1 part.
				//Use the default catalog and schema
				theResult[0] = defaultCatalog;
				theResult[1] = defaultSchema;
				theResult[2] = names[0];
				return theResult;
			}
			if(names[2] == null)
			{
				//name is only 2 parts. So no catalog provided. use default catalog
				theResult[0] = defaultCatalog;
				theResult[1] = names[0];
				theResult[2] = names[1];
				return theResult;
			}
			return names;
		}

        /// <summary>
        /// Returns a SQL name's canonical form.  
        /// Not delimtied -> self upshifted 
        /// Delimited and all upper and no specials -> self dequoted 
        /// Else -> self
        /// </summary>
        /// <param name="aTrafodionColumnName"></param>
        /// <returns></returns>
		public static string CanonicalForm(string aTrafodionColumnName)
		{
			int theLength = aTrafodionColumnName.Length;
        
			// May be empty
			if (theLength == 0)
			{
				return "";
			}
        
			// If it's not delimited, it's itself in uppercase
			if (!aTrafodionColumnName.StartsWith("\""))
			{
				return aTrafodionColumnName.ToUpper();
			}
        
			// Is it a delimited name that's not done yet?
			if ((theLength == 1) || (aTrafodionColumnName[theLength - 1] != '"'))
			{
				return aTrafodionColumnName;
			}
        
			// Is this a delimited reserved word?
			if (ReservedWords.IsReserved(aTrafodionColumnName.Substring(1, theLength - 2)))
			{
            
				// Yes, return it unchanged
				return aTrafodionColumnName;
            
			}
       
			// If it's delimited we need to check to see if it's all uppercase and has no specials.  If so,
			// it's equivalent to itself without quotes.
			for (int theOffset = 1; theOffset < theLength - 1; theOffset++)
			{
				char theChar = aTrafodionColumnName[theOffset];
				if ((!Char.IsLetterOrDigit(theChar) && (theChar != '_')) || !Char.IsUpper(theChar))
				{
                
					// Special or lower found it's itself .
					return aTrafodionColumnName;
                
				}
			}
        
			// If we get here, it's delimited, all uppercase with no specials.  It's equivalent to
			// itself without the quotes.
			return aTrafodionColumnName.Substring(1, theLength - 2);
        
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
    
		/// <summary>
        /// Returns a SQL name's external form.  
		/// </summary>
		/// <param name="anInternalName"></param>
		/// <returns></returns>
 		public static string ExternalForm(string anInternalName)
		{
            string theName = anInternalName.TrimEnd();

            // May be empty?
            if (theName.Length == 0)
            {
                return "";
            }

            // If it contains specials, it needs to be delimited.
            if (!theName.StartsWith("_") && Regex.IsMatch(theName, "^[A-Z0-9_]+$"))
            {

                // No specials, it's itself
                return theName;

            }

            theName = theName.Replace("\"", "\"\"");

            // It has specials; delimit it.
            return "\"" + theName + "\"";

        }
    		/// <summary>
        /// Returns a SQL name's internal form.  
		/// </summary>
		/// <param name="anExternalName"></param>
		/// <returns></returns>
 		public static string InternalForm(string anExternalName)
		{
			int theLength = anExternalName.Length;

			if ((theLength > 1) && (anExternalName.StartsWith("\"")) && (anExternalName.EndsWith("\"")))
			{
				return anExternalName.Substring(1, theLength - 2);
			}
			return anExternalName.ToUpper();
		}
    
        /// <summary>
        /// Checks if internal and external names are same
        /// </summary>
        /// <param name="anIneternalOrExternalName"></param>
        /// <returns></returns>
		static public bool ExternalInternalFormSame(string anIneternalOrExternalName)
		{
            if (!anIneternalOrExternalName.Equals(anIneternalOrExternalName.ToUpper()))
                return false;

            string internalFormName = InternalForm(anIneternalOrExternalName);
            string externalFormName = ExternalForm(internalFormName);
            return (externalFormName.Equals(internalFormName));
		}

        /// <summary>
        /// Composes the 3 part ansi name for a sql schema object
        /// </summary>
        /// <param name="aCatalogName"></param>
        /// <param name="aSchemaName"></param>
        /// <param name="anObjectName"></param>
        /// <returns></returns>
		static public string ComposePathname(string aCatalogName, string aSchemaName, string anObjectName)
		{
			return (ComposePathname(aCatalogName, aSchemaName) + "." + anObjectName);
		}
    
        /// <summary>
        /// Composes the 2 part ansi name for a schema
        /// </summary>
        /// <param name="aCatalogName"></param>
        /// <param name="aSchemaName"></param>
        /// <returns></returns>
		static public string ComposePathname(string aCatalogName, string aSchemaName)
		{
			return (aCatalogName + "." + aSchemaName);
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
			return CanonicalForm(aName) == theCanonicalForm;
		}
    
        /// <summary>
        /// Compares two identifier names
        /// </summary>
        /// <param name="aTrafodionName"></param>
        /// <param name="anotherTrafodionName"></param>
        /// <returns></returns>
		public static bool Equals(string aTrafodionName, string anotherTrafodionName)
		{
			return CanonicalForm(aTrafodionName) == CanonicalForm(anotherTrafodionName);
		}
    
        /// <summary>
        /// Given an objectype and name space, identifies the object type name
        /// </summary>
        /// <param name="anObjectType"></param>
        /// <param name="anObjectNameSpace"></param>
        /// <returns></returns>
		public static string GetObjectTypeName(string anObjectType, string anObjectNameSpace)
		{
			if (theObjectTypeNameHashTable == null)
			{
				theObjectTypeNameHashTable = new Hashtable();
				theObjectTypeNameHashTable.Add("BT", "Table");
				theObjectTypeNameHashTable.Add("IX", "Index");
				theObjectTypeNameHashTable.Add("MV", "Materialized View");
				//theObjectTypeNameHashTable.Add("??", "Materialized View Index");
				theObjectTypeNameHashTable.Add("UR", "Stored Procedure");
				theObjectTypeNameHashTable.Add("SY", "Synonym");
				theObjectTypeNameHashTable.Add("RG", "Materialized View Group");
				theObjectTypeNameHashTable.Add("VI", "View");
			}
      
			string theObjectTypeName = (string) theObjectTypeNameHashTable[anObjectType];
        
			if (theObjectTypeName == null)
			{
				theObjectTypeName = "Object";
			}
			return theObjectTypeName;
		}
	}
}
