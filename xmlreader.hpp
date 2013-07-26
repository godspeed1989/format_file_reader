#ifndef __XML_READER_H__
#define __XML_READER_H__

#include <vector>
#include <libxml2/libxml/xmlreader.h>
using namespace std;

#define MLEN         128

// range type
typedef enum range_t
{
	T_VALUE, T_RANGE, T_ANY, T_NULL
}range_t;
typedef struct range
{
	range_t type;
	long low, high;
}range;

struct PARA_entity;
// PARA's length, be a constant value or depend on other's value
typedef struct length
{
	int lb;                 // length in bit
	const PARA_entity* le;  // dependent entity
}length;

// PARA type
typedef enum { T_PARA, T_PARACHOICE } PARA_entity_t;
// PARA entity
typedef struct PARA_entity
{
	int depth;
	PARA_entity_t type;
	const PARA_entity * refer;   // used by "PARACHOICE"
	struct attr
	{
		const xmlChar * name;    // name="" attr in PARA
		int type;                // type="" attr in PARA
		length len;              // length="" attr in PARA
		const xmlChar * depend;  // depend="" attr in PARA
		range rng;               // value=a~b attr in PARACHOICE
	}a;
}PARA_entity;

// structure of each log
typedef struct log_format
{
	range rng;               // <LOG value=$rng>
	vector<PARA_entity*> entities;
}log_format;
// structure of the data file
typedef struct file_format
{
	vector<PARA_entity*> file_head;
	vector<PARA_entity*> log_head;
	vector<log_format*> log_fmt;
}file_format;

typedef class xmlreader
{
public:
	file_format format_file;
	int processFile(const char* file);
	void printOut(FILE *fout);
	void cleanup();
	xmlreader()
	{
		LIBXML_TEST_VERSION
	}
	~xmlreader()
	{
		cleanup();
		// cleanup function for the XML library
		xmlCleanupParser();
	}
private:
	vector<PARA_entity*>* processing;
	void processNode(xmlTextReaderPtr reader);
}xmlreader;

/* the "type=" attribute cases */
// length is a value
#define T_BIT_CASE          case 4 : case 17
#define T_BYTE_CASE         case 0 : case 9 : case 10 : case 11
// length is a value or a ref
#define T_BIT_REF_CASE      case 1
// length is a value or a ref
#define T_BYTE_REF_CASE     case 13
// followed by a parachoice without value=
#define T_BLK_CASE          case 2 : case 5 : case 6 : case 7
// without length=, followed by a parachoice without value=
#define T_NULL_CASE         case 16 : case 101
// followed by a parachoice with value=
#define T_COND_BLK_CASE     case 3

#endif

