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
using System.Collections;
using System.IO;
using System.Text;
using System.Text.RegularExpressions; 

namespace Trafodion.Manager.DatabaseArea.Queries
{
    public class ParameterDocumentationReader
    {
        public const string START_MARKER = "<<";
        public const string END_MARKER = ">>";
        public  string[][] tokens = {
                                     new string[1]{ReportParameterProcessor.PARAMETER_NAME_TOKEN}, 
                                     new string[1]{ReportParameterProcessor.DISPLAY_NAME_TOKEN},
                                     new string[3]{ReportParameterProcessor.TOOLTIP_TOKEN, "<<", ">>"},
                                     new string[1]{ReportParameterProcessor.EXAMPLE_TOKEN},
                                     new string[3]{ReportParameterProcessor.LEGAL_VALUES_TOKEN, "<<", ">>"}   
                                    };

        
        public ParameterDocumentationReader()
        {
        }

        public DocumentationObject getDocumentationObject(ReportDefinition reportDefinition)
        {
            DocumentationObject ret = null;
            try
            {
                if (reportDefinition != null)
                {
                    //get the aReportDefinition type
                    int reportType = ReportDefinition.TEXT_REPORT;
                    if (reportDefinition.GetProperty(ReportDefinition.TYPE) != null)
                    {                
                        reportType =  (int)(reportDefinition.GetProperty(ReportDefinition.TYPE));
                    }
                    //extract parameters based on the aReportDefinition type
                    switch(reportType)
                    {
                        case ReportDefinition.TEXT_REPORT:
                        {
                            //Get parameter documentation from the report documentation
                            string rawQuery = ReportParameterProcessor.Instance.getRawQuery(reportDefinition);
                            string commentBlock  = ReportParameterProcessor.Instance.getBlockComment(rawQuery);
                            string line = null;
                            StringReader sr = null;
                            Hashtable map = new Hashtable();
                            if ((commentBlock != null) && (commentBlock.Trim().Length > 0))
                            {
                                sr = new StringReader(commentBlock);
                                map = this.getDocumentationTokens(sr);
                                if ((map != null) && (map.Keys.Count > 0))
                                {
                                    ret = new DocumentationObject();
                                     string value = null;
                                    foreach (string key in map.Keys)
                                    {
                                        value = (string)map[key];
                                        if (value != null)
                                        {
                                            if (key.Equals(DocumentationObject.QUERY_TITLE))
                                            {
                                                ret.QueryTitle = value.Substring(value.IndexOf(key, StringComparison.InvariantCultureIgnoreCase) + key.Length).Trim();
                                            }
                                            else if (key.Equals(DocumentationObject.QUERY_FILENAME))
                                            {
                                                ret.QueryFileName = value.Substring(value.IndexOf(key, StringComparison.InvariantCultureIgnoreCase) + key.Length).Trim();
                                            }
                                            else if (key.Equals(DocumentationObject.QUERY_REVNUM))
                                            {
                                                ret.QueryRevNum = value.Substring(value.IndexOf(key, StringComparison.InvariantCultureIgnoreCase) + key.Length).Trim();
                                            }
                                            else if (key.Equals(DocumentationObject.REPOS_REVNUM))
                                            {
                                                ret.ReposRevNum = value.Substring(value.IndexOf(key, StringComparison.InvariantCultureIgnoreCase) + key.Length).Trim();
                                            }
                                            else if (key.Equals(DocumentationObject.APPLICATION))
                                            {
                                                ret.Application = value.Substring(value.IndexOf(key, StringComparison.InvariantCultureIgnoreCase) + key.Length).Trim();
                                            }
                                            else if (key.Equals(DocumentationObject.QUERY_STYLE_REVNUM))
                                            {
                                                ret.QueryStyleRevNum = value.Substring(value.IndexOf(key, StringComparison.InvariantCultureIgnoreCase) + key.Length).Trim();
                                            }
                                            else if (key.Equals(DocumentationObject.QUERY_SHORT_DESC))
                                            {
                                                ret.ShortDescription = value.Substring(value.IndexOf(key, StringComparison.InvariantCultureIgnoreCase) + key.Length).Trim();
                                            }
                                            else if (key.Equals(DocumentationObject.QUERY_LONG_DESC))
                                            {
                                                ret.LongDescription = value.Substring(value.IndexOf(key, StringComparison.InvariantCultureIgnoreCase) + key.Length).Trim();
                                            }
                                            else if (key.Equals(DocumentationObject.QUERY_PARAM_DESC))
                                            {
                                                StringReader tempSr = new StringReader(value);
                                                ret.Parameters = this.getParameterValues(tempSr);
                                            }
                                        }
                                    }
                                }
                                
                            }
                        }
                        break;
                        case ReportDefinition.CRYSTAL_REPORT:
                        {
                        }
                        break;
                    }
                }
             }
             catch(Exception ex)
             {
                //do nothing
                 Console.WriteLine(ex.Message);
             }
             return ret;
        }
        
