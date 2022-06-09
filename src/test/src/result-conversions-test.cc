/*--------
 * Module:			result-conversions-test.cc
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *--------
 */
/*
 * Test conversion of values received from server to SQL datatypes.
 *
 * The tests work in the following way
 * 1. Connect to the server
 * 2. Look up the correct result in the map
 * 3. Make a query CASTing it to the type you want to test
 * 4. Call SQLGetData passing it the sql type (found in sql.h and sqlext.h) you want to convert to and a buffer to write it
 *    This forces the ODBC Driver to make the conversion to that type
 * 5. Serialize that output into a string for comparison with the expected result. (It would be great to compare directly but that is a lot more effort)
 * 
 * We run the tests with every combination of database type (VARCHAR, BOOL, TEXT) and type defined in the sqltypes vector
 */
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <ctime>
#include <tuple>
#include <string>
#include <iostream>
#include <utility>
#include <map>

#ifdef WIN32
#include <float.h>
#endif

#include "common.h"
#include <gtest/gtest.h>

/* Visual Studio didn't have isinf and isinf until VS2013. */
#if defined(WIN32) && (_MSC_VER < 1800)
#define isinf(x) ((_fpclass(x) == _FPCLASS_PINF) || (_fpclass(x) == _FPCLASS_NINF))
#define isnan(x) _isnan(x)
#endif

struct ResultConversionTestParams
{
	std::string type;
	std::string value;
	int sqltype;
	std::string sqltypestr;
	int buflen;
	int use_time; // TODO: change to boolean

	public:

	ResultConversionTestParams(std::tuple<std::pair<std::string, std::string>, std::pair<int, std::string> > value) {
		const std::pair<std::string, std::string> temp = std::get<0>(value);
		const std::pair<int, std::string> temp2 = std::get<1>(value);


		this->value = std::get<0>(temp);
		type = std::get<1>(temp);
	
		sqltype = std::get<0>(temp2);
		sqltypestr = std::get<1>(temp2);

		buflen = 100;
		use_time = 0;
	}
};

class ResultConversionsTest : public ::testing::TestWithParam< std::tuple<std::pair<std::string, std::string>, std::pair<int, std::string> > >
{
};

// represents the requested c type the odbc driver should convert the data to
static const std::vector<std::pair<int, std::string> > odbc_sql_c_types = {
	std::pair<int, std::string>(SQL_C_CHAR, "SQL_C_CHAR"),
	std::pair<int, std::string>(SQL_C_WCHAR, "SQL_C_WCHAR"),
	std::pair<int, std::string>(SQL_C_BIT, "SQL_C_BIT"),
	std::pair<int, std::string>(SQL_C_NUMERIC, "SQL_C_NUMERIC"),
	std::pair<int, std::string>(SQL_C_STINYINT, "SQL_C_STINYINT"),
	std::pair<int, std::string>(SQL_C_UTINYINT, "SQL_C_UTINYINT"),
	std::pair<int, std::string>(SQL_C_SBIGINT, "SQL_C_SBIGINT"),
	std::pair<int, std::string>(SQL_C_UBIGINT, "SQL_C_UBIGINT"),
	std::pair<int, std::string>(SQL_C_SSHORT, "SQL_C_SSHORT"),
	std::pair<int, std::string>(SQL_C_USHORT, "SQL_C_USHORT"),
	std::pair<int, std::string>(SQL_C_SLONG, "SQL_C_SLONG"),
	std::pair<int, std::string>(SQL_C_ULONG, "SQL_C_ULONG"),
	std::pair<int, std::string>(SQL_C_FLOAT, "SQL_C_FLOAT"),
	std::pair<int, std::string>(SQL_C_DOUBLE, "SQL_C_DOUBLE"),
	std::pair<int, std::string>(SQL_C_BINARY, "SQL_C_BINARY"),
	std::pair<int, std::string>(SQL_C_BOOKMARK, "SQL_C_BOOKMARK"),
	std::pair<int, std::string>(SQL_C_VARBOOKMARK, "SQL_C_VARBOOKMARK"),

	std::pair<int, std::string>(SQL_C_TYPE_DATE, "SQL_C_TYPE_DATE"),
	std::pair<int, std::string>(SQL_C_TYPE_TIME, "SQL_C_TYPE_TIME"),
	std::pair<int, std::string>(SQL_C_TYPE_TIMESTAMP, "SQL_C_TYPE_TIMESTAMP"),
	std::pair<int, std::string>(SQL_C_INTERVAL_YEAR, "SQL_C_INTERVAL_YEAR"),
	std::pair<int, std::string>(SQL_C_INTERVAL_MONTH, "SQL_C_INTERVAL_MONTH"),
	std::pair<int, std::string>(SQL_C_INTERVAL_DAY, "SQL_C_INTERVAL_DAY"),
	std::pair<int, std::string>(SQL_C_INTERVAL_HOUR, "SQL_C_INTERVAL_HOUR"),
	std::pair<int, std::string>(SQL_C_INTERVAL_MINUTE, "SQL_C_INTERVAL_MINUTE"),
	std::pair<int, std::string>(SQL_C_INTERVAL_SECOND, "SQL_C_INTERVAL_SECOND"),
	std::pair<int, std::string>(SQL_C_INTERVAL_YEAR_TO_MONTH, "SQL_C_INTERVAL_YEAR_TO_MONTH"),
	std::pair<int, std::string>(SQL_C_INTERVAL_DAY_TO_HOUR, "SQL_C_INTERVAL_DAY_TO_HOUR"),
	std::pair<int, std::string>(SQL_C_INTERVAL_DAY_TO_MINUTE, "SQL_C_INTERVAL_DAY_TO_MINUTE"),
	std::pair<int, std::string>(SQL_C_INTERVAL_DAY_TO_SECOND, "SQL_C_INTERVAL_DAY_TO_SECOND"),
	std::pair<int, std::string>(SQL_C_INTERVAL_HOUR_TO_MINUTE, "SQL_C_INTERVAL_HOUR_TO_MINUTE"),
	std::pair<int, std::string>(SQL_C_INTERVAL_HOUR_TO_SECOND, "SQL_C_INTERVAL_HOUR_TO_SECOND"),
	std::pair<int, std::string>(SQL_C_INTERVAL_MINUTE_TO_SECOND, "SQL_C_INTERVAL_MINUTE_TO_SECOND"),

	std::pair<int, std::string>(SQL_C_GUID, "SQL_C_GUID"),
};

static const std::vector<std::pair<std::string, std::string> > floating_point_edge_cases = {
			std::pair<std::string, std::string>("'NaN'", "float"),
			std::pair<std::string, std::string>("'NaN'", "double"),
			std::pair<std::string, std::string>("'Infinity'", "float"),
			std::pair<std::string, std::string>("'Infinity'", "double"),
			std::pair<std::string, std::string>("'-Infinity'", "float"),
			std::pair<std::string, std::string>("'-Infinity'", "double"),
};

