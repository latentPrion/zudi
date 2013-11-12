#ifndef _Z_UDI_INDEX_H
	#define _Z_UDI_INDEX_H

	#define UDI_VERSION	0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <stdint.h>

/**	EXPLANATION:
 * This header is contained in all UDI index files. The kernel uses it to
 * quickly gain information about the whole of the index at a glance. Versioning
 * of the struct layouts used is also included for forward expansion.
 **/
#define ZUDI_MESSAGE_MAXLEN		(150)
#define ZUDI_FILENAME_MAXLEN		(64)

namespace zudi
{
	struct headerS
	{
		// Version of the record format used in this index file.
		// "endianness" is a NULL-terminated string of either "le" or "be".
		char		endianness[4];
		uint16_t	majorVersion, minorVersion;
		uint32_t	nRecords, nextDriverId;
		uint8_t		reserved[64];
	};

	namespace device
	{
		struct headerS
		{
			uint32_t	driverId;
			uint16_t	index;
			uint16_t	messageIndex, metaIndex;
			uint8_t		nAttributes;
			// uint32_t	dataFileOffset;
		};

		enum attrTypeE {
			ATTR_STRING=0, ATTR_UBIT32, ATTR_BOOL, ATTR_ARRAY8 };

		#define ZUDI_DEVICE_MAX_NATTRS		(20)
		struct attrDataS
		{
			uint8_t		type, size;
			char		name[UDI_MAX_ATTR_NAMELEN];
			union
			{
				char	string[UDI_MAX_ATTR_SIZE];
				uint8_t	array8[UDI_MAX_ATTR_SIZE];
				uint32_t	u32val;
				uint8_t		boolval;
			} value;
		};

		struct deviceS
		{
			struct headerS		h;
			struct attrDataS	d[ZUDI_DEVICE_MAX_NATTRS];
		};
	}

	#define ZUDI_DRIVER_SHORTNAME_MAXLEN		(16)
	#define ZUDI_DRIVER_RELEASE_MAXLEN		(32)
	#define ZUDI_DRIVER_BASEPATH_MAXLEN		(128)
	namespace driver
	{
		enum typeE	{ DRIVERTYPE_DRIVER, DRIVERTYPE_METALANGUAGE };

		struct headerS
		{
			// TODO: Add support for custom attributes.
			uint32_t	id, type;
			uint16_t	nameIndex, supplierIndex, contactIndex,
			// Category index is only valid for meta libs, not drivers.
					categoryIndex;
			char		shortName[ZUDI_DRIVER_SHORTNAME_MAXLEN];
			char		releaseString[ZUDI_DRIVER_RELEASE_MAXLEN];
			char		releaseStringIndex;
			uint32_t	requiredUdiVersion;
			char		basePath[ZUDI_DRIVER_BASEPATH_MAXLEN];

			/* dataFileOffset is the offset within driver-data.zudi-index.
			 * rankFileOffset is the offset within ranks.zudi-index.
			 **/
			uint32_t	dataFileOffset, rankFileOffset,
					deviceFileOffset;
			uint8_t		nMetalanguages, nChildBops, nParentBops,
					nInternalBops,
					nModules, nRequirements,
					nMessages, nDisasterMessages,
					nMessageFiles, nReadableFiles, nRegions,
					nDevices, nRanks, nProvides;

			uint32_t	requirementsOffset, metalanguagesOffset,
					childBopsOffset, parentBopsOffset,
					internalBopsOffset, modulesOffset;
		};

		#define ZUDI_DRIVER_MAX_NREQUIREMENTS		(16)
		#define ZUDI_DRIVER_MAX_NMETALANGUAGES		(16)
		#define ZUDI_DRIVER_MAX_NCHILD_BOPS		(12)
		#define ZUDI_DRIVER_MAX_NPARENT_BOPS		(8)
		#define ZUDI_DRIVER_MAX_NINTERNAL_BOPS		(24)
		#define ZUDI_DRIVER_MAX_NMODULES		(16)

