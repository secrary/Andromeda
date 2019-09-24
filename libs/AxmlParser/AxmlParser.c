/* AXML Parser
 * https://github.com/claudxiao/AndTools
 * Claud Xiao <iClaudXiao@gmail.com>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32	/* Windows */

#pragma warning(disable:4996)

#ifndef snprintf
#define snprintf _snprintf
#endif

#endif /* _WIN32 */

#include "AxmlParser.h"

 /* chunks' magic numbers */
enum {
	CHUNK_HEAD = 0x00080003,

	CHUNK_STRING = 0x001c0001,
	CHUNK_RESOURCE = 0x00080180,

	CHUNK_STARTNS = 0x00100100,
	CHUNK_ENDNS = 0x00100101,
	CHUNK_STARTTAG = 0x00100102,
	CHUNK_ENDTAG = 0x00100103,
	CHUNK_TEXT = 0x00100104,
};

/* attributes' types */
enum {
	ATTR_NULL = 0,
	ATTR_REFERENCE = 1,
	ATTR_ATTRIBUTE = 2,
	ATTR_STRING = 3,
	ATTR_FLOAT = 4,
	ATTR_DIMENSION = 5,
	ATTR_FRACTION = 6,

	ATTR_FIRSTINT = 16,

	ATTR_DEC = 16,
	ATTR_HEX = 17,
	ATTR_BOOLEAN = 18,

	ATTR_FIRSTCOLOR = 28,
	ATTR_ARGB8 = 28,
	ATTR_RGB8 = 29,
	ATTR_ARGB4 = 30,
	ATTR_RGB4 = 31,
	ATTR_LASTCOLOR = 31,

	ATTR_LASTINT = 31,
};

/* string table */
typedef struct {
	uint32_t count;		/* count of all strings */
	uint32_t* offsets;	/* each string's offset in raw data block */

	unsigned char* data;	/* raw data block, contains all strings encoded by UTF-16LE */
	size_t len;		/* length of raw data block */

	unsigned char** strings;/* string table, point to strings encoded by UTF-8 */
} StringTable_t;

/* attribute structure within tag */
typedef struct {
	uint32_t uri;		/* uri of its namespace */
	uint32_t name;
	uint32_t string;	/* attribute value if type == ATTR_STRING */
	uint32_t type;		/* attribute type, == ATTR_* */
	uint32_t data;		/* attribute value, encoded on type */
} Attribute_t;

typedef struct AttrStack_t {
	Attribute_t* list;	/* attributes of current tag */
	uint32_t count;		/* count of these attributes */
	struct AttrStack_t* next;
} AttrStack_t;

/* namespace record */
typedef struct NsRecord {
	uint32_t prefix;
	uint32_t uri;
	struct NsRecord* next;	/* yes, it's a single linked list */
} NsRecord_t;

/* a parser, also a axml parser handle for user */
typedef struct {
	unsigned char* buf;	/* origin raw data, to be parsed */
	size_t size;		/* size of raw data */
	size_t cur;		/* current parsing position in raw data */

	StringTable_t* st;

	NsRecord_t* nsList;
	int nsNew;		/* if a new namespace coming */

	uint32_t tagName;	/* current tag's name */
	uint32_t tagUri;	/* current tag's namespace's uri */
	uint32_t text;		/* when tag is text, its content */

	AttrStack_t* attr;	/* attributes */
} Parser_t;

#define UTF8_FLAG (1 << 8)
unsigned char isUTF8 = 0;

/* get a 4-byte integer, and mark as parsed */
/* uses byte oprations to avoid little or big-endian conflict */
static uint32_t
GetInt32(Parser_t* ap)
{
	uint32_t value = 0;
	unsigned char* p = ap->buf + ap->cur;
	value = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
	ap->cur += 4;
	return value;
}

static void
CopyData(Parser_t* ap, unsigned char* to, size_t size)
{
	memcpy(to, ap->buf + ap->cur, size);
	ap->cur += size;
	return;
}

