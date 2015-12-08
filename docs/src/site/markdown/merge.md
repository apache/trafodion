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
This page describes how a committer merges changes into the git repository.

Additional information about the Apache committer process can be found on the [Git at the Apache Software Foundation](https://git-wip-us.apache.org/) page. 

# Initial Set Up

<table>
  <tr>
    <th width="15%">Step</th>
    <th width="35%">Task</th>
    <th width="50%">How-To</th>
  </tr>
  <tr>
    <td><strong>Configure git E-Mail</strong></td>
    <td>Configure <strong><code>git</code></strong> to use your Apache e-mail address.</td>
    <td><pre>git config --global user.email myusername@apache.org</pre></td>
  </tr>
  <tr>
    <td><strong>Check <code>trafodion-contributors</code> group</strong></td>
    <td>Check that your github user is a <strong>public</strong> member in the <strong><code>trafodion-contributors</code></strong> group. This allows some permissions with the Jenkins test server.</td>
    <td><a href="https://github.com/orgs/trafodion-contributors/people">https://github.com/orgs/trafodion-contributors/people</a></td>
  </tr>
  <tr>
    <td><strong>Set Up Work Space</strong></td>
    <td>Set up your work space so that you can merge pull requests.</td>
    <td>
      <ul>
        <li>In VNC/Gnome environment, either add to <strong><code>.bashrc</code></strong> or type at your current shell: <pre>unset SSH_ASKPASS</pre></li>
        <li>Pushing code to the Apache repository requires password authentication.</li>
        <li>Ensure that your work space is cloned from github: <pre>git clone https://github.com/apache/incubator-trafodion</pre></li>
        <li>Ensure that you have a remote pointing to the Apache repo. (Setting only the push URL with username does not seem to work.) <pre>git remote add apache https://USERNAME@git-wip-us.apache.org/repos/asf/incubator-trafodion.git</pre></li>
      </ul>  
    </td>
  </tr>
</table>  

# Automated Testing
You can interact with Jenkins testing via pull request (PR) comments. All of these commands should start with "jenkins," to not confuse other users. You can add more to the end of the message if you want. Jenkins just pattern matches the string, and will ignore trailing comments.

* If an unknown user submits a PR, then Jenkins automation will post a message to github asking if it is okay to test.
    * Review the pull request. If the code is not malicious and is okay to test, post a comment <pre>jenkins, ok</pre>

* If the author is a trusted contributor, you can add them to a white-list of known users.
    * Post a comment <pre>jenkins, add user</pre>

* Consider inviting them to the **```trafodion-contributors```** github group as well.
    * New commits to the PR will trigger a new build. You can also trigger a retest without a new commit.
    * Post a comment <pre>jenkins, retest</pre>
    
# Validate Review Criteria
The project committee (PPMC) has agreed that the following review criteria are used for contributions:

* Code Review(s)
* Time available for comments
* Testing
    * **Be sure that you wait for all pending tests!**
    * New commits, even those that are just merging with latest master branch, trigger new test run.
* Legal
* Other

# Merge Pull Request
Use the following procedure to merge a pull request.

<table>
  <tr>
    <th width="15%">Step</th>
    <th width="35%">Task</th>
    <th width="50%">Commands</th>
  </tr>
  <tr>
    <td><strong>Check Status</strong></td>
    <td>
      <p>Check the pull request status on github, at the bottom of the pull request page. It will tell you if there are any merge conflicts with master branch.</p>
      <p><strong>NOTE</strong></p>
      <p>If there are conflicts, either ask the contributor to merge up, or be prepared to resolve the conflicts yourself.</p>
    </td>
    <td></td>
  </tr>
  <tr>
    <td><strong>Create Local Merge Branch</strong></td>
    <td>Create a local merge branch, based on the latest, greatest.</td>
    <td><pre># You will be prompted for your Apache password
git fetch apache
git checkout -b mrg_12345 apache/master</pre></td>
  </tr>
  <tr>
    <td><strong>Fetch Pull Request Branch</strong></td>
    <td>Fetch pull request branch to default destination <strong><code>FETCH_HEAD</code></strong></td>
    <td><pre>git fetch origin +refs/pull/12345/head</pre></td>
  </tr>
  <tr>
    <td><strong>Merge Locally</strong></td>
    <td>Merge locally, giving message that includes JIRA ID.</td>
    <td>
      <p><pre>git merge --no-ff -m "Merge [TRAFODION-XYZ] PR-12345 Whizbang feature" \
FETCH_HEAD</pre></p>
      <p><strong>NOTES</strong></p>
      <ul>
        <li>Sometimes you might want to squash their branch into a single commit. If so, add the <strong><code>--squash</code></strong> option.</li>
        <li>If you forget the <strong><code>-m</code></strong> option, you end up with a less than helpful default comment.
          <ul>
            <li><strong>Before you push the commit</strong>, you can fix the comment by:<pre>git commit --amend</pre></li>
          </ul>
        </li>
      </ul>
    </td>
  </tr>
  <tr>
    <td><strong>Additional Checks</strong></td>
    <td>Additional checks of what you are preparing to push.</td>
    <td>
      <pre>git log apache/master..HEAD
git diff apache/master HEAD</pre></td>
  </tr>
  <tr>
    <td><strong>Push Changes</strong></td>
    <td>Push changes to the Apache repository, specifying the source and the destination branch.</td>
    <td><pre># You will be prompted for your Apache password
git push apache HEAD:master</pre></td>
  </tr> 
</table>

# Completion

1. Close Jira, if appropriate, or ask the contributor to do so.
2. If ASF automation does not close the pull request, ask the contributor to do so.