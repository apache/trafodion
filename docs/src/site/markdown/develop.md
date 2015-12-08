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
This page describes how to help develop the Trafodion source tree. Please refer to the [Contribute](contribute.html) page for information about other ways to contribute to the Trafodion project.

# Prerequisites
You need to register as a Trafodion contributor before you can help us develop Trafodion. Please perform the following registration actions:

<table>
    <tr>
      <th style="width:30%;">Area</th>
      <th style="width:55%;">Notes</th>
      <th style="width:15%;">URL</th>
    </tr>
    <tr>
       <td><strong>Individual Contributor License Agreement (ICLA)</strong></td>
       <td>You should sign the ICLA before contributing content to the Trafodion source tree. (Required to become a committer.)</td>
       <td><a href="https://www.apache.org/licenses/icla.txt">ICLA Agreement</a><br />
           <a href="http://www.apache.org/licenses/">Approval Process</a>
       </td>
    </tr>
    <tr>
       <td><strong>Source Control</strong></td>
       <td>You must have a git account in order to contribute to the Trafodion source. If you haven't already done so, please join git.</td>
       <td><a href="https://github.com/join">Git Signup</a></td>
    </tr>
    <tr>
       <td><strong>Defect Tracking</strong></td>
       <td>In order to have certain permissions, including assigning issues to yourself, you need to be a Contributor in the project. Be sure to sign up for a JIRA account if you don't have one.</td> 
       <td><a href="https://issues.apache.org/jira/secure/Signup!default.jspa">Jira Signup</a></td>
    </tr>
</table>

Please send an e-mail to the [Trafodion development list](mail-lists.html) with the approved ICLA attached. Include your git and Jira IDs.
   
Wait for the response and then you're ready to help us develop Trafodion.

# Development Environment
You use the following tools and guidelines to develop Trafodion:

<table>
  <body>
    <tr>
      <th style="width:15%;">Area</th>
      <th style="width:15%;">Tool</th>
      <th style="width:55%;">Notes</th>
      <th style="width:15%;">Location</th>
    </tr>
    <tr>
       <td><strong>Trafodion Architecture</strong></td>
       <td>Document</td>
       <td>Please review the Trafodion architecture to ensure that you understand how the different components related to each other.</td>
       <td><a href="architecture-overview.html">Trafodion Architecture</a></td>
    </tr>
    <tr>
       <td><strong>Defect Tracking</strong></td>
       <td>Jira</td>
       <td>View all the Trafodion defects and enhancements requests in the Jira system hosted by Apache.</td>
       <td><a href="https://issues.apache.org/jira/browse/TRAFODION">Trafodion Jiras</a></td>
    </tr>
    <tr>
       <td><strong>Defect Management</strong></td>
       <td>Document</td>
       <td>Please read about our approach to defect management. Mostly, any changes you'll make will be in response to a defect reported in Jira.</td>
       <td><a href="defect-management.html">Defect Management (TBD)</a></td>
    </tr>
    <tr>
       <td><strong>Git Tools</strong></td>
       <td>git</td>
       <td><p>Most of the Trafodion development is done on Linux. Development of the web site and/or documentation can successfully be done on Windows.</p><p>Please download the appropriate tool version; Linux or Windows.</p><p>Then, please refer to <a href="https://help.github.com/articles/set-up-git/">GitHub Documentation</a> for information on how to set up your git environment. Ensure that you register your <a href="https://github.com/settings/ssh">ssh keys</a>.</p></td>
       <td><a href="http://git-scm.com/downloads">Download git</a></td>
    </tr>
    <tr>
       <td><strong>Code Repository</strong></td>
       <td>git</td>
       <td>The full Trafodion source tree can be retrieved from either of these repositories.</td>
       <td><a href="https://git-wip-us.apache.org/repos/asf/incubator-trafodion.git">Apache Repository</a><br /><a href="https://github.com/apache/incubator-trafodion">GitHub Mirror</a>
       </td>
    <tr>
    </tr>
       <td><strong>Code Organization</strong></td>
       <td>Document</td>
       <td>Please familiarize yourself with the Trafodion code organization.</td>
       <td><a href="code-organization.html">Code Organization</a></td>
    </tr>
    <tr>
       <td><strong>C++ Coding Guidelines</strong></td>
       <td>Document</td>
       <td>Please read the coding guidelines for the Trafodion C++ code before making changes.</td>
       <td><a href="cplusplus-coding-guidelines.html">C++ Coding Guidelines</a></td>
    </tr>
    <tr>
       <td><strong>Debugging Tips</strong></td>
       <td>Document</td>
       <td>Documented tips describing how to debug your code in unit testing.</td>
       <td><a href="debugging-tips.html">Debugging Tips (TBD)</a></td>
    </tr>
    <tr>
       <td><strong>Testing</strong></td>
       <td>Document</td>
       <td>Trafodion has a rich set of test suites for each of its components. You'll need to run the tests before submitting a code change for review.</td>
       <td><a href="testing.html">How to Test</a></td>
    </tr>
    <tr>
       <td><strong>Code Reviews</strong></td>
       <td>git</td>
       <td>
          <p>We use GitHub pull-requests for code review. All of the activity on github is captured in ASF JIRA and/or ASF project mail archives by ASF INFRA team automation. In this way, we do not depend on github for accurate history of where contributions come from.</p>
          <p>Each pull-request title should start with a JIRA ID in brackets, so that activity can be logged to the correct JIRA issue.</p>
          <p>Regardless of the title, the pull-request activity is also logged to the <a href="http://mail-archives.apache.org/mod_mbox/incubator-trafodion-codereview/">code-review mail list</a>.</p>
       </td>
       <td><a href="https://github.com/apache/incubator-trafodion/pulls">Current Pull Requests</a></td>
    </tr>
  </body>
