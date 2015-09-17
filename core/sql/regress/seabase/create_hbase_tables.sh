
swhbase << EOF

create 'test028tbl2', {NAME => 'cf1', VERSIONS => 1 }, {SPLITS => ['333','666','999']}

tbl2=get_table 'test028tbl2'

# region 0
tbl2.put '100', 'cf1:#1', 'v100'
tbl2.put '200', 'cf1:#1', 'v200'
tbl2.put '300', 'cf1:#1', 'v300'

# region 1
tbl2.put '400', 'cf1:#1', 'v400'
tbl2.put '500', 'cf1:#1', 'v500'
tbl2.put '600', 'cf1:#1', 'v600'

# region 2
tbl2.put '700', 'cf1:#1', 'v700'
tbl2.put '720', 'cf1:#1', 'v720'

# region 3
tbl2.put '999', 'cf1:#1', 'v999'
tbl2.put '9990', 'cf1:#1', 'v9990'

tbl2.scan


EOF