/*
(value, type, sqltype) -> correct conversion
*/
static const std::map< std::tuple<std::string, std::string, std::string>, std::string> conversion_to_correct_output = {
{ std::make_tuple("true", "boolean", "SQL_C_CHAR"), "true" },
{ std::make_tuple("true", "boolean", "SQL_C_WCHAR"), "true" },
{ std::make_tuple("true", "boolean", "SQL_C_SSHORT"), "1" },
{ std::make_tuple("true", "boolean", "SQL_C_USHORT"), "1" },
{ std::make_tuple("true", "boolean", "SQL_C_SLONG"), "1" },
{ std::make_tuple("true", "boolean", "SQL_C_ULONG"), "1" },
{ std::make_tuple("true", "boolean", "SQL_C_FLOAT"), "1.000000" },
{ std::make_tuple("true", "boolean", "SQL_C_DOUBLE"), "1.000000" },
{ std::make_tuple("true", "boolean", "SQL_C_BIT"), "1" },
{ std::make_tuple("true", "boolean", "SQL_C_STINYINT"), "1" },
{ std::make_tuple("true", "boolean", "SQL_C_UTINYINT"), "1" },
{ std::make_tuple("true", "boolean", "SQL_C_SBIGINT"), "1" },
{ std::make_tuple("true", "boolean", "SQL_C_UBIGINT"), "1" },
{ std::make_tuple("true", "boolean", "SQL_C_BINARY"), "hex: 01" },
{ std::make_tuple("true", "boolean", "SQL_C_BOOKMARK"), "1" },
{ std::make_tuple("true", "boolean", "SQL_C_VARBOOKMARK"), "hex: 01" },
{ std::make_tuple("true", "boolean", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_NUMERIC"), "precision: 1 scale: 0 sign: 0 val: 01000000000000000000000000000000" },
{ std::make_tuple("true", "boolean", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("true", "boolean", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },

{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_CHAR"), "464F4F" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_WCHAR"), "464F4F" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_SSHORT"), "0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_USHORT"), "0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_SLONG"), "0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_ULONG"), "0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_FLOAT"), "0.000000" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_DOUBLE"), "0.000000" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_BIT"), "0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_STINYINT"), "0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_UTINYINT"), "0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_SBIGINT"), "0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_UBIGINT"), "0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_BINARY"), "hex: 464F4F" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_BOOKMARK"), "0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_VARBOOKMARK"), "hex: 464F4F" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_NUMERIC"), "precision: 0 scale: 0 sign: 0 val: 00000000000000000000000000000000" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },

// should be the same as char
{ std::make_tuple("'x'", "varchar", "SQL_C_CHAR"), "x" },
{ std::make_tuple("'x'", "varchar", "SQL_C_WCHAR"), "x" },
{ std::make_tuple("'x'", "varchar", "SQL_C_SSHORT"), "0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_USHORT"), "0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_SLONG"), "0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_ULONG"), "0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_FLOAT"), "0.000000" },
{ std::make_tuple("'x'", "varchar", "SQL_C_DOUBLE"), "0.000000" },
{ std::make_tuple("'x'", "varchar", "SQL_C_BIT"), "0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_STINYINT"), "0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_UTINYINT"), "0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_SBIGINT"), "0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_UBIGINT"), "0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_BINARY"), "hex: 78" },
{ std::make_tuple("'x'", "varchar", "SQL_C_BOOKMARK"), "0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_VARBOOKMARK"), "hex: 78" },
{ std::make_tuple("'x'", "varchar", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_NUMERIC"), "precision: 0 scale: 0 sign: 0 val: 00000000000000000000000000000000" },
{ std::make_tuple("'x'", "varchar", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "varchar", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },

{ std::make_tuple("'x'", "char", "SQL_C_CHAR"), "x" },
{ std::make_tuple("'x'", "char", "SQL_C_WCHAR"), "x" },
{ std::make_tuple("'x'", "char", "SQL_C_SSHORT"), "0" },
{ std::make_tuple("'x'", "char", "SQL_C_USHORT"), "0" },
{ std::make_tuple("'x'", "char", "SQL_C_SLONG"), "0" },
{ std::make_tuple("'x'", "char", "SQL_C_ULONG"), "0" },
{ std::make_tuple("'x'", "char", "SQL_C_FLOAT"), "0.000000" },
{ std::make_tuple("'x'", "char", "SQL_C_DOUBLE"), "0.000000" },
{ std::make_tuple("'x'", "char", "SQL_C_BIT"), "0" },
{ std::make_tuple("'x'", "char", "SQL_C_STINYINT"), "0" },
{ std::make_tuple("'x'", "char", "SQL_C_UTINYINT"), "0" },
{ std::make_tuple("'x'", "char", "SQL_C_SBIGINT"), "0" },
{ std::make_tuple("'x'", "char", "SQL_C_UBIGINT"), "0" },
{ std::make_tuple("'x'", "char", "SQL_C_BINARY"), "hex: 78" },
{ std::make_tuple("'x'", "char", "SQL_C_BOOKMARK"), "0" },
{ std::make_tuple("'x'", "char", "SQL_C_VARBOOKMARK"), "hex: 78" },
{ std::make_tuple("'x'", "char", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_NUMERIC"), "precision: 0 scale: 0 sign: 0 val: 00000000000000000000000000000000" },
{ std::make_tuple("'x'", "char", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'x'", "char", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },


{ std::make_tuple("1234567890", "bigint", "SQL_C_CHAR"), "1234567890" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_WCHAR"), "1234567890" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_SSHORT"), "722" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_USHORT"), "722" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_SLONG"), "1234567890" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_ULONG"), "1234567890" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_FLOAT"), "1234567936.000000" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_DOUBLE"), "1234567890.000000" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_BIT"), "210" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_STINYINT"), "-46" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_UTINYINT"), "210" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_SBIGINT"), "1234567890" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_UBIGINT"), "1234567890" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_BINARY"), "hex: D202964900000000" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_BOOKMARK"), "1234567890" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_VARBOOKMARK"), "hex: D202964900000000" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_NUMERIC"), "precision: 38 scale: 0 sign: 0 val: d2029649000000000000000000000000" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567890", "bigint", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_CHAR"), "12345" },
{ std::make_tuple("12345", "integer", "SQL_C_WCHAR"), "12345" },
{ std::make_tuple("12345", "integer", "SQL_C_SSHORT"), "12345" },
{ std::make_tuple("12345", "integer", "SQL_C_USHORT"), "12345" },
{ std::make_tuple("12345", "integer", "SQL_C_SLONG"), "12345" },
{ std::make_tuple("12345", "integer", "SQL_C_ULONG"), "12345" },
{ std::make_tuple("12345", "integer", "SQL_C_FLOAT"), "12345.000000" },
{ std::make_tuple("12345", "integer", "SQL_C_DOUBLE"), "12345.000000" },
{ std::make_tuple("12345", "integer", "SQL_C_BIT"), "57" },
{ std::make_tuple("12345", "integer", "SQL_C_STINYINT"), "57" },
{ std::make_tuple("12345", "integer", "SQL_C_UTINYINT"), "57" },
{ std::make_tuple("12345", "integer", "SQL_C_SBIGINT"), "12345" },
{ std::make_tuple("12345", "integer", "SQL_C_UBIGINT"), "12345" },
{ std::make_tuple("12345", "integer", "SQL_C_BINARY"), "hex: 39300000" },
{ std::make_tuple("12345", "integer", "SQL_C_BOOKMARK"), "12345" },
{ std::make_tuple("12345", "integer", "SQL_C_VARBOOKMARK"), "hex: 39300000" },
{ std::make_tuple("12345", "integer", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_NUMERIC"), "precision: 38 scale: 0 sign: 0 val: 39300000000000000000000000000000" },
{ std::make_tuple("12345", "integer", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("12345", "integer", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_CHAR"), "1234567" },
{ std::make_tuple("1234567", "integer", "SQL_C_WCHAR"), "1234567" },
{ std::make_tuple("1234567", "integer", "SQL_C_SSHORT"), "-10617" },
{ std::make_tuple("1234567", "integer", "SQL_C_USHORT"), "54919" },
{ std::make_tuple("1234567", "integer", "SQL_C_SLONG"), "1234567" },
{ std::make_tuple("1234567", "integer", "SQL_C_ULONG"), "1234567" },
{ std::make_tuple("1234567", "integer", "SQL_C_FLOAT"), "1234567.000000" },
{ std::make_tuple("1234567", "integer", "SQL_C_DOUBLE"), "1234567.000000" },
{ std::make_tuple("1234567", "integer", "SQL_C_BIT"), "135" },
{ std::make_tuple("1234567", "integer", "SQL_C_STINYINT"), "-121" },
{ std::make_tuple("1234567", "integer", "SQL_C_UTINYINT"), "135" },
{ std::make_tuple("1234567", "integer", "SQL_C_SBIGINT"), "1234567" },
{ std::make_tuple("1234567", "integer", "SQL_C_UBIGINT"), "1234567" },
{ std::make_tuple("1234567", "integer", "SQL_C_BINARY"), "hex: 87D61200" },
{ std::make_tuple("1234567", "integer", "SQL_C_BOOKMARK"), "1234567" },
{ std::make_tuple("1234567", "integer", "SQL_C_VARBOOKMARK"), "hex: 87D61200" },
{ std::make_tuple("1234567", "integer", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_NUMERIC"), "precision: 38 scale: 0 sign: 0 val: 87d61200000000000000000000000000" },
{ std::make_tuple("1234567", "integer", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234567", "integer", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },

{ std::make_tuple("1.234", "float", "SQL_C_CHAR"), "1.234" },
{ std::make_tuple("1.234", "float", "SQL_C_WCHAR"), "1.234" },
{ std::make_tuple("1.234", "float", "SQL_C_SSHORT"), "1" },
{ std::make_tuple("1.234", "float", "SQL_C_USHORT"), "1" },
{ std::make_tuple("1.234", "float", "SQL_C_SLONG"), "1" },
{ std::make_tuple("1.234", "float", "SQL_C_ULONG"), "1" },
{ std::make_tuple("1.234", "float", "SQL_C_FLOAT"), "1.234000" },
{ std::make_tuple("1.234", "float", "SQL_C_DOUBLE"), "1.234000" },
{ std::make_tuple("1.234", "float", "SQL_C_BIT"), "1" },
{ std::make_tuple("1.234", "float", "SQL_C_STINYINT"), "1" },
{ std::make_tuple("1.234", "float", "SQL_C_UTINYINT"), "1" },
{ std::make_tuple("1.234", "float", "SQL_C_SBIGINT"), "1" },
{ std::make_tuple("1.234", "float", "SQL_C_UBIGINT"), "1" },
{ std::make_tuple("1.234", "float", "SQL_C_BINARY"), "hex: B6F39D3F" },
{ std::make_tuple("1.234", "float", "SQL_C_BOOKMARK"), "1" },
{ std::make_tuple("1.234", "float", "SQL_C_VARBOOKMARK"), "hex: B6F39D3F" },
{ std::make_tuple("1.234", "float", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_NUMERIC"), "precision: 38 scale: 0 sign: 0 val: 01000000000000000000000000000000" },
{ std::make_tuple("1.234", "float", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.234", "float", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_CHAR"), "1.23456789012" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_WCHAR"), "1.23456789012" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_SSHORT"), "1" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_USHORT"), "1" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_SLONG"), "1" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_ULONG"), "1" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_FLOAT"), "1.234568" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_DOUBLE"), "1.234568" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_BIT"), "1" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_STINYINT"), "1" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_UTINYINT"), "1" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_SBIGINT"), "1" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_UBIGINT"), "1" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_BINARY"), "hex: 2B1D8C42CAC0F33F" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_BOOKMARK"), "1" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_VARBOOKMARK"), "hex: 2B1D8C42CAC0F33F" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_NUMERIC"), "precision: 38 scale: 0 sign: 0 val: 01000000000000000000000000000000" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1.23456789012", "double", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },

{ std::make_tuple("'foobar'", "varchar", "SQL_C_CHAR"), "foobar" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_WCHAR"), "foobar" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_SSHORT"), "0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_USHORT"), "0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_SLONG"), "0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_ULONG"), "0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_FLOAT"), "0.000000" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_DOUBLE"), "0.000000" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_BIT"), "0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_STINYINT"), "0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_UTINYINT"), "0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_SBIGINT"), "0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_UBIGINT"), "0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_BINARY"), "hex: 666F6F626172" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_BOOKMARK"), "0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_VARBOOKMARK"), "hex: 666F6F626172" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_NUMERIC"), "precision: 0 scale: 0 sign: 0 val: 00000000000000000000000000000000" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'foobar'", "varchar", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_CHAR"), "2011-02-13" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_WCHAR"), "2011-02-13" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_SSHORT"), "2011" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_USHORT"), "2011" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_SLONG"), "2011" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_ULONG"), "2011" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_FLOAT"), "2011.000000" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_DOUBLE"), "2011.000000" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_TYPE_DATE"), "y: 2011 m: 2 d: 13" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_TYPE_TIMESTAMP"), "y: 2011 m: 2 d: 13 h: 0 m: 0 s: 0 f: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_BIT"), "219" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_STINYINT"), "-37" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_UTINYINT"), "219" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_SBIGINT"), "2011" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_UBIGINT"), "2011" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_BINARY"), "hex: DB0702000D00" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_BOOKMARK"), "2011" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_VARBOOKMARK"), "SQLGetData failed" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_NUMERIC"), "precision: 4 scale: 0 sign: 0 val: db070000000000000000000000000000" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 year 2011 month: 2" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },

