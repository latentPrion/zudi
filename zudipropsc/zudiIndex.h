#ifndef _Z_UDI_INDEX_H
	#define _Z_UDI_INDEX_H

	#include <stdint.h>

/**	EXPLANATION:
 * This header is contained in all UDI index files. The kernel uses it to
 * quickly gain information about the whole of the index at a glance. Versioning
 * of the struct layouts used is also included for forward expansion.
 **/
#define ZUDI_MESSAGE_MAXLEN		(150)
#define ZUDI_FILENAME_MAXLEN		(64)

struct zudiIndexHeaderS
{
	// Version of the record format used in this index file.
	uint8_t		endianness;
	uint16_t	majorVersion, minorVersion;
	uint16_t	nRecords;
	uint8_t		reserved[128];
};

enum zudiIndexDeviceAttrTypeE {
	ZUDI_INDEX_DEVATTR_STRING=0, ZUDI_INDEX_DEVATTR_UBIT32,
	ZUDI_INDEX_DEVATTR_BOOL, ZUDI_INDEX_DEVATTR_ARRAY };

#define ZUDI_DEVICE_MAX_NATTRIBUTES		(16)
struct zudiIndexDeviceS
{
	uint16_t	id, driverId;
	uint16_t	deviceNameIndex, metaIndex;
	uint8_t		nAttributes;
	struct
	{
		enum zudiIndexDeviceAttrTypeE	type;
		char				name[32], value[64];
	} attributes[ZUDI_DEVICE_MAX_NATTRIBUTES];
};

#define ZUDI_DRIVER_MAX_NREQUIREMENTS		(10)
#define ZUDI_DRIVER_MAX_NMETALANGUAGES		(12)
#define ZUDI_DRIVER_MAX_NCHILD_BOPS		(12)
#define ZUDI_DRIVER_MAX_NPARENT_BOPS		(4)
#define ZUDI_DRIVER_MAX_NINTERNAL_BOPS		(16)
#define ZUDI_DRIVER_MAX_NMODULES		(4)

#define ZUDI_DRIVER_SHORTNAME_MAXLEN		(16)
#define ZUDI_DRIVER_RELEASE_MAXLEN		(16)
#define ZUDI_DRIVER_REQUIREMENT_MAXLEN		(16)
#define ZUDI_DRIVER_METALANGUAGE_MAXLEN		(16)

struct zudiIndexDriverS
{
	// TODO: Add support for custom attributes.
	uint16_t	id;
	uint32_t	nameIndex, supplierIndex, contactIndex;
	char		shortName[ZUDI_DRIVER_SHORTNAME_MAXLEN];
	char		releaseString[ZUDI_DRIVER_RELEASE_MAXLEN];
	char		releaseStringIndex;

	uint8_t		nMetalanguages, nChildBops, nParentBops, nInternalBops,
			nModules, nRequirements,
			nMessages, nDisasterMessages,
			nMessageFiles, nReadableFiles, nRegions;

	struct
	{
		uint32_t	version;
		char		name[ZUDI_DRIVER_REQUIREMENT_MAXLEN];
	} requirements[ZUDI_DRIVER_MAX_NREQUIREMENTS];

	struct
	{
		uint16_t	index;
		char		name[ZUDI_DRIVER_METALANGUAGE_MAXLEN];
	} metalanguages[ZUDI_DRIVER_MAX_NMETALANGUAGES];

	struct
	{
		uint16_t	metaIndex, regionIndex, opsIndex;
	} childBops[ZUDI_DRIVER_MAX_NCHILD_BOPS];

	struct
	{
		uint16_t	metaIndex, regionIndex, opsIndex, bindCbIndex;
	} parentBops[ZUDI_DRIVER_MAX_NPARENT_BOPS];

	struct
	{
		uint16_t	metaIndex, regionIndex,
				opsIndex0, opsIndex1, bindCbIndex;
	} internalBops[ZUDI_DRIVER_MAX_NINTERNAL_BOPS];

	struct
	{
		uint16_t	index;
		char		fileName[ZUDI_FILENAME_MAXLEN];
	} modules[ZUDI_DRIVER_MAX_NMODULES];
};

enum zudiIndexRegionPrioE	{
	ZUDI_INDEX_REGIONPRIO_LOW=0, ZUDI_INDEX_REGIONPRIO_MEDIUM,
	ZUDI_INDEX_REGIONPRIO_HIGH };

enum zudiIndexRegionLatencyE {
	ZUDI_INDEX_REGIONLAT_NON_CRITICAL=0, ZUDI_INDEX_REGIONLAT_NON_OVER,
	ZUDI_INDEX_REGIONLAT_RETRY, ZUDI_INDEX_REGIONLAT_OVER,
	ZUDI_INDEX_REGIONLAT_POWERFAIL_WARN };

#define	ZUDI_REGION_FLAGS_FP		(1<<0)
#define	ZUDI_REGION_FLAGS_DYNAMIC	(1<<1)
#define	ZUDI_REGION_FLAGS_INTERRUPT	(1<<1)
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
	char		message[ZUDI_MESSAGE_MAXLEN];
};

struct zudiIndexDisasterMessageS
{
	uint16_t	id, driverId;
	char		message[ZUDI_MESSAGE_MAXLEN];
};

struct zudiIndexMessageFileS
{
	uint16_t	driverId;
	char		fileName[ZUDI_FILENAME_MAXLEN];
};

struct zudiIndexReadableFileS
{
	uint16_t	id, driverId;
	char		fileName[ZUDI_FILENAME_MAXLEN];
};

#endif

