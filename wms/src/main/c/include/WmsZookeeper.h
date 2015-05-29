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
#ifndef LIBWMS_WMSZOOKEEPER_H
#define LIBWMS_WMSZOOKEEPER_H

#define WMS_SERVERS "/wms/servers/running"
#define WMS_CLIENTS "/wms/clients"

void watcher(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx);
void getZkServerList();
void getHostAddr();
void removeClientZnode();
void closeZkSession();

#endif /* LIBWMS_WMSZOOKEEPER_H */