{ std::make_tuple("'13:23:34'", "time", "SQL_C_CHAR"), "13:23:34" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_WCHAR"), "13:23:34" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_SSHORT"), "13" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_USHORT"), "13" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_SLONG"), "13" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_ULONG"), "13" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_FLOAT"), "13.000000" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_DOUBLE"), "13.000000" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_BIT"), "13" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_STINYINT"), "13" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_UTINYINT"), "13" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_SBIGINT"), "13" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_UBIGINT"), "13" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_BINARY"), "hex: 0D001700220000000000" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_BOOKMARK"), "13" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_VARBOOKMARK"), "SQLGetData failed" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_TYPE_TIME"), "h: 13 m: 23 s: 34" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_TYPE_TIMESTAMP"), "y: 2022 m: 5 d: 26 h: 13 m: 23 s: 34 f: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_NUMERIC"), "precision: 2 scale: 0 sign: 0 val: 0d000000000000000000000000000000" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'13:23:34'", "time", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },

{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_CHAR"), "2011-02-15 15:49:18" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_WCHAR"), "2011-02-15 15:49:18" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_SSHORT"), "2011" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_USHORT"), "2011" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_SLONG"), "2011" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_ULONG"), "2011" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_FLOAT"), "2011.000000" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_DOUBLE"), "2011.000000" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_BIT"), "219" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_STINYINT"), "-37" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_UTINYINT"), "219" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_SBIGINT"), "2011" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_UBIGINT"), "2011" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_BINARY"), "hex: DB0702000F000F003100120000000000" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_BOOKMARK"), "2011" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_VARBOOKMARK"), "SQLGetData failed" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_TYPE_DATE"), "y: 2011 m: 2 d: 15" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_TYPE_TIME"), "h: 15 m: 49 s: 18" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_TYPE_TIMESTAMP"), "y: 2011 m: 2 d: 15 h: 15 m: 49 s: 18 f: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_NUMERIC"), "precision: 4 scale: 0 sign: 0 val: db070000000000000000000000000000" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 year 2011 month: 2" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },

{ std::make_tuple("1", "boolean", "SQL_C_CHAR"), "true" },
{ std::make_tuple("1", "boolean", "SQL_C_WCHAR"), "true" },
{ std::make_tuple("1", "boolean", "SQL_C_SSHORT"), "1" },
{ std::make_tuple("1", "boolean", "SQL_C_USHORT"), "1" },
{ std::make_tuple("1", "boolean", "SQL_C_SLONG"), "1" },
{ std::make_tuple("1", "boolean", "SQL_C_ULONG"), "1" },
{ std::make_tuple("1", "boolean", "SQL_C_FLOAT"), "1.000000" },
{ std::make_tuple("1", "boolean", "SQL_C_DOUBLE"), "1.000000" },
{ std::make_tuple("1", "boolean", "SQL_C_BIT"), "1" },
{ std::make_tuple("1", "boolean", "SQL_C_STINYINT"), "1" },
{ std::make_tuple("1", "boolean", "SQL_C_UTINYINT"), "1" },
{ std::make_tuple("1", "boolean", "SQL_C_SBIGINT"), "1" },
{ std::make_tuple("1", "boolean", "SQL_C_UBIGINT"), "1" },
{ std::make_tuple("1", "boolean", "SQL_C_BINARY"), "hex: 01" },
{ std::make_tuple("1", "boolean", "SQL_C_BOOKMARK"), "1" },
{ std::make_tuple("1", "boolean", "SQL_C_VARBOOKMARK"), "hex: 01" },
{ std::make_tuple("1", "boolean", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_NUMERIC"), "precision: 1 scale: 0 sign: 0 val: 01000000000000000000000000000000" },
{ std::make_tuple("1", "boolean", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1", "boolean", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },

{ std::make_tuple("1234.567890", "decimal", "SQL_C_CHAR"), "1235" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_WCHAR"), "1235" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_SSHORT"), "1235" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_USHORT"), "1235" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_SLONG"), "1235" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_ULONG"), "1235" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_FLOAT"), "1235.000000" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_DOUBLE"), "1235.000000" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_BIT"), "210" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_STINYINT"), "-46" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_UTINYINT"), "210" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_SBIGINT"), "1235" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_UBIGINT"), "1235" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_BINARY"), "hex: 00000000D3040000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100FFFF0100FFFF" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_BOOKMARK"), "1234" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_VARBOOKMARK"), "SQLGetData failed" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_NUMERIC"), "precision: 38 scale: 0 sign: 0 val: d3040000000000000000000000000000" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "decimal", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },

{ std::make_tuple("1234.567890", "numeric", "SQL_C_CHAR"), "1235" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_WCHAR"), "1235" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_SSHORT"), "1235" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_USHORT"), "1235" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_SLONG"), "1235" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_ULONG"), "1235" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_FLOAT"), "1235.000000" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_DOUBLE"), "1235.000000" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_BIT"), "210" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_STINYINT"), "-46" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_UTINYINT"), "210" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_SBIGINT"), "1235" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_UBIGINT"), "1235" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_BINARY"), "hex: 00000000D3040000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100300101000000" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_BOOKMARK"), "1235" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_VARBOOKMARK"), "SQLGetData failed" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_NUMERIC"), "precision: 38 scale: 0 sign: 0 val: d3040000000000000000000000000000" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_GUID"), "SQLGetData failed" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_YEAR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_DAY"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_YEAR_TO_MONTH"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_DAY_TO_HOUR"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_DAY_TO_MINUTE"), "SQLGetData failed" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_DAY_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_HOUR_TO_MINUTE"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_HOUR_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },
{ std::make_tuple("1234.567890", "numeric", "SQL_C_INTERVAL_MINUTE_TO_SECOND"), "interval sign: 0 unknown interval type: 0" },

