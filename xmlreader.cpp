#include "xmlreader.hpp"
#include <cstdio>
#include <cctype>
#include <cassert>
#include <cstring>

#define S_AREADESC    (const xmlChar*)"AREADESC"
#define S_FILEHEAD    (const xmlChar*)"FILEHEAD"
#define S_LOGHEAD     (const xmlChar*)"LOGHEAD"
#define S_LOGTYPE     (const xmlChar*)"LOGTYPE"
#define S_LOG         (const xmlChar*)"LOG"
#define S_PARA        (const xmlChar*)"PARA"
#define S_PARACHOICE  (const xmlChar*)"PARACHOICE"

#define S_NAME        (const xmlChar*)"name"
#define S_LENGTH      (const xmlChar*)"length"
#define S_TYPE        (const xmlChar*)"type"
#define S_DISP        (const xmlChar*)"disp"
#define S_VALUE       (const xmlChar*)"value"
#define S_DEPEND      (const xmlChar*)"depend"

#ifdef LIBXML_READER_ENABLED

#include "xmlreader_static.hpp"

/**
 * process one XML node
 */
void xmlreader::processNode(xmlTextReaderPtr reader)
{
	xmlChar *s;
	const xmlChar *node_name;
	PARA_entity entity;
	memset(&entity, 0, sizeof(PARA_entity));
	// XML node type
	int type = xmlTextReaderNodeType(reader);
	if(type != XML_READER_TYPE_ELEMENT) // use <node> not <node/>
	{
		return;
	}
	// XML node depth
	entity.depth = xmlTextReaderDepth(reader);
	// <node_name attr=""></node_name>
	node_name = xmlTextReaderConstName(reader);
	if(node_name == NULL)
	{
		return;
	}
	// process the XML node depend on its name
	if(xmlStrncasecmp(node_name, S_AREADESC, MLEN) == 0)// 0 <AREADESC>
	{
		printf("**Processing <%s> ...\n", S_AREADESC);
		assert(entity.depth == 0);
	}
	else if(xmlStrncasecmp(node_name, S_FILEHEAD, MLEN) == 0)// 1 <FILEHEAD>
	{
		printf("***Processing <%s> ...\n", S_FILEHEAD);
		assert(entity.depth == 1);
		processing = &format_file.file_head;
	}
	else if(xmlStrncasecmp(node_name, S_LOGHEAD, MLEN) == 0)// 1 <LOGHEAD>
	{
		printf("***Processing <%s> ...\n", S_LOGHEAD);
		assert(entity.depth == 1);
		processing = &format_file.log_head;
	}
	else if(xmlStrncasecmp(node_name, S_LOGTYPE, MLEN) == 0)// 1 <LOGTYPE>
	{
		printf("***Processing <%s> ...\n", S_LOGTYPE);
		assert(entity.depth == 1);
	}
	else if(xmlStrncasecmp(node_name, S_LOG, MLEN) == 0)// 2 <LOG type="" ...
	{
		printf("****Processing %s type=", S_LOG);
		assert(entity.depth == 2);
		assert(xmlTextReaderAttributeCount(reader) > 0);
		log_format *one_log_fmt = new log_format;
		// get "type=" attr to setup range
		s = xmlTextReaderGetAttribute(reader, S_TYPE);
		assert(xmlStrlen(s));
		resolve_range(one_log_fmt->rng, s);
		xmlFree(s);
		show_range(one_log_fmt->rng, stdout);
		// output describe infomation if exist
		s = xmlTextReaderGetAttribute(reader, S_DISP);
		if(s)
		{
			printf(" '%s'", s);
			xmlFree(s);
		}
		printf("\n");
		format_file.log_fmt.push_back(one_log_fmt);
		processing = &one_log_fmt->entities;
	}
	else if(xmlStrncasecmp(node_name, S_PARA, MLEN) == 0)// <PARA name="" ...
	{
		entity.type = T_PARA;
		assert(xmlTextReaderAttributeCount(reader) >= 3);

		// depend attribute
		s = xmlTextReaderGetAttribute(reader, S_DEPEND);
		if(xmlStrlen(s))
			entity.a.depend = s;

		// length attribute
		s = xmlTextReaderGetAttribute(reader, S_LENGTH);
		if(xmlStrlen(s) && isdigit(s[0]))
			entity.a.len.lb = atoi((const char*)s);
		xmlFree(s);

		// type attribute
		s = xmlTextReaderGetAttribute(reader, S_TYPE);
		assert(xmlStrlen(s));
		entity.a.type = atoi((const char*)s);
		xmlFree(s);
		switch(entity.a.type)
		{
			// really exist PARA
			T_BYTE_CASE:        entity.a.len.lb <<= 3;    break;
			T_BIT_CASE:                                   break;
			T_BYTE_REF_CASE:
				entity.a.len.lb <<= 3;
			T_BIT_REF_CASE:
				if(entity.a.len.lb == 0)
				{
					s = xmlTextReaderGetAttribute(reader, S_LENGTH);
					entity.a.len.le = get_ref_by_name(processing, s);
				}
				break;
			// logic exist PARA
			T_COND_BLK_CASE:    assert(entity.a.depend);  break;
			T_BLK_CASE: T_NULL_CASE:                      break;
			default:
				printf("unknown attr %d %s\n", entity.a.type, entity.a.name);
				throw;
		}

		// name attribute
		s = xmlTextReaderGetAttribute(reader, S_NAME);
		assert(xmlStrlen(s));
		entity.a.name = s;

		processing->push_back(dup_PARA_entity(&entity));
	}
	else if(xmlStrncasecmp(node_name, S_PARACHOICE, MLEN) == 0)// <PARACHOCE ...
	{
		entity.type = T_PARACHOICE;
		// backward find first PARA with depth less 1
		// this PARA *must* be a logic exist one
		vector<PARA_entity*>::reverse_iterator rit;
		for(rit=processing->rbegin(); rit!=processing->rend(); ++rit)
		{
			if((*rit)->depth == entity.depth-1 && (*rit)->type == T_PARA)
				break;
		}
		if(rit == processing->rend())
		{
			printf("can't find PARACHOICE dependency\n");
			throw;
		}
		// does exist value="a~b" attr?
		s = xmlTextReaderGetAttribute(reader, S_VALUE);
		if(xmlStrlen(s))
		{
			switch((*rit)->a.type)
			{
			T_COND_BLK_CASE:
				assert((*rit)->a.depend);
				entity.refer = get_ref_by_name(processing, (*rit)->a.depend);
				resolve_range(entity.a.rng, s);
				xmlFree(s);
				break;
			default:
				throw;
			}
		}
		else
		{
			switch((*rit)->a.type)
			{
			T_BLK_CASE: T_NULL_CASE:
				if((*rit)->a.depend)
				{
					entity.a.rng.type = T_BOOL;
					entity.refer = get_ref_by_name(processing, (*rit)->a.depend);
				}
				else
					entity.a.rng.type = T_NULL;
				break;
			default:
				throw;
			}
		}
		processing->push_back(dup_PARA_entity(&entity));
	}
	else// <UNKNOWN ...
	{
		printf("Unknow XML node name [%s]\n", node_name);
	}
}

