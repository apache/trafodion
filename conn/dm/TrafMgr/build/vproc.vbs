'    @@@ START COPYRIGHT @@@                                                 
'                                                                            
'    (C) Copyright 2015 Hewlett-Packard Development Company, L.P.            
'                                                                            
'    Licensed under the Apache License, Version 2.0 (the "License");         
'    you may not use this file except in compliance with the License.        
'    You may obtain a copy of the License at                                 
'                                                                            
'         http://www.apache.org/licenses/LICENSE-2.0                         
'                                                                            
'    Unless required by applicable law or agreed to in writing, software     
'    distributed under the License is distributed on an "AS IS" BASIS,       
'    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
'    See the License for the specific language governing permissions and     
'    limitations under the License.                                          
'                                                                            
'    @@@ END COPYRIGHT @@@                                                   
Const ForReading = 1
Const ForWriting = 2

Set TypeLib = CreateObject("Scriptlet.TypeLib")

strFileName = Wscript.Arguments(0)
strCompVersion = Wscript.Arguments(1)
strRelVersion =  Wscript.Arguments(2)
strBuildNum = Wscript.Arguments(3)

Set objFSO = CreateObject("Scripting.FileSystemObject")


'---------------------------------------------------------------------------
'START Update the version.
'---------------------------------------------------------------------------

strDate = pd(DAY(date()),2) & " " & _ 
        GetMonthString(date) & _ 
        " " & YEAR(date()) 

strNewText = "AssemblyDescription(""Version " & strCompVersion & " Release " & strRelVersion & " (Build " & strBuildNum & ", Date " & strDate & ")"")"

Set objFile = objFSO.OpenTextFile(strFileName, ForReading)

strText = objFile.ReadAll
objFile.Close

Set regEx = New RegExp
regEx.Pattern = "AssemblyDescription\(""Version\s[\w.]{5}\sRelease\s[\w.]{5}\s\(Build\s[\d]+\,\sDate\s[\w]+\s[\w]+\s[\w]+\)""\)"
regEx.IgnoreCase = True
regEx.Global = True
regEx.MultiLine = False
StringToExtract = regEx.Replace(strText, strNewText)

'Open the project file for writing
Set objFile = objFSO.OpenTextFile(strFileName, ForWriting)

'Write the new project file to disk.
objFile.WriteLine StringToExtract

objFile.Close


Function pd(n, totalDigits) 
  if totalDigits > len(n) then 
     pd = String(totalDigits-len(n),"0") & n 
  else 
     pd = n 
  end if 
End Function

Function GetMonthString (myDate)
 strMonth = ""
 Select case Month (myDate)
 case 1  strMonth = "Jan"
 case 2  strMonth = "Feb"
 case 3  strMonth = "Mar"
 case 4  strMonth = "Apr"
 case 5  strMonth = "May"
 case 6  strMonth = "Jun"
 case 7  strMonth = "Jul"
 case 8  strMonth = "Aug"
 case 9  strMonth = "Sep"
 case 10  strMonth = "Oct"
 case 11  strMonth = "Nov"
 case 12  strMonth = "Dec"
 End Select
 GetMonthString = strMonth
 End Function 