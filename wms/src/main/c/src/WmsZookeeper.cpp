/**
 *(C) Copyright 2013 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "GlobalHeader.h"
#include "WmsException.h"
#include "WmsZookeeper.h"

static const char* state2String(int state){
  if (state == 0)
    return "CLOSED_STATE";
  if (state == ZOO_CONNECTING_STATE)
    return "CONNECTING_STATE";
  if (state == ZOO_ASSOCIATING_STATE)
    return "ASSOCIATING_STATE";
  if (state == ZOO_CONNECTED_STATE)
    return "CONNECTED_STATE";
  if (state == ZOO_EXPIRED_SESSION_STATE)
    return "EXPIRED_SESSION_STATE";
  if (state == ZOO_AUTH_FAILED_STATE)
    return "AUTH_FAILED_STATE";

  return "INVALID_STATE";
}

void
ensureConnected(){
	int               rc;
	static struct timespec time_to_wait = {0, 0};

	pthread_mutex_lock(&lock);
    while (zoo_state(zh)!=ZOO_CONNECTED_STATE) {
    	time_to_wait.tv_sec = time(NULL) + WAIT_TIME_SECONDS;
        rc = pthread_cond_timedwait(&cond, &lock, &time_to_wait);
        if (rc == ETIMEDOUT) {
        	pthread_mutex_unlock(&lock);
        	stringstream message;
			message << "ZOOKEEPER is not running [" << zk_ip_port.str().c_str() << "]";
			throw WmsException(ERROR_OPEN, 0, message.str().c_str());
        }
    }
	pthread_mutex_unlock(&lock);
}

void
watcher(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx) {
    /* Be careful using zh here rather than zzh - as this may be mt code
     * the client lib may call the watcher before zookeeper_init returns */
	(void)watcherCtx;

    printf( "Watcher %d state = %s", type, state2String(state));
    if (path && strlen(path) > 0) {
      printf(" for path %s", path);
    }
    printf("\n");

    if(type == ZOO_SESSION_EVENT){
        if(state == ZOO_CONNECTED_STATE){
            pthread_mutex_lock(&lock);
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&lock);
        }
    }

    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            const clientid_t *id = zoo_client_id(zzh);
            if (myid.client_id == 0 || myid.client_id != id->client_id) {
                myid = *id;
                printf("Got a new session id: %lld\n", (long long)myid.client_id);
             }
        } else if (state == ZOO_AUTH_FAILED_STATE) {
            printf("Authentication failure.\n");
            zookeeper_close(zzh);
            zh=0;
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            printf("Session expired.\n");
            zookeeper_close(zzh);
            zh=0;
        }
    }
}

