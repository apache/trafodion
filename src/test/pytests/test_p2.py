'''
/**
*(C) Copyright 2014 Hewlett-Packard Development Company, L.P.
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
PYODBC TESTS
'''
import unittest
import pyodbc 
import ConfigParser

cnxn = 0
Config = ConfigParser.ConfigParser()
Config.read("./config.ini")            
dsn = Config.get("pytest","dsn")
usr = Config.get("pytest","usr")
pwd = Config.get("pytest","pwd")
tcp = Config.get("pytest","tcp")
catalog = Config.get("pytest","catalog")
schema = Config.get("pytest","schema")

class ConnectTest(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test11(self):
        try:
            connect_str = 'DSN=' + dsn + ';UID=' + usr + ';PWD=' + pwd
            print connect_str
            cnxn = pyodbc.connect(connect_str, autocommit=True)
        except Exception,e:
            print str(e)
            assert 0, 'DSN Connection failed'
        else:
            cnxn.close();

    def test12(self):
        try:
            cnxn = pyodbc.connect('Driver=Trafodion;Server=' + tcp + ';UID=' + usr + ';PWD=' + pwd + ';', autocommit=True)
        except Exception,e: 
            print str(e)
            assert 0, 'DSNless connection failed'
        else:
            cnxn.close()
    
class SQLTest(unittest.TestCase):

    def setUp(self):
        global cnxn
        connect_str = 'DSN=' + dsn + ';UID=' + usr + ';PWD=' + pwd + ';'
        cnxn = pyodbc.connect(connect_str, autocommit=True)
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        pass

    def tearDown(self):
        global cnxn
        cnxn.close()
        pass

    def test21(self):
        #global cnxn
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        cnxn.execute('DROP TABLE IF EXISTS T21')
        cnxn.execute('CREATE TABLE T21 (C1 INT NOT NULL, C2 CHAR(10), PRIMARY KEY(C1))')
        cursor = cnxn.execute('GET TABLES')
        found = 0
        while 1:
            row = cursor.fetchone()
            if not row:
                break
            if (row[0] == 'T21'):
                found = 1
        assert found == 1, 'T21 should be listed in the output'   
    
    def test22(self):
        global cnxn
        cursor = cnxn.cursor()
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        cursor.execute('DROP TABLE IF EXISTS EMP')
        cursor.execute('CREATE TABLE EMP (EMPNUM INT NOT NULL, EMPNAME VARCHAR(20), PRIMARY KEY(EMPNUM))')
        cursor.execute('INSERT INTO EMP VALUES (20001, \'VITTAL RAO\')')
        cursor.execute('SELECT * FROM EMP')
        found = 0
        while 1:
            row = cursor.fetchone()
            if not row:
                break
            if (row[1] == 'VITTAL RAO'):
                found = 1
        assert found == 1, 'Fetching data using column number failed'

    def test23(self):
        global cnxn
        cursor = cnxn.cursor()
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        cursor.execute('DROP TABLE IF EXISTS EMP')
        cursor.execute('CREATE TABLE EMP (EMPNUM INT NOT NULL, EMPNAME VARCHAR(20), PRIMARY KEY(EMPNUM))')
        cursor.execute('INSERT INTO EMP VALUES (20001, \'VITTAL RAO\')')
        cursor.execute('SELECT * FROM EMP')
        found = 0
        while 1:
            row = cursor.fetchone()
            if not row:
                break
            if (row.EMPNAME == 'VITTAL RAO'):
                found = 1
        assert found == 1, 'Fetching data using column name failed'

    def test24(self):
        global cnxn
        cursor = cnxn.cursor()
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        cursor.execute('DROP TABLE IF EXISTS T24')
        cursor.execute('CREATE TABLE T24(C INT)')
        cursor.execute('INSERT INTO T24 VALUES (1), (-200), (3467), (0)')
        cursor.execute('SELECT * FROM T24 ORDER BY 1')
        rows = cursor.fetchall()
        l = []
        for row in rows:
            l.append(row[0])
        assert l == [-200, 0, 1, 3467], 'Integer data not returned correctly'

    def test25(self):
        global cnxn
        cursor = cnxn.cursor()
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        cursor.execute('DROP TABLE IF EXISTS T25')
        cursor.execute('CREATE TABLE T25(C INT)')
        cursor.execute('INSERT INTO T25 VALUES (1), (-200), (3467), (0)')
        x = 200
        cursor.execute('SELECT * FROM T25 WHERE C > ? ORDER BY 1', x)
        rows = cursor.fetchall()
        l = []
        for row in rows:
            l.append(row[0])
        assert l == [3467], 'Integer data not returned correctly'
        
    def test26(self):
        global cnxn
        cursor = cnxn.cursor()
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        cursor.execute('DROP TABLE IF EXISTS T26')
        cursor.execute('CREATE TABLE T26(C INT)')
        cursor.execute('INSERT INTO T26 VALUES (1), (-200), (3467), (0)')
        x = 200
        cursor.execute('DELETE FROM T26')
        assert cursor.rowcount == 4, 'Number of deleted rows must be 4.'
        
    def test27(self):
        global cnxn
        cursor = cnxn.cursor()
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        cursor.execute('DROP TABLE IF EXISTS T27')
        cursor.execute('CREATE TABLE T27(C INT)')
        cursor.execute('INSERT INTO T27 VALUES (1), (-200), (3467), (0)')
        x = 200
        assert cursor.execute('DELETE FROM T27 WHERE C > ?', x).rowcount == 1, 'Number of deleted rows must be 1.'

    def test28(self):
        global cnxn
        cursor = cnxn.cursor()
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        cursor.execute('DROP TABLE IF EXISTS T28')
        cursor.execute('CREATE TABLE T28(C INT)')
        cursor.execute('INSERT INTO T28 VALUES (1), (-200), (3467), (0)')
        x = 0
        assert cursor.execute("UPDATE T28 SET C = 200 WHERE C = ?", x).rowcount == 1, 'Number of updated rows must be 1.'
        cursor.execute("SELECT * FROM T28 ORDER BY 1")
        rows = cursor.fetchall()
        l = []
        for row in rows:
            l.append(row[0])
        assert l == [-200, 1, 200, 3467], 'Integer data not returned correctly'
        
    def test29(self):
        global cnxn
        cursor = cnxn.cursor()
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        cursor.execute('DROP TABLE IF EXISTS T29')
        cursor.execute("CREATE TABLE T29(C1 INT NOT NULL, C2 CHAR(10), PRIMARY KEY(C1))")
        cursor.execute("INSERT INTO T29 VALUES (1, 'abc'), (-200, 'xyz'), (3467, 'pqr')")
        cursor.execute("UPSERT INTO T29 VALUES (1, 'xyz'), (-200, 'xyz'), (3467, 'xyz')")
        cursor.execute("SELECT C2 FROM T29")
        found = 0
        while 1:
            row = cursor.fetchone()
            if not row:
                break
            if (row.C2 != 'xyz'):
                found = 1
        assert found == 1, 'Upsert failed'
        
class DataTest(unittest.TestCase):

    def setUp(self):
        global cnxn
        connect_str = 'DSN=' + dsn + ';UID=' + usr + ';PWD=' + pwd + ';'
        cnxn = pyodbc.connect(connect_str, autocommit=True)
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        pass

    def tearDown(self):
        global cnxn
        cnxn.close()
        pass

    def test31(self):
        try:
            cnxn.execute('CREATE SCHEMA ' + catalog + '.' + schema + ';')
        except:
            pass
        cnxn.execute('SET SCHEMA ' + catalog + '.' + schema + ';')
        cnxn.execute('DROP TABLE IF EXISTS TDATA')
        cnxn.execute("""
        CREATE TABLE TDATA (
        C1 INT NOT NULL, C2 CHAR(10), C3 VARCHAR(1000),
        C4 DATE, C5 TIME, C6 TIMESTAMP,
        C7 INTERVAL YEAR TO MONTH,
        C8 SMALLINT UNSIGNED, C9 LARGEINT, C10 DECIMAL,
        C11 FLOAT, C12 REAL, C13 DOUBLE PRECISION, C14 NUMERIC (9,3),
        PRIMARY KEY(C1))
        """)
        
if __name__ == "__main__":
    #import sys;sys.argv = ['', 'Test.testName']
    #unittest.main()
    suite = unittest.TestLoader().loadTestsFromTestCase(ConnectTest)
    unittest.TextTestRunner(verbosity=2).run(suite)
    suite = unittest.TestLoader().loadTestsFromTestCase(SQLTest)
    unittest.TextTestRunner(verbosity=2).run(suite)
    suite = unittest.TestLoader().loadTestsFromTestCase(DataTest)
    unittest.TextTestRunner(verbosity=2).run(suite)
