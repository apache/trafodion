<!--
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
      http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expreess or implied.
  See the License for the specific language governing permissions and
  limitations under the 
  License.
-->
This page describes how to modify the Trafodion documentation. Please refer to the [Contribute](contribute.html) page for information about other ways to contribute to the Trafodion project.

# Source 
Documents do **not** include version information as part of the file name.

## Source Location

Document                  | Source Format         | Source Tree                                    | Output Format              
--------------------------|-----------------------|------------------------------------------------|----------------------------
Client Installation Guide | asciidoc              | ```docs/client_install/```                     | Web Book, PDF
Command Interface Guide   | asciidoc              | ```docs/comand_interface/```                   | Web Book, PDF
DCS Reference Guide       | asciidoc              | ```dcs/src/main/asciidoc/```                   | Web Book
DCS APIs                  | javadoc               | ```dcs/src/main/java/```                       | Web Book
odb User Guide            | asciidoc              | ```docs/odb/```                                | Web Book, PDF
REST Reference Guide      | asciidoc              | ```core/rest/src/main/asciidoc/```             | Web Book
REST APIs                 | javadoc               | ```core/rest/src/main/java/```                 | Web Book
SQL Reference Manual      | asciidoc              | ```/docs/sql_reference/```                     | Web Book, PDF

## Source Tree Organization

### DCS and REST

### All Other
All other documents share a common web-book stylesheet definition, which is located in **```docs/css/trafodion-manuals.css```**.

The source tree for each manual is organized as follows:

File/Directory                                | Content
----------------------------------------------|-----------------------------------------------------------------------------------------------------------
**```pom.xml```**                             | Maven Project Object Model (POM) used to build the document. 
**```src/```**                                | The source files used to define the document.
**```src/asciidoc```**                        | Asciidoc files for the document.
**```src/asciidoc/index.adoc```**             | Main asciidoc defining the document. Includes the different chapters from the **```_chapters```** directory.
**```src/asciidoc/_chapters/```**             | Source files for the different chapters in the document.
**```images/```**                             | Images used in the document.
**```resources/```**                          | Other include materials; for example, source examples that are included with the document.
**```target/```**                             | Build output directory. Contains the web book, the PDF file, and supporting directories.
**```target/index.pdf```**                    | Generated PDF version of the document.
**```target/images/```**                      | Copy of the **```images/```** directory.
**```target/resources/```**                   | Copy of the **```resources/```** directory.
**```target/site/```**                        | Generated web-book directory.
**```target/site/index.html```**              | Generated web book.
**```target/site/css/```**                    | Stylesheets related to the web book. The common stylesheet is included in the index.html file.
**```target/site/images/```**                 | Copy of the **```images/```** directory.
**```target/site/resources/```**              | Copy of the **```resources/```** directory.
  
# Making Changes

Please refer to the following web sites for guidance for information about working on asciidoc-based documentation.