        //This method shall be used to get the parsed information from the comment
        public void populateParamsFromComment(ReportDefinition reportDefinition, List<ReportParameter> parameters)
        {
            try
            {
                if (reportDefinition != null)
                {
                    //get the aReportDefinition type
                    int reportType = ReportDefinition.TEXT_REPORT;
                    if (reportDefinition.GetProperty(ReportDefinition.TYPE) != null)
                    {
                        reportType = (int)(reportDefinition.GetProperty(ReportDefinition.TYPE));
                    }
                    //extract parameters based on the aReportDefinition type
                    switch(reportType)
                    {
                        case ReportDefinition.TEXT_REPORT:
                        {
                            //Get parameter documentation from the report documentation
                            string rawQuery = ReportParameterProcessor.Instance.getRawQuery(reportDefinition);
                            string commentBlock  = ReportParameterProcessor.Instance.getBlockComment(rawQuery);
                            string line = null;
                            StringReader sr = null;

                            if ((commentBlock != null) && (commentBlock.Trim().Length > 0))
                            {
                                sr = new StringReader(commentBlock);
                                Hashtable  paramDecorations = getParameterValues(sr);
                                Hashtable fromComment = null;
                                Object tempObj = null;
                                foreach (ReportParameter parameter in parameters)
                                {
                                     fromComment = (Hashtable)paramDecorations[parameter.ParamName];
                                     if (fromComment != null)
                                     {
                                        //Label
                                        tempObj = fromComment[ReportParameterProcessor.DISPLAY_NAME_TOKEN];
                                        parameter.DisplayName = ((tempObj != null) && (tempObj.ToString().Trim().Length > 0)) ? tempObj.ToString() : parameter.DisplayName;

                                        //tooltip
                                        tempObj = fromComment[ReportParameterProcessor.TOOLTIP_TOKEN];
                                        parameter.Description =  ( (tempObj != null) &&  (tempObj.ToString().Trim().Length > 0)) ? tempObj.ToString() :  parameter.Description;

                                        //possible values
                                        tempObj = fromComment[ReportParameterProcessor.LEGAL_VALUES_TOKEN];
                                        string valueToAdd = null;
                                        if (tempObj != null)
                                        {
                                            string[] values = tempObj.ToString().Split( new char[]{','});
                                            if (values.Length > 0)
                                            {
                                                //populate the possible values wiith the legal values
                                                if (parameter.PossibleValues == null)
                                                {
                                                    parameter.PossibleValues = new ArrayList();
                                                }
                                                for (int j = 0; j < values.Length; j++)
                                                {
                                                    valueToAdd = values[j].Trim();
                                                    if ((valueToAdd.Trim().Length > 0) && (parameter.PossibleValues.IndexOf(valueToAdd) < 0))
                                                    {
                                                        parameter.PossibleValues.Add(valueToAdd);
                                                    }
                                                }
                                                //make sure that the value is the 0th element
                                                if (parameter.Value != null)
                                                {
                                                    if (parameter.PossibleValues.IndexOf(parameter.Value) >= 0)
                                                    {
                                                        parameter.PossibleValues.Remove(parameter.Value);
                                                        parameter.PossibleValues.Insert(0, parameter.Value);
                                                    }
                                                }
                                                else
                                                {
                                                    parameter.Value = parameter.PossibleValues[0];
                                                }
                                            }
                                        }
                                     }
                                }
                            }
                        }
                        break;
                        case ReportDefinition.CRYSTAL_REPORT:
                        {
                        }
                        break;
                    }
                }
             }
             catch(Exception ex)
             {
                //do nothing
                //ex.printStackTrace();
             }
        }

        //Gets all of the components ov the documentation as a hashmap
        private Hashtable getDocumentationTokens(StringReader sr)
        {
            StringWriter sw  = null;
            string line = null;
            string parameter = null;
            Hashtable parameters = new Hashtable();

            while ((line = sr.ReadLine()) != null)
            {
                //A new token is starting
                for (int i = 0; i < DocumentationObject.DOCUMENTATION_TOKENS.Length; i++)
                {
                    if (line.IndexOf( DocumentationObject.DOCUMENTATION_TOKENS[i], StringComparison.InvariantCultureIgnoreCase) >= 0)
                    {
                        if ((sw != null) && (sw.ToString().Length > 0) && (parameter != null))
                        {
                            parameters.Add(parameter, sw.ToString());
                        }
                        parameter = DocumentationObject.DOCUMENTATION_TOKENS[i];
                        sw = new StringWriter();
                        break;
                    }
                }
                if (sw != null)
                {
                    sw.WriteLine(line);
                }

                if ((sw != null) && (sw.ToString().Length > 0) && (parameter != null))
                {
                    parameters.Add(parameter, sw.ToString());
                    sw = null;
                }
            }
            return parameters;
        }//getDocumentationTokens

