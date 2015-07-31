-- @@@ START COPYRIGHT @@@
--
-- Licensed to the Apache Software Foundation (ASF) under one
-- or more contributor license agreements.  See the NOTICE file
-- distributed with this work for additional information
-- regarding copyright ownership.  The ASF licenses this file
-- to you under the Apache License, Version 2.0 (the
-- "License"); you may not use this file except in compliance
-- with the License.  You may obtain a copy of the License at
--
--   http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing,
-- software distributed under the License is distributed on an
-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
-- KIND, either express or implied.  See the License for the
-- specific language governing permissions and limitations
-- under the License.
--
-- @@@ END COPYRIGHT @@@
BEGIN { 
	
	inStatement = 0
	printStmt = 0
}

{
	if (inStatement == 0)
	{
		print $0

		if ($0 ~ /Compiling/)
		{
			inStatement = 1
		}
		
		next
	}

	if ($0 ~ /INTERNAL/ || $0 ~ /SELECT/ ||	$0 ~ /FIRST/)
	{
		printStmt = 1
	}

	if (printStmt == 1)
	{
		print $0
	}

	if ($0 ~ /;/)
	{
		inStatement = 0
		printStmt = 0
	}
}