{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_CHAR"), "464f4f" },
{ std::make_tuple("'\\x464f4f'", "varbinary", "SQL_C_WCHAR"), "464f4f" },
{ std::make_tuple("543c5e21-435a-440b-943c-64af1ad571f1", "text", "SQL_C_GUID"), "d1: 543C5E21 d2: 435A d3: 440B d4: 943C64AF1AD571F1" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_DATE"), "y: 2011 m: 2 d: 13" },
{ std::make_tuple("'2011-02-13'", "date", "SQL_C_TIMESTAMP"), "y: 2011 m: 2 d: 13 h: 0 m: 0 s: 0 f: 0" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_DATE"), "y: 2011 m: 2 d: 15" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_TIMESTAMP"), "y: 2011 m: 2 d: 15 h: 15 m: 49 s: 18 f: 0" },

{ std::make_tuple("", "text", "SQL_C_TYPE_DATE"), "y: 0 m: 0 d: 0" },
{ std::make_tuple("", "text", "SQL_C_TYPE_TIME"), "h: 0 m: 0 s: 0" },
{ std::make_tuple("", "text", "SQL_C_TYPE_TIMESTAMP"), "y: 0 m: 0 d: 0 h: 0 m: 0 s: 0 f: 0" },

{ std::make_tuple("", "text", "SQL_C_WCHAR"), "\\FF00\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF\\FFFF (truncated)" },
{ std::make_tuple("'2011-02-15 15:49:18'", "timestamp", "SQL_C_CHAR"), "2011-02-15 15:49:1 (truncated)" },
{ std::make_tuple("2011-02-15 15:49:18 BC", "timestamp", "SQL_C_CHAR"), "2011-02-15 15:49:18 (truncated)" },

// floating point edge cases
{ std::make_tuple("'NaN'", "float", "SQL_C_FLOAT"), "nan" },
{ std::make_tuple("'Infinity'", "float", "SQL_C_FLOAT"), "inf" },
{ std::make_tuple("'-Infinity'", "float", "SQL_C_FLOAT"), "-inf" },
{ std::make_tuple("'NaN'", "double", "SQL_C_FLOAT"), "nan" },
{ std::make_tuple("'Infinity'", "double", "SQL_C_FLOAT"), "inf" },
{ std::make_tuple("'-Infinity'", "double", "SQL_C_FLOAT"), "-inf" },
{ std::make_tuple("'NaN'", "float", "SQL_C_DOUBLE"), "nan" },
{ std::make_tuple("'Infinity'", "float", "SQL_C_DOUBLE"), "inf" },
{ std::make_tuple("'-Infinity'", "float", "SQL_C_DOUBLE"), "-inf" },
{ std::make_tuple("'NaN'", "double", "SQL_C_DOUBLE"), "nan" },
{ std::make_tuple("'Infinity'", "double", "SQL_C_DOUBLE"), "inf" },
{ std::make_tuple("'-Infinity'", "double", "SQL_C_DOUBLE"), "-inf" },

// edge case
{ std::make_tuple("", "text", "SQL_C_CHAR"), ""},
};

static HSTMT hstmt = SQL_NULL_HSTMT;

std::string wchar_to_string(SQLWCHAR *wstr)
{
	size_t length = 50; // tracks how much room is left in the buffer
	char buf[50] = "";
	int i = 0;
	/*
	 * a backstop to make sure we terminate if the string isn't null-terminated
	 * properly
	 */
	int MAXLEN = 50;

	while (*wstr && i < MAXLEN)
	{
		if ((*wstr & 0xFFFFFF00) == 0 && isprint(*wstr)) {
			char temp[2] = "";
			snprintf(temp, 2, "%c", *wstr);
			strncat(buf, temp, length);
			length = length - 1;
		}
		else {
			char temp[6] = "";
			snprintf(temp, 6, "\\%4X", *wstr);
			strncat(buf, temp, length);
			length = length - 1;
		}
		wstr++;
		i++;
	}
	return std::string(buf);
}

std::string hex_to_string(unsigned char *b, SQLLEN len)
{
	size_t length = 500;
	char buf[500] = "";
	SQLLEN i;

	const char* prefix = "hex: "; 

	snprintf(buf, length, "%s", prefix);
	length = length - strlen(prefix);
	for (i = 0; i < len; i++) {
		char temp[3] = ""; // two for the character 1 for null
		snprintf(temp, 3, "%02X", b[i]);
		strncat(buf, temp, length);
		length = length - 2;
	}

	return std::string(buf);
}

std::string double_to_string(double d)
{
	if (std::isnan(d)) {
		return "nan";
	}
	else if (d < 0 && std::isinf(d)) {
		return "-inf";
	}
	else if (std::isinf(d)) {
		return "inf";
	}
	else {
		char buf[500] = "";
		snprintf(buf, 500, "%f", d);
		return std::string(buf);
	}
}

/**
 * @brief sql_c_type_to_string takes a sqltype, buffer of the data (void*), string length or index and flag to use_time or not and returns that string representation
 * 
 * @param sql_c_type 
 * @param buf 
 * @param strlen_or_ind 
 * @param use_time 
 */
