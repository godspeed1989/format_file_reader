#ifndef __XML_TYPE_HPP__
#define __XML_TYPE_HPP__

struct PARA_entity;

// range type
typedef enum range_t
{
	T_VALUE, T_RANGE, T_ANY, T_NULL, T_BOOL
}range_t;
typedef struct range
{
	range_t type;
	long low, high;
}range;

// length type
typedef struct length
{
	int lb;                 // a constant value
	const PARA_entity * le;  // depend on other's value
}length;

// PARA type
#define T_PARA          0
#define T_PARACHOICE    1
// PARA entity
typedef struct PARA_entity
{
	int depth;                   // depth of XML node
	bool type;                   // PARA or PARACHOICE
	const PARA_entity * refer;   // used by PARACHOICE
	struct attr
	{
		const xmlChar * name;    // name="" attr in PARA
		int type;                // type="" attr in PARA
		length len;              // length="" attr in PARA
		const xmlChar * depend;  // depend="" attr in PARA
		range rng;               // value=a~b attr in PARACHOICE
	}a;
}PARA_entity;

// ----- really exist PARA -----//
// length is a value
#define T_BIT_CASE          case 4 : case 17
#define T_BYTE_CASE         case 0 : case 9 : case 10 : case 11
// length is a value or a ref
#define T_BIT_REF_CASE      case 1
// length is a value or a ref
#define T_BYTE_REF_CASE     case 13

// ----- logic exist PARA, *must* followed a PARACHOICE -----//
// followed one without value=
#define T_BLK_CASE          case 2 : case 5 : case 6 : case 7
// without length=, followed one without value=
#define T_NULL_CASE         case 16 : case 101
// followed one with value=
#define T_COND_BLK_CASE     case 3

#endif

