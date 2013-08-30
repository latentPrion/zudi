#ifndef _Z_UDIPROPS_COMPILER_H
	#define _Z_UDIPROPS_COMPILER_H

	#include <stdint.h>
	#include <stdio.h>
	#include <stdlib.h>

enum programModeE { KERNEL, USER };
extern enum programModeE	programMode;

inline static void printAndExit(char *progname, const char *msg, int errcode)
{
	printf("%s: %s.\n", progname, msg);
	exit(errcode);
}

/**	EXPLANATION:
 * This header is contained in all UDI index files. The kernel uses it to
 * quickly gain information about the whole of the index at a glance. Versioning
 * of the struct layouts used is also included for forward expansion.
 **/
struct zudiIndexHeaderS
{
	// Version of the record format used in this index file.
	uint16_t	majorVersion, minorVersion;
	uint16_t	nRecords;
	uint8_t		reserved[128];
};

enum zudiIndexDeviceAttrTypeE {
	ZUDI_INDEX_DEVATTR_STRING=0, ZUDI_INDEX_DEVATTR_UBIT32,
	ZUDI_INDEX_DEVATTR_BOOL, ZUDI_INDEX_DEVATTR_ARRAY };

struct zudiIndexDeviceS
{
	uint16_t	id, driverId;
	uint16_t	deviceNameIndex, metaIndex;
	uint8_t		nAttributes;
	struct
	{
		enum zudiIndexDeviceAttrTypeE	type;
		char				name[32], value[64];
	} attributes[16];
};

struct zudiIndexDriverS
{
	// TODO: Add support for custom attributes.
	uint16_t	id;
	uint32_t	nameIndex, supplierIndex, contactIndex;
	char		name[16];
	uint32_t	releaseNo;
	char		releaseString[64];
	char		requires[10][16];

	uint8_t		nMetalanguages, nChildBindOps, nParentBindOps,
			nInternalBindOps, nModules, nReadableFiles, nRegions,
			nMessages, nDisasterMessages, nMessageFiles;

	struct
	{
		uint16_t	index;
		char		name[16];
	} metalanguages[12];

	struct
	{
		uint16_t	index, regionIndex, opsIndex;
	} childBindOps[12];

	struct
	{
		uint16_t	index, regionIndex, opsIndex, bindCbIndex;
	} parentBindOps[4];

	struct
	{
		uint16_t	index, regionIndex,
				opsIndex0, opsIndex1, bindCbIndex;
	} internalBindOps[16];

	struct
	{
		uint16_t	index;
		char		fileName[64];
	} modules[4];

	struct
	{
		char		fileName[64];
	} readableFiles[8];

};

enum zudiIndexRegionPrioE	{
	ZUDI_INDEX_REGIONPRIO_LOW=0, ZUDI_INDEX_REGIONPRIO_MEDIUM,
	ZUDI_INDEX_REGIONPRIO_HIGH };

enum zudiIndexRegionLatencyE {
	ZUDI_INDEX_REGIONLAT_NON_CRITICAL=0, ZUDI_INDEX_REGIONLAT_NON_OVER,
	ZUDI_INDEX_REGIONLAT_RETRY, ZUDI_INDEX_REGIONLAT_OVER,
	ZUDI_INDEX_REGIONLAT_POWERFAIL_WARN };

#define	ZUDI_REGIONINDEX_FLAGS_FP		(1<<0)
#define	ZUDI_REGIONINDEX_FLAGS_DYNAMIC		(1<<1)
#define	ZUDI_REGIONINDEX_FLAGS_INTERRUPT	(1<<1)
struct zudiIndexRegionS
{
	uint16_t	id, driverId, index, moduleIndex;
	enum zudiIndexRegionPrioE	priority;
	enum zudiIndexRegionLatencyE	latency;
	uint32_t	flags;
};

struct zudiIndexMessageS
{
	uint16_t	id, driverId;
	char		message[128];
};

struct zudiIndexDisasterMessageS
{
	uint16_t	id, driverId;
	char		message[128];
};

struct zudiIndexMessageFileS
{
	uint16_t	id, driverId;
	char		fileName[128];
};


#endif

