#ifndef __FILEREADER_HPP__
#define __FILEHEADER_HPP__

#include "xmlreader.hpp"
#include "./bit_file/bitfile.hpp"
#include <vector>
using namespace std;

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

typedef struct data
{
	const PARA_entity* ref;
	u32 lenb;
	void * p;
	data() : ref(NULL), lenb(-1), p(NULL) {}
}data;

// content of a data file
typedef struct file_data
{
	vector<data> head;     // file header
	vector<data> content;  // file content
	data left;             // left data after read in file content
}file_data;

typedef class filereader
{
private:
	char fmt_file_name[MAX_PATH];
	char dat_file_name[MAX_PATH];
	xmlreader xfreader;
public:
	file_data data_file;
	filereader(const char *fmt_file, const char *dat_file);
	int parse_fmt_file();
	int parse_data_file();
	void summary();
	void dump_all_dat(const char *file);
	void dump_fmt_info(const char *file); 
	~filereader()
	{
		xfreader.cleanup();
	}
}filereader;

#endif

