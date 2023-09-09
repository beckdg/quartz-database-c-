# QuartzDB File Format Specification

## Overview

QuartzDB stores data in a single file with a `.qdb` extension. The file is divided into fixed-size pages (4096 bytes by default). The first few pages are reserved for system metadata, followed by user data pages.

## Binary Layout

```
+------------------+
| Database Header  |  Page 0 - 128 bytes at offset 0
+------------------+
| Superblock       |  Page 1 - 128 bytes at offset 4096
+------------------+
| Free List Root   |  Page 2 - Free page tracking
+------------------+
| Reserved Pages   |  Pages 3-7 (future system use)
+------------------+
| Data Pages       |  Pages 8+ (user data)
+------------------+
```

## Database Header (Page 0, offset 0, 128 bytes)

The database header occupies the first 128 bytes of the file. It is always located at the start of page 0.

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 4 | magic | Magic number `0x51444231` ("QDB1") |
| 4 | 4 | majorVersion | Format major version |
| 8 | 4 | minorVersion | Format minor version |
| 12 | 4 | pageSize | Page size in bytes (typically 4096) |
| 16 | 16 | databaseId | 128-bit UUID identifying this database |
| 32 | 8 | creationTimestamp | Unix timestamp of database creation |
| 40 | 8 | modificationTimestamp | Unix timestamp of last modification |
| 48 | 8 | featureFlags | Feature flag bitfield |
| 56 | 4 | superblockPageId | Page ID containing the superblock |
| 60 | 4 | headerChecksum | CRC-32 of bytes 0-119 (reserved) |
| 64 | 12 | reserved1[3] | Reserved for future header fields |
| 76 | 64 | reserved2[8] | Reserved expansion space |
| | **128** | **Total** | |

## Superblock (Page 1, offset 4096, 128 bytes)

The superblock resides at the start of page 1.

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 4 | magic | Must match `0x51444253` ("QDBS") |
| 4 | 16 | databaseId | Matches DatabaseHeader::databaseId |
| 20 | 8 | totalPages | Total pages in database |
| 28 | 8 | allocatedPages | Pages currently allocated |
| 36 | 8 | freePages | Pages currently free |
| 44 | 8 | reservedPages | Reserved system pages (typically 8) |
| 52 | 4 | firstFreePage | Page ID of first free page |
| 56 | 4 | lastAllocatedPage | Highest allocated page ID |
| 60 | 4 | freeListPage | Page ID of the free list root |
| 64 | 4 | metadataPage | Page ID of the metadata section |
| 68 | 8 | featureFlags | Feature flag bitfield |
| 76 | 4 | minReaderMajor | Minimum reader major version |
| 80 | 4 | minReaderMinor | Minimum reader minor version |
| 84 | 48 | reserved[7] | Reserved for future use |
| | **128** | **Total** | |

## Page Header (per-page, 64 bytes)

Every page (including system pages) begins with a 64-byte header.

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 4 | magic | Page magic number |
| 4 | 4 | version | Format version |
| 8 | 4 | pageId | Unique page identifier |
| 12 | 1 | pageType | Type of this page |
| 13 | 1 | flags | Generic flags (reserved) |
| 14 | 2 | payloadSize | Bytes of actual payload in this page |
| 16 | 2 | reserved1 | Reserved for future use |
| 18 | 4 | timestamp | Timestamp placeholder (future) |
| 22 | 4 | checksum | Checksum placeholder (future) |
| 26 | 8 | generation | Monotonically increasing generation |
| 34 | 28 | reserved2[7] | Future expansion |
| | **64** | **Total** | |

## Page Reference (20 bytes)

A lightweight reference to a page.

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 4 | pageId | Referenced page ID |
| 4 | 8 | generation | Page generation number |
| 12 | 1 | pageType | Page type identifier |
| 13 | 1 | flags | Reference flags |
| 14 | 2 | reserved1 | Reserved for future use |
| 16 | 4 | checksum | Checksum placeholder |
| | **20** | **Total** | |

## Object ID (16 bytes)

A 128-bit universally unique identifier. Generated as UUID version 4 (random) per RFC 4122.

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 8 | high | High 64 bits |
| 8 | 8 | low | Low 64 bits |
| | **16** | **Total** | |

## Feature Flags (64-bit bitfield)

| Bit | Flag | Description |
|-----|------|-------------|
| 0 | checksums | Page-level checksums enabled |
| 1 | compression | Page-level compression enabled |
| 2 | encryption | Page-level encryption enabled |
| 3 | journaling | Write-ahead logging enabled |
| 4 | largePages | Non-default page size |
| 5 | extendedIds | 64-bit page IDs |
| 6 | customPageSize | Custom page size configured |
| 7 | metadataRegion | Extended metadata region |
| 8 | userFlags | User-defined flags |
| 9-63 | reserved | Must be zero |

## Version Evolution Strategy

QuartzDB uses a major.minor.patch version scheme:

- **Major version** changes indicate breaking format changes. Readers with a different major version cannot read the file.
- **Minor version** changes indicate backward-compatible additions. Readers with a higher or equal minor version can read the file.
- **Patch version** changes indicate no format changes; only semantics or documentation.

Forward compatibility: A file written with version X.Y can be read by any reader with version X.Z where Z >= Y.

## Reserved Regions

Several regions in the header and superblock are reserved for future use. These fields must be zero in the current format version. Future versions may define meaning for these fields while maintaining backward compatibility.

## Magic Numbers

| Name | Value | ASCII |
|------|-------|-------|
| kDatabaseMagic | 0x51444231 | "QDB1" |
| kSuperblockMagic | 0x51444253 | "QDBS" |
| kPageMagic | 0x50475132 | "PGQ2" |
| kFreeListMagic | 0x464C5133 | "FLQ3" |
| kMetadataMagic | 0x4D445134 | "MDQ4" |
| kJournalMagic | 0x4A524E35 | "JRN5" |
| kFormatMagicV1 | 0x51444231 | "QDB1" |
| kFormatMagicV2 | 0x51444232 | "QDB2" |
