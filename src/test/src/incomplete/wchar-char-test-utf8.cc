
static int utf8_test_one(HSTMT hstmt)
{
	int rc;

	SQLLEN		ind, cbParam, cbParam2;
	SQLINTEGER	cbQueryLen;
	unsigned char   lovedt[100] = {0x95, 0x4e, 0x0a, 0x4e, 0x5a, 0x53, 0xf2, 0x53, 0x0, 0x0};
	unsigned char	lovedt2[100] = {0xf2, 0x53, 0x5a, 0x53, 0x0a, 0x4e, 0x95, 0x4e, 0x0, 0x0};
	SQLWCHAR	wchar[100];
	SQLCHAR		str[100];
	SQLCHAR		chardt[100];
	// SQLTCHAR	query[] = _T("select '私は' || ?::text || 'です。貴方は' || ?::text || 'さんですね？  𠀋𡈽𡌛𡑮𡢽𪐷𪗱𪘂𪘚𪚲'");
	SQLTCHAR	query[512] = _T("select '私は' || ?::text || 'です。貴方は' || ?::text || 'さんですね？  𠀋𡈽𡌛𡑮𡢽𪐷𪗱𪘂'");

	rc = SQLBindCol(hstmt, 1, SQL_C_CHAR, (SQLPOINTER) chardt, sizeof(chardt), &ind);
	CHECK_STMT_RESULT(rc, "SQLBindCol to SQL_C_CHAR failed", hstmt);

	// cbParam = SQL_NTS;
	cbParam = strlen(lovedt);
	strcat(lovedt, lovedt2);
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
		SQL_C_WCHAR,	/* value type */
		SQL_WCHAR,			/* param type */
		sizeof(lovedt) / sizeof(lovedt[0]),			/* column size */
		0,			/* dec digits */
		lovedt,		// param1,		/* param value ptr */
		sizeof(lovedt),			/* buffer len */
		&cbParam	/* StrLen_or_IndPtr */);
	CHECK_STMT_RESULT(rc, "SQLBindParameter 1 failed", hstmt);
	// cbParam2 = SQL_NTS;
	strncpy((char *) str, "斉藤浩", sizeof(str));
	cbParam2 = strlen(str);
	strcat((char *) str, "斉藤浩");
	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, sizeof(str), 0, str, sizeof(str), &cbParam2);
	CHECK_STMT_RESULT(rc, "SQLBindParameter 2 failed", hstmt);
	cbQueryLen = (SQLINTEGER) strlen(query);
	strcat((char *) query, "斉藤浩");
	rc = SQLExecDirect(hstmt, query, cbQueryLen);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed to return SQL_C_CHAR", hstmt);
	while (SQL_SUCCEEDED(SQLFetch(hstmt)))
		printf("ANSI=%s\n", chardt);
	fflush(stdout);
	SQLFreeStmt(hstmt, SQL_CLOSE);

	rc = SQLBindCol(hstmt, 1, SQL_C_WCHAR, (SQLPOINTER) wchar, sizeof(wchar) / sizeof(wchar[0]), &ind);
	CHECK_STMT_RESULT(rc, "SQLBindCol to SQL_C_WCHAR failed", hstmt);


	rc = SQLExecDirect(hstmt, query, cbQueryLen);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed to return SQL_C_WCHAR", hstmt);
	while (SQL_SUCCEEDED(rc = SQLFetch(hstmt)))
	{
		print_utf16_le(wchar);
	}
	SQLFreeStmt(hstmt, SQL_CLOSE);

	return rc;
}

int static utf8_test(HSTMT hstmt)
{
	int rc;

	rc = SQLSetConnectAttr(conn, 65548, (SQLPOINTER) 0, 0);
	printf("\t*** wcs_debug = 0 ***\n");
	fflush(stdout);
	rc = utf8_test_one(hstmt);
	rc = SQLSetConnectAttr(conn, 65548, (SQLPOINTER) 1, 0);
	printf("\t*** wcs_debug = 1 ***\n");
	fflush(stdout);
	rc = utf8_test_one(hstmt);

	return rc;
}
