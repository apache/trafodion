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

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// 
	/// </summary>
	public class ReservedWords
	{
    
		/** Adds quotes to the subvolume and/or file in a Guardian file name if they are reserved words.
		 * @param aGuardianName A syntactically correct three or four part Guardian filename.
		 * @return The Guardian name with the subvolume and/or filename enclosed in double quotes if they are reserved words.
		 */    
		static public string QuoteGuardianName(string aGuardianName)
		{
	
			// Where we'll build the pieces
			string theSystemName = "";
			string theVolumeName;
			string theSubvolumeName;
			string theFileName;
	
			// Assume three parts
			bool threePartName = true;
	
			// For cracking the name at the dots
			int dotBegin = 0;
			int dotEnd = -1;
	
			// Check for four part name
			if (aGuardianName.StartsWith("\\"))
			{
	    
				// Collect system name
				dotEnd = aGuardianName.IndexOf('.', dotBegin);
				theSystemName = aGuardianName.Substring(dotBegin, dotEnd);
				dotBegin = dotEnd + 1;
	    
				// Four parts
				threePartName = false;
	    
			}
	
			// Collect the volume name
			dotEnd = aGuardianName.IndexOf('.', dotBegin);
			theVolumeName = aGuardianName.Substring(dotBegin, dotEnd);
			dotBegin = dotEnd + 1;
	
			// Collect the subvolume name
			dotEnd = aGuardianName.IndexOf('.', dotBegin);
			theSubvolumeName = aGuardianName.Substring(dotBegin, dotEnd);
			dotBegin = dotEnd + 1;
	
			// Collect the file name
			theFileName = aGuardianName.Substring(dotBegin, aGuardianName.Length);
	
			// Check to see how to deal with the pieces
			if (threePartName)
			{
	    
				// Process a three part name
				return QuoteGuardianName(theVolumeName, theSubvolumeName, theFileName);
			}
	    
			// Process a four part name
			return QuoteGuardianName(theSystemName, theVolumeName, theSubvolumeName, theFileName);
	
		}
    
		/** Builds a Guardian filename from its separate parts.  Adds quotes to the subvolume and/or file if they are reserved words.
		 * @param aSystemName A system name beginning with backslash
		 * @param aVolumeName A volume name beginning with a dollar sign
		 * @param aSubvolumeName a subvolume name
		 * @param aFileName a simple filename
		 * @return The Guardian name with the subvolume and/or filename enclosed in double quotes if they are reserved words.
		 */    
		static public string QuoteGuardianName(string aSystemName, string aVolumeName, string aSubvolumeName, string aFileName)
		{
			return aSystemName + '.' + aVolumeName + '.' + QuoteReservedWord(aSubvolumeName) + '.' + QuoteReservedWord(aFileName);
		}
    
		/** Builds a Guardian filename from its separate parts.  Adds quotes to the subvolume and/or file if they are reserved words.
		 * @param aVolumeName a volume name beginning with a dollar sign
		 * @param aSubvolumeName a subvolume name
		 * @param aFileName a simple filename
		 * @return The Guardian name with the subvolume and/or filename enclosed in double quotes if they are  reserved words.
		 */    
		static public string QuoteGuardianName(string aVolumeName, string aSubvolumeName, string aFileName)
		{
			return aVolumeName + '.' + QuoteReservedWord(aSubvolumeName) + '.' + QuoteReservedWord(aFileName);
		}
    
		/** Reports whether or not a given word is a reserved word.
		 * @param aWord A word to be checked.  Case insensitive.
		 * @return True if the word is reserved else false.
		 */    
		static public bool IsReserved(string aWord)
		{
	
			// Make a copy of the word in uppercase
			string theWord = aWord.ToUpper();
	
			// Check to see if the hash tables have been initialized
			if (theMXReservedWordsHashtable == null)
			{
	    
				// Initialize the two hash tables
				theMXReservedWordsHashtable = InitializeHashtable(theMXReservedWordsArray);
				theMXMPReservedWordsHashtable = InitializeHashtable(theMXMPReservedWordsArray);
	    
			}
	
			// Report whether or not the word is in either has table
			return ((theMXReservedWordsHashtable[theWord] != null) || (theMXMPReservedWordsHashtable[theWord] != null));
	
		}
    
		/** Returns a word in double quotes if it is a reserved word.  Otherwise returns its original input.
		 * @param aWord A word to be checked.  Case insensitive.
		 * @return The original word in double quotes if it is a reserved word else the original word.  Case is preserved.
		 */    
		static public string QuoteReservedWord(string aWord)
		{
	
			// Check for reserved word
			if (IsReserved(aWord))
			{
	    
				// It is reserved, return it with double quotes around it
				return '\"' + aWord + '\"';
	    
			}
	
			// Not reserved, return it plain
			return aWord;
	
		}
    
		// Create a hash table from a string array.
		static private Hashtable InitializeHashtable(string[] aStringArray)
		{
			int theLength = aStringArray.Length;
			Hashtable theHashtable = new Hashtable(theLength);
	
			// Loop over the strings
			for (int theIndex = 0; theIndex < theLength; theIndex++)
			{
	    
				// Put each in the table as a key.  We don't care about the value so use an empty string.
				theHashtable.Add(aStringArray[theIndex], "");
	    
			}
	
			return theHashtable;
	
		}
    
		// Reserved in SQL Text Only
		static private string[] theMXReservedWordsArray =
		{
			"ACTION", "COALESCE", "DESTROY",
		"ADD", "COLLATION", "DESTRUCTOR",
		"ADMIN", "COLUMN", "DETERMINISTIC",
		"AFTER", "COMPLETION", "DIAGNOSTICS",
		"AGGREGATE", "CONNECT", "DICTIONARY",
		"ALIAS", "CONNECTION", "DISCONNECT",
		"ALLOCATE", "CONSTRAINT", "DOMAIN",
		"ALTER", "CONSTRAINTS", "DOUBLE",
		"ARE", "CONSTRUCTOR", "DROP",
		"ARRAY", "CONTINUE", "DYNAMIC",
		"ASSERTION", "CONVERT", "EACH",
		"ASYNC", "CORRESPONDING", "ELSE",
		"AT", "CREATE", "ELSEIF",
		"AUTHORIZATION", "CROSS", "END",
		"BEFORE", "CUBE", "EQUALS",
		"BINARY", "CURRENT_DATE", "EXCEPT",
		"BIT", "CURRENT_PATH", "EXCEPTION",
		"BIT_LENGTH", "CURRENT_ROLE", "EXEC",
		"BLOB", "CURRENT_TIME", "EXECUTE",
		"BOOLEAN", "CURRENT_TIMESTAMP", "EXTERNAL",
		"BREADTH", "CURRENT_USER", "EXTRACT",
		"CALL", "CYCLE", "FALSE",
		"CASCADE", "DEALLOCATE", "FIRST",
		"CASCADED", "DEFERRABLE", "FLOAT",
		"CAST", "DEFERRED", "FOREIGN",
		"CHAR_LENGTH", "DEPTH", "FOUND",
		"CHARACTER_LENGTH", "DEREF", "FREE",
		"CLASS", "DESCRIBE", "FULL",
		"CLOB", "DESCRIPTOR", "FUNCTION",
		"GENERAL", "MODIFY", "PRIMARY",
		"GET", "MODULE", "PRIOR",
		"GLOBAL", "NAMES", "PRIVATE",
		"GO", "NATIONAL", "PRIVILEGES",
		"GOTO", "NATURAL", "PROCEDURE",
		"GRANT", "NCHAR", "PROTECTED",
		"GROUPING", "NCLOB", "PROTOTYPE",
		"HOST", "NEW", "PUBLIC",
		"IDENTITY", "NEXT", "READ",
		"IF", "NO", "READS",
		"IGNORE", "NONE", "REAL",
		"IMMEDIATE", "NULLIF", "RECURSIVE",
		"INDICATOR", "OBJECT", "REF",
		"INITIALLY", "OCTET_LENGTH", "REFERENCES",
		"INOUT", "OFF", "REFERENCING",
		"INSENSITIVE", "OID", "RELATIVE",
		"INTERSECT", "OLD", "REPLACE",
		"ISOLATION", "ONLY", "RESIGNAL",
		"ITERATE", "OPERATORS", "RESTRICT",
		"LANGUAGE", "ORDINALITY", "RESULT",
		"LARGE", "OTHERS", "RETURN",
		"LAST", "OUT", "RETURNS",
		"LATERAL", "OUTER", "REVOKE",
		"LEAVE", "OUTPUT", "ROLE",
		"LESS", "OVERLAPS", "ROLLUP",
		"LEVEL", "PAD", "ROUTINE",
		"LIMIT", "PARAMETER", "ROW",
		"LOCAL", "PARAMETERS", "ROWS",
		"LOCALTIME", "PARTIAL", "SAVEPOINT",
		"LOCALTIMESTAMP", "PENDANT", "SCHEMA",
		"LOCATOR", "POSITION", "SCOPE",
		"LOOP", "POSTFIX", "SCROLL",
		"LOWER", "PRECISION", "SEARCH",
		"MAP", "PREFIX", "SECTION",
		"MATCH", "PREORDER", "SENSITIVE",
		"MIN", "PREPARE", "SESSION",
		"MODIFIES", "PRESERVE", "SESSION_USER",
		"SETS", "TRANSLATION",
		"SIGNAL", "TRANSPOSE",
		"SIMILAR", "TREAT",
		"SIZE", "TRIGGER",
		"SPACE", "TRIM",
		"SPECIFIC", "TRUE",
		"SPECIFICTYPE", "UNDER",
		"SQL", "UNKNOWN",
		"SQL_CHAR", "UNNEST",
		"SQL_DATE", "UPPER",
		"SQL_DECIMAL", "UPSHIFT",
		"SQL_DOUBLE", "USAGE",
		"SQL_FLOAT", "USER",
		"SQL_INT", "USING",
		"SQL_INTEGER", "VALUE",
		"SQL_REAL", "VARCHAR",
		"SQL_SMALLINT", "VARIABLE",
		"SQL_TIME", "VARYING",
		"SQL_TIMESTAMP", "VIRTUAL",
		"SQL_VARCHAR", "VISIBLE",
		"SQLCODE", "WAIT",
		"SQLERROR", "WHENEVER",
		"SQLEXCEPTION", "WHILE",
		"SQLSTATE", "WITHOUT",
		"SQLWARNING", "WRITE",
		"STRUCTURE", "ZONE",
		"SUBSTRING",
		"SYSTEM_USER",
		"TEMPORARY",
		"TERMINATE",
		"TEST",
		"THAN",
		"THERE",
		"TIMEZONE_HOUR",
		"TIMEZONE_MINUTE",
		"TRANSACTION",
		"TRANSLATE"
	};
    
	// Reserved in Stored Text
	static private string[] theMXMPReservedWordsArray =
{
	"ALL", "CLOSE", "DISTINCT", "INTEGER", "OF",
	"AND", "COLLATE", "ESCAPE", "INTERVAL", "ON",
	"ANY", "COMMIT", "EXISTS", "INTO", "OPEN",
	"AS", "COUNT", "FETCH", "IS", "OPTION",
	"ASC", "CURRENT", "FOR", "JOIN", "OR",
	"AVG", "CURSOR", "FRACTION", "KEY", "ORDER",
	"BEGIN", "DATE", "FROM", "LEADING", "RIGHT",
	"BETWEEN", "DATETIME", "GROUP", "LEFT", "ROLLBACK",
	"BOTH", "DAY", "HAVING", "LIKE", "SECOND",
	"BY", "DEC", "HOUR", "MAX", "SELECT",
	"CASE", "DECIMAL", "IN", "MINUTE", "SET",
	"CATALOG", "DECLARE", "INNER", "MONTH", "SMALLINT",
	"CHAR", "DEFAULT", "INPUT", "NOT", "SOME",
	"CHARACTER", "DELETE", "INSERT", "NULL", "SUM",
	"CHECK", "DESC", "INT", "NUMERIC", "TABLE",
	"THEN", "TO", "UNIQUE", "VIEW", "WITH",
	"TIME", "TRAILING", "UPDATE", "WHEN", "WORK",
	"TIMESTAMP", "UNION", "VALUES", "WHERE", "YEAR",
};
    
	// The hash tables of reserved words.  We don't associate any values.  We're just checking for existence in the tables.
	static private Hashtable theMXReservedWordsHashtable = null;
	static private Hashtable theMXMPReservedWordsHashtable = null;
    
}
}
