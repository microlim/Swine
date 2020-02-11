#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef USE_SQLITE3

#include <cf_base.h>
#include <cf_db.h>

int
cf_db_timet( const char *datetime_string ) {
	struct tm stm;
	strptime(datetime_string, "%Y-%m-%d %H:%M:%S", &stm);
	return (int)mktime(&stm);  // t is now your desired time_t
}

int
cf_db_timestring( char *buffer, int buffer_max , int timet ) {
	time_t cv_time = timet ;
	struct tm *ptm = localtime(&cv_time);
	return strftime(buffer, buffer_max,"%Y-%m-%d %H:%M:%S", ptm);
}

int
cf_db_busy (void *handle, int ntry) {
	if (handle == NULL)
		fprintf(stderr, "db busy [%d/3]\n", ntry);
	else
		fprintf(stderr, "db busy [%d/3] : %s\n", ntry, (char *)handle);
	return 3 - ntry;
}

cf_ret_t
cf_init_db (cf_db_t *pdb, char *dbstr) {
	pdb->db = NULL;
	pdb->dbstr = strdup (dbstr);
	CF_ERR_RETURN (pdb->dbstr == NULL, "DB string copy failed");
	return CF_OK;
}

void
cf_release_db (cf_db_t *pdb) {
	if (pdb->db != NULL)
		cf_db_close (pdb);
	CF_FREE (pdb->dbstr);
}

cf_ret_t
cf_db_open (cf_db_t *pdb) {
	if (pdb->db == NULL)
		CF_ERR_RETURN (SQLITE_OK != sqlite3_open (pdb->dbstr, &(pdb->db)), "DB open failed");
	return CF_OK;
}

cf_ret_t
cf_db_close (cf_db_t *pdb) {
	if (pdb->db != NULL)
		CF_ERR_RETURN (SQLITE_OK != sqlite3_close (pdb->db), "DB close failed");
	pdb->db = NULL;
	return CF_OK;
}

void 
cf_errorMsg(const char *errFmt, ...)
{
  va_list argptr;

  fflush(stdout);

  va_start(argptr, errFmt);
  vfprintf(stderr, errFmt, argptr);
  va_end(argptr);

  fflush(stderr);  /* redundant */
}

#endif