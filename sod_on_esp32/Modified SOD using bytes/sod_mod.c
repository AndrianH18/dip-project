/*
* NOTE: PORTED FOR EE3080 DIP PROJECT; TO RUN ON ESP32 BOARD.
*		Only necessary functions are kept, everything else is discarded.
*
* ======================================================================================
*
* SOD - An Embedded Computer Vision & Machine Learning Library.
* Copyright (C) 2018 - 2020 PixLab| Symisc Systems. https://sod.pixlab.io
* Version 1.1.8
*
* Symisc Systems employs a dual licensing model that offers customers
* a choice of either our open source license (GPLv3) or a commercial
* license.
*
* For information on licensing, redistribution of the SOD library, and for a DISCLAIMER OF ALL WARRANTIES
* please visit:
*     https://pixlab.io/sod
* or contact:
*     licensing@symisc.net
*     support@pixlab.io
*/
/*
* This file is part of Symisc SOD - Open Source Release (GPLv3)
*
* SOD is free software : you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* SOD is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with SOD. If not, see <http://www.gnu.org/licenses/>.
*/
/* $SymiscID: sod.c v1.1.8 Win10 2019-11-16 03:23 stable <devel@symisc.net> $ */

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
/*
* Ignore Microsoft compilers warnings on fopen() which is used only
* by the CNN layer for reading SOD models or saving Realnets models to disk.
*/
#define _CRT_SECURE_NO_WARNINGS
#endif /*_CRT_SECURE_NO_WARNINGS*/
/* Disable the double to float warning */
#pragma warning(disable:4244)
#pragma warning(disable:4305)
#endif /* _MSC_VER */
/* Standard C library includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>
#ifdef SOD_MEM_DEBUG
/* Memory leak detection which is done under Visual Studio only */
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif /* SOD_MEM_DEBUG */
#include <float.h>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif /* _USE_MATH_DEFINES */
#include <math.h>
#include <string.h>
#include <limits.h>
#include "sod_mod.h"

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif /* M_PI */

/* Generic dynamic set */
typedef struct SySet SySet;
struct SySet
{
	void *pBase;               /* Base pointer */
	size_t nUsed;              /* Total number of used slots  */
	size_t nSize;              /* Total number of available slots */
	size_t eSize;              /* Size of a single slot */
	void *pUserData;           /* User private data associated with this container */
};

typedef struct SyBlob SyBlob;
struct SyBlob
{
	void   *pBlob;	          /* Base pointer */
	size_t  nByte;	          /* Total number of used bytes */
	size_t  mByte;	          /* Total number of available bytes */
	int  nFlags;	          /* Blob internal flags, see below */
};

typedef struct sod_vfs sod_vfs;
#define LIBCOX_VFS_VERSION 2900 /* 2.9 */
struct sod_vfs {
	const char *zName;       /* Name of this virtual file system [i.e: Windows, UNIX, etc.] */
	int iVersion;            /* Structure version number (currently 2.6) */
	int szOsFile;           
	int mxPathname;          /* Maximum file pathname length */
							 /* Directory functions */
	int(*xChdir)(const char *);                     /* Change directory */
	int(*xGetcwd)(SyBlob *);                /* Get the current working directory */
	int(*xMkdir)(const char *, int, int);           /* Make directory */
	int(*xRmdir)(const char *);                     /* Remove directory */
	int(*xIsdir)(const char *);                     /* Tells whether the filename is a directory */
	int(*xRename)(const char *, const char *);       /* Renames a file or directory */
	int(*xRealpath)(const char *, SyBlob *);    /* Return canonicalized absolute pathname*/
												/* Dir handle */
	int(*xOpenDir)(const char *, void **);    /* Open directory handle */
	void(*xCloseDir)(void *pHandle);                           /* Close directory handle */
	int(*xDirRead)(void *pHandle, SyBlob *);     /* Read the next entry from the directory handle */
	void(*xDirRewind)(void *pHandle);                   /* Rewind the cursor */
														/* Systems functions */
	int(*xUnlink)(const char *);                    /* Deletes a file */
	int(*xFileExists)(const char *);                /* Checks whether a file or directory exists */
	int64_t(*xFreeSpace)(const char *);        /* Available space on filesystem or disk partition */
	int64_t(*xTotalSpace)(const char *);       /* Total space on filesystem or disk partition */
	int64_t(*xFileSize)(const char *);         /* Gets file size */
	int(*xIsfile)(const char *);                    /* Tells whether the filename is a regular file */
	int(*xReadable)(const char *);                  /* Tells whether a file exists and is readable */
	int(*xWritable)(const char *);                  /* Tells whether the filename is writable */
	int(*xExecutable)(const char *);                /* Tells whether the filename is executable */
	int(*xGetenv)(const char *, SyBlob *);      /* Gets the value of an environment variable */
	int(*xSetenv)(const char *, const char *);       /* Sets the value of an environment variable */
	int(*xMmap)(const char *, void **, size_t *); /* Read-only memory map of the whole file */
	void(*xUnmap)(void *, size_t);                /* Unmap a memory view */
	void(*xTempDir)(SyBlob *);                 /* Get path of the temporary directory */
	float(*xTicks)();                          /* High precision timer */
};


