import data
into table "[SCHEMA]"."[TABLE]"
from 'data.csv'
    record delimited by '\n'
    field delimited by ','
    optionally enclosed by '"'
error log 'data.err'
