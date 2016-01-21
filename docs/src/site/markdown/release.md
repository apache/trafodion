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
This page describes how to create a Trafodion release. You have to be a Trafodion committer to use these instructions.

# Prerequisites

## Create PGP Key
If you haven't done so already, then you need to create a PGP key so that you can sign the release. Please refer to: http://www.apache.org/dev/openpgp.html#generate-key.

Please remember to store your private key in a secure place.

**Example**

    gpg --gen-key   (verify that sha1 is avoided (last on list – see above web site)
    gpg -k  (shows public key)
    gpg -K (shows private key)

### Upload Public Key
Upload your public key to a public key server. We recommend using https://pgp.mit.edu/.

**Example**

    gpg --send-keys <keyID> --keyserver pgp.mit.edu

### Create Revocation Certificate
Create a revocation certification using the instructions at: http://www.apache.org/dev/openpgp.html#revocation-certs.

Please remember to store it in a secure place separate from your PGP keys.

**Example**

    gpg --output revoke-<keyD>.asc --armor --gen-revoke <keyID>

# Add PGP to KEYS File
Do the following:

    svn co https://dist.apache.org/repos/dist/release/incubator/trafodion traf_release
    cd traf_release 
    gpg --list-sigs <keyID> >> KEYS
    gpg  -armor –export <keyID>
    svn commit –m “added new public key to KEYS file“

# Prepare Artifacts
## Prepare New Release

1. Send a message out to the community indicating that a new release is being planned.  In this message, indicate what is planned for the release and when the release is scheduled.
2. Give contributors enough time to assimilate this information so they can make plans to deliver their changes.  Recommend giving the community several weeks notice.
3. Review open issues and planned features; determine what JIRA's should be included in the release.

## Verify Release Requirements
You need to ensure that:

* A DISCLAIMER file exists in the top level directory containing correct information. Please refer to: http://incubator.apache.org/guides/branding.html#disclaimers.
* NOTICE and LICENSE files exist in the top level directory which includes all third party licenses used in the product. Please refer to:  http://www.apache.org/dev/licensing-howto.html.
* A README file exists and is up to date in the top level directory describing the release.
* The source release contains source code only, no binaries.
* The provenance of all source files is clear.
* All source files have Apache license headers, where possible.  Where not possible, then the exceptions are written up in the RAT_README file located in the top level directory.
* RAT report is clean.
* Copyright dates are current.
* Build instructions are provided and can be run successfully.
* Test instructions are provided and can be run successfully.

## Create Release Branch
Prior to releasing, send a message to the community indicating that a new release is imminent and that a new branch will be created to build the artifacts.

After the new release branch is created, send another message to the community indicating that the branch is available and the deliveries will be monitored.  Allow deliveries on the main branch to continue.

Verify that all required changes have been delivered.

## Create Artifacts
Trafodion uses git as its repository.  When a new version is created, mark the repository with the tag to make sure it source tar can be recreated.

### Create Tag

**Example: Release x.x.x and release candidate 1 (rc1)**

    git checkout -b tagx.x.x <release branch name>
    git tag -a x.x.xrc1
    git show x.x.xrc1
    git push apache x.x.xrc1
    git tag

Once completed, a new source tar file exist in the distribution directory. 

### Create Artifact Checksums and Signatures
**Assumption**: You've already created the signing key and registered it at the https://pgp.mit.edu/ repository.

    gpg --armor --output apache-trafodion-x.x.x-incubating-src.tar.gz.asc --detach-sig apache-trafodion-x.x.x-incubating-src.tar.gz
    gpg --verify apache-trafodion-x.x.x-incubating-src.tar.gz.asc
    md5sum apache-trafodion-x.x.x-incubating-src.tar.gz > apache-trafodion-x.x.x-incubating-src.tar.gz.md5
    sha1sum apache-trafodion-x.x.x-incubating-src.tar.gz > apache-trafodion-x.x.x-incubating-src.tar.gz.sha 

# Test Artifacts

## Build and Test Source tar File
Build and test the source tar file using the [Build Trafodion](build.html) instructions. You should perform this test on the following environments:

* Test build on a fresh VM.
* Test build using the tagged version from git

## Compare Tagged Version with Source tar File
Compare the code from the source tar file with the tagged version to make sure they match. The follow example assumes that the branch artifacts contains the release candidates.

    mkdir traf_test
    cd traf_test
    cp <git dir>/incubator-trafodion/distribution/* .
    tar zxf apache-trafodion-x.x.x-incubating-src.tar.gz

Compare the two versions; for example, by using BCompare and the "Folder Compare Report" feature:

    old: traf_test/incubator-trafodion 
    new: <git dir>/incubator-trafodion

**Note:**  The git version will have some additional git folders and the distribution directory.

## Verify Apache Requirements
Verify checksums and signatures using the [Verify Signature](#Verify_Signatures) instructions below.

Ensure that the high-level directory contains valid version of:

* **```DISCLAIMER.txt```**
* **```LICENSE.txt```**
* **```NOTICE.txt```**
* **```RAT_README.txt```**
* **```README.txt```**

# Stage Artifacts
Once all the artifacts have been created and tested, then it's time to stage them.  Upload the artifacts to the https://dist.apache.org/repos/dist/dev/incubator/trafodion directory.

1. Make sure **```svn```** exists. (It can be downloaded using **```yum```**.)
    * **```which svn```**
    * **```svn --version```** (version 1.6.11 works)
2. Create a directory to store the **```svn```** repositoy
3. Checkout source code. This creates a directory called incubator.
    * **```svn co https://dist.apache.org/repos/dist/dev/incubator```**
4. **```cd trafodion```**
5. Create a new directory for the release: **```mkdir apache-trafodion-x.x.x-incubating```**
6. **```cd <apache-trafodion-x.x.x-incubating>```**
7. Copy the four files to the incubating directory.
8. Ensure that you do an **```svn add```** for the new directory and all four files
9. Ask for a review of the changes
10. Commit your changes
    * **```svn status```**
    * **```svn commit –m "message…"```**
    * Go to https://dist.apache.org/repos/dist/dev/incubator to see if your changes were committed

# Verification
All artifacts have been uploaded to the staging area.

## Verify Signatures
Download all the artifacts from the staging area including:

    apache-trafodion-x.x.x-incubating-src.tar.gz
    apache-trafodion-x.x.x-incubating-src.tar.gz.asc
    apache-trafodion-x.x.x-incubating-src.tar.gz.md5
    apache-trafodion-x.x.x-incubating-src.tar.gz.sha 

Check signatures and checksums.

**```apache-trafodion-x.x.x-incubating-src.tar.gz.asc```**

    # View public key
    gpg apache-trafodion-x.x.x-incubating-src.tar.gz.asc
    
    # Expect
      gpg: Signature made Tue 03 Nov 2015 12:59:10 AM UTC using RSA key ID A44C5A05
      gpg: Can't check signature: No public key

    # Extract public key from key ID returned above
    gpg --keyserver pgpkeys.mit.edu --recv-key A44C5A05

    # Expect:
      gpg: requesting key A44C5A05 from hkp server pgpkeys.mit.edu
      gpg: /home/centos/.gnupg/trustdb.gpg: trustdb created
      gpg: key A44C5A05: public key "Jane Doe (CODE SIGNING KEY) <jdoe@apache.org>" imported
    
    # Verify signature
    gpg --verify apache-trafodion-x.x.x-incubating-src.tar.gz.asc
    # Expect:
      gpg: Signature made <date> using RSA key ID A44C5A05
      gpg: Good signature from "Roberta Marton (CODE SIGNING KEY) <rmarton@apache.org>"
      gpg: WARNING: This key is not certified with a trusted signature!
      gpg: There is no indication that the signature belongs to the owner.
 
 **```apache-trafodion-x.x.x-incugating-src.tar.gz.md5```**
 
     md5sum -c apache-trafodion-x.x.x-incubating-src.tar.gz.md5
     
     # Expect:
       apache-trafodion-x.x.x-incubating-src.tar.gz: OK

**```apache-trafodion-x.x.x-incubating-x.x.x-incubating-src.tar.gz.sha```**

    sha1sum -c apache-trafodion-x.x.x-incubating-src.tar.gz.sha
    
    # Expect:
      apache-trafodion-x.x.x-incubating-src.tar.gz: OK

## Verify Apache Requirements
Ensure that the high-level directory contains valid version of:

* **```DISCLAIMER.txt```**
* **```LICENSE.txt```**
* **```NOTICE.txt```**
* **```RAT_README.txt```**
* **```README.txt```**

Next, run **```rat```** to make sure all files have Apache copyrights.

# Complete Release

To be completed.