#if defined (_WIN32) || defined (WIN32) ||  defined (_WIN64) || defined (WIN64) || defined(__MINGW32__) || defined (_MSC_VER)
/* Windows Systems */
#if !defined(__WINNT__)
#define __WINNT__
#endif 
#else
/*
* By default we will assume that we are compiling on a UNIX like (iOS and Android included) system.
* Otherwise the OS_OTHER directive must be defined.
*/
#if !defined(OS_OTHER)
#if !defined(__UNIXES__)
#define __UNIXES__
#endif /* __UNIXES__ */
#else
#endif /* OS_OTHER */
#endif /* __WINNT__/__UNIXES__ */
/*
* SOD Built-in VFS which is Based on Libcox another open source library developed by Symisc Systems.
*/
/*
* Symisc libcox: Cross Platform Utilities & System Calls.
* Copyright (C) 2014, 2015 Symisc Systems http://libcox.symisc.net/
* Version 1.7
* For information on licensing, redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES
* please contact Symisc Systems via:
*       legal@symisc.net
*       licensing@symisc.net
*       contact@symisc.net
* or visit:
*      http://libcox.symisc.net/
*/
/* $SymiscID: vfs.c v1.3 FreeBSD 2017-05-22 01:19 stable <chm@symisc.net> $ */
/*
* Virtual File System (VFS) for libcox (SOD modified).
*/
/*
* Wrapper function used to mimic some Libcox code that is no longer needed here.
*/
static inline int libcox_result_string(SyBlob *pBlob, const char *zBuf, int nLen)
{
	int rc;
	if (nLen < 0) {
		nLen = (int)strlen(zBuf);
	}
	rc = SyBlobAppend(&(*pBlob), zBuf, (size_t)nLen);
	SyBlobNullAppend(&(*pBlob));
	return rc;
}
#ifdef __WINNT__
/*
* Windows VFS implementation for the LIBCOX engine.
* Authors:
*    Symisc Systems, devel@symisc.net.
*    Copyright (C) Symisc Systems, http://libcox.symisc.net
* Status:
*    Stable.
*/
/* What follows here is code that is specific to windows systems. */
#include <Windows.h>
/*
** Convert a UTF-8 string to microsoft unicode (UTF-16?).
**
** Space to hold the returned string is obtained from HeapAlloc().
** Taken from the sqlite3 source tree
** status: Public Domain
*/
static WCHAR *utf8ToUnicode(const char *zFilename) {
	int nChar;
	WCHAR *zWideFilename;

	nChar = MultiByteToWideChar(CP_UTF8, 0, zFilename, -1, 0, 0);
	zWideFilename = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, nChar * sizeof(zWideFilename[0]));
	if (zWideFilename == 0) {
		return 0;
	}
	nChar = MultiByteToWideChar(CP_UTF8, 0, zFilename, -1, zWideFilename, nChar);
	if (nChar == 0) {
		HeapFree(GetProcessHeap(), 0, zWideFilename);
		return 0;
	}
	return zWideFilename;
}
/*
** Convert a UTF-8 filename into whatever form the underlying
** operating system wants filenames in.Space to hold the result
** is obtained from HeapAlloc() and must be freed by the calling
** function.
** Taken from the sqlite3 source tree
** status: Public Domain
*/
static void *convertUtf8Filename(const char *zFilename) {
	void *zConverted;
	zConverted = utf8ToUnicode(zFilename);
	return zConverted;
}
/*
** Convert microsoft unicode to UTF-8.  Space to hold the returned string is
** obtained from HeapAlloc().
** Taken from the sqlite3 source tree
** status: Public Domain
*/
static char *unicodeToUtf8(const WCHAR *zWideFilename) {
	char *zFilename;
	int nByte;

	nByte = WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, 0, 0, 0, 0);
	zFilename = (char *)HeapAlloc(GetProcessHeap(), 0, nByte);
	if (zFilename == 0) {
		return 0;
	}
	nByte = WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, zFilename, nByte, 0, 0);
	if (nByte == 0) {
		HeapFree(GetProcessHeap(), 0, zFilename);
		return 0;
	}
	return zFilename;
}
/* int (*xchdir)(const char *) */
static int WinVfs_chdir(const char *zPath)
{
	void * pConverted;
	BOOL rc;
	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return -1;
	}
	rc = SetCurrentDirectoryW((LPCWSTR)pConverted);
	HeapFree(GetProcessHeap(), 0, pConverted);
	return rc ? SOD_OK : -1;
}
/* int (*xGetcwd)(SyBlob *) */
static int WinVfs_getcwd(SyBlob *pCtx)
{
	WCHAR zDir[2048];
	char *zConverted;
	DWORD rc;
	/* Get the current directory */
	rc = GetCurrentDirectoryW(sizeof(zDir), zDir);
	if (rc < 1) {
		return -1;
	}
	zConverted = unicodeToUtf8(zDir);
	if (zConverted == 0) {
		return -1;
	}
	libcox_result_string(pCtx, zConverted, -1/*Compute length automatically*/); /* Will make it's own copy */
	HeapFree(GetProcessHeap(), 0, zConverted);
	return SOD_OK;
}
/* int (*xMkdir)(const char *, int, int) */
static int WinVfs_mkdir(const char *zPath, int mode, int recursive)
{
	void * pConverted;
	BOOL rc;
	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return -1;
	}
	mode = 0; /* MSVC warning */
	recursive = 0;
	rc = CreateDirectoryW((LPCWSTR)pConverted, 0);
	HeapFree(GetProcessHeap(), 0, pConverted);
	return rc ? SOD_OK : -1;
}
/* int (*xRmdir)(const char *) */
static int WinVfs_rmdir(const char *zPath)
{
	void * pConverted;
	BOOL rc;
	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return -1;
	}
	rc = RemoveDirectoryW((LPCWSTR)pConverted);
	HeapFree(GetProcessHeap(), 0, pConverted);
	return rc ? SOD_OK : -1;
}
/* int (*xIsdir)(const char *) */
static int WinVfs_isdir(const char *zPath)
{
	void * pConverted;
	DWORD dwAttr;
	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return -1;
	}
	dwAttr = GetFileAttributesW((LPCWSTR)pConverted);
	HeapFree(GetProcessHeap(), 0, pConverted);
	if (dwAttr == INVALID_FILE_ATTRIBUTES) {
		return -1;
	}
	return (dwAttr & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
}
/* int (*xRename)(const char *, const char *) */
static int WinVfs_Rename(const char *zOld, const char *zNew)
{
	void *pOld, *pNew;
	BOOL rc = 0;
	pOld = convertUtf8Filename(zOld);
	if (pOld == 0) {
		return -1;
	}
	pNew = convertUtf8Filename(zNew);
	if (pNew) {
		rc = MoveFileW((LPCWSTR)pOld, (LPCWSTR)pNew);
	}
	HeapFree(GetProcessHeap(), 0, pOld);
	if (pNew) {
		HeapFree(GetProcessHeap(), 0, pNew);
	}
	return rc ? SOD_OK : -1;
}
/* int (*xRealpath)(const char *, SyBlob *) */
static int WinVfs_Realpath(const char *zPath, SyBlob *pCtx)
{
	WCHAR zTemp[2048];
	void *pPath;
	char *zReal;
	DWORD n;
	pPath = convertUtf8Filename(zPath);
	if (pPath == 0) {
		return -1;
	}
	n = GetFullPathNameW((LPCWSTR)pPath, 0, 0, 0);
	if (n > 0) {
		if (n >= sizeof(zTemp)) {
			n = sizeof(zTemp) - 1;
		}
		GetFullPathNameW((LPCWSTR)pPath, n, zTemp, 0);
	}
	HeapFree(GetProcessHeap(), 0, pPath);
	if (!n) {
		return -1;
	}
	zReal = unicodeToUtf8(zTemp);
	if (zReal == 0) {
		return -1;
	}
	libcox_result_string(pCtx, zReal, -1); /* Will make it's own copy */
	HeapFree(GetProcessHeap(), 0, zReal);
	return SOD_OK;
}
/* int (*xUnlink)(const char *) */
static int WinVfs_unlink(const char *zPath)
{
	void * pConverted;
	BOOL rc;
	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return -1;
	}
	rc = DeleteFileW((LPCWSTR)pConverted);
	HeapFree(GetProcessHeap(), 0, pConverted);
	return rc ? SOD_OK : -1;
}
/* int64_t (*xFreeSpace)(const char *) */
static int64_t WinVfs_DiskFreeSpace(const char *zPath)
{
	DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
	void * pConverted;
	WCHAR *p;
	BOOL rc;
	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return 0;
	}
	p = (WCHAR *)pConverted;
	for (; *p; p++) {
		if (*p == '\\' || *p == '/') {
			*p = '\0';
			break;
		}
	}
	rc = GetDiskFreeSpaceW((LPCWSTR)pConverted, &dwSectPerClust, &dwBytesPerSect, &dwFreeClusters, &dwTotalClusters);
	if (!rc) {
		return 0;
	}
	return (int64_t)dwFreeClusters * dwSectPerClust * dwBytesPerSect;
}
/* int64_t (*xTotalSpace)(const char *) */
static int64_t WinVfs_DiskTotalSpace(const char *zPath)
{
	DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
	void * pConverted;
	WCHAR *p;
	BOOL rc;
	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return 0;
	}
	p = (WCHAR *)pConverted;
	for (; *p; p++) {
		if (*p == '\\' || *p == '/') {
			*p = '\0';
			break;
		}
	}
	rc = GetDiskFreeSpaceW((LPCWSTR)pConverted, &dwSectPerClust, &dwBytesPerSect, &dwFreeClusters, &dwTotalClusters);
	if (!rc) {
		return 0;
	}
	return (int64_t)dwTotalClusters * dwSectPerClust * dwBytesPerSect;
}
/* int (*xFileExists)(const char *) */
static int WinVfs_FileExists(const char *zPath)
{
	void * pConverted;
	DWORD dwAttr;
	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return -1;
	}
	dwAttr = GetFileAttributesW((LPCWSTR)pConverted);
	HeapFree(GetProcessHeap(), 0, pConverted);
	if (dwAttr == INVALID_FILE_ATTRIBUTES) {
		return -1;
	}
	return SOD_OK;
}
/* Open a file in a read-only mode */
static HANDLE OpenReadOnly(LPCWSTR pPath)
{
	DWORD dwType = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
	DWORD dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
	DWORD dwAccess = GENERIC_READ;
	DWORD dwCreate = OPEN_EXISTING;
	HANDLE pHandle;
	pHandle = CreateFileW(pPath, dwAccess, dwShare, 0, dwCreate, dwType, 0);
	if (pHandle == INVALID_HANDLE_VALUE) {
		return 0;
	}
	return pHandle;
}
/* int64_t (*xFileSize)(const char *) */
static int64_t WinVfs_FileSize(const char *zPath)
{
	DWORD dwLow, dwHigh;
	void * pConverted;
	int64_t nSize;
	HANDLE pHandle;

	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return -1;
	}
	/* Open the file in read-only mode */
	pHandle = OpenReadOnly((LPCWSTR)pConverted);
	HeapFree(GetProcessHeap(), 0, pConverted);
	if (pHandle) {
		dwLow = GetFileSize(pHandle, &dwHigh);
		nSize = dwHigh;
		nSize <<= 32;
		nSize += dwLow;
		CloseHandle(pHandle);
	}
	else {
		nSize = -1;
	}
	return nSize;
}
/* int (*xIsfile)(const char *) */
static int WinVfs_isfile(const char *zPath)
{
	void * pConverted;
	DWORD dwAttr;
	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return -1;
	}
	dwAttr = GetFileAttributesW((LPCWSTR)pConverted);
	HeapFree(GetProcessHeap(), 0, pConverted);
	if (dwAttr == INVALID_FILE_ATTRIBUTES) {
		return -1;
	}
	return (dwAttr & (FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE)) ? SOD_OK : -1;
}
/* int (*xWritable)(const char *) */
static int WinVfs_iswritable(const char *zPath)
{
	void * pConverted;
	DWORD dwAttr;
	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return -1;
	}
	dwAttr = GetFileAttributesW((LPCWSTR)pConverted);
	HeapFree(GetProcessHeap(), 0, pConverted);
	if (dwAttr == INVALID_FILE_ATTRIBUTES) {
		return -1;
	}
	if ((dwAttr & (FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL)) == 0) {
		/* Not a regular file */
		return -1;
	}
	if (dwAttr & FILE_ATTRIBUTE_READONLY) {
		/* Read-only file */
		return -1;
	}
	/* File is writable */
	return SOD_OK;
}
/* int (*xExecutable)(const char *) */
static int WinVfs_isexecutable(const char *zPath)
{
	void * pConverted;
	DWORD dwAttr;
	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return -1;
	}
	dwAttr = GetFileAttributesW((LPCWSTR)pConverted);
	HeapFree(GetProcessHeap(), 0, pConverted);
	if (dwAttr == INVALID_FILE_ATTRIBUTES) {
		return -1;
	}
	if ((dwAttr & FILE_ATTRIBUTE_NORMAL) == 0) {
		/* Not a regular file */
		return -1;
	}
	/* FIXEME: GetBinaryType or another call to make sure this thing is executable  */

	/* File is executable */
	return SOD_OK;
}
/* int (*xGetenv)(const char *, SyBlob *) */
static int WinVfs_Getenv(const char *zVar, SyBlob *pCtx)
{
	char zValue[1024];
	DWORD n;
	/*
	* According to MSDN
	* If lpBuffer is not large enough to hold the data, the return
	* value is the buffer size, in characters, required to hold the
	* string and its terminating null character and the contents
	* of lpBuffer are undefined.
	*/
	n = sizeof(zValue);
	/* Extract the environment value */
	n = GetEnvironmentVariableA(zVar, zValue, sizeof(zValue));
	if (!n) {
		/* No such variable*/
		return -1;
	}
	libcox_result_string(pCtx, zValue, (int)n);
	return SOD_OK;
}
/* int (*xSetenv)(const char *, const char *) */
static int WinVfs_Setenv(const char *zName, const char *zValue)
{
	BOOL rc;
	rc = SetEnvironmentVariableA(zName, zValue);
	return rc ? SOD_OK : -1;
}
/* int (*xMmap)(const char *, void **, size_t *) */
static int WinVfs_Mmap(const char *zPath, void **ppMap, size_t *pSize)
{
	DWORD dwSizeLow, dwSizeHigh;
	HANDLE pHandle, pMapHandle;
	void *pConverted, *pView;

	pConverted = convertUtf8Filename(zPath);
	if (pConverted == 0) {
		return -1;
	}
	pHandle = OpenReadOnly((LPCWSTR)pConverted);
	HeapFree(GetProcessHeap(), 0, pConverted);
	if (pHandle == 0) {
		return -1;
	}
	/* Get the file size */
	dwSizeLow = GetFileSize(pHandle, &dwSizeHigh);
	/* Create the mapping */
	pMapHandle = CreateFileMappingW(pHandle, 0, PAGE_READONLY, dwSizeHigh, dwSizeLow, 0);
	if (pMapHandle == 0) {
		CloseHandle(pHandle);
		return -1;
	}
	*pSize = ((int64_t)dwSizeHigh << 32) | dwSizeLow;
	/* Obtain the view */
	pView = MapViewOfFile(pMapHandle, FILE_MAP_READ, 0, 0, (SIZE_T)(*pSize));
	if (pView) {
		/* Let the upper layer point to the view */
		*ppMap = pView;
	}
	/* Close the handle
	* According to MSDN it's OK the close the HANDLES.
	*/
	CloseHandle(pMapHandle);
	CloseHandle(pHandle);
	return pView ? SOD_OK : -1;
}
/* void (*xUnmap)(void *, size_t)  */
static void WinVfs_Unmap(void *pView, size_t nSize)
{
	nSize = 0; /* Compiler warning */
	UnmapViewOfFile(pView);
}
/* void (*xTempDir)(SyBlob *) */
static void WinVfs_TempDir(SyBlob *pCtx)
{
	CHAR zTemp[1024];
	DWORD n;
	n = GetTempPathA(sizeof(zTemp), zTemp);
	if (n < 1) {
		/* Assume the default windows temp directory */
		libcox_result_string(pCtx, "C:\\Windows\\Temp", -1/*Compute length automatically*/);
	}
	else {
		libcox_result_string(pCtx, zTemp, (int)n);
	}
}
/* void (*GetTicks)() */
static float WinVfs_GetTicks()
{
	static double freq = -1.0;
	LARGE_INTEGER lint;

	if (freq < 0.0) {
		if (!QueryPerformanceFrequency(&lint))
			return -1.0f;
		freq = lint.QuadPart;
	}
	if (!QueryPerformanceCounter(&lint))
		return -1.0f;
	return (float)(lint.QuadPart / freq);
}
/* An instance of the following structure is used to record state information
* while iterating throw directory entries.
*/
typedef struct WinDir_Info WinDir_Info;
struct WinDir_Info
{
	HANDLE pDirHandle;
	void *pPath;
	WIN32_FIND_DATAW sInfo;
	int rc;
};
/* int (*xOpenDir)(const char *, void **) */
static int WinDir_Open(const char *zPath, void **ppHandle)
{
	WinDir_Info *pDirInfo;
	void *pConverted;
	char *zPrep;
	size_t n;
	/* Prepare the path */
	n = strlen(zPath);
	zPrep = (char *)HeapAlloc(GetProcessHeap(), 0, n + sizeof("\\*") + 4);
	if (zPrep == 0) {
		return -1;
	}
	memcpy(zPrep, (const void *)zPath, n);
	zPrep[n] = '\\';
	zPrep[n + 1] = '*';
	zPrep[n + 2] = 0;
	pConverted = convertUtf8Filename(zPrep);
	HeapFree(GetProcessHeap(), 0, zPrep);
	if (pConverted == 0) {
		return -1;
	}
	/* Allocate a new instance */
	pDirInfo = (WinDir_Info *)HeapAlloc(GetProcessHeap(), 0, sizeof(WinDir_Info));
	if (pDirInfo == 0) {
		return -1;
	}
	pDirInfo->rc = SOD_OK;
	pDirInfo->pDirHandle = FindFirstFileW((LPCWSTR)pConverted, &pDirInfo->sInfo);
	if (pDirInfo->pDirHandle == INVALID_HANDLE_VALUE) {
		/* Cannot open directory */
		HeapFree(GetProcessHeap(), 0, pConverted);
		HeapFree(GetProcessHeap(), 0, pDirInfo);
		return -1;
	}
	/* Save the path */
	pDirInfo->pPath = pConverted;
	/* Save our structure */
	*ppHandle = pDirInfo;
	return SOD_OK;
}
/* void (*xCloseDir)(void *) */
static void WinDir_Close(void *pUserData)
{
	WinDir_Info *pDirInfo = (WinDir_Info *)pUserData;
	if (pDirInfo->pDirHandle != INVALID_HANDLE_VALUE) {
		FindClose(pDirInfo->pDirHandle);
	}
	HeapFree(GetProcessHeap(), 0, pDirInfo->pPath);
	HeapFree(GetProcessHeap(), 0, pDirInfo);
}
/* int (*xDirRead)(void *, SyBlob *) */
static int WinDir_Read(void *pUserData, SyBlob *pVal)
{
	WinDir_Info *pDirInfo = (WinDir_Info *)pUserData;
	LPWIN32_FIND_DATAW pData;
	char *zName;
	BOOL rc;
	size_t n;
	if (pDirInfo->rc != SOD_OK) {
		/* No more entry to process */
		return -1;
	}
	pData = &pDirInfo->sInfo;
	for (;;) {
		zName = unicodeToUtf8(pData->cFileName);
		if (zName == 0) {
			/* Out of memory */
			return -1;
		}
		n = strlen(zName);
		/* Ignore '.' && '..' */
		if (n > sizeof("..") - 1 || zName[0] != '.' || (n == sizeof("..") - 1 && zName[1] != '.')) {
			break;
		}
		HeapFree(GetProcessHeap(), 0, zName);
		rc = FindNextFileW(pDirInfo->pDirHandle, &pDirInfo->sInfo);
		if (!rc) {
			return -1;
		}
	}
	/* Return the current file name */
	libcox_result_string(pVal, zName, -1);
	HeapFree(GetProcessHeap(), 0, zName);
	/* Point to the next entry */
	rc = FindNextFileW(pDirInfo->pDirHandle, &pDirInfo->sInfo);
	if (!rc) {
		pDirInfo->rc = -1;
	}
	return SOD_OK;
}
/* void (*xRewindDir)(void *) */
static void WinDir_Rewind(void *pUserData)
{
	WinDir_Info *pDirInfo = (WinDir_Info *)pUserData;
	FindClose(pDirInfo->pDirHandle);
	pDirInfo->pDirHandle = FindFirstFileW((LPCWSTR)pDirInfo->pPath, &pDirInfo->sInfo);
	if (pDirInfo->pDirHandle == INVALID_HANDLE_VALUE) {
		pDirInfo->rc = -1;
	}
	else {
		pDirInfo->rc = SOD_OK;
	}
}
#ifndef MAX_PATH
#define MAX_PATH 260
#endif /* MAX_PATH */
/* Export the windows vfs */
static const sod_vfs sWinVfs = {
	"Windows_vfs",
	LIBCOX_VFS_VERSION,
	0,
	MAX_PATH,
	WinVfs_chdir,    /* int (*xChdir)(const char *) */
	WinVfs_getcwd,   /* int (*xGetcwd)(SyBlob *) */
	WinVfs_mkdir,    /* int (*xMkdir)(const char *, int, int) */
	WinVfs_rmdir,    /* int (*xRmdir)(const char *) */
	WinVfs_isdir,    /* int (*xIsdir)(const char *) */
	WinVfs_Rename,   /* int (*xRename)(const char *, const char *) */
	WinVfs_Realpath, /*int (*xRealpath)(const char *, SyBlob *)*/
   /* Directory */
	WinDir_Open,
	WinDir_Close,
	WinDir_Read,
	WinDir_Rewind,
	/* Systems function */
	WinVfs_unlink, /* int (*xUnlink)(const char *) */
	WinVfs_FileExists, /* int (*xFileExists)(const char *) */
	WinVfs_DiskFreeSpace, /* int64_t (*xFreeSpace)(const char *) */
	WinVfs_DiskTotalSpace, /* int64_t (*xTotalSpace)(const char *) */
	WinVfs_FileSize, /* int64_t (*xFileSize)(const char *) */
	WinVfs_isfile,     /* int (*xIsfile)(const char *) */
	WinVfs_isfile,     /* int (*xReadable)(const char *) */
	WinVfs_iswritable, /* int (*xWritable)(const char *) */
	WinVfs_isexecutable, /* int (*xExecutable)(const char *) */
	WinVfs_Getenv,     /* int (*xGetenv)(const char *, SyBlob *) */
	WinVfs_Setenv,     /* int (*xSetenv)(const char *, const char *) */
	WinVfs_Mmap,       /* int (*xMmap)(const char *, void **, size_t *) */
	WinVfs_Unmap,      /* void (*xUnmap)(void *, size_t);  */
	WinVfs_TempDir,    /* void (*xTempDir)(SyBlob *) */
	WinVfs_GetTicks  /* float (*xTicks)(void) */
};

