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
This page describes how to change the Trafodion web pages. Please refer to the [Contribute](contribute.html) page for information about other ways to contribute to the Trafodion project.

#Organization

**Source**: **```docs/src```** in the Trafodion source tree.

**Publication:** https://git-wip-us.apache.org/repos/asf/incubator-trafodion-site.git

You develop and test all changes in the Trafodion source tree. Once checked in and built, the content of ```docs/target``` plus different [documentation](document.html) are copied to https://git-wip-us.apache.org/repos/asf/incubator-trafodion-site.git. The changes are then pushed out by Apache [svnpubsub](http://svn.apache.org/viewvc/subversion/trunk/tools/server-side/svnpubsub/), thereby populating http://incubator.trafodion.apache.org.

# Making Changes
The following information helps you understand how to make changes to the web-site content.

----

## Technology

The Trafodion website uses the following technologies:

* **Framework**: [Apache Maven Site](https://maven.apache.org)
* **Skin**: [Reflow Maven Skin](http://andriusvelykis.github.io/reflow-maven-skin/) 
* **Theme**: [Bootswatch Cerulean](http://bootswatch.com/cerulean)
* **Markdown**: [markdown](https://guides.github.com/features/mastering-markdown/) 

<a href="#" class="btn btn-primary btn-xs">Note</a> **```markdown```** was chosen since it supports inline HTML, which provides better table control than APT. Asciidoc does not work well with the Reflow Maven Skin — we tested.

**Note**: Markdown supports basic tables only; that is, you'll need to use ```<table>``` HTML definitions for formatted tables such as cells with bullet lists.

## Code Organization

The code is located in the **```docs```** directory. The code organization follows the [Maven standard](https://maven.apache.org/guides/mini/guide-site.html).

**```docs/src/site.xml```** is configured per the [Reflow Maven Skin documentation](http://andriusvelykis.github.io/reflow-maven-skin/skin/). Pages and menus are defined and configured in this file. By default, all pages use an automated table of contents; override as needed.

**```docs/src/site/markdown```** contains the files that generate the different HTML files.

**```docs/target```** contains the generated HTML files after you run a build.

## Managing Pages

You add/rename/delete pages in **```docs/src/site/markdown```**. You make corresponding changes in **```docs/src/site.xml```** adding/renaming/deleting pages from menus and defining page configuration; for example: removal of the table of contents bar and the special page formatting provided by the Reflow skin. Refer to the [Maven Documentation](http://maven.apache.org/plugins/maven-site-plugin/examples/sitedescriptor.html) for more information.

## Providing Content

When possible, ensure that you write in active voice and to the point. 

Special functions such as buttons etc. can be access by clicking **Preview** in the theme preview. There's a \<\> feature on each function, which allows you to copy the special **```\<div\>```** you need to insert the selected object.

## Development Environment

Typically, you'll use Eclipse to develop and build the website pages. The configuration goal is: **```clean site```**. The **```pom.xml```** file in the top-level directory drives the build steps for the web site.

## Testing Changes

The website files are located in **```docs/target```**. Open **```index.html```** from your browser and test your changes. For example, you want to validate the page layout, page navigation, links, and review the overall content on the pages you modified or added/deleted.

----

# Publishing

<div class="alert alert-dismissible alert-info">
  <button type="button" class="close" data-dismiss="alert">&close;</button>
  <p style="color:black">Publication is done when a committer is ready to update the external web site. You do <strong>not</strong> perform these steps as part of checking in changes.</p></div>

Do the following:

1. Check in your changes to the Trafodion source tree.
2. Build Trafodion.
3. If there are documentation changes: Follow the [documentation publishing](document.html#Publishing) instructions.
3. ```svn checkout``` https://git-wip-us.apache.org/repos/asf/incubator-trafodion-site.git
4. Copy content of ```docs/target``` into the svn ```asf-site``` directory.
5. ```svn commit``` the changes. 

Once committed, Apache [svnpubsub](http://svn.apache.org/viewvc/subversion/trunk/tools/server-side/svnpubsub/) takes care of populating http://incubator.trafodion.apache.org with your new changes.