/*
 * parse and read in XML format file
 */
int xmlreader::processFile(const char* file)
{
	int ret;
	xmlTextReaderPtr reader;
	reader = xmlReaderForFile(file, NULL, XML_PARSE_DTDATTR | XML_PARSE_RECOVER);
	if(reader != NULL)
	{
		processing = NULL;
		cleanup();
		ret = xmlTextReaderRead(reader);
		while (ret == 1) // process each node in XML file
		{
			processNode(reader);
			ret = xmlTextReaderRead(reader);
		}
		xmlFreeTextReader(reader);
		if(ret != 0)
		{
			fprintf(stderr, "Error in parse %s\n", file);
			return ret;
		}
		return 0;
	}
	else
	{
		fprintf(stderr, "Open %s error\n", file);
		return 1;
	}
}

// print out the XML file content
void xmlreader::printOut(FILE *fout)
{
	vector<PARA_entity*>::iterator it;
	vector<log_format*>::iterator lit;
	fprintf(fout, "======== Output read in to check ========\n");
	fprintf(fout, "-----<File head info(%zu)>-----\n", format_file.file_head.size());
	for(it = format_file.file_head.begin(); it != format_file.file_head.end(); ++it)
		show_PARA_entity(*it, fout);
	fprintf(fout, "-----<Log head info(%zu)>-----\n", format_file.log_head.size());
	for(it = format_file.log_head.begin(); it != format_file.log_head.end(); ++it)
		show_PARA_entity(*it, fout);
	fprintf(fout, "-----<Log type info(%zu)>-----\n", format_file.log_fmt.size());
	for(lit = format_file.log_fmt.begin(); lit != format_file.log_fmt.end(); ++lit)
		show_one_log_fmt(*lit, fout);
	fprintf(fout, "========= Finish output read in =========\n");
}

// clean up function
void xmlreader::cleanup()
{
	vector<PARA_entity*>::iterator it;
	vector<log_format*>::iterator lit;
	for(it = format_file.file_head.begin(); it != format_file.file_head.end(); ++it)
		free_PARA_entity(*it);
	for(it = format_file.log_head.begin(); it != format_file.log_head.end(); ++it)
		free_PARA_entity(*it);
	for(lit = format_file.log_fmt.begin(); lit != format_file.log_fmt.end(); ++lit)
	{
		for(it = (*lit)->entities.begin(); it != (*lit)->entities.end(); ++it)
			free_PARA_entity(*it);
		delete (*lit);
	}
	format_file.file_head.clear();
	format_file.log_head.clear();
	format_file.log_fmt.clear();
}

#endif