		#define ZUDI_DRIVER_METALANGUAGE_MAXLEN		(32)
		#define ZUDI_DRIVER_REQUIREMENT_MAXLEN		\
					(ZUDI_DRIVER_METALANGUAGE_MAXLEN)

		struct driverS
		{
			struct zudi::driver::headerS	h;
			struct requirementS
			{
				uint32_t	version;
				char		name[ZUDI_DRIVER_REQUIREMENT_MAXLEN];
			} requirements[ZUDI_DRIVER_MAX_NREQUIREMENTS];

			struct metalanguageS
			{
				uint16_t	index;
				char		name[ZUDI_DRIVER_METALANGUAGE_MAXLEN];
			} metalanguages[ZUDI_DRIVER_MAX_NMETALANGUAGES];

			struct childBopS
			{
				uint16_t	metaIndex, regionIndex, opsIndex;
			} childBops[ZUDI_DRIVER_MAX_NCHILD_BOPS];

			struct parentBopS
			{
				uint16_t	metaIndex, regionIndex, opsIndex,
						bindCbIndex;
			} parentBops[ZUDI_DRIVER_MAX_NPARENT_BOPS];

			struct internalBopS
			{
				uint16_t	metaIndex, regionIndex,
						opsIndex0, opsIndex1, bindCbIndex;
			} internalBops[ZUDI_DRIVER_MAX_NINTERNAL_BOPS];

			struct moduleS
			{
				uint16_t	index;
				char		fileName[ZUDI_FILENAME_MAXLEN];
			} modules[ZUDI_DRIVER_MAX_NMODULES];
		};
	}

	enum regionPrioE {
		REGION_PRIO_LOW=0, REGION_PRIO_MEDIUM, REGION_PRIO_HIGH };

	enum regionLatencyE {
		REGION_LAT_NON_CRITICAL=0, REGION_LAT_NON_OVER,
		REGION_LAT_RETRY, REGION_LAT_OVER,
		REGION_LAT_POWERFAIL_WARN };

	#define	ZUDI_REGION_FLAGS_FP		(1<<0)
	#define	ZUDI_REGION_FLAGS_DYNAMIC	(1<<1)
	#define	ZUDI_REGION_FLAGS_INTERRUPT	(1<<2)
	struct regionS
	{
		uint32_t	driverId;
		uint16_t	index, moduleIndex;
		uint8_t		priority;
		uint8_t		latency;
		uint32_t	flags;
	};

	struct messageS
	{
		uint32_t	driverId;
		uint16_t	index;
		char		message[ZUDI_MESSAGE_MAXLEN];
	};

	struct disasterMessageS
	{
		uint32_t	driverId;
		uint16_t	index;
		char		message[ZUDI_MESSAGE_MAXLEN];
	};

	struct messageFileS
	{
		uint32_t	driverId;
		uint16_t	index;
		char		fileName[ZUDI_FILENAME_MAXLEN];
	};

	struct readableFileS
	{
		uint16_t	driverId, index;
		char		fileName[ZUDI_FILENAME_MAXLEN];
	};

	#define ZUDI_PROVISION_NAME_MAXLEN	(ZUDI_DRIVER_METALANGUAGE_MAXLEN)
	struct provisionS
	{
		uint32_t	driverId;
		uint32_t	version;
		char		name[ZUDI_PROVISION_NAME_MAXLEN];
	};

	namespace rank
	{
		#define ZUDI_RANK_MAX_NATTRS		(ZUDI_DEVICE_MAX_NATTRS)
		#define ZUDI_RANK_ATTR_NAME_MAXLEN	(UDI_MAX_ATTR_NAMELEN)
		struct headerS
		{
			uint32_t	driverId;
			uint8_t		nAttributes, rank;
		};

		struct rankAttrS
		{
			char		name[ZUDI_RANK_ATTR_NAME_MAXLEN];
		};

		struct rankS
		{
			struct zudi::rank::headerS	h;
			struct zudi::rank::rankAttrS	d[ZUDI_RANK_MAX_NATTRS];
		};
	}
}
#endif