std::string sql_c_type_to_string(int sql_c_type, void *buf, SQLLEN strlen_or_ind, int use_time)
{
	size_t length = 500; // choosen because the buf is only set to 500 in the test_conversion function 
	char output_buffer[500];
	memset(output_buffer, '\0', sizeof(output_buffer));

	switch (sql_c_type)
	{
	case SQL_C_CHAR:
		return std::string((char *)buf);
	case SQL_C_WCHAR:
		return wchar_to_string((SQLWCHAR *)buf);
	case SQL_C_SSHORT:
		snprintf(output_buffer, length, "%hd", *((short *)buf));
		return std::string(output_buffer);
	case SQL_C_USHORT:
		snprintf(output_buffer, length, "%hu", *((unsigned short *)buf));
		return std::string(output_buffer);
	case SQL_C_SLONG:
		/* always 32-bits, regardless of native 'long' type */
		snprintf(output_buffer, length, "%d", (int)*((SQLINTEGER *)buf));
		return std::string(output_buffer);
	case SQL_C_ULONG:
		snprintf(output_buffer, length, "%u", (unsigned int)*((SQLUINTEGER *)buf));
		return std::string(output_buffer);
	case SQL_C_FLOAT:
		return double_to_string(*((SQLREAL *)buf));
	case SQL_C_DOUBLE:
		return double_to_string(*((SQLDOUBLE *)buf));
	case SQL_C_BIT:
		snprintf(output_buffer, length, "%u", *((unsigned char *)buf));
		return std::string(output_buffer);
	case SQL_C_STINYINT:
		snprintf(output_buffer, length, "%d", *((signed char *)buf));
		return std::string(output_buffer);
	case SQL_C_UTINYINT:
		snprintf(output_buffer, length, "%u", *((unsigned char *)buf));
		return std::string(output_buffer);
	case SQL_C_SBIGINT:
		/* XXX: the %ld format string won't handle the full 64-bit range on
		 * all platforms. */
		snprintf(output_buffer, length, "%ld", (long)*((SQLBIGINT *)buf));
		return std::string(output_buffer);
	case SQL_C_UBIGINT:
		/* XXX: the %lu format string won't handle the full 64-bit range on
		 * all platforms. */
		snprintf(output_buffer, length, "%lu", (unsigned long)*((SQLUBIGINT *)buf));
		return std::string(output_buffer);
	case SQL_C_BINARY:
		return hex_to_string((unsigned char *)buf, strlen_or_ind);
	/* SQL_C_BOOKMARK has same value as SQL_C_UBIGINT */
	/* SQL_C_VARBOOKMARK has same value as SQL_C_BINARY */
	case SQL_C_TYPE_DATE:
	{
		DATE_STRUCT *ds = (DATE_STRUCT *)buf;
		if (use_time != 0)
		{
			time_t t = 0;
			struct tm *tim;

			t = time(NULL);
			tim = localtime(&t);
			snprintf(output_buffer, length, "y: %d m: %u d: %u",
				   ds->year - (tim->tm_year + 1900),
				   ds->month - (tim->tm_mon + 1),
				   ds->day - tim->tm_mday);
		}
		else {
			snprintf(output_buffer, length, "y: %d m: %u d: %u", ds->year, ds->month, ds->day);
		}
		return std::string(output_buffer);
	}
	case SQL_C_TYPE_TIME:
	{
		TIME_STRUCT *ts = (TIME_STRUCT *)buf;

		if (use_time != 0)
		{
			time_t t = 0;

			t = time(NULL);
		}
		else {
			snprintf(output_buffer, length, "h: %d m: %u s: %u", ts->hour, ts->minute, ts->second);
		}
		return std::string(output_buffer);
	}
	case SQL_C_TYPE_TIMESTAMP:
	{
		TIMESTAMP_STRUCT *tss = (TIMESTAMP_STRUCT *)buf;

		if (use_time)
		{
			time_t t = 0;
			struct tm *tim;

			t = time(NULL);
			tim = localtime(&t);
			snprintf(output_buffer, length, "y: %d m: %u d: %u h: %d m: %u s: %u f: %u",
				   tss->year - (tim->tm_year + 1900),
				   tss->month - (tim->tm_mon + 1),
				   tss->day - tim->tm_mday,
				   tss->hour, tss->minute, tss->second,
				   (unsigned int)tss->fraction);
		}
		else {
			snprintf(output_buffer, length, "y: %d m: %u d: %u h: %d m: %u s: %u f: %u",
				   tss->year, tss->month, tss->day,
				   tss->hour, tss->minute, tss->second,
				   (unsigned int)tss->fraction);
		}
		return std::string(output_buffer);
	}
	case SQL_C_NUMERIC:
	{
		// we write into a local buffer then memcopy to the other buffer 
		size_t prefix_len = 500;
		char prefix[500];
		SQL_NUMERIC_STRUCT *ns = (SQL_NUMERIC_STRUCT *)buf;
		int i;
		snprintf(prefix, prefix_len, "precision: %u scale: %d sign: %d val: ",
			   ns->precision, ns->scale, ns->scale);
		strncat(output_buffer, prefix, length);
		length = length - strlen(prefix);
		for (i = 0; i < SQL_MAX_NUMERIC_LEN; i++) {
			char temp[3]; // two for the chars and 1 for the null
			snprintf(temp, 3, "%02x", ns->val[i]);
			strncat(output_buffer, temp, length);
			length = length - 2;
		}
		return std::string(output_buffer);
	}
	case SQL_C_GUID:
	{
		SQLGUID *g = (SQLGUID *)buf;
		snprintf(output_buffer, length, "d1: %04X d2: %04X d3: %04X d4: %02X%02X%02X%02X%02X%02X%02X%02X",
			   (unsigned int)g->Data1, g->Data2, g->Data3,
			   g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
			   g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);

		return std::string(output_buffer);
	}
	case SQL_C_INTERVAL_YEAR:
	case SQL_C_INTERVAL_MONTH:
	case SQL_C_INTERVAL_DAY:
	case SQL_C_INTERVAL_HOUR:
	case SQL_C_INTERVAL_MINUTE:
	case SQL_C_INTERVAL_SECOND:
	case SQL_C_INTERVAL_YEAR_TO_MONTH:
	case SQL_C_INTERVAL_DAY_TO_HOUR:
	case SQL_C_INTERVAL_DAY_TO_MINUTE:
	case SQL_C_INTERVAL_DAY_TO_SECOND:
	case SQL_C_INTERVAL_HOUR_TO_MINUTE:
	case SQL_C_INTERVAL_HOUR_TO_SECOND:
	case SQL_C_INTERVAL_MINUTE_TO_SECOND:
	{
		SQL_INTERVAL_STRUCT *s = (SQL_INTERVAL_STRUCT *)buf;

		size_t temp_length = 500;
		char temp[500];
		memset(temp, '\0', sizeof(temp));

		snprintf(temp, temp_length, "interval sign: %u ", s->interval_sign);
		strncpy(output_buffer, temp, length);
		length = length - strlen(temp);
		
		switch (s->interval_type)
		{
		case SQL_IS_YEAR:
		{
			snprintf(temp, temp_length, "year: %u", (unsigned int)s->intval.year_month.year);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);
			break;
		}
		case SQL_IS_MONTH:
		{	
			snprintf(temp, temp_length, "year: %u", (unsigned int)s->intval.year_month.month);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);
			break;
		}
		case SQL_IS_DAY:
		{
			snprintf(temp, temp_length, "day: %u", (unsigned int)s->intval.day_second.day);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);
			break;
		}
		case SQL_IS_HOUR:
		{
			snprintf(temp, temp_length, "hour: %u", (unsigned int)s->intval.day_second.hour);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);

			break;
		}
		case SQL_IS_MINUTE:
		{
			snprintf(temp, temp_length, "minute: %u", (unsigned int)s->intval.day_second.minute);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);

			break;
		}
		case SQL_IS_SECOND:
		 {
			snprintf(temp, temp_length, "second: %u", (unsigned int)s->intval.day_second.second);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);

			break;
		 }
		case SQL_IS_YEAR_TO_MONTH:
		 {
			snprintf(temp, temp_length, "year %u month: %u",
				   (unsigned int)s->intval.year_month.year,
				   (unsigned int)s->intval.year_month.month);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);

			break;
		 }
		case SQL_IS_DAY_TO_HOUR:
		{
			snprintf(temp, temp_length, "day: %u hour: %u",
				   (unsigned int)s->intval.day_second.day,
				   (unsigned int)s->intval.day_second.hour);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);

			break;
		}
		case SQL_IS_DAY_TO_MINUTE:
		 {
			snprintf(temp, temp_length, "day: %u hour: %u minute: %u",
				   (unsigned int)s->intval.day_second.day,
				   (unsigned int)s->intval.day_second.hour,
				   (unsigned int)s->intval.day_second.minute);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);

			break;
		 }
		case SQL_IS_DAY_TO_SECOND:
		 {
			snprintf(temp, temp_length, "day: %u hour: %u minute: %u second: %u fraction: %u",
				   (unsigned int)s->intval.day_second.day,
				   (unsigned int)s->intval.day_second.hour,
				   (unsigned int)s->intval.day_second.minute,
				   (unsigned int)s->intval.day_second.second,
				   (unsigned int)s->intval.day_second.fraction);

			strncat(output_buffer, temp, length);
			length = length - strlen(temp);
			break;
		 }
		case SQL_IS_HOUR_TO_MINUTE:
		{
			snprintf(temp, temp_length, "hour: %u minute: %u",
				   (unsigned int)s->intval.day_second.hour,
				   (unsigned int)s->intval.day_second.minute);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);
			break;
		}
		case SQL_IS_HOUR_TO_SECOND:
		 {
			snprintf(temp, temp_length, "hour: %u minute: %u second: %u fraction: %u",
				   (unsigned int)s->intval.day_second.hour,
				   (unsigned int)s->intval.day_second.minute,
				   (unsigned int)s->intval.day_second.second,
				   (unsigned int)s->intval.day_second.fraction);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);

			break;
		 }
		case SQL_IS_MINUTE_TO_SECOND:
		{
			snprintf(temp, temp_length, "minute: %u second: %u fraction: %u",
				   (unsigned int)s->intval.day_second.minute,
				   (unsigned int)s->intval.day_second.second,
				   (unsigned int)s->intval.day_second.fraction);
			strncat(output_buffer, temp, length);
			length = length - strlen(temp);

			break;
		}
		default:
			snprintf(output_buffer, length, "unknown interval type: %u", s->interval_type);
			break;
		}
		return std::string(output_buffer);
	}
	break;
	default:
		snprintf(output_buffer, length, "unknown SQL C type: %u", sql_c_type);
		return std::string(output_buffer);
	}
}

/*
 * Get the size of fixed-length data types, or -1 for var-length types
 */
int get_sql_type_size(int sql_c_type)
{
	switch (sql_c_type)
	{
	case SQL_C_CHAR:
		return -1;
	case SQL_C_WCHAR:
		return -1;
	case SQL_C_SSHORT:
		return sizeof(short);
	case SQL_C_USHORT:
		return sizeof(unsigned short);
	case SQL_C_SLONG:
		/* always 32-bits, regardless of native 'long' type */
		return sizeof(SQLINTEGER);
	case SQL_C_ULONG:
		return sizeof(SQLUINTEGER);
	case SQL_C_FLOAT:
		return sizeof(SQLREAL);
	case SQL_C_DOUBLE:
		return sizeof(SQLDOUBLE);
	case SQL_C_BIT:
		return sizeof(unsigned char);
	case SQL_C_STINYINT:
		return sizeof(signed char);
	case SQL_C_UTINYINT:
		return sizeof(unsigned char);
	case SQL_C_SBIGINT:
		return sizeof(SQLBIGINT);
	case SQL_C_UBIGINT:
		return sizeof(SQLUBIGINT);
	case SQL_C_BINARY:
		return -1;
	/* SQL_C_BOOKMARK has same value as SQL_C_UBIGINT */
	/* SQL_C_VARBOOKMARK has same value as SQL_C_BINARY */
	case SQL_C_TYPE_DATE:
		return sizeof(DATE_STRUCT);
	case SQL_C_TYPE_TIME:
		return sizeof(TIME_STRUCT);
	case SQL_C_TYPE_TIMESTAMP:
		return sizeof(TIMESTAMP_STRUCT);
	case SQL_C_NUMERIC:
		return sizeof(SQL_NUMERIC_STRUCT);
	case SQL_C_GUID:
		return sizeof(SQLGUID);
	case SQL_C_INTERVAL_YEAR:
	case SQL_C_INTERVAL_MONTH:
	case SQL_C_INTERVAL_DAY:
	case SQL_C_INTERVAL_HOUR:
	case SQL_C_INTERVAL_MINUTE:
	case SQL_C_INTERVAL_SECOND:
	case SQL_C_INTERVAL_YEAR_TO_MONTH:
	case SQL_C_INTERVAL_DAY_TO_HOUR:
	case SQL_C_INTERVAL_DAY_TO_MINUTE:
	case SQL_C_INTERVAL_DAY_TO_SECOND:
	case SQL_C_INTERVAL_HOUR_TO_MINUTE:
	case SQL_C_INTERVAL_HOUR_TO_SECOND:
	case SQL_C_INTERVAL_MINUTE_TO_SECOND:
		return sizeof(SQL_INTERVAL_STRUCT);
	default:
		printf("unknown SQL C type: %u", sql_c_type);
		return -1;
	}
}

static char *resultbuf = NULL;