void
getZkServerList() {
	stringstream message;
	String_vector servers;
	stringstream path;

	getHostAddr();

	try {

		// +++ Temp fix for ZK session expiry issue.
		if( myZkHandle )
		{
			zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
			zh = zookeeper_init(zk_ip_port.str().c_str(), watcher, 30000, &myid, NULL, 0);
			ensureConnected();
		}

		struct Stat stat;
		path << "/" << getenv("USER") << WMS_SERVERS;
		if (ZNONODE == zoo_exists(zh, path.str().c_str(), 0, &stat)) {
			message << "znode " << path << " does not exist";
			throw message.str();
		}

		int rc = zoo_get_children(zh, path.str().c_str(), 0, &servers);
		switch(rc) {
		case ZOK:
			printf("zoo_get_children succeeded\n");
			break;
		case ZNONODE:
			message << "The node does not exist";
			throw message.str();
			break;
		case ZNOAUTH:
			message << "The client does not have permission";
			throw message.str();
			break;
		case ZBADARGUMENTS:
			message << "Invalid input parameters";
			throw message.str();
			break;
		case ZINVALIDSTATE:
			message << "zhandle state is either ZOO_SESSION_EXPIRED_STATE or ZOO_AUTH_FAILED_STATE";
			throw message.str();
			break;
		case ZMARSHALLINGERROR:
			message << "Failed to marshall a request; possibly, out of memory";
			throw message.str();
			break;
		}

		path.str("");
		path << "/" << getenv("USER") << WMS_CLIENTS << "/" << myIpAddr << ":" << connectionInfo;
        rc = zoo_create(zh, path.str().c_str(), NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, 0, 0);
		switch(rc) {
		case ZOK:
			printf("created znode successfully\n");
			break;
		case ZNONODE:
			message << "The node does not exist";
			throw message.str();
			break;
		case ZNOAUTH:
			message << "The client does not have permission";
			throw message.str();
			break;
		case ZBADARGUMENTS:
			message << "Invalid input parameters";
			throw message.str();
			break;
		case ZINVALIDSTATE:
			message << "zhandle state is either ZOO_SESSION_EXPIRED_STATE or ZOO_AUTH_FAILED_STATE";
			throw message.str();
			break;
		case ZMARSHALLINGERROR:
			message << "Failed to marshall a request; possibly, out of memory";
			throw message.str();
			break;
		}
	} catch (string str) {
		closeZkSession();
		throw WmsException(ERROR_OPEN, 0, str.c_str());
	}

//	if (zh != 0)
//		zookeeper_close(zh);

	wmsServers.clear();
	for (int i=0; i < servers.count; i++) {
		wmsServers.push_back(servers.data[i]);
	}
}

void removeClientZnode()
{
	stringstream path;
	stringstream message;

	if( !zh )
		return;

	try {
		path.str("");
		path << "/" << getenv("USER") << WMS_CLIENTS << "/" << myIpAddr << ":" << connectionInfo;
		int rc = zoo_delete(zh, path.str().c_str(), -1);
		switch(rc) {
		case ZOK:
			printf("deleted znode successfully\n");
			break;
		case ZNONODE:
			message << "The node does not exist";
			throw message.str();
			break;
		case ZNOAUTH:
			message << "The client does not have permission";
			throw message.str();
			break;
		case ZBADARGUMENTS:
			message << "Invalid input parameters";
			throw message.str();
			break;
		case ZINVALIDSTATE:
			message << "zhandle state is either ZOO_SESSION_EXPIRED_STATE or ZOO_AUTH_FAILED_STATE";
			throw message.str();
			break;
		case ZMARSHALLINGERROR:
			message << "Failed to marshall a request; possibly, out of memory";
			throw message.str();
			break;
		}
	} catch (string str) {
		closeZkSession();
		throw WmsException(ERROR_OPEN, 0, str.c_str());
	}
}

void closeZkSession()
{
	removeClientZnode();
	if (myZkHandle && zh != 0)
		zookeeper_close(zh);
}

void
getHostAddr() {
	struct ifaddrs *myaddrs, *ifa;
	void *in_addr;
	char buf[64];

	if(getifaddrs(&myaddrs) != 0)
		throw WmsException(ERROR_OTHER, 0, strerror(errno));

	for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;
		//if (!(ifa->ifa_flags & IFF_UP))
		//	continue;

		switch (ifa->ifa_addr->sa_family) {
		case AF_INET:
		{
			struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
			in_addr = &s4->sin_addr;
			break;
		}
		//case AF_INET6:
		//{
		//	struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)ifa->ifa_addr;
		//	in_addr = &s6->sin6_addr;
			break;
		//}
		default:
			continue;
		}

		if (!inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf))){
			//printf("%s: inet_ntop failed!\n", ifa->ifa_name);
			stringstream message;
			message << "inet_ntop failed! " << ifa->ifa_name;
			freeifaddrs(myaddrs);
			throw WmsException(ERROR_OPEN, 0, message.str().c_str());
		} else {
			//printf("%s: %s\n", ifa->ifa_name, buf);
			strcpy(myHostName,ifa->ifa_name);
			strcpy(myIpAddr,buf);
		}
	}

	freeifaddrs(myaddrs);
}