/* skip some uknown of useless fields, don't parse them */
static void
SkipInt32(Parser_t* ap, size_t num)
{
	ap->cur += 4 * num;
	return;
}

/* if no more byte need to be parsed */
static int
NoMoreData(Parser_t* ap)
{
	return ap->cur >= ap->size;
}

static int
ParseHeadChunk(Parser_t* ap)
{
	/* file magic */
	if (GetInt32(ap) != CHUNK_HEAD)
	{
		fprintf(stderr, "Error: not valid AXML file.\n");
		return -1;
	}

	/* file size */
	if (GetInt32(ap) != ap->size)
	{
		fprintf(stderr, "Error: not complete file.\n");
		return -1;
	}

	return 0;
}

static int
ParseStringChunk(Parser_t* ap)
{
	uint32_t chunkSize;

	uint32_t styleCount;
	uint32_t stringOffset;
	uint32_t styleOffset;

	uint32_t flags;

	size_t i;

	/* chunk type */
	if (GetInt32(ap) != CHUNK_STRING)
	{
		fprintf(stderr, "Error: not valid string chunk.\n");
		return -1;
	}

	/* chunk size */
	chunkSize = GetInt32(ap);

	/* count of strings */
	ap->st->count = GetInt32(ap);

	/* count of styles */
	styleCount = GetInt32(ap);

	/* flags field */
	flags = GetInt32(ap);
	isUTF8 = ((flags & UTF8_FLAG) != 0);

	/* offset of string raw data in chunk */
	stringOffset = GetInt32(ap);

	/* offset of style w=raw data in chunk */
	styleOffset = GetInt32(ap);

	/* strings' offsets table */
	ap->st->offsets = (uint32_t*)malloc(ap->st->count * sizeof(uint32_t));
	if (ap->st->offsets == NULL)
	{
		fprintf(stderr, "Error: init strings' offsets table.\n");
		return -1;
	}

	for (i = 0; i < ap->st->count; i++)
		ap->st->offsets[i] = GetInt32(ap);

	/* init string table */
	ap->st->strings = (unsigned char**)malloc(ap->st->count * sizeof(unsigned char*));
	if (ap->st->strings == NULL)
	{
		fprintf(stderr, "Error: init string table.\n");
		free(ap->st->offsets);
		ap->st->offsets = NULL;
		return -1;
	}
	for (i = 0; i < ap->st->count; i++)
		ap->st->strings[i] = NULL;

	/* skip style offset table */
	if (styleCount != 0)
		SkipInt32(ap, styleCount);

	/* save string raw data */
	ap->st->len = (styleOffset ? styleOffset : chunkSize) - stringOffset;
	ap->st->data = (unsigned char*)malloc(ap->st->len);
	if (ap->st->data == NULL)
	{
		fprintf(stderr, "Error: init string raw data.\n");
		free(ap->st->strings);
		ap->st->strings = NULL;
		free(ap->st->offsets);
		ap->st->offsets = NULL;
		return -1;
	}

	CopyData(ap, ap->st->data, ap->st->len);

	/* skip style raw data */
	if (styleOffset != 0)
		SkipInt32(ap, (chunkSize - styleOffset) / 4);

	return 0;
}

static int
ParseResourceChunk(Parser_t* ap)
{
	uint32_t chunkSize;

	/* chunk type */
	if (GetInt32(ap) != CHUNK_RESOURCE)
	{
		fprintf(stderr, "Error: not valid resource chunk.\n");
		return -1;
	}

	/* chunk size */
	chunkSize = GetInt32(ap);
	if (chunkSize % 4 != 0)
	{
		fprintf(stderr, "Error: not valid resource chunk.\n");
		return -1;
	}

	/* skip res id table */
	SkipInt32(ap, chunkSize / 4 - 2);

	return 0;
}