#elif defined(__UNIXES__)
/*
* UNIX VFS implementation for the LIBCOX engine.
* Authors:
*    Symisc Systems, devel@symisc.net.
*    Copyright (C) Symisc Systems, http://libcox.symisc.net
* Status:
*    Stable.
*/
#include <sys/types.h>
#include <limits.h>
#ifdef SOD_ENABLE_NET_TRAIN
#include <sys/time.h>
#else
#include <time.h>
#endif /* SOD_ENABLE_NET_TRAIN */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <dirent.h>
#include <utime.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif /* PATH_MAX */

/* int (*xchdir)(const char *) */
static int UnixVfs_chdir(const char *zPath)
{
	int rc;
	rc = chdir(zPath);
	return rc == 0 ? SOD_OK : -1;
}
/* int (*xGetcwd)(SyBlob *) */
static int UnixVfs_getcwd(SyBlob *pCtx)
{
	char zBuf[4096];
	char *zDir;
	/* Get the current directory */
	zDir = getcwd(zBuf, sizeof(zBuf));
	if (zDir == 0) {
		return -1;
	}
	libcox_result_string(pCtx, zDir, -1/*Compute length automatically*/);
	return SOD_OK;
}
/* int (*xMkdir)(const char *, int, int) */
static int UnixVfs_mkdir(const char *zPath, int mode, int recursive)
{
	int rc;
	rc = mkdir(zPath, mode);
	recursive = 0; /* cc warning */
	return rc == 0 ? SOD_OK : -1;
}
/* int (*xIsdir)(const char *) */
static int UnixVfs_isdir(const char *zPath)
{
	struct stat st;
	int rc;
	rc = stat(zPath, &st);
	if (rc != 0) {
		return -1;
	}
	rc = S_ISDIR(st.st_mode);
	return rc ? 1 : 0;
}
/* int (*xUnlink)(const char *) */
static int UnixVfs_unlink(const char *zPath)
{
	int rc;
	rc = unlink(zPath);
	return rc == 0 ? SOD_OK : -1;
}
/* int (*xFileExists)(const char *) */
static int UnixVfs_FileExists(const char *zPath)
{
	int rc;
	rc = access(zPath, F_OK);
	return rc == 0 ? SOD_OK : -1;
}
/* int64_t (*xFileSize)(const char *) */
static int64_t UnixVfs_FileSize(const char *zPath)
{
	struct stat st;
	int rc;
	rc = stat(zPath, &st);
	if (rc != 0) {
		return -1;
	}
	return (int64_t)st.st_size;
}
/* int (*xIsfile)(const char *) */
static int UnixVfs_isfile(const char *zPath)
{
	struct stat st;
	int rc;
	rc = stat(zPath, &st);
	if (rc != 0) {
		return -1;
	}
	rc = S_ISREG(st.st_mode);
	return rc ? SOD_OK : -1;
}
/* int (*xReadable)(const char *) */
static int UnixVfs_isreadable(const char *zPath)
{
	int rc;
	rc = access(zPath, R_OK);
	return rc == 0 ? SOD_OK : -1;
}
/* int (*xWritable)(const char *) */
static int UnixVfs_iswritable(const char *zPath)
{
	int rc;
	rc = access(zPath, W_OK);
	return rc == 0 ? SOD_OK : -1;
}
/* int (*xExecutable)(const char *) */
static int UnixVfs_isexecutable(const char *zPath)
{
	int rc;
	rc = access(zPath, X_OK);
	return rc == 0 ? SOD_OK : -1;
}
/* int (*xMmap)(const char *, void **, size_t *) */
static int UnixVfs_Mmap(const char *zPath, void **ppMap, size_t *pSize)
{
	struct stat st;
	void *pMap;
	int fd;
	int rc;
	/* Open the file in a read-only mode */
	fd = open(zPath, O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	/* stat the handle */
	fstat(fd, &st);
	/* Obtain a memory view of the whole file */
	pMap = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	rc = SOD_OK;
	if (pMap == MAP_FAILED) {
		rc = -1;
	}
	else {
		/* Point to the memory view */
		*ppMap = pMap;
		*pSize = st.st_size;
	}
	close(fd);
	return rc;
}
/* void (*xUnmap)(void *, size_t)  */
static void UnixVfs_Unmap(void *pView, size_t nSize)
{
	munmap(pView, nSize);
}
/* int (*xOpenDir)(const char *, void **) */
static int UnixDir_Open(const char *zPath, void **ppHandle)
{
	DIR *pDir;
	/* Open the target directory */
	pDir = opendir(zPath);
	if (pDir == 0) {
		return -1;
	}
	/* Save our structure */
	*ppHandle = pDir;
	return SOD_OK;
}
/* void (*xCloseDir)(void *) */
static void UnixDir_Close(void *pUserData)
{
	closedir((DIR *)pUserData);
}
/* (*xReadDir)(void *, SyBlob *) */
static int UnixDir_Read(void *pUserData, SyBlob *pVal)
{
	DIR *pDir = (DIR *)pUserData;
	struct dirent *pEntry;
	char *zName = 0; /* cc warning */
	uint32_t n = 0;
	for (;;) {
		pEntry = readdir(pDir);
		if (pEntry == 0) {
			/* No more entries to process */
			return -1;
		}
		zName = pEntry->d_name;
		n = strlen(zName);
		/* Ignore '.' && '..' */
		if (n > sizeof("..") - 1 || zName[0] != '.' || (n == sizeof("..") - 1 && zName[1] != '.')) {
			break;
		}
		/* Next entry */
	}
	/* Return the current file name */
	libcox_result_string(pVal, zName, (int)n);
	return SOD_OK;
}
/* void (*xRewindDir)(void *) */
static void UnixDir_Rewind(void *pUserData)
{
	rewinddir((DIR *)pUserData);
}
/* float xTicks() */
static float UnixVfs_get_ticks()
{
	/* This is merely used for reporting training time between
	* epochs so we really don't care about precision here.
	*/
#ifdef SOD_ENABLE_NET_TRAIN
	struct timeval time;
	gettimeofday(&time, 0);
	return (float)time.tv_sec + 1000*time.tv_usec;
#else
	return (float)time(0);
#endif
}
/* Export the UNIX vfs */
static const sod_vfs sUnixVfs = {
	"Unix_vfs",
	LIBCOX_VFS_VERSION,
	0,
	PATH_MAX,
	UnixVfs_chdir,    /* int (*xChdir)(const char *) */
	UnixVfs_getcwd,   /* int (*xGetcwd)(SyBlob *) */
	UnixVfs_mkdir,    /* int (*xMkdir)(const char *, int, int) */
	0,    /* int (*xRmdir)(const char *) */
	UnixVfs_isdir,    /* int (*xIsdir)(const char *) */
	0,   /* int (*xRename)(const char *, const char *) */
	0, /*int (*xRealpath)(const char *, SyBlob *)*/
	/* Directory */
	UnixDir_Open,
	UnixDir_Close,
	UnixDir_Read,
	UnixDir_Rewind,
   /* Systems */
    UnixVfs_unlink,   /* int (*xUnlink)(const char *) */
    UnixVfs_FileExists, /* int (*xFileExists)(const char *) */
	0,             /* int64_t (*xFreeSpace)(const char *) */
	0,             /* int64_t (*xTotalSpace)(const char *) */
	UnixVfs_FileSize, /* int64_t (*xFileSize)(const char *) */
	UnixVfs_isfile,     /* int (*xIsfile)(const char *) */
	UnixVfs_isreadable, /* int (*xReadable)(const char *) */
	UnixVfs_iswritable, /* int (*xWritable)(const char *) */
	UnixVfs_isexecutable, /* int (*xExecutable)(const char *) */
	0,     /* int (*xGetenv)(const char *, SyBlob *) */
	0,     /* int (*xSetenv)(const char *, const char *) */
	UnixVfs_Mmap,       /* int (*xMmap)(const char *, void **, int64_t *) */
	UnixVfs_Unmap,      /* void (*xUnmap)(void *, int64_t);  */
	0,    /* void (*xTempDir)(SyBlob *) */
	UnixVfs_get_ticks   /* float (*xTicks)() */
};
#endif /* __WINNT__/__UNIXES__ */
/*
* Export the builtin vfs.
* Return a pointer to the builtin vfs if available.
* Otherwise return the null_vfs [i.e: a no-op vfs] instead.
* Note:
*  The built-in vfs is always available for Windows/UNIX systems.
* Note:
*  If the engine is compiled with the LIBCOX_DISABLE_DISK_IO/LIBCOX_DISABLE_BUILTIN_FUNC
*  directives defined then this function return the null_vfs instead.
*/

#define SySetBasePtr(S)           ((S)->pBase)
#define SySetBasePtrJump(S, OFFT)  (&((char *)(S)->pBase)[OFFT*(S)->eSize])
#define SySetUsed(S)              ((S)->nUsed)
#define SySetSize(S)              ((S)->nSize)
#define SySetElemSize(S)          ((S)->eSize)
#define SySetSetUserData(S, DATA)  ((S)->pUserData = DATA)
#define SySetGetUserData(S)       ((S)->pUserData)

static const sod_vfs * sodExportBuiltinVfs(void)
{
#ifdef __WINNT__
	return &sWinVfs;
#else
	/* Assume UNIX-Like */
	return &sUnixVfs;
#endif /* __WINNT__/__UNIXES__ */
}


sod_img sod_img_load_from_file(const char *zFile, int nChannels)
{
	const sod_vfs *pVfs = sodExportBuiltinVfs();
	unsigned char *data;
	void *pMap = 0;
	size_t sz = 0; /* gcc warn */
	int w, h, c;
	int i, j, k;
	if (SOD_OK != pVfs->xMmap(zFile, &pMap, &sz)) {
		data = stbi_load(zFile, &w, &h, &c, nChannels);
	}
	else {
		data = stbi_load_from_memory((const unsigned char *)pMap, (int)sz, &w, &h, &c, nChannels);
	}
	if (!data) {
		return sod_make_empty_image(0, 0, 0);
	}
	if (nChannels) c = nChannels;
	sod_img im = sod_make_image(w, h, c);
	if (im.data) {
		for (k = 0; k < c; ++k) {
			for (j = 0; j < h; ++j) {
				for (i = 0; i < w; ++i) {
					int dst_index = i + w * j + w * h*k;
					int src_index = k + c * i + c * w*j;
					im.data[dst_index] = (uint8_t)data[src_index];
				}
			}
		}
	}
	free(data);
	if (pMap) {
		pVfs->xUnmap(pMap, sz);
	}
	return im;
}

static int SySetInit(SySet *pSet, size_t ElemSize)
{
	pSet->nSize = 0;
	pSet->nUsed = 0;
	pSet->eSize = ElemSize;
	pSet->pBase = 0;
	pSet->pUserData = 0;
	return 0;
}
static int SySetPut(SySet *pSet, const void *pItem)
{
	unsigned char *zbase;
	if (pSet->nUsed >= pSet->nSize) {
		void *pNew;
		if (pSet->nSize < 1) {
			pSet->nSize = 8;
		}
		pNew = realloc(pSet->pBase, pSet->eSize * pSet->nSize * 2);
		if (pNew == 0) {
			return SOD_OUTOFMEM;
		}
		pSet->pBase = pNew;
		pSet->nSize <<= 1;
	}
	if (pItem) {
		zbase = (unsigned char *)pSet->pBase;
		memcpy((void *)&zbase[pSet->nUsed * pSet->eSize], pItem, pSet->eSize);
		pSet->nUsed++;
	}
	return SOD_OK;
}

/* Freeing functions */

void sod_free_image(sod_img m)
{
	if (m.data) {
		free(m.data);
	}
}

void sod_hough_lines_release(sod_pts * pLines)
{
	free(pLines);
}

/* Image creation functions */

sod_img sod_make_empty_image(int w, int h, int c)
{
	sod_img out;
	out.data = 0;
	out.h = h;
	out.w = w;
	out.c = c;
	return out;
}

sod_img sod_make_image(int w, int h, int c)
{
	sod_img out = sod_make_empty_image(w, h, c);
	out.data = (uint8_t*) calloc(h*w*c, sizeof(uint8_t));
	return out;
}

sod_img sod_copy_image(sod_img m)
{
	sod_img copy = m;
	copy.data = (uint8_t*) calloc(m.h*m.w*m.c, sizeof(uint8_t));
	if (copy.data && m.data) {
		memcpy(copy.data, m.data, m.h*m.w*m.c * sizeof(uint8_t));
	}
	return copy;
}

/* Drawing functions */

static inline void set_pixel(sod_img m, int x, int y, int c, uint8_t val)
{
	/* x, y, c are already validated by upper layers */
	if (m.data)
		m.data[c*m.h*m.w + y * m.w + x] = val;
}

void sod_image_draw_line(sod_img im, sod_pts start, sod_pts end, uint8_t r, uint8_t g, uint8_t b)
{
	int x1, x2, y1, y2, dx, dy, err, sx, sy, e2;
	if (im.c == 1) {
		/* Draw on grayscale image */
		r = (uint8_t) (b * 0.114 + g * 0.587 + r * 0.299);
	}
	x1 = start.x;
	x2 = end.x;
	y1 = start.y;
	y2 = end.y;
	if (x1 < 0) x1 = 0;
	if (x1 >= im.w) x1 = im.w - 1;
	if (x2 < 0) x2 = 0;
	if (x2 >= im.w) x2 = im.w - 1;

	if (y1 < 0) y1 = 0;
	if (y1 >= im.h) y1 = im.h - 1;
	if (y2 < 0) y2 = 0;
	if (y2 >= im.h) y2 = im.h - 1;

	dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
	dy = abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
	err = (dx > dy ? dx : -dy) / 2;

	for (;;) {
		set_pixel(im, x1, y1, 0, r);
		if (im.c == 3) {
			set_pixel(im, x1, y1, 1, g);
			set_pixel(im, x1, y1, 2, b);
		}
		if (x1 == x2 && y1 == y2) break;
		e2 = err;
		if (e2 > -dx) { err -= dy; x1 += sx; }
		if (e2 < dy) { err += dx; y1 += sy; }
	}
}

/* Gaussian noise reduce */
/* INPUT IMAGE MUST BE GRAYSCALE */

void sod_gaussian_noise_reduce(sod_img grayscale, sod_img out)
{
	int w, h, x, y, max_x, max_y;
	if (!grayscale.data || grayscale.c != SOD_IMG_GRAYSCALE) {
		return ;
	}
	w = grayscale.w;
	h = grayscale.h;
	img_out = sod_make_image(w, h, 1);
	if (img_out.data) {
		max_x = w - 2;
		max_y = w * (h - 2);
		for (y = w * 2; y < max_y; y += w) {
			for (x = 2; x < max_x; x++) {
				img_out.data[x + y] = (2 * grayscale.data[x + y - 2 - w - w] +
					4 * grayscale.data[x + y - 1 - w - w] +
					5 * grayscale.data[x + y - w - w] +
					4 * grayscale.data[x + y + 1 - w - w] +
					2 * grayscale.data[x + y + 2 - w - w] +
					4 * grayscale.data[x + y - 2 - w] +
					9 * grayscale.data[x + y - 1 - w] +
					12 * grayscale.data[x + y - w] +
					9 * grayscale.data[x + y + 1 - w] +
					4 * grayscale.data[x + y + 2 - w] +
					5 * grayscale.data[x + y - 2] +
					12 * grayscale.data[x + y - 1] +
					15 * grayscale.data[x + y] +
					12 * grayscale.data[x + y + 1] +
					5 * grayscale.data[x + y + 2] +
					4 * grayscale.data[x + y - 2 + w] +
					9 * grayscale.data[x + y - 1 + w] +
					12 * grayscale.data[x + y + w] +
					9 * grayscale.data[x + y + 1 + w] +
					4 * grayscale.data[x + y + 2 + w] +
					2 * grayscale.data[x + y - 2 + w + w] +
					4 * grayscale.data[x + y - 1 + w + w] +
					5 * grayscale.data[x + y + w + w] +
					4 * grayscale.data[x + y + 1 + w + w] +
					2 * grayscale.data[x + y + 2 + w + w]) / 159;
			}
		}
	}
	return;
}

// Sobel operator for edge detection

void sod_sobel_image(sod_img im, sod_img out)
{
	int weight[3][3] = { { -1,  0,  1 },
	{ -2,  0,  2 },
	{ -1,  0,  1 } };
	int pixel_value;
	int min, max;
	int x, y, i, j;  /* Loop variable */

	if (!im.data || im.c != SOD_IMG_GRAYSCALE) {
		/* Only grayscale images */
		return;
	}
	out = sod_make_image(im.w, im.h, im.c);
	if (!out.data) {
		return;
	}
	/* Maximum values calculation after filtering*/
	min = 4*255;
	max = -4*255;
	for (y = 1; y < im.h - 1; y++) {
		for (x = 1; x < im.w - 1; x++) {
			pixel_value = 0;
			for (j = -1; j <= 1; j++) {
				for (i = -1; i <= 1; i++) {
					pixel_value += weight[j + 1][i + 1] * im.data[(im.w * (y + j)) + x + i];
				}
			}
			if (pixel_value < min) min = pixel_value;
			if (pixel_value > max) max = pixel_value;
		}
	}
	if ((max - min) == 0) {
		return;
	}
	/* Generation of image2 after linear transformation */
	for (y = 1; y < out.h - 1; y++) {
		for (x = 1; x < out.w - 1; x++) {
			pixel_value = 0;
			for (j = -1; j <= 1; j++) {
				for (i = -1; i <= 1; i++) {
					pixel_value += weight[j + 1][i + 1] * im.data[(im.w * (y + j)) + x + i];
				}
			}
			out.data[out.w * y + x] = (uint8_t)((float) (pixel_value - min) / (max - min) * 255);
		}
	}
	return;
}




/* Sobel operator, needed for Canny edge detection */

static void canny_calc_gradient_sobel(sod_img * img_in, int *g, int *dir) {
	int w, h, x, y, max_x, max_y, g_x, g_y;
	float g_div;
	w = img_in->w;
	h = img_in->h;
	max_x = w - 3;
	max_y = w * (h - 3);
	for (y = w * 3; y < max_y; y += w) {
		for (x = 3; x < max_x; x++) {
			g_x = (int)((2 * img_in->data[x + y + 1]
				+ img_in->data[x + y - w + 1]
				+ img_in->data[x + y + w + 1]
				- 2 * img_in->data[x + y - 1]
				- img_in->data[x + y - w - 1]
				- img_in->data[x + y + w - 1]));
			g_y = (int)((2 * img_in->data[x + y - w]
				+ img_in->data[x + y - w + 1]
				+ img_in->data[x + y - w - 1]
				- 2 * img_in->data[x + y + w]
				- img_in->data[x + y + w + 1]
				- img_in->data[x + y + w - 1]));

			g[x + y] = sqrt(g_x * g_x + g_y * g_y);

			if (g_x == 0) {
				dir[x + y] = 2;
			}
			else {
				g_div = g_y / (float)g_x;
				if (g_div < 0) {
					if (g_div < -2.41421356237) {
						dir[x + y] = 0;
					}
					else {
						if (g_div < -0.414213562373) {
							dir[x + y] = 1;
						}
						else {
							dir[x + y] = 2;
						}
					}
				}
				else {
					if (g_div > 2.41421356237) {
						dir[x + y] = 0;
					}
					else {
						if (g_div > 0.414213562373) {
							dir[x + y] = 3;
						}
						else {
							dir[x + y] = 2;
						}
					}
				}
			}
		}
	}
}

/* Non-max suppression */

static void canny_non_max_suppression(sod_img * img, int *g, int *dir) {

	int w, h, x, y, max_x, max_y;
	w = img->w;
	h = img->h;
	max_x = w;
	max_y = w * h;
	for (y = 0; y < max_y; y += w) {
		for (x = 0; x < max_x; x++) {
			switch (dir[x + y]) {
			case 0:
				if (g[x + y] > g[x + y - w] && g[x + y] > g[x + y + w]) {
					if (g[x + y] > 255) {
						img->data[x + y] = 255.;
					}
					else {
						img->data[x + y] = (float)g[x + y];
					}
				}
				else {
					img->data[x + y] = 0;
				}
				break;
			case 1:
				if (g[x + y] > g[x + y - w - 1] && g[x + y] > g[x + y + w + 1]) {
					if (g[x + y] > 255) {
						img->data[x + y] = 255.;
					}
					else {
						img->data[x + y] = (float)g[x + y];
					}
				}
				else {
					img->data[x + y] = 0;
				}
				break;
			case 2:
				if (g[x + y] > g[x + y - 1] && g[x + y] > g[x + y + 1]) {
					if (g[x + y] > 255) {
						img->data[x + y] = 255.;
					}
					else {
						img->data[x + y] = (float)g[x + y];
					}
				}
				else {
					img->data[x + y] = 0;
				}
				break;
			case 3:
				if (g[x + y] > g[x + y - w + 1] && g[x + y] > g[x + y + w - 1]) {
					if (g[x + y] > 255) {
						img->data[x + y] = 255.;
					}
					else {
						img->data[x + y] = (float)g[x + y];
					}
				}
				else {
					img->data[x + y] = 0;
				}
				break;
			default:
				break;
			}
		}
	}
}

/* Canny estimate threshold, for Canny edge detection */

#define LOW_THRESHOLD_PERCENTAGE 0.8 /* percentage of the high threshold value that the low threshold shall be set at */
#define HIGH_THRESHOLD_PERCENTAGE 0.10 /* percentage of pixels that meet the high threshold - for example 0.15 will ensure that at least 15% of edge pixels are considered to meet the high threshold */

static void canny_estimate_threshold(sod_img * img, int * high, int * low) {

	int i, max, pixels, high_cutoff;
	int histogram[256];
	max = img->w * img->h;
	for (i = 0; i < 256; i++) {
		histogram[i] = 0;
	}
	for (i = 0; i < max; i++) {
		histogram[(int)img->data[i]]++;
	}
	pixels = (max - histogram[0]) * HIGH_THRESHOLD_PERCENTAGE;
	high_cutoff = 0;
	i = 255;
	while (high_cutoff < pixels) {
		high_cutoff += histogram[i];
		i--;
	}
	*high = i;
	i = 1;
	while (histogram[i] == 0) {
		i++;
	}
	*low = (*high + i) * LOW_THRESHOLD_PERCENTAGE;
}

/* Canny trace and canny range, needed for Canny hysteresis */

static int canny_range(sod_img * img, int x, int y)
{
	if ((x < 0) || (x >= img->w)) {
		return(0);
	}
	if ((y < 0) || (y >= img->h)) {
		return(0);
	}
	return(1);
}

static int canny_trace(int x, int y, int low, sod_img * img_in, sod_img * img_out)
{
	int y_off, x_off;
	if (img_out->data[y * img_out->w + x] == 0)
	{
		img_out->data[y * img_out->w + x] = 255;
		for (y_off = -1; y_off <= 1; y_off++)
		{
			for (x_off = -1; x_off <= 1; x_off++)
			{
				if (!(y == 0 && x_off == 0) && canny_range(img_in, x + x_off, y + y_off) && (int)(img_in->data[(y + y_off) * img_out->w + x + x_off]) >= low) {
					if (canny_trace(x + x_off, y + y_off, low, img_in, img_out))
					{
						return(1);
					}
				}
			}
		}
		return(1);
	}
	return(0);
}

/* Canny hysteresis, for Canny edge detection */

static void canny_hysteresis(int high, int low, sod_img * img_in, sod_img * img_out)
{
	int x, y, n, max;
	max = img_in->w * img_in->h;
	for (n = 0; n < max; n++) {
		img_out->data[n] = 0;
	}
	for (y = 0; y < img_out->h; y++) {
		for (x = 0; x < img_out->w; x++) {
			if ((int)(img_in->data[y * img_out->w + x]) >= high) {
				canny_trace(x, y, low, img_in, img_out);
			}
		}
	}
}

/* Canny edge detection */

sod_img sod_canny_edge_image(sod_img im, int reduce_noise)
{
	if (im.data && im.c == SOD_IMG_GRAYSCALE) {
		sod_img out, sobel, clean;
		int high, low, *g, *dir;
		if (reduce_noise) {
			clean = sod_gaussian_noise_reduce(im);
			if (!clean.data)return sod_make_empty_image(0, 0, 0);
		}
		else {
			clean = im;
		}
		sobel = sod_make_image(im.w, im.h, 1);
		out = sod_make_image(im.w, im.h, 1);
		g = (int*) malloc(im.w *(im.h + 16) * sizeof(int));
		dir = (int*) malloc(im.w *(im.h + 16) * sizeof(int));
		if (g && dir && sobel.data && out.data) {
			canny_calc_gradient_sobel(&clean, &g[im.w], &dir[im.w]);
			canny_non_max_suppression(&sobel, &g[im.w], &dir[im.w]);
			canny_estimate_threshold(&sobel, &high, &low);
			canny_hysteresis(high, low, &sobel, &out);
		}
		if (g)free(g);
		if (dir)free(dir);
		if (reduce_noise)sod_free_image(clean);
		sod_free_image(sobel);
		return out;
	}
	/* Make a grayscale version of your image using sod_grayscale_image() or sod_img_load_grayscale() first */
	return sod_make_empty_image(0, 0, 0);
}

/* Hough lines detect */

sod_pts * sod_hough_lines_detect(sod_img im, int threshold, int *nPts)
{
#define DEG2RAD 0.017453293f
	double center_x, center_y;
	unsigned int *accu;
	int accu_w, accu_h;
	int img_w, img_h;
	double hough_h;
	SySet aLines;
	sod_pts pts;
	int x, y;
	int r, t;

	if (!im.data || im.c != SOD_IMG_GRAYSCALE) {
		/* Require a binary image using sod_canny_edge_image() */
		*nPts = 0;
		return 0;
	}
	SySetInit(&aLines, sizeof(sod_pts));
	img_w = im.w;
	img_h = im.h;

	hough_h = ((sqrt(2.0) * (double)(im.h>im.w ? im.h : im.w)) / 2.0);
	accu_h = hough_h * 2.0; /* -r -> +r */
	accu_w = 180;

	accu = (unsigned int*)calloc(accu_h * accu_w, sizeof(unsigned int));
	if (accu == 0) {
		*nPts = 0;
		return 0;
	}
	center_x = im.w / 2;
	center_y = im.h / 2;
	for (y = 0; y < img_h; y++)
	{
		for (x = 0; x < img_w; x++)
		{
			if (im.data[y * img_w + x] > 250 /*> 250/255.*/)
			{
				for (t = 0; t < 180; t++)
				{
					double ra = (((double)x - center_x) * cos((double)t * DEG2RAD)) + (((double)y - center_y) * sin((double)t * DEG2RAD));
					accu[(int)((round(ra + hough_h) * 180.0)) + t]++;
				}
			}
		}
	}
	if (threshold < 1) threshold = im.w > im.h ? im.w / 3 : im.h / 3;
	for (r = 0; r < accu_h; r++)
	{
		for (t = 0; t < accu_w; t++)
		{
			if ((int)accu[(r*accu_w) + t] >= threshold)
			{
				int ly, lx;
				/* Is this point a local maxima (9x9) */
				int max = (int)accu[(r*accu_w) + t];
				for (ly = -4; ly <= 4; ly++)
				{
					for (lx = -4; lx <= 4; lx++)
					{
						if ((ly + r >= 0 && ly + r < accu_h) && (lx + t >= 0 && lx + t < accu_w))
						{
							if ((int)accu[((r + ly)*accu_w) + (t + lx)] > max)
							{
								max = (int)accu[((r + ly)*accu_w) + (t + lx)];
								ly = lx = 5;
							}
						}
					}
				}
				if (max >(int)accu[(r*accu_w) + t])
					continue;


				int x1, y1, x2, y2;
				x1 = y1 = x2 = y2 = 0;

				if (t >= 45 && t <= 135)
				{
					/*y = (r - x cos(t)) / sin(t)*/
					x1 = 0;
					y1 = ((double)(r - (accu_h / 2)) - ((x1 - (img_w / 2)) * cos(t * DEG2RAD))) / sin(t * DEG2RAD) + (img_h / 2);
					x2 = img_w - 0;
					y2 = ((double)(r - (accu_h / 2)) - ((x2 - (img_w / 2)) * cos(t * DEG2RAD))) / sin(t * DEG2RAD) + (img_h / 2);
				}
				else
				{
					/* x = (r - y sin(t)) / cos(t);*/
					y1 = 0;
					x1 = ((double)(r - (accu_h / 2)) - ((y1 - (img_h / 2)) * sin(t * DEG2RAD))) / cos(t * DEG2RAD) + (img_w / 2);
					y2 = img_h - 0;
					x2 = ((double)(r - (accu_h / 2)) - ((y2 - (img_h / 2)) * sin(t * DEG2RAD))) / cos(t * DEG2RAD) + (img_w / 2);
				}
				pts.x = x1; pts.y = y1;
				SySetPut(&aLines, &pts);
				pts.x = x2; pts.y = y2;
				SySetPut(&aLines, &pts);
			}
		}
	}
	free(accu);
	*nPts = (int)SySetUsed(&aLines);
	return (sod_pts *)SySetBasePtr(&aLines);
}

/* Connectivity detection, needed for Hilditch thinning function below */

static int hilditch_func_nc8(int *b)
{
	int n_odd[4] = { 1, 3, 5, 7 };  /* odd-number neighbors */
	int i, j, sum, d[10];           /* control variable */

	for (i = 0; i <= 9; i++) {
		j = i;
		if (i == 9) j = 1;
		if (abs(*(b + j)) == 1) {
			d[i] = 1;
		}
		else {
			d[i] = 0;
		}
	}
	sum = 0;
	for (i = 0; i < 4; i++) {
		j = n_odd[i];
		sum = sum + d[j] - d[j] * d[j + 1] * d[j + 2];
	}
	return sum;
}

/* Hilditch thinning */
/* Input MUST BE A BINARY IMAGE! */

sod_img sod_hilditch_thin_image(sod_img im)
{
	/* thinning of binary image via Hilditch's algorithm */
	int offset[9][2] = { { 0,0 },{ 1,0 },{ 1,-1 },{ 0,-1 },{ -1,-1 },
	{ -1,0 },{ -1,1 },{ 0,1 },{ 1,1 } }; /* offsets for neighbors */
	int n_odd[4] = { 1, 3, 5, 7 };      /* odd-number neighbors */
	int px, py;                         /* X/Y coordinates  */
	int b[9];                           /* gray levels for 9 neighbors */
	int condition[6];                   /* valid for conditions 1-6 */
	int counter;                        /* number of changing points  */
	int i, x, y, copy, sum;             /* control variable          */
	sod_img out;

	if (im.data == 0 || im.c != SOD_IMG_GRAYSCALE) {
		/* Must be a binary image (canny_edge, thresholding, etc..) */
		return sod_make_empty_image(0, 0, 0);
	}
	/* initialization of output */
	out = sod_copy_image(im);
	if (out.data == 0) {
		return sod_make_empty_image(0, 0, 0);
	}
	/* processing starts */
	do {
		counter = 0;
		for (y = 0; y < im.h; y++) {
			for (x = 0; x < im.w; x++) {
				/* substitution of 9-neighbor gray values */
				for (i = 0; i < 9; i++) {
					b[i] = 0;
					px = x + offset[i][0];
					py = y + offset[i][1];
					if (px >= 0 && px < im.w &&
						py >= 0 && py < im.h) {
						if (out.data[py * im.w + px] == 0) {
							b[i] = 1;
						}
						else if (out.data[py * im.w + px] == 2 /* Temp marker */) {
							b[i] = -1;
						}
					}
				}
				for (i = 0; i < 6; i++) {
					condition[i] = 0;
				}

				/* condition 1: figure point */
				if (b[0] == 1) condition[0] = 1;

				/* condition 2: boundary point */
				sum = 0;
				for (i = 0; i < 4; i++) {
					sum = sum + 1 - abs(b[n_odd[i]]);
				}
				if (sum >= 1) condition[1] = 1;

				/* condition 3: endpoint conservation */
				sum = 0;
				for (i = 1; i <= 8; i++) {
					sum = sum + abs(b[i]);
				}
				if (sum >= 2) condition[2] = 1;

				/* condition 4: isolated point conservation */
				sum = 0;
				for (i = 1; i <= 8; i++) {
					if (b[i] == 1) sum++;
				}
				if (sum >= 1) condition[3] = 1;

				/* condition 5: connectivity conservation */
				if (hilditch_func_nc8(b) == 1) condition[4] = 1;

				/* condition 6: one-side elimination for line-width of two */
				sum = 0;
				for (i = 1; i <= 8; i++) {
					if (b[i] != -1) {
						sum++;
					}
					else {
						copy = b[i];
						b[i] = 0;
						if (hilditch_func_nc8(b) == 1) sum++;
						b[i] = copy;
					}
				}
				if (sum == 8) condition[5] = 1;

				/* final decision */
				if (condition[0] && condition[1] && condition[2] &&
					condition[3] && condition[4] && condition[5]) {
					out.data[y * im.w + x] = 2; /* Temp */
					counter++;
				}
			} /* end of x */
		} /* end of y */

		if (counter != 0) {
			for (y = 0; y < im.h; y++) {
				for (x = 0; x < im.w; x++) {
					if (out.data[y * im.w + x] == 2) out.data[y *im.w + x] = 255;
				}
			}
		}
	} while (counter != 0);

	return out;
}