        public Hashtable getParameterValues(StringReader sr)
        {
            string[] lineTokens = null;
            StringWriter sw  = null;
            string line = null;
            bool inScope = false;
            bool textStarted = false;
            int startIdx = -1;
            int endIdx = -1;
            Hashtable paramDetails = null;
            string parameter = "";
            Hashtable parameters = new Hashtable();
            string parameterName = null;

            while((line = sr.ReadLine()) != null)
            {
                lineTokens = null;
                //A new parameter is starting
                if (line.IndexOf(ReportParameterProcessor.PARAMETER_NAME_TOKEN) >= 0)
                {
                    if ((paramDetails != null) && (parameterName != null))
                    {                    
                        //parameters.put( ((string)paramDetails.get(ReportParameterProcessor.PARAMETER_NAME_TOKEN)).Trim(), paramDetails);
                        parameters.Add( parameterName.Trim(), paramDetails);
                    }
                    paramDetails = new Hashtable();
                }

                //Check if the line start with a parameter attribute
                if ( ! inScope)
                {
                    for (int i = 0; i < tokens.Length; i++)
                    {
                        if (line.IndexOf( tokens[i][0]) >= 0)
                        {
                            lineTokens = tokens[i];
                            parameter = lineTokens[0];
                            inScope = true;
                            textStarted = false;
                            sw = new StringWriter();
                            break;
                        }
                    }
                }
                
                //this will be the case if it's one of the lines of the comment that start with the token
                if( lineTokens != null)
                {
                    //if the length is one, it means that it has the name and enerything else till the end of
                    //line is the content
                    
                    if (lineTokens.Length == 1)
                    {               
                        startIdx = line.IndexOf(lineTokens[0]) + lineTokens[0].Length;
                        string temp = line.Substring(startIdx);
                        if (temp.Trim().StartsWith(":"))
                        {
                             sw.Write(temp.Trim().Substring(1).Trim());
                        }
                        else
                        {
                            sw.Write(temp.Trim());
                        }
                        inScope = false;              
                        textStarted = false;
                        paramDetails.Add(lineTokens[0], sw.ToString());
                        if (parameter.Equals(ReportParameterProcessor.PARAMETER_NAME_TOKEN))
                        {
                            parameterName = sw.ToString();
                        }
                    }
                    else
                    //look for the content inside the start and end marker for the text
                    {
                        startIdx = line.IndexOf(START_MARKER);
                        endIdx = line.IndexOf(END_MARKER);
                        if ((startIdx >= 0 ) && (endIdx >= 0))
                        {
                            sw.Write(line.Substring(startIdx + START_MARKER.Length, endIdx).Trim());
                            inScope = false;              
                            textStarted = false;
                            paramDetails.Add(lineTokens[0], sw.ToString());
                        }
                        else  if (startIdx >= 0 )
                        {
                            sw.Write(line.Substring(startIdx + START_MARKER.Length).Trim());
                            textStarted = true;
                        }
                        else  if (endIdx >= 0 )
                        {
                            sw.Write(" ");
                            sw.Write(line.Substring(0, endIdx).Trim());
                            inScope = false;              
                            textStarted = false;
                            paramDetails.Add(lineTokens[0], sw.ToString());
                        }
                    }
                }
                else
                {
                    if (inScope)
                    {
                        if (textStarted)
                        {
                             endIdx = line.IndexOf(END_MARKER);
                             if (endIdx >= 0 )
                             {
                                sw.Write(" ");
                                sw.Write(line.Substring(0, endIdx).Trim());
                                inScope = false;              
                                textStarted = false;
                                paramDetails.Add(parameter, sw.ToString());
                             }
                             else
                             {
                                sw.Write(" ");
                                sw.Write(line.Trim());
                             }
                        }
                        else
                        {
                            startIdx = line.IndexOf(START_MARKER);
                            endIdx = line.IndexOf(END_MARKER);
                            if ((startIdx >= 0 ) && (endIdx >= 0))
                            {
                                 sw.Write(line.Substring(startIdx + START_MARKER.Length, endIdx).Trim());
                                inScope = false;              
                                textStarted = false;
                                paramDetails.Add(parameter, sw.ToString());
                            }
                            else  if (startIdx >= 0 )
                            {
                                sw.Write(line.Substring(startIdx + START_MARKER.Length).Trim());
                                textStarted = true;
                            }
                            else  if (endIdx >= 0 )
                            {
                                sw.Write(" ");
                                sw.Write(line.Substring(0, endIdx).Trim());
                                inScope = false;              
                                textStarted = false;
                                paramDetails.Add(parameter, sw.ToString());
                            }
                        }
                    }
                }
            }   
            //add the last one
            if ((paramDetails != null) && (parameterName != null))
            {                    
                //parameters.put( ((string)paramDetails.get(ReportParameterProcessor.PARAMETER_NAME_TOKEN)).Trim(), paramDetails);
                parameters.Add(parameterName.Trim(), paramDetails);
            }

            return parameters;
        }//getParameterValues
    }
}
