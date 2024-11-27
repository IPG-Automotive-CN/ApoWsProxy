/*
******************************************************************************
**  CarMaker - Version 13.1
**  Vehicle Dynamics Simulation Toolkit
**
**  Copyright (C)   IPG Automotive GmbH
**                  Bannwaldallee 60             Phone  +49.721.98520.0
**                  76185 Karlsruhe              Fax    +49.721.98520.99
**                  Germany                      WWW    www.ipg-automotive.com
******************************************************************************
*/

#ifndef _INFOUTILS_H__
#define _INFOUTILS_H__

#if defined(__GNUC__)
# ifndef CM_PRINTF
#  if __GNUC__ >= 5 || __GNUC__ >= 4 && __GNUC_MINOR__ >= 4
#   define CM_PRINTF gnu_printf
#  else
#   define CM_PRINTF printf
#  endif
# endif
#else
# ifndef __attribute__
#   define __attribute__(x) /*nothing*/
# endif
#endif

#include <stdio.h>

#include <infoc.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
** GetInfoErrorCount ()
**
** get error counter of InfoUtil module
*/

unsigned GetInfoErrorCount (void);


/*
** iRead2 ()
**
** read the info file with <filename>
**
** Return:
**  	The absolute value of the return value is the number of error messages
**  < 0 file access error ("hard error")
**  = 0 ok
**  > 0 syntax error ("soft errors, warnings")
**
** *perrors	list of error messages
*/

int iRead2 (
    tErrorMsg		**pPErrors,
    tInfos		*inf,
    const char  	*filename,
    const char		*MsgPrefix);



/*
** iWrite ()
**
** write info file to file <filename>
**
** Rueckgabe:
**   0 	ok
** < 0	error
*/

int iWrite (tInfos *inf, const char *filename);


/*
** iEntryExists ()
**
** Returns 1 if entry <key> exists, 0 otherwise.
*/
int iEntryExists (const tInfos *Inf, const char *key);
#define iEntryExist(i,k)	iEntryExists((i),(k))


/*
** iEntryHasValue ()
**
** Returns 1 if entry <key> exists and has a non-empty value, 0 otherwise.
*/
int iEntryHasValue (const tInfos *inf, const char *key);



char *iGetStr     (const tInfos *Inf, const char *key);
char *iGetStrOpt  (const tInfos *Inf, const char *key, const char *OptVal);
char *iGetStrA    (const tInfos *Inf, const char *key);
char *iGetStrAOpt (const tInfos *Inf, const char *key, const char *OptVal);

int iGetInt     (const tInfos *Inf, const char *key);
int iGetIntOpt  (const tInfos *Inf, const char *key,  int OptVal);

long iGetLong     (const tInfos *Inf, const char *key);
long iGetLongOpt  (const tInfos *Inf, const char *key,  long OptVal);

double iGetDbl        (const tInfos *Inf, const char *key);
double iGetDblOpt     (const tInfos *Inf, const char *key, double OptVal);
double iGetDblPercOpt (const tInfos *Inf, const char *key, double RefVal, double OptVal);

char **iGetTxt    (const tInfos *Inf, const char *key);
char **iGetTxtOpt (const tInfos *Inf, const char *key);

int iGetBool     (const tInfos *Inf, const char *key);
int iGetBoolOpt  (const tInfos *Inf, const char *key,  int OptVal);



/*
** iGetTable ()
**
** Read column oriented table data from text entry <key>.
** The data is supposed to have <nCols> columns.
** The data is read into <vec> with room for <vecsize> elements.
** In case of success the number of rows read is stored into <nRows>.
**
** The function returns 0 in case of success, -1 otherwise.
** Typical error reasons:
** - Info File entry <key> does not exist / is empty.
** - The number of elements is not a multiple of <nCols>.
** - The table size exceeds <vecsize> elements.
*/
int iGetTable (const tInfos *inf, const char *key,
	       double *vec, int vecsize, int nCols, int *nRows);


/*
** iGetTable2 ()
** iGetTableFlt2 ()
**
** Like iGetTable(), only that the data is allocated automatically
** and is thus not limited in size.
**
** Returns NULL in case of an error, see iGetTable().
*/
double *iGetTable2 (const tInfos *Inf, const char *key,
		    int nCols, int *nRows);

float  *iGetTableFlt2 (const tInfos *Inf, const char *key,
		       int nCols, int *nRows);


