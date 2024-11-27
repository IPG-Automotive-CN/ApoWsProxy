/******************************************************************************/
/*  Infofile-API.							      */
/*  ------------------------------------------------------------------------  */
/*  (c) IPG Automotive GmbH    www.ipg-automotive.com   Fon: +49.721.98520-0  */
/*  Bannwaldallee 60    D-76185 Karlsruhe     Germany   Fax: +49.721.98520-99 */
/******************************************************************************/

#ifndef __INFOC_H__
#define __INFOC_H__

#include <stddef.h>

#if defined(__GNUC__)
# ifndef INFO_PRINTF
#  if __GNUC__ >= 5 || __GNUC__ >= 4 && __GNUC_MINOR__ >= 4
#   define INFO_PRINTF gnu_printf
#  else
#   define INFO_PRINTF printf
#  endif
# endif
#else
# ifndef __attribute__
#   define __attribute__(x) /*nothing*/
# endif
#endif

#ifdef __cplusplus
# define __INFOC_CPP_BEGIN	extern "C" {
# define __INFOC_CPP_END	}
__INFOC_CPP_BEGIN
#endif

extern const char InfoVersion[];
extern const int  InfoNumVersion;
extern const char InfoArch[];
extern const char InfoCompFlags[];


/******************************************************************************/

typedef struct tInfos tInfos;
typedef struct tInfoIter tInfoIter;

typedef struct tErrorMsg {
    char *Msg;
    int LineNo;
    char *Line;
    struct tErrorMsg *Next;
} tErrorMsg;


/** Creating, Reading, Writing, Deleting Infofiles ****************************/


tInfos *InfoNew (void);
int InfoDelete (tInfos *inf);


void InfoMerge (tInfos *dstinf, const tInfos *srcinf,
		int (*skip_key) (void *data, const char *key), void *data);


int InfoRead (tErrorMsg **perrors, tInfos *inf, const char *filename);
int InfoReadMem (tErrorMsg **perrors, tInfos *inf,
		 char *bufStart, /*Puffer wird modifiziert*/
		 int bufSize);


int InfoWrite (tInfos *inf, const char *filename);
char *InfoWriteMem (tInfos *inf, int *size);


const char *InfoGetFilename (const tInfos *inf);


char **InfoGetDataPools (void);
void InfoSetDataPools  (char *const *dirs);
void InfoSetDataPoolsV (const char *firstdir, ...);
int  InfoGetDataPoolsMaxLen (void);


char *InfoLocateFile (const char *path, int ignoreinfofilesuffixes);


/** Accessing Individual Infofile-Entries *************************************/


typedef enum {
    Keys,
    Subkeys,
    Unread_Keys,
    All_Keys,
    Next_Level_Keys = Keys	/* COMPAT */
} tIterKind;

char **InfoListKeys (const tInfos *inf, const char *prefix, tIterKind kind);


typedef enum {
    No_Key,
    String_Key,
    Text_Key
#define Empty_Key  50000	/* COMPAT, no longer in use */
#define Prefix_Key 50001	/* COMPAT, no longer in use */
} tKeyKind;

tKeyKind InfoKeyKind (const tInfos *inf, const char *key);


int  InfoDelKey (tInfos *inf, const char *key);


int  InfoGetStr	 (char     **pval, const tInfos *inf, const char *key);
int  InfoGetLLong(long long *pval, const tInfos *inf, const char *key);
int  InfoGetLong (long      *pval, const tInfos *inf, const char *key);
int  InfoGetInt  (int       *pval, const tInfos *inf, const char *key);
int  InfoGetDbl	 (double    *pval, const tInfos *inf, const char *key);
int  InfoGetFlt	 (float     *pval, const tInfos *inf, const char *key);
int  InfoGetBin  (char     **pval, int *pvalsize, tInfos *inf, const char *key);
int  InfoGetTxt	 (char    ***pval, const tInfos *inf, const char *key);


