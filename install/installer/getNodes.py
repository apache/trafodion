#!/usr/bin/env python

import sqlite3
import os

conn = sqlite3.connect('sqconfig.db')

cur = conn.cursor()

cur.execute('SELECT nodeName FROM pnode')

nodes = cur.fetchall()

listOfNodes = list()

for node in nodes:
    listOfNodes.append(node[0])


nodelist = str()
for node in listOfNodes:
    nodelist = node + " " + nodelist

print nodelist

