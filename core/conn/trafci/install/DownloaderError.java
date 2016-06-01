// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

public interface DownloaderError
{
    String CONNECTION_ERR = "Connection was refused to the remote host.";
    String SERVER_UNAVAILABLE_ERR = "The remote server is currently unavailable.";
	String HTTP_INVALID_URL_ERR = "An invalid or malformed URL was specified.";
	String INVALID_LOCAL_FILE_ERR = "Invalid target filename specified for local disk.";
    String INVALID_REMOTE_FILE_ERR = "Invalid URL specified, remote file not found.";
	String BIND_ERR = "Unable to bind to socket.";
	String PROXY_ERR="Unable to connect to proxy server.";
	String SOCKET_ERR= "The connection was reset.";
	String UNKNOWN_ERR= "An unknown error has occurred, download failed to complete. Check proxy settings.";
}