void test_conversion(const char *pgtype, const char *pgvalue, int sqltype, const char *sqltypestr, int buflen, int use_time)
{
	char sql[500];
	SQLRETURN rc;
	SQLLEN len_or_ind;
	int fixed_len;
	
	// key is "'{pgvalue}' ({pgtype}) as {sqltypestr}"
	const auto key = std::make_tuple(std::string(pgvalue), std::string(pgtype), std::string(sqltypestr));
	auto expected_str_iter = conversion_to_correct_output.find(key);
	if (expected_str_iter == conversion_to_correct_output.end()) {
		FAIL() << "Could not find test case in list of correct outputs";
	}
	const std::string expected_str = expected_str_iter->second;


	if (resultbuf == NULL)
		resultbuf = (char *)malloc(500);

	memset(resultbuf, 0xFF, 500);

	fixed_len = get_sql_type_size(sqltype);
	if (fixed_len != -1)
		buflen = fixed_len;
	ASSERT_NE(buflen, -1) << "no buffer length given";

	/*
	 * Use dollar-quotes to make the test case insensitive to
	 * standards_conforming_strings. Some of the test values we use contain
	 * backslashes.
	 */
	snprintf(sql, sizeof(sql),
			 "SELECT CAST(%s AS %s) AS %s_col /* convert to %s */",
			 pgvalue, pgtype, pgtype, sqltypestr);

	rc = SQLExecDirect(hstmt, (SQLCHAR *)sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed", hstmt);

	rc = SQLFetch(hstmt);
	CHECK_STMT_RESULT(rc, "SQLFetch failed", hstmt);

	rc = SQLGetData(hstmt, 1, sqltype, resultbuf, buflen, &len_or_ind);
	CHECK_STMT_RESULT(rc, "SQLGetData failed", hstmt);

	std::string output_str = sql_c_type_to_string(sqltype, resultbuf, len_or_ind, use_time);

	if (rc == SQL_SUCCESS_WITH_INFO)
	{
		SQLCHAR sqlstate[10];

		rc = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, NULL, NULL, 0, NULL);
		if (!SQL_SUCCEEDED(rc) && SQL_NO_DATA != rc)
		{
			ASSERT_TRUE(SQL_SUCCEEDED(rc) || SQL_NO_DATA == rc) << "SQLGetDiagRec failed";
		}

		if (memcmp(sqlstate, "01004", 5) == 0)
		{
			output_str.append(" (truncated)"); 
		}
		else if (SQL_NO_DATA == rc && IsAnsi()) /* maybe */
		{
			output_str.append(" (truncated)");
		}
		else
		{
			print_diag("SQLGetData success with info", SQL_HANDLE_STMT, hstmt);
		}
	}
	else if (1 <= buflen &&
			 SQL_C_WCHAR == sqltype &&
			 0 == len_or_ind &&
			 0 == strcmp(pgtype, "text") &&
			 IsAnsi())
	{
		/* just in order to fix ansi driver test on Windows */
		output_str.append(" (truncated)"); 
	}

	/* Check that the driver didn't write past the buffer */

	int msg_len = 500;
	char msg[500];
	snprintf(msg, msg_len, "For %s Driver wrote byte %02X past result buffer of size %d!\n", sql, (unsigned char)resultbuf[buflen], buflen);
	ASSERT_FALSE((unsigned char)resultbuf[buflen] != 0xFF) << std::string(msg);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* Finally check that the result we read out is expected */
	EXPECT_EQ(output_str, expected_str) << "actual and expected string differ";

}