* [DCS Contributing to Documentation](https://github.com/apache/incubator-trafodion/blob/master/dcs/src/main/asciidoc/_chapters/appendix_contributing_to_documentation.adoc) 
* [AsciiDoc User Guide](http://www.methods.co.nz/asciidoc/chunked/index.html)
* [AsciiDoc cheatsheet](http://powerman.name/doc/asciidoc)

Once you've made the desired changes, then do the following:

## DCS and REST

1. Build the document using **```mvn clean site```** against the directory containing the document; for example: **```dcs```** or **```core/rest```**.
2. Verify the content in the generated **```target```** directory. The **```target/index.html```** file provides the entry point for the web book; the **```target/apidocs/index.html```** file contains the entry point for API documentation.

## Other Documents

1. Build the document using **```mvn clean site```** against the directory containing the document; for example: **```docs/client_install```** or **```docs/odb_user```**.
2. Verify the content in the generated **```target```** directory. The **```target/index.pdf```** file contains the PDF version of the document while 
**```target/site/index.html```** contains the web-book version of the document. 

# Build Trafodion Document Tree
The external version of the Trafodion Document Tree is published to http://trafodion.incubator.apache.org/docs. Please refer to [Publish](#publish) below.

The build version of the  Trafodion Document Tree is located in **```docs/target/docs```**, which is created when you build the Trafodion web site in Maven. 

<table><tr><td><strong>NOTE</strong><br />Build the web site <strong>before</strong> you use the <strong><code>post-site</code></strong> phase to copy in the document files.
If you don't, then the web-site build will wipe out the Trafodion Document Tree.</td></tr></table>

## Version Directories

The Trafodion Document Tree consists of **Version Directories**:

Version Directory           | Content                                           | Web Site Directory
----------------------------|---------------------------------------------------|----------------------------------------------------
**```latest```**            | Known place for the latest version of a document. | **```trafodion.incubator.apache.org/docs/<document-name>```**
**```<version>```**         | Release-specific version of a document.           | **```trafodion.incubator.apache.org/docs/<version>/<document-directory>```**

* **```latest```**: Provides a well-known place for each document. This practice makes it possible to link to a document in instructional text, web sites, and
other documents.
* **```<version>```**: Provides per-release versions of documents. Previous versions are kept in the web site's SVN repository ensuring that N versions of the documentation
are available.

## Document Directories
Each document is placed in its own **Document Directory**:

Document                  | Document Directory Name
--------------------------|------------------------------------------
Client Installation Guide | **```client_install```**
Command Interface Guide   | **```command_interface```**
DCS Reference Guide       | **```dcs_reference```**
odb User Guide            | **```odb_user```**
REST Reference Guide      | **```rest_reference```**
SQL Reference Manual      | **```sql_reference```**

The Document Directories are organized as follows. Files and sub-directories may or may not be present in the Document Directory depending on document. 

File/Directory               | Content
-----------------------------|------------------------------------------------------------------------------------------------------
**```*.pdf```**              | The PDF version of the document. For example, **```Trafodion_SQL_Reference_Guide.pdf```**.
**```index.html```**         | The web book version of the document. Generated by asciidoc.
**```apidocs```**            | API documentation provided as a web book. Generated by javadoc.
**```apidocs/index.html```** | Entry point for API documentation. Generated by javadoc.
**```css```**                | CSS definitions used by the web-version of the document. Populated by asciidoc.
**```images```**             | Images used by the web-version of the document. Populated by asciidoc.
**```resouces```**           | Resource files referenced for source download etc. Populated by asciidoc.

The Document Directories are copied under the Version Directories thereby creating the web-accessible Trafodion document tree. 

## Populate Trafodion Documentation Tree
The build version of the Trafodion Document Tree is populated as follows:

<table>
  <tr>
    <th>Document</th>
    <th>Instructions</th>
  </tr> 
  <tr>
    <td><strong>Client Installation Guide</strong></td>
    <td>Run maven <strong><code>post-site</code></strong> build step.</td>
  </tr>
  <tr>
    <td><strong>DCS Reference Guide Guide</strong></td>
    <td>
      <ol>
         <li>Create Version Directories, if needed.</li>
         <li>Create Document Directories, if needed.</li>
         <li>Copy <strong><code>$SQ_HOME/dcs/target/site/</code></strong> to the appropriate Document Directories.</li>          
      </ol>
    </td>
  </tr>
  <tr>
    <td><strong>odb User Guide</strong></td>
    <td>Run maven <strong><code>post-site</code></strong> build step.</td>
  </tr>
  <tr>
    <td><strong>odb User Guide</strong></td>
    <td>Run maven <strong><code>post-site</code></strong> build step.</td>
  </tr>
  <tr>
    <td><strong>REST Reference Guide Guide</strong></td>
    <td>
      <ol>
         <li>Create Version Directories, if needed.</li>
         <li>Create Document Directories, if needed.</li>
         <li>Copy <strong><code>$SQ_HOME/core/rest/target/site/</code></strong> to the appropriate Document Directories.</li>          
      </ol>
    </td>
  </tr>
  <tr>
    <td><strong>SQL Reference Guide Guide</strong></td>
    <td>Run maven <strong><code>post-site</code></strong> build step.</td>
  </tr>
</table>


# Publish

<div class="alert alert-dismissible alert-info">
  <button type="button" class="close" data-dismiss="alert">&close;</button>
  <p style="color:black">Publication is done when a committer is ready to update the external web site. You do <strong>not</strong> perform these steps as part of checking in changes.</p></div>

Do the following: 

1. Build the web site.
2. Build the different document as described in [Making Changes](#making_changes) above.
3. Build the Trafodion Document Tree as described in [Build Trafodion Document Tree](#build_trafodion_document_tree) above.

The resulting **```docs/target/docs/```** directory is checked into the web-site SVN branch.



