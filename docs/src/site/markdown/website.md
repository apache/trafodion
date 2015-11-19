<!--
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the 
  License.
-->
# Making Changes

The Trafodion web site is part of the Trafodion source tree. The following information helps you understand how to make changes to the web-site content.

----

## Technology

The Trafodion website uses the following technologies:

* **Framework**: [Apache Maven Site](https://maven.apache.org)
* **Skin**: [Reflow Maven Skin](http://andriusvelykis.github.io/reflow-maven-skin/) 
* **Theme**: [Bootswatch Cerulean](http://bootswatch.com/cerulean)
* **Markup**: [markdown](https://daringfireball.net/projects/markdown/syntax) 

<a href="#" class="btn btn-primary btn-xs">Note</a> **<code>markdown</code>** was chosen since it supports inline HTML, which provides better table control than APT. Asciidoc does not work well with the Reflow Maven Skin — we tested.

## Code Organization

The code is located in the **<code>docs</code>** directory. The code organization follows the [Maven standard](https://maven.apache.org/guides/mini/guide-site.html).

**<code>docs/src/site.xml</code>** is configured per the [Reflow Maven Skin documentation](http://andriusvelykis.github.io/reflow-maven-skin/skin/). Pages and menus are defined and configured in this file. By default, all pages use an automated table of contents; override as needed.

**<code>docs/src/site/markdown</code>** contains the files that generate the different HTML files.

**<code>docs/target</code>** contains the generated HTML files after you run a build.

## Managing Pages

You add/rename/delete pages in **<code>docs/src/site/markdown</code>**. You make corresponding changes in **<code>docs/src/site.xml</code>** adding/renaming/deleting pages from menus and defining page configuration; for example: removal of the table of contents bar and the special page formatting provided by the Reflow skin. Refer to the [Maven Documentation](http://maven.apache.org/plugins/maven-site-plugin/examples/sitedescriptor.html) for more information.

## Providing Content

When possible, ensure that you write in active voice and to the point. 

Special functions such as buttons etc. can be access by clicking **Preview** in the theme preview. There's a \<\> feature on each function, which allows you to copy the special **<code>\<div\></code>** you need to insert the selected object.

## Development Environment

Typically, you'll use Eclipse to develop and build the website pages. The configuration goal is: **<code>clean site</code>**. The **<code>pom.xml</code>** file in the top-level directory drives the build steps for the web site.

## Testing Changes

The website files are located in **<code>docs/target</code>**. Open **<code>index.html</code>** from your browser and test your changes. For example, you want to validate the page layout, page navigation, links, and review the overall content on the pages you modified or added/deleted.

----

# Publishing Web Site

To be written.   