void*
AxmlOpen(char* buffer, size_t size)
{
	Parser_t* ap;

	if (buffer == NULL)
	{
		fprintf(stderr, "Error: AxmlOpen get an invalid parameter.\n");
		return NULL;
	}

	ap = (Parser_t*)malloc(sizeof(Parser_t));
	if (ap == NULL)
	{
		fprintf(stderr, "Error: init parser.\n");
		return NULL;
	}

	/* init parser */
	ap->buf = (unsigned char*)buffer;
	ap->size = size;
	ap->cur = 0;

	ap->nsList = NULL;
	ap->nsNew = 0;

	ap->attr = NULL;

	ap->tagName = (uint32_t)(-1);
	ap->tagUri = (uint32_t)(-1);
	ap->text = (uint32_t)(-1);

	ap->st = (StringTable_t*)malloc(sizeof(StringTable_t));
	if (ap->st == NULL)
	{
		fprintf(stderr, "Error: init string table struct.\n");
		free(ap);
		return NULL;
	}

	/* parse first three chunks */
	if (ParseHeadChunk(ap) != 0 ||
		ParseStringChunk(ap) != 0 ||
		ParseResourceChunk(ap) != 0)
	{
		free(ap->st);
		free(ap);
		return NULL;
	}

	return (void*)ap;
}

int
AxmlClose(void* axml)
{
	Parser_t* ap;
	uint32_t i;

	if (axml == NULL)
	{
		fprintf(stderr, "Error: AxmlClose get an invalid parameter.\n");
		return -1;
	}

	ap = (Parser_t*)axml;

	if (ap->st->data)
		free(ap->st->data);

	if (ap->st->strings)
	{
		for (i = 0; i < ap->st->count; i++)
			if (ap->st->strings[i])
				free(ap->st->strings[i]);
		free(ap->st->strings);
	}

	if (ap->st->offsets)
		free(ap->st->offsets);

	if (ap->st)
		free(ap->st);

	if (ap)
		free(ap);

	return 0;
}

AxmlEvent_t
AxmlNext(void* axml)
{
	static AxmlEvent_t event;

	Parser_t* ap;
	uint32_t chunkType;

	/* when init */
	if (event == AE_UNINITIALIZED)
	{
		event = AE_STARTDOC;
		return event;
	}

	ap = (Parser_t*)axml;

	/* when buffer ends */
	if (NoMoreData(ap))
		event = AE_ENDDOC;

	if (event == AE_ENDDOC)
		return event;

	/* common chunk head */
	chunkType = GetInt32(ap);
	SkipInt32(ap, 1);	/* chunk size, unused */
	SkipInt32(ap, 1);	/* line number, unused */
	SkipInt32(ap, 1);	/* unknown field */

	if (chunkType == CHUNK_STARTTAG)
	{
		uint32_t i;
		AttrStack_t* attr;

		attr = (AttrStack_t*)malloc(sizeof(AttrStack_t));
		if (attr == NULL)
		{
			fprintf(stderr, "Error: init attribute.\n");
			return AE_ERROR;
		}

		ap->tagUri = GetInt32(ap);
		ap->tagName = GetInt32(ap);
		SkipInt32(ap, 1);	/* flags, unknown usage */

		attr->count = GetInt32(ap) & 0x0000ffff;
		SkipInt32(ap, 1);	/* classAttribute, unknown usage */

		attr->list = (Attribute_t*)malloc(
			attr->count * sizeof(Attribute_t));
		if (attr->list == NULL)
		{
			fprintf(stderr, "Error: init attribute list.\n");
			free(attr);
			return AE_ERROR;
		}

		/* attribute list */
		for (i = 0; i < attr->count; i++)
		{
			attr->list[i].uri = GetInt32(ap);
			attr->list[i].name = GetInt32(ap);
			attr->list[i].string = GetInt32(ap);
			/* note: type must >> 24 */
			attr->list[i].type = GetInt32(ap) >> 24;
			attr->list[i].data = GetInt32(ap);
		}

		attr->next = ap->attr;
		ap->attr = attr;

		event = AE_STARTTAG;
	}
	else if (chunkType == CHUNK_ENDTAG)
	{
		AttrStack_t* attr;

		ap->tagUri = GetInt32(ap);
		ap->tagName = GetInt32(ap);

		if (ap->attr != NULL)
		{
			attr = ap->attr;
			ap->attr = ap->attr->next;

			free(attr->list);
			free(attr);
		}

		event = AE_ENDTAG;
	}
	else if (chunkType == CHUNK_STARTNS)
	{
		NsRecord_t* ns = (NsRecord_t*)malloc(sizeof(NsRecord_t));
		if (ns == NULL)
		{
			fprintf(stderr, "Error: init namespace.\n");
			return AE_ERROR;
		}

		ns->prefix = GetInt32(ap);
		ns->uri = GetInt32(ap);

		ns->next = ap->nsList;
		ap->nsList = ns;
		/* get a new namespace */
		ap->nsNew = 1;

		/* note this recursion rather than return a event*/
		return AxmlNext(ap);
	}
	else if (chunkType == CHUNK_ENDNS)
	{
		NsRecord_t* ns = ap->nsList;
		if (ns == NULL)
		{
			fprintf(stderr, "Error: end a namespace.\n");
			return AE_ERROR;
		}

		SkipInt32(ap, 1);	/* ended prefix */
		SkipInt32(ap, 1);	/* ended uri */

		ap->nsList = ns->next;
		free(ns);

		/* note this recursion rather than return a event*/
		return AxmlNext(ap);
	}
	else if (chunkType == CHUNK_TEXT)
	{
		ap->text = GetInt32(ap);
		SkipInt32(ap, 2);	/* unknown fields */
		event = AE_TEXT;
	}
	else
	{
		event = AE_ERROR;
	}

	return event;
}

