/**
 * @file mpqapi.h
 *
 * Interface of functions for creating and editing MPQ files.
 */
#pragma once

#include <stdint.h>

namespace devilution {

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FILEHEADER {
	int signature;
	int headersize;
	int filesize;
	Uint16 version;
	Sint16 sectorsizeid;
	int hashoffset;
	int blockoffset;
	int hashcount;
	int blockcount;
	Uint8 pad[72];
} _FILEHEADER;

typedef struct _HASHENTRY {
	Uint32 hashcheck[2];
	Uint32 lcid;
	Uint32 block;
} _HASHENTRY;

typedef struct _BLOCKENTRY {
	Uint32 offset;
	Uint32 sizealloc;
	Uint32 sizefile;
	Uint32 flags;
} _BLOCKENTRY;

void mpqapi_remove_hash_entry(const char *pszName);
void mpqapi_remove_hash_entries(bool (*fnGetName)(DWORD, char *));
bool mpqapi_write_file(const char *pszName, const BYTE *pbData, DWORD dwLen);
void mpqapi_rename(char *pszOld, char *pszNew);
bool mpqapi_has_file(const char *pszName);
bool OpenMPQ(const char *pszArchive, DWORD dwChar);
bool mpqapi_flush_and_close(const char *pszArchive, bool bFree, DWORD dwChar);

#ifdef __cplusplus
}
#endif

}