</table>

# Initial Setup
This set of tasks is performed **after** downloading the git tools. Refer to [Development Environment](#development_environment) above.

You should not have to perform these tasks more than once.

## Setting Up the Git Enviroment
If you have not done so already, now is the time to set up your **<code>git</code>** environment. Refer to the [GitHub Documentation](https://help.github.com/articles/set-up-git/) for information on how to set up your Git environment. Please ensure that you register your [ssh keys](https://github.com/settings/ssh).

## Fork the Trafodion Repository
You create a private fork of Trafodion on <https://github.com/apache/incubator-trafodion>. Use the **fork** button top-right on the page to create your fork, which will be named **\<your-git-id\>_fork.**

The following examples use **trafdeveloper** to represent **\<your-git-id\>**.

## Clone the Trafodion Repository
Use the **git shell** to perform this task.

    # Move to the directory where you want to install the Trafodion source code.
    cd mysource
    # Clone the Trafodion source code
    git clone git://git.apache.org/incubator-trafodion.git
    # Register your fork as a remote branch
    git remote add trafdeveloper_fork git@github.com:trafdeveloper/incubator-trafodion

At this point, you've finished all preparation steps. Now, you can start making changes.

# Making Changes
## Create a Task Branch
You create a task branch to make changes to the Trafodion source. Typically, we name the branches after the Jira we are working on. In this example, the Jira is: **TRAFODION-1507**.

    # Ensure that you have the latest changes
    git fetch --all
    # Checkout source
    git checkout -b TRAFODION-1507 origin/master
   
## Change Recipes
The procedure to make changes depends on what type of problem or feature you're working on.

Change Type                      | Refer To
---------------------------------|------------------------------------------------------------
**Code**                         | [Modify Code](code.html)
**Documentation**                | [Modify Documentation](document.html)
**QA Tests**                     | [Modify Tests](tests.html)
**Web Site**                     | [Modify Web Site](website.html)

## Commit Changes

<div class="alert alert-dismissible alert-info">
  <button type="button" class="close" data-dismiss="alert">&close;</button>
  <p style="color:black"><strong>Reminder</strong></p>
  <p style="color:black">If making code changes: please ensure that you run the <a href="testing.html">Regression Tests</a> before committing changes.</p>
</div>

Perform the following steps to commit your changes.

    # Commit changes
    git commit -a
    # Dry-run check
    git push -n trafdeveloper_fork HEAD
    # Push changes to your private fork
    git push trafdeveloper_fork TRAFODION-1507

## Create Pull Request
Your changed code needs to be reviewed by a Trafodion committer. Therefore, you need to create a pull request for your private repositoryc.

    # Generate pull request
    git pull-request

Ensure that you include the Jira ID at the beginning of the title in your pull request. For example:

    [TRAFODION-1507] Explanation of the changes you made.

[Automated Tests](#automated_tests) are normally triggered to run on every pull request. If you have modified the documentation or the web site, then you can skip the automated testing by adding the following phrase to the comments of the pull request:

    jenkins, skip test

## Review Comments
The pull request gets reviewed by the committers and once you get a consensus, then the committer merges your changes into the main incubator-trafodion branch.

## Address Review Comments
Follow the GitHub conversation on your pull request (you should be automatically subscribed). Respond to questions and issues.

If you need to make additional changes, then do the following:

1. Check out the code: **<code>git checkout TRAFODION-1507</code>**
2. Make the requested changes.
3. Run regression tests.
4. Commit the changes: **<code>git commit -a</code>**
5. Push the changes back to your private git fork: **<code>git push trafdeveloper_fork TRAFODION-1507</code>**

## Merge Changes
If all is well, a committer will merge your change into the Apache repository, which is mirrored on github.

You may be asked to close out the JIRA or other follow up.

Your change is done. Thanks for your contribution to Trafodion.

# Automated Tests
Automated tests take several hours to complete from when your pull-request was approved by a committer or updated with a new commit.

Normally, the Traf-Jenkins user will post a message in the pull-request with a link to the results. You can also check the Jenkins server to see the status even before the tests are finished. Look in the **Build History** table for the build/test job that matches your pull-request. For example, the master branch tests are located at: <https://jenkins.esgyn.com/job/Check-PR-master/>

## Reviewing Logs

There are two approaches to reviewing logs.

### Approach 1

* The first two columns in build-job table are links to the specific sub-job. Clicl on the link to drill down.
* The console log of each job has a link to the log file directories (close to the top). Look for **Detailed logs**.

### Approach 2

* Go to: <http://traf-logs.esgyn.com/PullReq/>
* Click on the number of the pull request. The next directory level is the build number. With multiple commits or re-tests, it is possible for a pull request to have multiple builds.
* Under the build number is a directory for each specific job. Example: <http://traf-logs.esgyn.com/PullReq/18/35/regress-seabase-ahw2.2/>

## More Information
The check tests do not include all of the automated daily tests. If you (or another contributor) want, you can run additional tests on the pull request. Refer [automated test setup (TBD)](automated-tests.html) for more information.