/** \brief Convert UTF-16LE string into UTF-8 string
 *
 *  You must call this function with to=NULL firstly to get UTF-8 size;
 *  then you should alloc enough memory to the string;
 *  at last call this function again to convert actually.
 *  \param to Pointer to target UTF-8 string
 *  \param from Pointer to source UTF-16LE string
 *  \param nch Count of UTF-16LE characters, including terminal zero
 *  \retval -1 Converting error.
 *  \retval positive Bytes of UTF-8 string, including terminal zero.
 */
static size_t
UTF16LEtoUTF8(unsigned char* to, unsigned char* from, size_t nch)
{
	size_t total = 0;
	while (nch > 0)
	{
		uint32_t ucs4;
		size_t count;

		/* utf-16le -> ucs-4, defined in RFC 2781 */
		ucs4 = from[0] + (from[1] << 8);
		from += 2;
		nch--;
		if (ucs4 < 0xd800 || ucs4 > 0xdfff)
		{
			;
		}
		else if (ucs4 >= 0xd800 && ucs4 <= 0xdbff)
		{
			unsigned int ext;
			if (nch <= 0)
				return -1;
			ext = from[0] + (from[1] << 8);
			from += 2;
			nch--;
			if (ext < 0xdc00 || ext >0xdfff)
				return -1;
			ucs4 = ((ucs4 & 0x3ff) << 10) + (ext & 0x3ff) + 0x10000;
		}
		else
		{
			return -1;
		}

		/* ucs-4 -> utf-8, defined in RFC 2279 */
		if (ucs4 < 0x80) count = 1;
		else if (ucs4 < 0x800) count = 2;
		else if (ucs4 < 0x10000) count = 3;
		else if (ucs4 < 0x200000) count = 4;
		else if (ucs4 < 0x4000000) count = 5;
		else if (ucs4 < 0x80000000) count = 6;
		else return 0;

		total += count;
		if (to == NULL)
			continue;

		switch (count)
		{
		case 6: to[5] = 0x80 | (ucs4 & 0x3f); ucs4 >>= 6; ucs4 |= 0x4000000;
		case 5:	to[4] = 0x80 | (ucs4 & 0x3f); ucs4 >>= 6; ucs4 |= 0x200000;
		case 4: to[3] = 0x80 | (ucs4 & 0x3f); ucs4 >>= 6; ucs4 |= 0x10000;
		case 3: to[2] = 0x80 | (ucs4 & 0x3f); ucs4 >>= 6; ucs4 |= 0x800;
		case 2: to[1] = 0x80 | (ucs4 & 0x3f); ucs4 >>= 6; ucs4 |= 0xc0;
		case 1: to[0] = ucs4; break;
		}
		to += count;
	}
	if (to != NULL)
		to[0] = '\0';
	return total + 1;
}

