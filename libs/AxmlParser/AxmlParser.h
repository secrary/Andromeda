/* AXML Parser
 * https://github.com/claudxiao/AndTools
 * Claud Xiao <iClaudXiao@gmail.com>
 */
#ifndef AXMLPARSER_H
#define AXMLPARSER_H

#include <stdint.h>

typedef enum {
	AE_UNINITIALIZED = -1,
	AE_STARTDOC,
	AE_ENDDOC,
	AE_STARTTAG,
	AE_ENDTAG,
	AE_TEXT,
	AE_ERROR,
} AxmlEvent_t;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

	void* AxmlOpen(char* buffer, size_t size);

	AxmlEvent_t AxmlNext(void* axml);

	char* AxmlGetTagPrefix(void* axml);
	char* AxmlGetTagName(void* axml);

	int AxmlNewNamespace(void* axml);
	char* AxmlGetNsPrefix(void* axml);
	char* AxmlGetNsUri(void* axml);

	uint32_t AxmlGetAttrCount(void* axml);
	char* AxmlGetAttrPrefix(void* axml, uint32_t i);
	char* AxmlGetAttrName(void* axml, uint32_t i);
	char* AxmlGetAttrValue(void* axml, uint32_t i);

	char* AxmlGetText(void* axml);

	int AxmlClose(void* axml);

	int AxmlToXml(char** outbuf, size_t* outsize, char* inbuf, size_t insize);

#ifdef __cplusplus
#if __cplusplus
};
#endif
#endif

#endif