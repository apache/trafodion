#!/bin/bash

# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e -x -u

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

source ${SCRIPT_DIR}/build-base-docker.sh

IMAGE_NAME="trafodion/base:${DOCKER_ENV_VERSION}"

if [ "$(uname -s)" == "Linux" ]; then
  USER_NAME=${SUDO_USER:=$USER}
  USER_ID=$(id -u "${USER_NAME}")
  GROUP_ID=$(id -g "${USER_NAME}")
else # boot2docker uid and gid
  USER_NAME=$USER
  USER_ID=1000
  GROUP_ID=50
fi

# create user home directory; download and install build tools
docker build -t "${IMAGE_NAME}-${USER_NAME}" - <<UserSpecificDocker
FROM ${IMAGE_NAME} 
RUN groupadd --non-unique -g ${GROUP_ID} ${USER_NAME}
RUN useradd -g ${GROUP_ID} -u ${USER_ID} -k /root -m ${USER_NAME}
ENV HOME /home/${USER_NAME}
RUN cd /home/${USER_NAME} \
 && mkdir download \
 && mkdir trafodion-build-tools \
 && wget https://raw.githubusercontent.com/apache/trafodion/master/install/traf_tools_setup.sh \
 && chmod +x traf_tools_setup.sh \
 && ./traf_tools_setup.sh -d ~/download -i ~/trafodion-build-tools \
 && rm -fr ./download \
 && yum install -y qt-devel qt-config
ENV TOOLSDIR /home/${USER_NAME}/trafodion-build-tools
ENV PATH $PATH:/home/${USER_NAME}/trafodion-build-tools/apache-maven-3.3.3/bin:/usr/lib64/qt4/bin/
UserSpecificDocker

# Go to root project folder
pushd ${SCRIPT_DIR}/../..

docker run -i -t \
  --rm=true \
  -w "/home/${USER_NAME}/trafodion" \
  -u "${USER_NAME}" \
  -v "$PWD:/home/${USER_NAME}/trafodion" \
  -v "$HOME/.m2:/home/${USER_NAME}/.m2" \
  --name TrafodionEnv \
  ${IMAGE_NAME}-${USER_NAME} \
  bash

popd