int InfoGetStrDef  (char     **pval, const tInfos *inf, const char *key, char     *def);
int InfoGetLLongDef(long long *pval, const tInfos *inf, const char *key, long long def);
int InfoGetLongDef (long      *pval, const tInfos *inf, const char *key, long      def);
int InfoGetIntDef  (int       *pval, const tInfos *inf, const char *key, int       def);
int InfoGetDblDef  (double    *pval, const tInfos *inf, const char *key, double    def);
int InfoGetFltDef  (float     *pval, const tInfos *inf, const char *key, float     def);
int InfoGetBinDef  (char     **pval, int *pvalsize, tInfos *inf, const char *key, char *def, int defsize);
int InfoGetTxtDef  (char    ***pval, const tInfos *inf, const char *key, char    **def);


char  *InfoFetchStr (const tInfos *inf, const char *key, int sepchar);
char **InfoFetchTxt (const tInfos *inf, const char *key);


double *InfoGetTable  (const tInfos *inf, const char *key, int ncols, int *mrows);
char  **InfoGetTableS (const tInfos *inf, const char *key, int ncols, int *mrows);


double *InfoGetTableDef2 (const tInfos *inf, const char *key,
			  int ncols, int *mrows, double *def, int mrowsdef);
char **InfoGetTableSDef2 (const tInfos *inf, const char *key,
			  int ncols, int *mrows, char **def, int mrowsdef);

double *InfoGetFixedTable    (const tInfos *inf, const char *key,
			      int ncols, int mrows);
char  **InfoGetFixedTableS   (const tInfos *inf, const char *key,
			      int ncols, int mrows);
double *InfoGetFixedTableDef2 (const tInfos *inf, const char *key,
			       int ncols, int  mrows, double *def);
char  **InfoGetFixedTableSDef2 (const tInfos *inf, const char *key,
				int ncols, int  mrows, char **def);

int InfoSetStr	(tInfos *inf, const char *key, const char *val);
int InfoSetStrF	(tInfos *inf, const char *key, const char *format, ...)
    __attribute__((format(INFO_PRINTF,3,4)));
int InfoSetLLong(tInfos *inf, const char *key, long long val);
int InfoSetLong	(tInfos *inf, const char *key, long val);
int InfoSetInt	(tInfos *inf, const char *key, int val);
int InfoSetDbl	(tInfos *inf, const char *key, double val);
int InfoSetFlt	(tInfos *inf, const char *key, float val);
int InfoSetBin  (tInfos *inf, const char *key, const char *val, int valsize);
int InfoSetZBin (tInfos *inf, const char *key, const char *val, int valsize);
int InfoSetTxt	(tInfos *inf, const char *key,        char *const *val);
int InfoSetTxtN (tInfos *inf, const char *key, int n, char *const *val);
int InfoSetTxtV	(tInfos *inf, const char *key, const char *first, ...);
int InfoTxtAppendF (tInfos *inf, const char *key, const char *format, ...)
    __attribute__((format(INFO_PRINTF,3,4)));
int InfoTxtAppendN (tInfos *inf, const char *key, int n, char *const *val);
int InfoTxtAppendV (tInfos *inf, const char *key, const char *first, ...);


int InfoMoveKeyBefore (tInfos *inf, const char *destkey, const char *key);
int InfoMoveKeyBehind (tInfos *inf, const char *destkey, const char *key);


int InfoAddLineBefore (tInfos *inf, const char *destkey, const char *val);
int InfoAddLineBehind (tInfos *inf, const char *destkey, const char *val);


/** Iterating Over Infofile-Entries *******************************************/


tInfoIter *	InfoINew (void);
int		InfoIDelete (tInfoIter *iter);


int	InfoIStart (tInfoIter *iter, tInfos *inf,
		    const char *prefix, tIterKind kind);
int	InfoIReset (tInfoIter *iter);


int InfoIGetKey  (char **pkey,                    tInfoIter *iter);
int InfoIGetStr  (char **pkey, char       **pval, tInfoIter *iter);
int InfoIGetLLong(char **pkey, long long   *pval, tInfoIter *iter);
int InfoIGetLong (char **pkey, long        *pval, tInfoIter *iter);
int InfoIGetInt  (char **pkey, int         *pval, tInfoIter *iter);
int InfoIGetDbl  (char **pkey, double      *pval, tInfoIter *iter);
int InfoIGetFlt  (char **pkey, float       *pval, tInfoIter *iter);
int InfoIGetBin  (char **pkey, char       **pval, int *pvalsize, tInfoIter *iter);
int InfoIGetTxt  (char **pkey, char      ***pval, tInfoIter *iter);


