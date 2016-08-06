#include <assert.h>
#include "../hdb_utils.h"

using namespace hdb;
using namespace std;

bool Test_Types()
{
    assert(string("UNKNOWN") == DataTypeToStr((DATA_TYPE_T)-1));
    assert(string("TINYINT") == DataTypeToStr(T_TINYINT));
    assert(string("TEXT") == DataTypeToStr(T_TEXT));

    assert(StrToDataType("AAAA") == T_UNKNOWN);
    assert(StrToDataType("UNKNOWN") == T_UNKNOWN);
    assert(StrToDataType("TINYINT") == T_TINYINT);
    assert(StrToDataType("TEXT") == T_TEXT);
    return true;
}

bool Test_Cols()
{
    TinyIntCol ticol("aaa");
    SmallIntCol sicol("bbb");
    IntCol icol("ccc");
    BigIntCol bigcol("bigint");
    DoubleCol dcol("ddd");

    CharCol charcol("CHAR", 5);
    NCharCol ncharcol("nchar", 8);
    VarCharCol vcharcol("VARCHAR", 10);
    NVarCharCol nvarchar("nvarchar", 20);
    AlphaNumCol alphanumcol("AlphaNum", 12);

    return true;
}

bool Test_Records()
{
    bool ok;
    ColRecords recs;
    ok = recs.AddCol("INT", T_INTEGER);
    assert(ok);
    ok = recs.AddCol("DOUBLE", T_DOUBLE);
    assert(ok);
    assert(recs.GetColCount() == 2);

    ok = recs.AddColsFromCreateSql("CREATE COLUMN TABLE \"I078212\".\"GPS29\" ("\
        "\"VECHID\" BIGINT  CS_FIXED, "\
        "\"LNG\" DOUBLE CS_DOUBLE, "\
        "\"LAT\" DOUBLE CS_DOUBLE, "\
        "\"SPEED\" DOUBLE CS_DOUBLE, "\
        "\"HEADING\" DOUBLE CS_DOUBLE, "\
        "\"GPSTIME\" LONGDATE CS_LONGDATE, "\
        "\"INLOAD\" TINYINT CS_INT, "\
        "\"INSERVICE\" TINYINT CS_INT, "\
        "\"SEGMENT_ID\" BIGINT CS_FIXED, "\
        "\"TIME_SLOT\" INTEGER CS_INT)");
    assert(ok);
    assert(recs.GetColCount() == 10);
    recs.GenerateFakeData(15);

    ok = recs.AddColsFromCreateSql("CREATE COLUMN TABLE \"HEB_OSM\".\"WAY_SEGMENTS\" ("\
        "\"ID\" DECIMAL(19,0) CS_FIXED, "\
        "\"FROM_LAT\" DOUBLE CS_DOUBLE, "\
        "\"FROM_LNG\" DOUBLE CS_DOUBLE, "\
        "\"TO_LAT\" DOUBLE CS_DOUBLE, "\
        "\"TO_LNG\" DOUBLE CS_DOUBLE, "\
        "\"WAY_ID\" DECIMAL(19,0) CS_FIXED, "\
        "\"ONE_WAY\" INTEGER CS_INT, "\
        "\"LENGTH\" DOUBLE CS_DOUBLE, "\
        "\"WEIGHT\" DOUBLE CS_DOUBLE)");
    assert(recs.GetColCount() == 9);
    assert(ok);

    ok = recs.AddColsFromCreateSql("CREATE COLUMN TABLE PROTO_ODBC_MULTI ("\
        "GPSDATA_ID BIGINT, "\
        "DEVID VARCHAR(20), "\
        "STIME TIMESTAMP, "\
        "ALARMFLAG INTEGER, "\
        "STATE INTEGER, "\
        "LATITUDE DOUBLE, "\
        "LONGTITUDE DOUBLE, "\
        "SPEED DOUBLE, "\
        "ORIENTATION DOUBLE, "\
        "GPSTIME TIMESTAMP, "\
        "ODOMETER DOUBLE, "\
        "OILGAUGE DOUBLE) "\
        "PARTITION BY HASH (DEVID, STIME) PARTITIONS GET_NUM_SERVERS();");
    assert(ok);
    assert(recs.GetColCount() == 12);

    ok = recs.AddColsFromCreateSql("CREATE COLUMN TABLE \"HEB_OSM\".\"WAY_SEGMENTS\" ("\
        "GPSDATA_ID ABCD_TYPE, "\
        "\"WEIGHT\" DOUBLE CS_DOUBLE)");
    assert(!ok);

    return true;
}

OdbcConn *Test_CreateConn(const char *dsn, const char *user, const char *passwd)
{
    OdbcConn *pConn = NULL;

    pConn = new OdbcConn(dsn, user, passwd);
    assert(pConn);

    printf("To connect to HDB, DSN:%s ...\n", dsn);
    bool ok = pConn->Connect();
    if (!ok) {
        printf("Error in SQLConnect(): %s\n", pConn->GetDbcErrorStr().c_str());
    } else {
        printf("Connected to HDB.\n");
    }
    assert(ok);

    return pConn;
}