static char*
GetString(Parser_t* ap, uint32_t id)
{
	static char* emptyString = "";

	unsigned char* offset;
	uint16_t chNum;
	size_t size;

	/* out of index range */
	if (id >= ap->st->count)
		return emptyString;

	/* already parsed, directly use previous result */
	if (ap->st->strings[id] != NULL)
		return (char*)(ap->st->strings[id]);

	/* point to string's raw data */
	offset = ap->st->data + ap->st->offsets[id];

	/* its first 2 bytes is string's characters count */
	if (isUTF8) {
		size = *(uint8_t*)offset;
		chNum = *(uint8_t*)(offset + 1);
		ap->st->strings[id] = (unsigned char*)malloc(chNum);
		memcpy(ap->st->strings[id], offset + 2, chNum);
		//		ap->st->strings[id][chNum] = 0;
	}
	else {
		chNum = *(uint16_t*)offset;
		size = UTF16LEtoUTF8(NULL, offset + 2, (size_t)chNum);
		if (size == (size_t)-1)
			return emptyString;
		ap->st->strings[id] = (unsigned char*)malloc(size);
		if (ap->st->strings[id] == NULL)
			return emptyString;

		UTF16LEtoUTF8(ap->st->strings[id], offset + 2, (size_t)chNum);
	}


	return (char*)(ap->st->strings[id]);
}

char*
AxmlGetTagName(void* axml)
{
	Parser_t* ap;
	ap = (Parser_t*)axml;
	return GetString(ap, ap->tagName);
}

char*
AxmlGetTagPrefix(void* axml)
{
	Parser_t* ap;
	NsRecord_t* ns;
	uint32_t nodePrefix = 0xffffffff;

	ap = (Parser_t*)axml;

	for (ns = ap->nsList; ns != NULL; ns = ns->next)
	{
		if (ns->uri == ap->tagUri)
			nodePrefix = ns->prefix;
	}

	/* mention that when index out of range, GetString() returns empty string */
	return GetString(ap, nodePrefix);
}

char*
AxmlGetText(void* axml)
{
	Parser_t* ap;
	ap = (Parser_t*)axml;
	return GetString(ap, ap->text);
}

uint32_t
AxmlGetAttrCount(void* axml)
{
	Parser_t* ap;
	ap = (Parser_t*)axml;
	return ap->attr->count;
}

char*
AxmlGetAttrPrefix(void* axml, uint32_t i)
{
	Parser_t* ap;
	NsRecord_t* ns;
	uint32_t prefix = 0xffffffff;
	uint32_t uri;

	ap = (Parser_t*)axml;
	uri = ap->attr->list[i].uri;

	for (ns = ap->nsList; ns != NULL; ns = ns->next)
	{
		if (ns->uri == uri)
			prefix = ns->prefix;
	}

	return GetString(ap, prefix);
}

char*
AxmlGetAttrName(void* axml, uint32_t i)
{
	Parser_t* ap;
	ap = (Parser_t*)axml;
	return GetString(ap, ap->attr->list[i].name);
}

