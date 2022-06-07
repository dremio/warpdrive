/* File:			bind.h
 *
 * Description:		See "bind.cc"
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *
 */

#ifndef __BIND_H__
#define __BIND_H__

#include "wdodbc.h"
#include "descriptor.h"

/*
 * BindInfoClass -- stores information about a bound column
 */
struct BindInfoClass_
{
	SQLLEN	buflen;			/* size of buffer */
	char	*buffer;		/* pointer to the buffer */
	SQLLEN	*used;			/* used space in the buffer (for strings
					 * not counting the '\0') */
	SQLLEN	*indicator;		/* indicator == used in many cases ? */
	SQLSMALLINT	returntype;	/* kind of conversion to be applied when
					 * returning (SQL_C_DEFAULT,
					 * SQL_C_CHAR... etc) */
	SQLSMALLINT	precision;	/* the precision for numeric or timestamp type */
	SQLSMALLINT	scale;		/* the scale for numeric type */
	/* area for work variables */
	char	dummy_data;		/* currently not used */
};

/* struct for SQLGetData */
typedef struct
{
	/* for BLOBs which don't hold the data */
	struct GetBlobDataClass {
		Int8	data_left64;	/* amount of large object data
					   left to read before conversion */
	} blob;
	/* for non-BLOBs which hold the data in ttlbuf after conversion */
	char	*ttlbuf;		/* to save the large result */
	SQLLEN	ttlbuflen;		/* the buffer length */
	SQLLEN	ttlbufused;		/* used length of the buffer */
	SQLLEN	data_left;		/* amount of data left to read */
}	GetDataClass;
#define GETDATA_RESET(gdc) ((gdc).blob.data_left64 = (gdc).data_left = -1)

/*
 * ParameterInfoClass -- stores information about a bound parameter
 */
struct ParameterInfoClass_
{
	SQLLEN	buflen;
	char	*buffer;
	SQLLEN	*used;
	SQLLEN	*indicator;	/* indicator == used in many cases ? */
	SQLSMALLINT	CType;
	SQLSMALLINT	precision;	/* the precision for numeric or timestamp type */
	SQLSMALLINT	scale;		/* the scale for numeric type */
	/* area for work variables */
	char	data_at_exec;
};

typedef struct
{
	SQLLEN	*EXEC_used;	/* amount of data */
	char	*EXEC_buffer; 	/* the data */
	OID	lobj_oid;
}	PutDataClass;

/*
 * ParameterImplClass -- stores implementation information about a parameter
 */
struct ParameterImplClass_
{
	pgNAME		paramName;	/* this is unavailable even in 8.1 */
	SQLSMALLINT	paramType;
	SQLSMALLINT	SQLType;
	OID		wdtype;
	SQLULEN		column_size;
	SQLSMALLINT	decimal_digits;
	SQLSMALLINT	precision;	/* the precision for numeric or timestamp type */
	SQLSMALLINT	scale;		/* the scale for numeric type */
};

typedef struct
{
	GetDataClass	fdata;
	SQLSMALLINT	allocated;
	GetDataClass	*gdata;
}	GetDataInfo;
typedef struct
{
	SQLSMALLINT	allocated;
	PutDataClass	*pdata;
}	PutDataInfo;

#define	PARSE_PARAM_CAST	FALSE
#define	EXEC_PARAM_CAST		TRUE
#define	SIMPLE_PARAM_CAST	TRUE

#define CALC_BOOKMARK_ADDR(book, offset, bind_size, index) \
	(book->buffer + offset + \
	(bind_size > 0 ? bind_size : (SQL_C_VARBOOKMARK == book->returntype ? book->buflen : sizeof(UInt4))) * index)

/* Macros to handle wdtype of parameters */
#define	PIC_get_wdtype(pari) ((pari).wdtype)
#define	PIC_set_wdtype(pari, type) ((pari).wdtype = (type))
#define	PIC_dsp_wdtype(conn, pari) ((pari).wdtype ? (pari).wdtype : sqltype_to_wdtype(conn, (pari).SQLType))

void	extend_column_bindings(ARDFields *opts, int num_columns);
void	reset_a_column_binding(ARDFields *opts, int icol);
void	extend_parameter_bindings(APDFields *opts, int num_params);
void	extend_iparameter_bindings(IPDFields *opts, int num_params);
void	reset_a_parameter_binding(APDFields *opts, int ipar);
void	reset_a_iparameter_binding(IPDFields *opts, int ipar);
int	CountParameters(const StatementClass *stmt, Int2 *inCount, Int2 *ioCount, Int2 *outputCount);
void	GetDataInfoInitialize(GetDataInfo *gdata);
void	extend_getdata_info(GetDataInfo *gdata, int num_columns, BOOL shrink);
void	reset_a_getdata_info(GetDataInfo *gdata, int icol);
void	GDATA_unbind_cols(GetDataInfo *gdata, BOOL freeall);
void	PutDataInfoInitialize(PutDataInfo *pdata);
void	extend_putdata_info(PutDataInfo *pdata, int num_params, BOOL shrink);
void	reset_a_putdata_info(PutDataInfo *pdata, int ipar);
void	PDATA_free_params(PutDataInfo *pdata, char option);
void	SC_param_next(const StatementClass*, int *param_number, ParameterInfoClass **, ParameterImplClass **);

RETCODE       prepareParameters(StatementClass *stmt, BOOL fake_params);
RETCODE       prepareParametersNoDesc(StatementClass *stmt, BOOL fake_params, BOOL param_cast);
int	decideHowToPrepare(StatementClass *stmt, BOOL force);

#endif