void Test_Insert(OdbcConn *pConn, const char *table_create_sql, const char *csv_lines)
{
    bool ok;
    InsertExecutor ins_exe(pConn);
    ColRecords records;
    PARSED_TABLE_T parsed_table;

    ok = records.AddColsFromCreateSql(table_create_sql);
    if (!ok) {
        printf("Error in parsing create table SQL: %s\n", records.GetErrStr());
    }
    assert(ok);

    std::string ins_stmt, err;
    ParseTableFromSql(table_create_sql, parsed_table, err);
    assert(!parsed_table.schema.empty());
    assert(!parsed_table.table_name.empty());

    ok = ins_exe.PrepareInsStmt(records.GetColumns(), (parsed_table.schema + '.' + parsed_table.table_name).c_str());
    if (!ok) {
        printf("Error in InsertExecutor::PrepareInsStmt(): %s\n", ins_exe.GetErrorStr().c_str());
    }
    assert(ok);

    records.ClearAllRows();
    if (csv_lines) {
        vector<string> lines;
        CsvLinePopulate(lines, csv_lines, '\n');
        for (size_t i = 0; i < lines.size(); i++) {
            records.AddRow(lines[i].c_str());
        }
    }
    if (records.GetRowCount() == 0) {
        records.GenerateFakeData(1000);
        assert(records.GetRowCount() == 1000);
    }

    ok = ins_exe.ExecuteInsert(records);
    if (!ok) {
        printf("Error in InsertExecutor::ExecuteInsert(): %s\n", ins_exe.GetErrorStr().c_str());
    }
    assert(ok);
}

void Test_Inserts(const char *dsn, const char *user, const char *passwd)
{
    OdbcConn *pConn = Test_CreateConn(dsn, user, passwd);

    static const struct {
        const char *create_table;
        const char *csv_lines;
    } test_params[] = {
        {
            "CREATE COLUMN TABLE I078212.TEST_DECIMAL (NAME1 DEC, NAME2 SMALLDECimal, NAME3 DEC (4,10), NAME4 DECIMAL (4,-6), NAME5 DECIMAL(12, 0));",
            "\"100.000000012341234234\",\"1234525.235231231\",\"0.0000001234\",\"0\",\"123456789012\""
        },
        {
            "CREATE COLUMN TABLE I078212.TEST_CHAR (NAME1 VARCHAR(5), NAME2 VARCHAR(10) NOT NULL , NAME3 NVARCHAR(6), NAME4 CHAR(8) CS_FIXEDSTRING, NAME5 NCHAR(3), NAME6 ALPHANUM(7) CS_ALPHANUM)",
            "A7613,A498550370,N23838,A6042970,N78,A368724\n"\
            "A7613,A498550370,N23838,A6042970,N78,A368724\n"\
            ",,,A6042970,N78,A368724\n"\
            "1,\xA1\xBF\xE4,3,4,555,Alpha\n"\
            "\"A8394\",\"A559617907\",\"N86199\",\"A5988647\",\"N05\",A427991",
        },
        {
            "CREATE COLUMN TABLE I078212.TEST_INT (NAME1 TINYINT CS_INT, NAME2 SMALLINT CS_INT, NAME3 INTEGER CS_INT, NAME4 BIGINT CS_FIXED)",
            "22,19737,18588,35822626\n"
            ", ,,\n"
            "114,16336,7811,60429700",
        },
        {
            "CREATE COLUMN TABLE I078212.TEST_APPROXIMATE (NAME1 DOUBLE, NAME2 REAL, NAME3 FLOAT, F20 FLOAT(20), F50 FLOAT(50));",
            "1959898.4825586719,60234,5672779.974120304,3.5823e+06,5967893.9312417982\n"\
            ", ,,,\n"\
            "-0, 0, -1, 0, 0\n"\
            "7613146.6125675226,49855,2383800.5367900631,6.043e+06,7822808.7517014062",
        },
#if 0
        {
            "CREATE COLUMN TABLE \"I078212\".\"EXT_TAXI_HEB\" ("\
            "\"GPSDATA_ID\" BIGINT CS_FIXED, "\
            "\"DEVID\" VARCHAR(20), "\
            "\"STIME\" LONGDATE CS_LONGDATE, "\
            "\"ALARMFLAG\" INTEGER CS_INT, "\
            "\"STATE\" INTEGER CS_INT, "\
            "\"LONGITUDE\" DOUBLE CS_DOUBLE, "\
            "\"LATITUDE\" DOUBLE CS_DOUBLE, "\
            "\"SPEED\" DOUBLE CS_DOUBLE, "\
            "\"GPSTIME\" LONGDATE CS_LONGDATE, "\
            "\"ODOMETER\" DOUBLE CS_DOUBLE, "\
            "\"OILGAUGE\" DOUBLE CS_DOUBLE, "\
            "\"ORIENTATION\" SMALLINT CS_INT, "\
            "\"GPS_STATUS\" TINYINT CS_INT, "\
            "\"OPERATION_STATE\" TINYINT CS_INT, "\
            "\"ORDER_STATE\" TINYINT CS_INT, "\
            "\"PASSENGER_STATE\" TINYINT CS_INT, "\
            "\"OIL_STATE\" TINYINT CS_INT, "\
            "\"OVERSPEED_STATE\" TINYINT CS_INT, "\
            "\"SQUARE_X\" INTEGER CS_INT, "\
            "\"SQUARE_Y\" INTEGER CS_INT, "\
            "\"WAY_SEGMENT_ID\" BIGINT CS_FIXED, "\
            "\"REVERSED\" TINYINT CS_INT, "\
            "\"TIME_SLOT\" TINYINT CS_INT, "\
            "\"WEEK_DAY\" TINYINT CS_INT, "\
            "\"DAY_OF_YEAR\" SMALLINT CS_INT, "\
            "\"HOUR\" TINYINT CS_INT, "\
            "\"INSERT_TIME\" LONGDATE CS_LONGDATE) "\
            "WITH PARAMETERS ('PARTITION_SPEC' = 'HASH 1 DAY_OF_YEAR')",

            "7317822,\"0300017588\",\"2012-01-23 10:15:03.0000000\",34816,0,126.560196,45.721435999999997,0,\"2012-01-23 10:15:20.0000000\",,,80,0,0,0,0,0,0,2379877,997366,-1,0,41,1,23,10,\"2013-02-02 04:12:09.3850000\"\n"\
            "7320718,\"0300016955\",\"2012-01-23 10:15:09.0000000\",34816,0,126.63844,45.495710000000003,0,\"2012-01-23 10:15:24.0000000\",,,18,0,0,0,0,0,0,2380484,999871,-1,0,41,1,23,10,\"2013-02-02 04:12:13.3920000\"\n"\
            "7326314,\"0300017583\",\"2012-01-23 10:14:46.0000000\",34816,0,126.65768,45.687130000000003,0,\"2012-01-23 10:15:44.0000000\",,,13,0,0,0,0,0,0,2380633,997747,-1,0,41,1,23,10,\"2013-02-02 04:12:33.4000000\"",
        },
#endif
    };

    for (int i = 0; i < sizeof(test_params)/sizeof(*test_params); i++) {
        Test_Insert(pConn, test_params[i].create_table, test_params[i].csv_lines);
    }

    delete pConn;
    pConn = NULL;
}