char*
AxmlGetAttrValue(void* axml, uint32_t i)
{
	static float RadixTable[] = { 0.00390625f, 3.051758E-005f, 1.192093E-007f, 4.656613E-010f };
	static const char* DimemsionTable[] = { "px", "dip", "sp", "pt", "in", "mm", "", "" };
	static const char* FractionTable[] = { "%", "%p", "", "", "", "", "", "" };

	Parser_t* ap;
	uint32_t type;
	uint32_t data;
	char* buf;

	ap = (Parser_t*)axml;
	type = ap->attr->list[i].type;

	if (type == ATTR_STRING)
	{
		char* str;
		str = GetString(ap, ap->attr->list[i].string);

		/* free by user */
		buf = (char*)malloc(strlen(str) + 1);

		memset(buf, 0, strlen(str) + 1);
		strncpy(buf, str, strlen(str));

		return buf;
	}

	data = ap->attr->list[i].data;

	/* free by user */
	buf = (char*)malloc(32);
	memset(buf, 0, 32);

	if (type == ATTR_NULL)
	{
		;
	}
	else if (type == ATTR_REFERENCE)
	{
		if (data >> 24 == 1)
			snprintf(buf, 18, "@android:%08X", data);
		else
			snprintf(buf, 10, "@%08X", data);
	}
	else if (type == ATTR_ATTRIBUTE)
	{
		if (data >> 24 == 1)
			snprintf(buf, 18, "?android:%08x", data);
		else
			snprintf(buf, 10, "?%08X", data);
	}
	else if (type == ATTR_FLOAT)
	{
		snprintf(buf, 20, "%g", *(float*)& data);
	}
	else if (type == ATTR_DIMENSION)
	{
		snprintf(buf, 20, "%f%s",
			(float)(data & 0xffffff00) * RadixTable[(data >> 4) & 0x03],
			DimemsionTable[data & 0x0f]);
	}
	else if (type == ATTR_FRACTION)
	{
		snprintf(buf, 20, "%f%s",
			(float)(data & 0xffffff00) * RadixTable[(data >> 4) & 0x03],
			FractionTable[data & 0x0f]);
	}
	else if (type == ATTR_HEX)
	{
		snprintf(buf, 11, "0x%08x", data);
	}
	else if (type == ATTR_BOOLEAN)
	{
		if (data == 0)
			strncpy(buf, "false", 32);
		else
			strncpy(buf, "true", 32);
	}
	else if (type >= ATTR_FIRSTCOLOR && type <= ATTR_LASTCOLOR)
	{
		snprintf(buf, 10, "#%08x", data);
	}
	else if (type >= ATTR_FIRSTINT && type <= ATTR_LASTINT)
	{
		snprintf(buf, 32, "%d", data);
	}
	else
	{
		snprintf(buf, 32, "<0x%x, type 0x%02x>", data, type);
	}

	return buf;
}

int
AxmlNewNamespace(void* axml)
{
	Parser_t* ap;
	ap = (Parser_t*)axml;
	if (ap->nsNew == 0)
		return 0;
	else
	{
		ap->nsNew = 0;
		return 1;
	}
}

char*
AxmlGetNsPrefix(void* axml)
{
	Parser_t* ap;
	ap = (Parser_t*)axml;
	return GetString(ap, ap->nsList->prefix);
}

char*
AxmlGetNsUri(void* axml)
{
	Parser_t* ap;
	ap = (Parser_t*)axml;
	return GetString(ap, ap->nsList->uri);
}

typedef struct {
	char* data;
	size_t size;
	size_t cur;
} Buff_t;

static int
InitBuff(Buff_t* buf)
{
	if (buf == NULL)
		return -1;
	buf->size = 32 * 1024;
	buf->data = (char*)malloc(buf->size);
	if (buf->data == NULL)
	{
		fprintf(stderr, "Error: init buffer.\n");
		return -1;
	}
	buf->cur = 0;
	return 0;
}

