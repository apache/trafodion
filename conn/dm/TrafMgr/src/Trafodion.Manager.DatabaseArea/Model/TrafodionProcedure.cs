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

using System.Data.Odbc;
using System.Text;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// Model for a sql procedure
	/// </summary>
	public class TrafodionProcedure
		: TrafodionRoutine
    {
        #region Public variables
        /// <summary>
        /// Constant that defines the procedure object name space
        /// </summary>
        public const string ObjectNameSpace = "TA";
        public const string ObjectType = "UR";
        public const string TRACE_SUB_AREA_NAME = "Procedure";

        #endregion Public variables

        #region Public Properties

        /// <summary>
        /// Object name space for the procedure
        /// </summary>
        override public string SchemaObjectNameSpace
        {
            get
            {
                return ObjectNameSpace;
            }
        }
        #endregion Public Properties

        #region Formatted Public Properties

        /// <summary>
        /// Format External File
        /// </summary>
        public string TheJavaDataType(int theParam)
        {
            //return _params[theParam];
            return ((TrafodionProcedureColumn)Columns[theParam]).JavaDataType;
        }

        /// <summary>
        /// Take java packed signature and make it user friendly
        /// examples: 
        /// (Ljava/math/BigDecimal;[Ljava/math/BigDecimal;)V  =>  (java.math.BigDecimal,java.math.BigDecimal[])
        /// (Ljava/sql/Date;[I)V    =>  (java.sql.Date,int[])
        /// 
        /// This procedure assumes that signature is in correct form
        /// </summary>
        public string FormattedSignature
        {
            get
            {

                if (_formattedSignatureText == null)
                {
                    if (_signatureText == null)
                    {
                        LoadAttributes();
                    }

                    _formattedSignatureText = FormatProcedureSignature(_signatureText);
                    string theParams = _formattedSignatureText;
                    theParams = theParams.Replace("(", "");
                    theParams = theParams.Replace(")", "");
                    _params = theParams.Split(',');
                    if (Columns.Count > 0 && Columns.Count <= _params.Length)
                    {
                        for (int i = 0; i < Columns.Count; i++)
                        {
                            ((TrafodionProcedureColumn)Columns[i]).JavaDataType = _params[i];
                        }
                    }
                }

                return _formattedSignatureText;

            }
        }

        #endregion Formatted Public Properties

        #region Private Properties
        /// <summary>
        /// The DDL string used in the create
        /// </summary>
        private string CreateDDLString
        {
            get
            {
                StringBuilder ddlString = new StringBuilder("CREATE PROCEDURE " + this.RealAnsiName);
                ddlString.Append("(");
                if (Columns.Count > 0)
                {
                    bool firstParam = true;
                    for (int i = 0; i < Columns.Count; i++)
                    {
                        TrafodionProcedureColumn column = Columns[i] as TrafodionProcedureColumn;

                        //Ignore result set columns
                        if (column.JavaDataType.Equals(DataTypeHelper.JAVA_RESULTSET) || column.JavaDataType.Equals(DataTypeHelper.JAVA_RESULTSET_ARRAY))
                            continue;

                        ddlString.Append(string.Format("{0}{1} {2} {3}",
                                            (firstParam ? " \n" : ",\n"),
                                            column.FormattedDirection(),
                                            column.ExternalName,
                                            column.FormattedDataType())
                                        );
                        firstParam = false;
                    }
                }

                ddlString.Append(")\n");

                ddlString.AppendLine(string.Format("EXTERNAL NAME '{0}.{1} ( {2} )'",
                                FormattedExternaClassFileName, FormattedMethodName, SignatureText));
                if (this.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                {
                    //M7
                    ddlString.AppendLine("LIBRARY " + this.FormattedLibraryName);
                }
                else
                {
                    //M6
                    ddlString.AppendLine("EXTERNAL PATH '" + this.FormattedExternalPath + "'");
                }

                if (this.TheTrafodionSchema.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120 &&
                    this.ExternalSecurity.Equals("D"))
                {
                    ddlString.AppendLine("EXTERNAL SECURITY DEFINER ");
                }
                if (this.TheTrafodionSchema.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140 &&
                   this.NewTransactionAttributes.Equals("NO"))
                {
                    ddlString.AppendLine("NO TRANSACTION REQUIRED ");
                }

                ddlString.AppendLine("LANGUAGE JAVA \nPARAMETER STYLE JAVA ");

                ddlString.AppendLine(string.Format("{0} \nDYNAMIC RESULT SETS {1}", FormattedSQLAccess, MaxResults));

                return ddlString.ToString();
            }
        }

        #endregion Private Properties

        /// <summary>
        /// Constructs a model for the procedure
        /// </summary>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="anInternalName"></param>
        /// <param name="aUID"></param>
        /// <param name="aCreateTime"></param>
        /// <param name="aRedefTime"></param>
        /// <param name="aSecurityClass"></param>
        /// <param name="anOwner">The owner of the object.</param>
        public TrafodionProcedure(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
		{
        }

        /// <summary>
        /// Formats the procedure java signature
        /// </summary>
        /// <param name="signatureText"></param>
        /// <returns></returns>
        public static string FormatProcedureSignature(string signatureText)
        {
            string unpackedSignature = "";
            string wrapperSubstring;
            bool isArray = false;
            bool endOfPrams = false;
            int index = 0;
            int sigCharIdx = 0;

            while (sigCharIdx < signatureText.Length)
            {


                switch (signatureText[sigCharIdx])
                {
                    case '(':
                        unpackedSignature += "(";
                        break;
                    case ')':
                        unpackedSignature += ")";
                        endOfPrams = true;
                        break;
                    case '[':
                        isArray = true;
                        break;

                    default:
                        switch (signatureText[sigCharIdx])
                        {
                            case 'L':
                                index = signatureText.IndexOf(";", sigCharIdx);
                                wrapperSubstring = signatureText.Substring(sigCharIdx + 1, index - (sigCharIdx + 1));
                                unpackedSignature += wrapperSubstring.Replace("/", ".");
                                sigCharIdx = index;
                                break;
                            case 'B':
                                unpackedSignature += "byte";
                                break;
                            case 'C':
                                unpackedSignature += "char";
                                break;
                            case 'D':
                                unpackedSignature += "double";
                                break;
                            case 'F':
                                unpackedSignature += "float";
                                break;
                            case 'I':
                                unpackedSignature += "int";
                                break;
                            case 'J':
                                unpackedSignature += "long";
                                break;
                            case 'S':
                                unpackedSignature += "short";
                                break;
                            case 'Z':
                                unpackedSignature += "boolean";
                                break;
                            case 'V':
                                break;
                            default:
                                unpackedSignature += signatureText[sigCharIdx];
                                break;

                        } // end switch

                        if (isArray)
                        {
                            unpackedSignature += "[]";
                            isArray = false;
                        }
                        // don't add "," if at end of signature
                        if (!endOfPrams) // we already saw ")" 
                            if (sigCharIdx < signatureText.Length - 1) // look ahead for ")" 
                                if (!(signatureText[sigCharIdx + 1].Equals(')')))
                                    unpackedSignature += ",";

                        break;
                } // end switch
                sigCharIdx++;



            } // end while

            return unpackedSignature;

        }

        /// <summary>
        /// Resets the Procedure model
        /// </summary>
        override public void Refresh()
        {
            //Create a temp model
            TrafodionProcedure aProcedure = this.TheTrafodionSchema.LoadProcedureByName(this.InternalName);

            //IF temp model is null, the object has been removed
            //So cleanup and notify the UI
            if (aProcedure == null)
            {
                this.TheTrafodionSchema.TrafodionProcedures.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(aProcedure) != 0)
            {
                //If sql object has been recreated, attach the new sql model to the parent.
                this.TheTrafodionSchema.TrafodionProcedures.Remove(this);
                this.TheTrafodionSchema.TrafodionProcedures.Add(aProcedure);
                //Notify the UI
                this.OnModelReplacedEvent(aProcedure);
            }
            else
            {
                base.Refresh();
            }
        }

        /// <summary>
        /// Creates the sql procedure
        /// </summary>
        public void Create()
        {
            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();
                OdbcCommand theQuery = new OdbcCommand(CreateDDLString, theConnection.OpenOdbcConnection);
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

                //Since the add succeeded add this procedure model to the parent schema and send an event
                TheTrafodionSchema.AddProcedure(this);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        public void Drop()
        {
            Connection theConnection = null;

            try
            {
                string dropDDLString = "DROP PROCEDURE " + this.RealAnsiName;
                theConnection = GetConnection();
                OdbcCommand theQuery = new OdbcCommand(dropDDLString, theConnection.OpenOdbcConnection);
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true);

                //Since the drop succeeded add this procedure model to the parent schema and send an event
                TheTrafodionSchema.DropProcedure(this);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }
	}

}