/** Utility Functions *********************************************************/


char **InfoMakeTxt (int linkonly, const char *first, ...);
void InfoFreeTxt (char **txt);

char **InfoDupTxt (char *const *orig);

int InfoTxtLength (char *const *txt);
int InfoTxtContains (char *const *txt, const char *s);
int InfoTxtCompare (char *const *ta, char *const *tb);

char *InfoJoinTxt (char *const *txt, int sepchar);
char **InfoSplitStr (const char *str, int sepchar);

char *InfoKeyJoin (char *subkeys[]);
char **InfoKeySplit (const char *key);

int InfoStrMatch (const char *pat, const char *str);


/** Error Codes ***************************************************************/

typedef enum {
    INFO_EOK = 0,

    INFO_EINTERNAL = 1,

    INFO_EBADFNAME = 2,
    INFO_EBADFTYPE = 3,
    INFO_ECONTENT = 4,
    INFO_EVERSION = 5,
    INFO_EREAD = 6,
    INFO_EWRITE = 7,

    INFO_EBADKEY = 8,
    INFO_EBADSUBKEY = 9,
    INFO_ETOOMANY = 10,
    INFO_ENOINFO = 11,
    INFO_EBADTYPE = 12,

    INFO_EMATBADTYPE = 13,
    INFO_EMATVERSION = 14,
    INFO_EMATBADHEADER = 15,
    INFO_EMATSYNTAX = 16,
    INFO_EMATBADSIZE = 17,

    INFO_ECRYPTCORRUPT = 18,
    INFO_ECRYPTWRONGSECRET = 19,
    INFO_ECRYPTALGODATED = 20,
    INFO_ECRYPTALGO = 21,
    INFO_ECRYPTEXPIRED = 22,
    INFO_ECRYPTACCESS = 23,
    INFO_ECRYPTNOPASSWD = 24,
    INFO_ECRYPTNOABSTRACT = 25,
    INFO_ECRYPTCONFIG = 26,
    INFO_ECRYPTCONFIGILLEGAL = 27,

    INFO_EBINCORRUPT = 28,
    INFO_EBINFORMAT = 29,

    INFO_ECRYPTDATECONFUSION = 30
} tInfoError;

extern tInfoError InfoErrno;
extern int InfoSysErrno;


const char *InfoStrError (void);


/** Matrix-API ****************************************************************/

typedef enum tInfoMatType {
    INFOMAT_UNKNOWN = 0,
    INFOMAT_DOUBLE,
    INFOMAT_SINGLE,
    INFOMAT_UINT32, INFOMAT_INT32,
    INFOMAT_UINT16, INFOMAT_INT16,
    INFOMAT_UINT8,  INFOMAT_INT8,
    INFOMAT_LOGICAL
} tInfoMatType;


typedef struct tInfoMat {
    tInfoMatType	Type;
    int			isComplex;
    int			nDims;		/* nDims>=2 */
    size_t		*Dims;		/* Dims[0]=mRows, Dims[1]=nCols */
    int			nElems;
    void		*RealData;	/* Data in column major order */
    void		*ImagData;	/* ImagData==NULL if not complex */
    int			isLinkedData;	/* RealData/ImagData externally allocated? */
} tInfoMat;


tInfoMat   *InfoGetMat    (const tInfos *inf, const char *key,
			   tInfoMatType fallbacktype, int fallbackncols);
tInfoMat   *InfoGetMatDef (const tInfos *inf, const char *key,
			   tInfoMatType fallbacktype, int fallbackncols,
			   tInfoMat *def);
int         InfoSetMat    (tInfos *inf, const char *key,
			   const tInfoMat *mat, const char *format);


