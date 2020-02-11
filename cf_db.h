#ifndef _CFLORA_DATABASE_H_
#define _CFLORA_DATABASE_H_


#ifdef USE_SQLITE3
#include <sqlite3.h>


#define BUSY_TIMEOUT	1000

/** sqlite3 트랜젝션 시작. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_BEGIN_TRANSACTION(d)	\
	do {	\
		int rc;	\
		rc = sqlite3_exec ((d)->db, "BEGIN TRANSACTION;", NULL, 0, NULL);	\
		if (rc != SQLITE_OK) {	\
			cf_errorMsg ( "database transaction start failed.");	\
			return CF_ERR;	\
		} \
	} while (0);

/** sqlite3 트랜젝션 종료. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_END_TRANSACTION(d)	\
	do {	\
		int rc;	\
		rc = sqlite3_exec ((d)->db, "END TRANSACTION;", NULL, 0, NULL);	\
		if (rc != SQLITE_OK) {	\
			cf_errorMsg( "database transaction end failed.");	\
			return CF_ERR;	\
		} \
	} while (0);

/** sqlite3 트랜젝션 롤백. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_ROLLBACK(d)	\
	do {	\
		int rc;	\
		rc = sqlite3_exec ((d)->db, "ROLLBACK;", NULL, 0, NULL);	\
		if (rc != SQLITE_OK) {	\
			cf_errorMsg( "database transaction rollback failed.");	\
			cf_errorMsg( " (%s :%d).\n", __FILE__, __LINE__);	\
			return CF_ERR;	\
		} \
	} while (0);

/** sqlite3 쿼리 실행. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_EXECUTE(d,q)	\
	do {							\
		int rc;						\
		char *errmsg;				\
		rc = sqlite3_exec ((d)->db, q, NULL, 0, &errmsg);	\
		if (rc != SQLITE_OK) {		\
			cf_errorMsg( "database query execution failed : %s (%s).", errmsg, q);	\
			cf_errorMsg( " (%s :%d).\n", __FILE__, __LINE__);	\
			sqlite3_free (errmsg); 	\
			return CF_ERR;			\
		} 							\
	} while (0);

/** sqlite3 쿼리를 실행하고 첫번째 선택된 값을 정수형으로 받아옴. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_EXECUTE_GET_INT(d,q,ref)	\
	do {								\
		int rc, rows, columns;			\
		char *errmsg;					\
		char **results;					\
		rc = sqlite3_get_table ((d)->db, q, &results, &rows, &columns, &errmsg);	\
		if (rc != SQLITE_OK) {			\
			cf_errorMsg( "database query execution failed : %s (%s).", errmsg, q);	\
			cf_errorMsg( " (%s :%d).\n", __FILE__, __LINE__);	\
			sqlite3_free (errmsg); 		\
			return CF_ERR;				\
		} 								\
		*(ref) = atoi (results[1]);		\
		sqlite3_free_table (results);	\
	} while (0);

/** sqlite3 쿼리를 실행하고 첫번째 선택된 값을 문자열형으로 받아옴. 문자열의 길이는 충분하다고 가정. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_EXECUTE_GET_TEXT(d,q,str)	\
	do {								\
		int rc, rows, columns;			\
		char *errmsg;					\
		char **results;					\
		rc = sqlite3_get_table ((d)->db, q, &results, &rows, &columns, &errmsg);	\
		if (rc != SQLITE_OK) {			\
			cf_errorMsg( "database query execution failed : %s (%s).", errmsg, q);	\
			cf_errorMsg( " (%s :%d).\n", __FILE__, __LINE__);	\
			sqlite3_free (errmsg); 		\
			return CF_ERR;				\
		} 								\
		strcpy (str, results[1]);		\
		sqlite3_free_table (results);	\
	} while (0);

/**
 * database 에서 얻어진 datetime 문자열을 time_t로 변환하는 함수
 * @param datetime_string datetime 문자열
 * @return 변환된 time_t.
 * @see strptime,mktime
 */
int
cf_db_timet( const char *datetime_string ) ;


/**
 * time_t를 database로 전달할 datetime 문자열로 변환하는 함수
 * @param datetime_string datetime 문자열
 * @return 변환된 time_t.
 * @see strftime,localtime
 */
int
cf_db_timestring( char *buffer, int buffer_max , int timet ) ;


/**
 * sqlite3 데이터베이스 처리시 락을 획득할 수 없는 경우에 호출되는 함수.
 * 디폴트로 3회 재시도를 수행함.
 * @param handle 출력할 문자열
 * @param ntry 재시도 회수
 * @return 0 이면 재시도를 수행하지 않음.
 * @see sqlite3_busy_handler
 */
int
cf_db_busy (void *handle, int ntry); 

/** 디비 핸들러 */
typedef struct {
	sqlite3 *db;	///< sqlite 핸들러
	char *dbstr;	///< 디비 문자열
} cf_db_t;

/**
 * 디비 핸들러를 초기화 한다.
 * @param pdb 데이터베이스 핸들러
 * @param dbstr 디비 문자열
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_init_db (cf_db_t *pdb, char *dbstr);

/**
 * 디비 핸들러를 해제한다.
 * @param pdb 데이터베이스 핸들러
 */
void
cf_release_db (cf_db_t *pdb);

/**
 * 디비를 연다.
 * @param pdb 데이터베이스 핸들러
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_db_open (cf_db_t *pdb);

/**
 * 디비를 닫는다.
 * @param pdb 데이터베이스 핸들러
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_db_close (cf_db_t *pdb);

/**
 * 에러 메세지 출력
 * @param pdb 에러 메시지 포맷, 가변 아규먼트
 */
void 
cf_errorMsg(const char *errFmt, ...) ;

/* 이하는 랩핑 - 추후 다른 데이터베이스 사용시 쉬운 변경을 위한 트릭 */

#define cf_db_stmt	sqlite3_stmt

#define cf_db_last_insert_rowid(d) sqlite3_last_insert_rowid((d)->db)
#define cf_db_errmsg(d) sqlite3_errmsg((d)->db)
#define cf_db_exec(d,q,f,n,e) sqlite3_exec((d)->db, q, f, n, e)
#define cf_db_busy_handler(d,f,m) sqlite3_busy_handler((d)->db, f, m)
#define cf_db_busy_timeout(d,tm) sqlite3_busy_timeout((d)->db, tm)
#define cf_db_get_table(d,q,v,r,c,e) sqlite3_get_table((d)->db, q, v, r, c, e)
#define cf_db_free_table(r) sqlite3_free_table(r)
#define cf_db_free(e) sqlite3_free(e)
#define cf_db_prepare_v2(d,q,l,st,t) sqlite3_prepare_v2((d)->db,q,l,st,t)
#define cf_db_step(st) sqlite3_step(st)
#define cf_db_reset(st) sqlite3_reset(st)
#define cf_db_finalize(st) sqlite3_finalize(st)
#define cf_db_bind_int(st,i,v) sqlite3_bind_int(st,i,v)
#define cf_db_bind_null(st,i) sqlite3_bind_null(st,i)
#define cf_db_bind_text(st,i,s,l,f) sqlite3_bind_text(st,i,s,l,f)

#define CF_DB_OK	SQLITE_OK

#elif defined(USE_MYSQL)

#include <cf_db_mysql.h>

#endif

#endif