static int
PrintToBuff(Buff_t* buf, size_t maxlen, const char* format, ...)
{
	va_list ap;
	size_t len;

	if (maxlen >= buf->size - buf->cur)
	{
		buf->size += 32 * 1024;
		buf->data = (char*)realloc(buf->data, buf->size);
		if (buf->data == NULL)
		{
			fprintf(stderr, "Error: realloc buffer.\n");
			return -1;
		}
	}

	va_start(ap, format);
	vsnprintf(buf->data + buf->cur, buf->size - buf->cur, format, ap);
	va_end(ap);

	len = strlen(buf->data + buf->cur);
	if (len > maxlen)
	{
		fprintf(stderr, "Error: length more than expected.\n");
		return -1;
	}
	buf->cur += len;
	return 0;
}

int
AxmlToXml(char** outbuf, size_t* outsize, char* inbuf, size_t insize)
{
	void* axml;
	AxmlEvent_t event;
	Buff_t buf;

	int tabCnt = 0;

	if (InitBuff(&buf) != 0)
		return -1;

	axml = AxmlOpen(inbuf, insize);
	if (axml == NULL)
		return -1;

	while ((event = AxmlNext(axml)) != AE_ENDDOC)
	{
		char* prefix;
		char* name;
		char* value;
		uint32_t i, n;

		switch (event) {
		case AE_STARTDOC:
			PrintToBuff(&buf, 50, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
			break;

		case AE_STARTTAG:
			PrintToBuff(&buf, tabCnt * 4 + 1, "%*s", tabCnt * 4, "");
			tabCnt++;

			prefix = AxmlGetTagPrefix(axml);
			name = AxmlGetTagName(axml);
			if (strlen(prefix) != 0)
				PrintToBuff(&buf, strlen(prefix) + strlen(name) + 5, "<%s:%s ", prefix, name);
			else
				PrintToBuff(&buf, strlen(name) + 3, "<%s ", name);

			if (AxmlNewNamespace(axml))
			{
				Parser_t* ap;
				ap = (Parser_t*)axml;
				for (NsRecord_t* ns = ap->nsList; ns != NULL; ns = ns->next)
				{
					prefix = GetString(ap, ns->prefix);
					name = GetString(ap, ns->uri);
					PrintToBuff(&buf, strlen(prefix) + strlen(name) + 12, "xmlns:%s=\"%s\" ", prefix, name);
				}
			}

			n = AxmlGetAttrCount(axml);
			for (i = 0; i < n; i++)
			{
				prefix = AxmlGetAttrPrefix(axml, i);
				name = AxmlGetAttrName(axml, i);
				value = AxmlGetAttrValue(axml, i);

				if (strlen(prefix) != 0)
					PrintToBuff(&buf, strlen(prefix) + strlen(name) + strlen(value) + 8,
						"%s:%s=\"%s\" ", prefix, name, value);
				else
					PrintToBuff(&buf, strlen(name) + strlen(value) + 6, "%s=\"%s\" ", name, value);

				/* must manually free attribute value here */
				free(value);
			}

			PrintToBuff(&buf, 3, ">\n");
			break;

		case AE_ENDTAG:
			--tabCnt;
			PrintToBuff(&buf, tabCnt * 4 + 1, "%*s", tabCnt * 4, "");

			prefix = AxmlGetTagPrefix(axml);
			name = AxmlGetTagName(axml);
			if (strlen(prefix) != 0)
				PrintToBuff(&buf, strlen(prefix) + strlen(name) + 7, "</%s:%s>\n", prefix, name);
			else
				PrintToBuff(&buf, strlen(name) + 5, "</%s>\n", name);
			break;

		case AE_TEXT:
			name = AxmlGetText(axml);
			PrintToBuff(&buf, strlen(name) + 2, "%s\n", name);
			break;

		case AE_ERROR:
			fprintf(stderr, "Error: AxmlNext() returns a AE_ERROR event.\n");
			AxmlClose(axml);
			return -1;
			break;

		default:
			break;
		}
	}

	AxmlClose(axml);

	(*outbuf) = buf.data;
	(*outsize) = buf.cur;

	return 0;
}