void Test_CsvParse()
{
    const char *csv_lines = 
        "7317822,\"0300017588\",\"2012-01-23 10:15:03.0000000\",34816,0,126.560196,45.721435999999997,0,\"2012-01-23 10:15:20.0000000\",,,80,0,0,0,0,0,0,2379877,997366,-1,0,41,1,23,10,\"2013-02-02 04:12:09.3850000\"\n"\
        "7320718,\"0300016955\",\"2012-01-23 10:15:09.0000000\",34816,0,126.63844,45.495710000000003,0,\"2012-01-23 10:15:24.0000000\",,,18,0,0,0,0,0,0,2380484,999871,-1,0,41,1,23,10,\"2013-02-02 04:12:13.3920000\"\n"\
        "7326314,\"0300017583\",\"2012-01-23 10:14:46.0000000\",34816,0,126.65768,45.687130000000003,0,\"2012-01-23 10:15:44.0000000\",,,13,0,0,0,0,0,0,2380633,997747,-1,0,41,1,23,10,\"2013-02-02 04:12:33.4000000\"";

    printf("\r%s: Start testing CSV Parse ...\n", ElapsedTimeStr().c_str());

    vector<string> lines, records;
    int repeat = 1000000;
    for (int i = 0; i < repeat; i++) {
        CsvLinePopulate(lines, csv_lines, '\n');
        for (size_t i = 0; i < lines.size(); i++) {
            CsvLinePopulate(records, lines[i].c_str(), ',');
        }
    }

    printf("\r%s: End testing CSV Parse.\n", ElapsedTimeStr().c_str());
}

void Test_Fetches(const char *dsn, const char *user, const char *passwd, const char *query_sql)
{
    bool ok;
    OdbcConn *pConn = Test_CreateConn(dsn, user, passwd);
    FetchExecutor fetch_exe(pConn, query_sql);

    ColRecords records;
    ok = fetch_exe.ExecuteFetchAll(records);
    if (!ok) {
        printf("Error in FetchExecutor::ExecuteFetchAll(): %s\n", fetch_exe.GetErrorStr().c_str());
    }
    assert(ok);
    
    delete pConn;
    pConn = NULL;
}

void Test_StrUtil()
{
    std::string str;
    string16 wstr(3, 0);
    wstr[0] = L'A';
    wstr[1] = L'B';
    str = WStrToStr(wstr);
    wstr = StrToWStr("\xA1\xBF\xE4");
}

bool TestHdb_Main(const char *dsn, const char *user, const char *passwd)
{
    //Test_CsvParse();
    Test_StrUtil();
    Test_Types();
    Test_Cols();
    Test_Records();
    Test_Inserts(dsn, user, passwd); // Need connect to server
    //Test_Fetches(dsn, user, passwd, "select * from I078212.EXT_TAXI_HEB"); // Need connect to server

    return true;
};