/* A helper function to execute one command */
void exec_cmd(char *sql)
{
	SQLRETURN rc;

	rc = SQLExecDirect(hstmt, (SQLCHAR *)sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed", hstmt);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	printf("Executed: %s\n", sql);
}

TEST_P(ResultConversionsTest, Basic)
{
	SQLRETURN rc;

	std::string err_msg;
	ASSERT_TRUE(test_connect(&err_msg)) << err_msg;

	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	CHECK_STMT_RESULT(rc, "failed to allocate stmt handle", hstmt);

	/*
	 * The interval stuff requires intervalstyle=postgres at the momemnt.
	 * Someone should fix the driver to understand other formats,
	 * postgres_verbose in particular...
	 */
	ResultConversionTestParams test ( GetParam() );

	test_conversion(test.type.c_str(), test.value.c_str(), test.sqltype, test.sqltypestr.c_str(), 100, 0);

	/* Clean up */
	std::string error_msg;
	ASSERT_TRUE(test_disconnect(&error_msg)) << error_msg; 
}

void other_tests()
{
	/*
	 * Test a few things separately that were not part of the exhaustive test
	 * above.
	 */

	/* Conversions from bytea, in the hex format */

	/*
	 * Use octal escape bytea format in the tests. We will test the conversion
	 * from the hex format separately later.
	 */
	test_conversion("varbinary", "'\\x464f4f'", SQL_C_CHAR, "SQL_C_CHAR", 100, 0);
	test_conversion("varbinary", "'\\x464f4f'", SQL_C_WCHAR, "SQL_C_WCHAR", 100, 0);

	/* Conversion to GUID throws error if the string is not of correct form */
	test_conversion("text", "543c5e21-435a-440b-943c-64af1ad571f1", SQL_C_GUID, "SQL_C_GUID", -1, 0);

	/* Date/timestamp tests of non-date input depends on current date */
	test_conversion("date", "'2011-02-13'", SQL_C_TYPE_DATE, "SQL_C_DATE", -1, 0);
	test_conversion("date", "'2011-02-13'", SQL_C_TYPE_TIMESTAMP, "SQL_C_TIMESTAMP", -1, 0);
	test_conversion("timestamp", "'2011-02-15 15:49:18'", SQL_C_TYPE_DATE, "SQL_C_DATE", -1, 0);
	test_conversion("timestamp", "'2011-02-15 15:49:18'", SQL_C_TYPE_TIMESTAMP, "SQL_C_TIMESTAMP", -1, 0);
	test_conversion("timestamptz", "2011-02-16 17:49:18+03", SQL_C_TYPE_DATE, "SQL_C_DATE", -1, 0);
	test_conversion("timestamptz", "2011-02-16 17:49:18+03", SQL_C_TYPE_TIMESTAMP, "SQL_C_TIMESTAMP", -1, 0);

	/* cast of empty text values using localtime() */
	test_conversion("text", "", SQL_C_TYPE_DATE, "SQL_C_TYPE_DATE", -1, 1);
	test_conversion("text", "", SQL_C_TYPE_TIME, "SQL_C_TYPE_TIME", -1, 0);
	test_conversion("text", "", SQL_C_TYPE_TIMESTAMP, "SQL_C_TYPE_TIMESTAMP", -1, 1);

	/*
	 * Test for truncations.
	 */
	test_conversion("text", "foobar", SQL_C_CHAR, "SQL_C_CHAR", 5, 0);
	test_conversion("text", "foobar", SQL_C_CHAR, "SQL_C_CHAR", 6, 0);
	test_conversion("text", "foobar", SQL_C_CHAR, "SQL_C_CHAR", 7, 0);
	test_conversion("text", "foobar", SQL_C_WCHAR, "SQL_C_WCHAR", 10, 0);
	test_conversion("text", "foobar", SQL_C_WCHAR, "SQL_C_WCHAR", 11, 0);
	test_conversion("text", "foobar", SQL_C_WCHAR, "SQL_C_WCHAR", 12, 0);
	test_conversion("text", "foobar", SQL_C_WCHAR, "SQL_C_WCHAR", 13, 0);
	test_conversion("text", "foobar", SQL_C_WCHAR, "SQL_C_WCHAR", 14, 0);

	test_conversion("text", "", SQL_C_CHAR, "SQL_C_CHAR", 1, 0);
	test_conversion("text", "", SQL_C_WCHAR, "SQL_C_WCHAR", 1, 0);

	test_conversion("timestamp", "'2011-02-15 15:49:18'", SQL_C_CHAR, "SQL_C_CHAR", 19, 0);
	
}

/*
IMPORTANT: Commented out pairs are types that according to the ODBC documentation should work but don't
(https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/converting-data-from-c-to-sql-data-types?view=sql-server-ver16)
*/

INSTANTIATE_TEST_SUITE_P(DISABLED_Boolean, ResultConversionsTest,
	::testing::Combine(
    	::testing::ValuesIn(
	 		std::vector {
				std::pair<std::string, std::string>("1", "boolean"), // equiv to postgres bit
				std::pair<std::string, std::string>("true", "boolean"),
	 		}
	 	),
		::testing::ValuesIn(
			std::vector {
				std::pair<int, std::string>(SQL_C_CHAR, "SQL_C_CHAR"),
				std::pair<int, std::string>(SQL_C_WCHAR, "SQL_C_WCHAR"),
				std::pair<int, std::string>(SQL_C_BIT, "SQL_C_BIT"),
				std::pair<int, std::string>(SQL_C_NUMERIC, "SQL_C_NUMERIC"),
				std::pair<int, std::string>(SQL_C_STINYINT, "SQL_C_STINYINT"),
				std::pair<int, std::string>(SQL_C_UTINYINT, "SQL_C_UTINYINT"),
				std::pair<int, std::string>(SQL_C_SBIGINT, "SQL_C_SBIGINT"),
				std::pair<int, std::string>(SQL_C_UBIGINT, "SQL_C_UBIGINT"),
				std::pair<int, std::string>(SQL_C_SSHORT, "SQL_C_SSHORT"),
				std::pair<int, std::string>(SQL_C_USHORT, "SQL_C_USHORT"),
				std::pair<int, std::string>(SQL_C_SLONG, "SQL_C_SLONG"),
				std::pair<int, std::string>(SQL_C_ULONG, "SQL_C_ULONG"),
				std::pair<int, std::string>(SQL_C_FLOAT, "SQL_C_FLOAT"),
				std::pair<int, std::string>(SQL_C_DOUBLE, "SQL_C_DOUBLE"),
				std::pair<int, std::string>(SQL_C_BINARY, "SQL_C_BINARY"),
				std::pair<int, std::string>(SQL_C_BOOKMARK, "SQL_C_BOOKMARK"),
			}
		)
	)
);

INSTANTIATE_TEST_SUITE_P(DISABLED_Varbinary, ResultConversionsTest,
	::testing::Combine(
    	::testing::ValuesIn(
	 		std::vector {
				std::pair<std::string, std::string>("'\\x464f4f'", "varbinary"), // equiv to postgres bytea
	 		}
	 	),
		::testing::ValuesIn(
			std::vector {
				std::pair<int, std::string>(SQL_C_CHAR, "SQL_C_CHAR"),
				std::pair<int, std::string>(SQL_C_WCHAR, "SQL_C_WCHAR"),
				std::pair<int, std::string>(SQL_C_BINARY, "SQL_C_BINARY"),
			}
		)
	)
);

INSTANTIATE_TEST_SUITE_P(DISABLED_CharAndVarchar, ResultConversionsTest,
	::testing::Combine(
    	::testing::ValuesIn(
	 		std::vector {
				std::pair<std::string, std::string>("'x'", "varchar"), // equiv to postgres char
				std::pair<std::string, std::string>("'x'", "char"),
 			}
	 	),
		::testing::ValuesIn(
			std::vector {
				std::pair<int, std::string>(SQL_C_CHAR, "SQL_C_CHAR"),
				std::pair<int, std::string>(SQL_C_WCHAR, "SQL_C_WCHAR"),
				// std::pair<int, std::string>(SQL_C_BIT, "SQL_C_BIT"),
				// std::pair<int, std::string>(SQL_C_NUMERIC, "SQL_C_NUMERIC"),
				// std::pair<int, std::string>(SQL_C_STINYINT, "SQL_C_STINYINT"),
				// std::pair<int, std::string>(SQL_C_UTINYINT, "SQL_C_UTINYINT"),
				// std::pair<int, std::string>(SQL_C_SBIGINT, "SQL_C_SBIGINT"),
				// std::pair<int, std::string>(SQL_C_UBIGINT, "SQL_C_UBIGINT"),
				// std::pair<int, std::string>(SQL_C_SSHORT, "SQL_C_SSHORT"),
				// std::pair<int, std::string>(SQL_C_USHORT, "SQL_C_USHORT"),
				// std::pair<int, std::string>(SQL_C_SLONG, "SQL_C_SLONG"),
				// std::pair<int, std::string>(SQL_C_ULONG, "SQL_C_ULONG"),
				// std::pair<int, std::string>(SQL_C_FLOAT, "SQL_C_FLOAT"),
				// std::pair<int, std::string>(SQL_C_DOUBLE, "SQL_C_DOUBLE"),
				std::pair<int, std::string>(SQL_C_BINARY, "SQL_C_BINARY"),
				// std::pair<int, std::string>(SQL_C_BOOKMARK, "SQL_C_BOOKMARK"),
				std::pair<int, std::string>(SQL_C_VARBOOKMARK, "SQL_C_VARBOOKMARK"),

				// std::pair<int, std::string>(SQL_C_TYPE_TIME, "SQL_C_TYPE_TIME"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_YEAR, "SQL_C_INTERVAL_YEAR"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_MONTH, "SQL_C_INTERVAL_MONTH"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_DAY, "SQL_C_INTERVAL_DAY"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_HOUR, "SQL_C_INTERVAL_HOUR"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_MINUTE, "SQL_C_INTERVAL_MINUTE"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_SECOND, "SQL_C_INTERVAL_SECOND"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_YEAR_TO_MONTH, "SQL_C_INTERVAL_YEAR_TO_MONTH"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_DAY_TO_HOUR, "SQL_C_INTERVAL_DAY_TO_HOUR"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_DAY_TO_MINUTE, "SQL_C_INTERVAL_DAY_TO_MINUTE"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_DAY_TO_SECOND, "SQL_C_INTERVAL_DAY_TO_SECOND"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_HOUR_TO_MINUTE, "SQL_C_INTERVAL_HOUR_TO_MINUTE"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_HOUR_TO_SECOND, "SQL_C_INTERVAL_HOUR_TO_SECOND"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_MINUTE_TO_SECOND, "SQL_C_INTERVAL_MINUTE_TO_SECOND"),
			}
		)
	)
);

INSTANTIATE_TEST_SUITE_P(DISABLED_Integers, ResultConversionsTest,
	::testing::Combine(
    	::testing::ValuesIn(
	 		std::vector {
				std::pair<std::string, std::string>("1234567890", "bigint"), // equiv to postgres int8
				std::pair<std::string, std::string>("12345", "integer"), // equiv to postgres int2
				std::pair<std::string, std::string>("1234567", "integer"), // equiv to postgres int4
	 		}
	 	),
		::testing::ValuesIn(
			std::vector {
				std::pair<int, std::string>(SQL_C_CHAR, "SQL_C_CHAR"),
				std::pair<int, std::string>(SQL_C_WCHAR, "SQL_C_WCHAR"),
				std::pair<int, std::string>(SQL_C_BIT, "SQL_C_BIT"),
				std::pair<int, std::string>(SQL_C_NUMERIC, "SQL_C_NUMERIC"),
				// std::pair<int, std::string>(SQL_C_STINYINT, "SQL_C_STINYINT"),
				// std::pair<int, std::string>(SQL_C_UTINYINT, "SQL_C_UTINYINT"),
				std::pair<int, std::string>(SQL_C_SBIGINT, "SQL_C_SBIGINT"),
				std::pair<int, std::string>(SQL_C_UBIGINT, "SQL_C_UBIGINT"),
				// std::pair<int, std::string>(SQL_C_SSHORT, "SQL_C_SSHORT"),
				// std::pair<int, std::string>(SQL_C_USHORT, "SQL_C_USHORT"),
				std::pair<int, std::string>(SQL_C_SLONG, "SQL_C_SLONG"),
				std::pair<int, std::string>(SQL_C_ULONG, "SQL_C_ULONG"),
				std::pair<int, std::string>(SQL_C_FLOAT, "SQL_C_FLOAT"),
				std::pair<int, std::string>(SQL_C_DOUBLE, "SQL_C_DOUBLE"),
				std::pair<int, std::string>(SQL_C_BINARY, "SQL_C_BINARY"),
				std::pair<int, std::string>(SQL_C_BOOKMARK, "SQL_C_BOOKMARK"),
				std::pair<int, std::string>(SQL_C_VARBOOKMARK, "SQL_C_VARBOOKMARK"),
			}
		)
	)
);

INSTANTIATE_TEST_SUITE_P(DISABLED_FloatingPoint, ResultConversionsTest,
	::testing::Combine(
    	::testing::ValuesIn(
	 		std::vector {
				std::pair<std::string, std::string>("1.234", "float"), // equiv to postgres float4
				std::pair<std::string, std::string>("1.23456789012", "double"), // equiv to postgres float8
	 		}
	 	),
		::testing::ValuesIn(
			std::vector {
				std::pair<int, std::string>(SQL_C_CHAR, "SQL_C_CHAR"),
				std::pair<int, std::string>(SQL_C_WCHAR, "SQL_C_WCHAR"),
				std::pair<int, std::string>(SQL_C_BIT, "SQL_C_BIT"),
				// std::pair<int, std::string>(SQL_C_NUMERIC, "SQL_C_NUMERIC"),
				std::pair<int, std::string>(SQL_C_STINYINT, "SQL_C_STINYINT"),
				std::pair<int, std::string>(SQL_C_UTINYINT, "SQL_C_UTINYINT"),
				std::pair<int, std::string>(SQL_C_SBIGINT, "SQL_C_SBIGINT"),
				std::pair<int, std::string>(SQL_C_UBIGINT, "SQL_C_UBIGINT"),
				std::pair<int, std::string>(SQL_C_SSHORT, "SQL_C_SSHORT"),
				std::pair<int, std::string>(SQL_C_USHORT, "SQL_C_USHORT"),
				std::pair<int, std::string>(SQL_C_SLONG, "SQL_C_SLONG"),
				std::pair<int, std::string>(SQL_C_ULONG, "SQL_C_ULONG"),
				std::pair<int, std::string>(SQL_C_FLOAT, "SQL_C_FLOAT"),
				std::pair<int, std::string>(SQL_C_DOUBLE, "SQL_C_DOUBLE"),
				// std::pair<int, std::string>(SQL_C_BINARY, "SQL_C_BINARY"),
				std::pair<int, std::string>(SQL_C_BOOKMARK, "SQL_C_BOOKMARK"),
				// std::pair<int, std::string>(SQL_C_VARBOOKMARK, "SQL_C_VARBOOKMARK"),
			}
		)
	)
);

INSTANTIATE_TEST_SUITE_P(DISABLED_Varchar, ResultConversionsTest,
	::testing::Combine(
    	::testing::ValuesIn(
	 		std::vector {
				std::pair<std::string, std::string>("'foobar'", "varchar"),
	 		}
	 	),
		::testing::ValuesIn(
			std::vector {
				std::pair<int, std::string>(SQL_C_CHAR, "SQL_C_CHAR"),
				std::pair<int, std::string>(SQL_C_WCHAR, "SQL_C_WCHAR"),
				// std::pair<int, std::string>(SQL_C_BIT, "SQL_C_BIT"),
				// std::pair<int, std::string>(SQL_C_NUMERIC, "SQL_C_NUMERIC"),
				// std::pair<int, std::string>(SQL_C_STINYINT, "SQL_C_STINYINT"),
				// std::pair<int, std::string>(SQL_C_UTINYINT, "SQL_C_UTINYINT"),
				// std::pair<int, std::string>(SQL_C_SBIGINT, "SQL_C_SBIGINT"),
				// std::pair<int, std::string>(SQL_C_UBIGINT, "SQL_C_UBIGINT"),
				// std::pair<int, std::string>(SQL_C_SSHORT, "SQL_C_SSHORT"),
				// std::pair<int, std::string>(SQL_C_USHORT, "SQL_C_USHORT"),
				// std::pair<int, std::string>(SQL_C_SLONG, "SQL_C_SLONG"),
				// std::pair<int, std::string>(SQL_C_ULONG, "SQL_C_ULONG"),
				// std::pair<int, std::string>(SQL_C_FLOAT, "SQL_C_FLOAT"),
				// std::pair<int, std::string>(SQL_C_DOUBLE, "SQL_C_DOUBLE"),
				std::pair<int, std::string>(SQL_C_BINARY, "SQL_C_BINARY"),
				// std::pair<int, std::string>(SQL_C_BOOKMARK, "SQL_C_BOOKMARK"),
				std::pair<int, std::string>(SQL_C_VARBOOKMARK, "SQL_C_VARBOOKMARK"),

				// std::pair<int, std::string>(SQL_C_TYPE_TIME, "SQL_C_TYPE_TIME"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_YEAR, "SQL_C_INTERVAL_YEAR"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_MONTH, "SQL_C_INTERVAL_MONTH"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_DAY, "SQL_C_INTERVAL_DAY"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_HOUR, "SQL_C_INTERVAL_HOUR"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_MINUTE, "SQL_C_INTERVAL_MINUTE"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_SECOND, "SQL_C_INTERVAL_SECOND"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_YEAR_TO_MONTH, "SQL_C_INTERVAL_YEAR_TO_MONTH"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_DAY_TO_HOUR, "SQL_C_INTERVAL_DAY_TO_HOUR"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_DAY_TO_MINUTE, "SQL_C_INTERVAL_DAY_TO_MINUTE"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_DAY_TO_SECOND, "SQL_C_INTERVAL_DAY_TO_SECOND"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_HOUR_TO_MINUTE, "SQL_C_INTERVAL_HOUR_TO_MINUTE"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_HOUR_TO_SECOND, "SQL_C_INTERVAL_HOUR_TO_SECOND"),
				// std::pair<int, std::string>(SQL_C_INTERVAL_MINUTE_TO_SECOND, "SQL_C_INTERVAL_MINUTE_TO_SECOND"),
			}
		)
	)
);

INSTANTIATE_TEST_SUITE_P(DISABLED_Date, ResultConversionsTest,
	::testing::Combine(
    	::testing::ValuesIn(
	 		std::vector {
				std::pair<std::string, std::string>("'2011-02-13'", "date"),
	 		}
	 	),
		::testing::ValuesIn(
			std::vector {
				std::pair<int, std::string>(SQL_C_CHAR, "SQL_C_CHAR"),
				std::pair<int, std::string>(SQL_C_WCHAR, "SQL_C_WCHAR"),
				std::pair<int, std::string>(SQL_C_BINARY, "SQL_C_BINARY"),
				std::pair<int, std::string>(SQL_C_TYPE_DATE, "SQL_C_TYPE_DATE"),
				std::pair<int, std::string>(SQL_C_TYPE_TIMESTAMP, "SQL_C_TYPE_TIMESTAMP"),
			}
		)
	)
);

INSTANTIATE_TEST_SUITE_P(DISABLED_Time, ResultConversionsTest,
	::testing::Combine(
    	::testing::ValuesIn(
	 		std::vector {
				std::pair<std::string, std::string>("'13:23:34'", "time"),
	 		}
	 	),
		::testing::ValuesIn(
			std::vector {
				std::pair<int, std::string>(SQL_C_CHAR, "SQL_C_CHAR"),
				std::pair<int, std::string>(SQL_C_WCHAR, "SQL_C_WCHAR"),
				std::pair<int, std::string>(SQL_C_BINARY, "SQL_C_BINARY"),
				std::pair<int, std::string>(SQL_C_TYPE_DATE, "SQL_C_TYPE_TIME"),
				std::pair<int, std::string>(SQL_C_TYPE_TIMESTAMP, "SQL_C_TYPE_TIMESTAMP"),
			}
		)
	)
);

INSTANTIATE_TEST_SUITE_P(DISABLED_Timestamp, ResultConversionsTest,
	::testing::Combine(
    	::testing::ValuesIn(
	 		std::vector {
				std::pair<std::string, std::string>("'2011-02-15 15:49:18'", "timestamp"),
	 		}
	 	),
		::testing::ValuesIn(
			std::vector {
				std::pair<int, std::string>(SQL_C_CHAR, "SQL_C_CHAR"),
				std::pair<int, std::string>(SQL_C_WCHAR, "SQL_C_WCHAR"),
				std::pair<int, std::string>(SQL_C_BINARY, "SQL_C_BINARY"),
				std::pair<int, std::string>(SQL_C_TYPE_DATE, "SQL_C_TYPE_DATE"),
				std::pair<int, std::string>(SQL_C_TYPE_TIME, "SQL_C_TYPE_TIME"),
				std::pair<int, std::string>(SQL_C_TYPE_TIMESTAMP, "SQL_C_TYPE_TIMESTAMP"),
			}
		)
	)
);

INSTANTIATE_TEST_SUITE_P(DISABLED_NumericAndDecimal, ResultConversionsTest,
	::testing::Combine(
    	::testing::ValuesIn(
	 		std::vector {
				std::pair<std::string, std::string>("1234.567890", "decimal"), // equiv to postgres decimal 
				std::pair<std::string, std::string>("1234.567890", "numeric"), // should be the same as decimal
	 		}
	 	),
		::testing::ValuesIn(
			std::vector {
				std::pair<int, std::string>(SQL_C_CHAR, "SQL_C_CHAR"),
				std::pair<int, std::string>(SQL_C_WCHAR, "SQL_C_WCHAR"),
				std::pair<int, std::string>(SQL_C_BIT, "SQL_C_BIT"),
				std::pair<int, std::string>(SQL_C_NUMERIC, "SQL_C_NUMERIC"),
				std::pair<int, std::string>(SQL_C_STINYINT, "SQL_C_STINYINT"),
				std::pair<int, std::string>(SQL_C_UTINYINT, "SQL_C_UTINYINT"),
				std::pair<int, std::string>(SQL_C_SBIGINT, "SQL_C_SBIGINT"),
				std::pair<int, std::string>(SQL_C_UBIGINT, "SQL_C_UBIGINT"),
				std::pair<int, std::string>(SQL_C_SSHORT, "SQL_C_SSHORT"),
				std::pair<int, std::string>(SQL_C_USHORT, "SQL_C_USHORT"),
				std::pair<int, std::string>(SQL_C_SLONG, "SQL_C_SLONG"),
				std::pair<int, std::string>(SQL_C_ULONG, "SQL_C_ULONG"),
				std::pair<int, std::string>(SQL_C_FLOAT, "SQL_C_FLOAT"),
				std::pair<int, std::string>(SQL_C_DOUBLE, "SQL_C_DOUBLE"),
				std::pair<int, std::string>(SQL_C_BINARY, "SQL_C_BINARY"),
			}
		)
	)
);

INSTANTIATE_TEST_SUITE_P(DISABLED_FloatingPointEdgeCombinations, ResultConversionsTest,
	::testing::Combine(
		::testing::ValuesIn(floating_point_edge_cases),
		::testing::ValuesIn( 
			std::vector {
				std::pair<int, std::string>(SQL_C_FLOAT, "SQL_C_FLOAT"),
				std::pair<int, std::string>(SQL_C_DOUBLE, "SQL_C_DOUBLE"),
			}
		)
	)
);

// INSTANTIATE_TEST_SUITE_P(NullCombinations, ResultConversionsTest,
// 	::testing::Combine(
// 		::testing::ValuesIn(types),
// 		::testing::Values(std::make_pair(0, null)), // TODO: this doesn't work as null isn't a valid value for std::pair
// 	)
// );