/*
** iGetFixedTable2 ()
**
** Like iGetTable2(), only that the table is expected to have exactly
** <nCols> columns and <nRows>, i.e. <nCols>*<nRows> elements.
*/
double *iGetFixedTable2 (const tInfos *Inf, const char *key,
			 int nCols, int nRows);


/*
** iGetTableOpt ()
**
** Read column oriented table data from text entry <key>.
** The data is supposed to have <nCols> columns.
** The data is read into <vec> with room for <vecsize> elements.
** In case of success the number of rows read is stored into <nRows>.
**
** Return value:
** -  0 in case of success
** -  1 in case of invalid table data
** - -1 in case the table size exceeds <vecsize> elements
**
** An error will be logged in case the entry for <key> exists,
** but does not contain valid table data.
*/
int iGetTableOpt (const tInfos *Inf, const char *key,
		  double *vec, int vecsize, int nCols, int *nRows);


/*
** iGetTableOpt2 ()
** iGetTableFltOpt2 ()
**
** Like iGetTableOpt(), only that the data is allocated automatically
** and is thus not limited in size. Additionally a default value can be
** provided (NULL allowed), which is always returned in any case of error.
**
** An error will be logged in case the entry for <key> exists,
** but does not contain valid table data.
*/
double *iGetTableOpt2 (const tInfos *Inf, const char *key,
		       double *DefVec, int nCols, int *nRows);

float  *iGetTableFltOpt2 (const tInfos *Inf, const char *key,
			  float *DefVec, int nCols, int *nRows);


/*
** iGetFixedTableOpt2 ()
**
** Like iGetTableOpt2(), only that the table is expected to have exactly
** <nCols> columns and <nRows>, i.e. <nCols>*<nRows> elements.
*/
double *iGetFixedTableOpt2 (const tInfos *Inf, const char *key,
			    double *DefVec, int nCols, int nRows);


/*
** iGetTableOpt3 ()
**
** Like iGetTableOpt2(), but an error is never logged.
*/
double *iGetTableOpt3 (const tInfos *Inf, const char *key,
		       double *DefVec, int nCols, int *nRows);



void  LogPErrors (const tErrorMsg *perrors);
void  WritePErrors (FILE *fp, const tErrorMsg *perrors);

char  *iGetStrOptV  (tInfos **Inf, const char *key, const char *OptVal);
int    iGetIntOptV  (tInfos **Inf, const char *key, int    OptVal);
long   iGetLongOptV (tInfos **Inf, const char *key, long   OptVal);
double iGetDblOptV  (tInfos **Inf, const char *key, double OptVal);



/*** Text entries, NULL-terminated: create and add to info handle */

typedef struct tCM_Txt {
    int		n;		/* lines used	*/
    int		N;		/* lines max	*/
    char	**Txt;
} tCM_Txt;


tCM_Txt	*Txt_New  (int nLines);
tCM_Txt	*Txt_Dup  (char **Txt);
void     Txt_Clear (tCM_Txt *p);
void     Txt_Delete (tCM_Txt *p);

void	 Txt_Append (tCM_Txt *p, char *const *src);

int
Txt_AddLine (
    tCM_Txt	*CMTxt,
    int	 	InsertPos,	/* -1: append at end */
    const char	*Line);



/*
** iSetStrOpt() / iSetTxtOpt()
**
** Create str- or txt-entry only, if an entry with that name
** does not already exist.
**
** Returns a boolean value indicating whether such an entry
** already existed.
*/
int iSetStrOpt (tInfos *inf, const char *key, const char *value);
int iSetTxtOpt (tInfos *inf, const char *key, char **txt);


/*
** iSetFStr() / iSetFStrOpt()
**
** Create a str-entry using sprintf()-like string formatting.
**
** iSetFStr() creates or overwrites the entry unconditionally and
** returns the value returned by InfoSetStr().
**
** iSetFStrOpt() create the entry only, if an entry with that name
** does not already exist. The function returns a boolean value
** indicating whether such an entry already existed.
*/
int iSetFStr (tInfos *inf, const char *key, const char *format, ...)
    __attribute__((format(CM_PRINTF,3,4)));

int iSetFStrOpt (tInfos *inf, const char *key, const char *format, ...)
    __attribute__((format(CM_PRINTF,3,4)));



void InfoUtils_Cleanup (void);


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _INFOUTILS_H__ */
