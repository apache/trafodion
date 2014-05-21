#!/usr/bin/python
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@

SP_SUCCESS                     = 0

class AMQPRoutingKey:
   def __init__(self, key = None):
      if key:
         self.SetFromString(key)

   def SetFromString(self, key):

      items = key.split('.')
      if len(items) == 6:
         self.__category = items[0]
         self.__package  = items[1]
         self.__scope    = items[2]
         self.__security = items[3]
         self.__protocol = items[4]
         self.__publication = items[5]
      elif len(items) == 2: # package and publication
         self.__category = 'health_state'
         self.__package  = items[0]
         self.__scope    = 'instance'
         self.__security = 'public'
         self.__protocol = 'gpb'
         self.__publication = items[1]
      elif len(items) == 3: # category, package and publication
         self.__category = items[0]
         self.__package  = items[1]
         self.__scope    = 'instance'
         self.__security = 'public'
         self.__protocol = 'gpb'
         self.__publication = items[2]
      else:
         raise TypeError('field number mismatch in %s' % key)

      if self.__protocol != 'gpb':
         raise TypeError('Protocol error %s' % key)

   def GetAsString(self):
      return '.'.join([self.__category, self.__package, self.__scope,
                       self.__security, self.__protocol, self.__publication])

   def GetTpaString(self):
      return '.'.join([self.__package, self.__scope,
                       self.__security, self.__publication])

   def packageName(self):
      return self.__package

   def messageName(self):
      return self.__publication

class TpaData:
   def __init__(self, client):
      self.__client = client
      self.__routingKey = None
      self.__body = None

   def clientName(self):
      return self.__client

   def routingKey(self, routingKey = None):
      if routingKey:
         self.__routingKey = routingKey

      return self.__routingKey

   def body(self, txt = None):
      if txt:
         self.__body = txt

      return self.__body

__all__ = ['SP_SUCCESS',
           'AMQPRoutingKey'
          ]

if __name__ == '__main__':
   print 'Wrapper definition'