tInfoMat   *InfoMakeMat (tInfoMatType type, int iscomplex,
			 int ndims, size_t *dims,
			 int nelems, void *realdata, void *imagdata, int islinkeddata);
void        InfoFreeMat   (tInfoMat *mat);


tInfoMat   *InfoMatMakeScalar    (tInfoMatType type, int iscomplex,
				  double re, double im);
tInfoMat   *InfoMatMakeRowVector (tInfoMatType type, int nelems,
				  const void *realdata, const void *imagdata);
tInfoMat   *InfoMatMakeColVector (tInfoMatType type, int nelems,
				  const void *realdata, const void *imagdata);


/** Crypto-API ****************************************************************/

int InfoReadPart (tInfos *inf, const char *filename, int size);

int InfoIsEncrypted (const tInfos *inf);
int InfoWasEncrypted (const tInfos *inf);

int InfoExpandAbstract (const tInfos *inf, tInfos *abstract);

int InfoDecryptCheck (tInfos *inf);
int InfoDecrypt (tInfos *inf, tInfos *config);

int  InfoWriteEncrypted (tInfos *inf, const char *filename,
			 tInfos *config);


void InfoTreatAsDecrypted (tInfos *inf);

void InfoClearMem (const tInfos *inf);
void InfoClearMemAll (void);

void InfoEmptyPool (void);
void InfoAddToPool (const char *passwd);

void InfoSetFileSuffixes (char *const *suffixes);
char **InfoGetFileSuffixes (void);
int  InfoGetFileSuffixesMaxLen (void);


/** Variable-API (Named/Key Values) *******************************************/

int  InfoGetValueSubstMode (void);
void InfoSetValueSubstMode (int enabled);

void InfoSetValue (const char *name, const char *value);

void InfoUnsetValue (const char *name);
void InfoUnsetAllValues (void);

int  InfoGetValueCount (void);
int  InfoGetValueInfo (int i, const char **pname, const char **pvalue);

int  InfoGetUnresolvedFileCount (void);
const char *InfoGetUnresolvedFile (int index);
char **InfoGetUnresolvedNames (int index, int *count);
void InfoClearUnresolved (void);

char **InfoGetUnreferencedValues (int *count);
void InfoResetAllValueCounters (void);

char **InfoListValueNames (const tInfos *inf);

// InfoKeyHasValueRef(): Returns boolean.
// Wildcards "*" (all keys) and "a.b*" (a.b and all keys below a.b.c) are supported.
int  InfoKeyHasValueRef (const tInfos *inf, const char *key);

void InfoAddToCategory (tInfos *inf, const char *category);
void InfoRemoveFromCategory (tInfos *inf, const char *category);

void InfoSetKeyValueStr  (const char *path, const char *value);
void InfoSetKeyValueTxt  (const char *path, char *const *value);
void InfoSetKeyValueTxtV (const char *path, const char *first, ...);

void InfoUnsetKeyValue (const char *path);
void InfoUnsetAllKeyValues (void);

int  InfoGetKeyValueCount (void);
int  InfoGetKeyValueInfo  (int i, const char **ppath, const char **pvalue);
int  InfoGetKeyValueInfo2 (int i, const char **ppath,
			   const char **pvalue, char ***ptxtvalue /*free(txtvalue)!*/);


/** Tcl Stuff *****************************************************************/

/* Tcl-related stuff below can be activated by #including <tcl.h> */
#ifdef TCL_VERSION

/* Initialize statically linked Tcl module. */
int Ifile_Init (Tcl_Interp *interp);

/* Retrieve tInfos handle from command name created with [ifile new]. */
tInfos *InfoGetHandleFromCmd (Tcl_Interp *interp, const char *cmdname);

#endif


/** Debugging/Internal Stuff **************************************************/

extern int InfoInitialized;

int InfoSetupLocale (void);

void InfoSetup (void);

void InfoCleanup (void);

void InfoShowHandles (void);

int  InfoDeleteAll (void);

int InfoWrite_v1 (tInfos *inf, const char *filename);


/******************************************************************************/

#ifdef __cplusplus
__INFOC_CPP_END
#endif

#endif /* !defined(__INFOC_H__) */

