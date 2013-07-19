#include "filereader.hpp"
#include "filereader_static.hpp"

filereader::filereader(const char *fmtfile, const char *datfile)
{
	strncpy(fmt_file_name, fmtfile, MAX_PATH);
	strncpy(dat_file_name, datfile, MAX_PATH);
}

// parse and read in XML format file
int filereader::parse_fmt_file()
{
	return xfreader.processFile(fmt_file_name);
}

// parse and read in data file
int filereader::parse_data_file()
{
	int ret;
	bitfile dfreader;
	if(dfreader.open(dat_file_name, READ))
	{
		printf("Open %s error\n", dat_file_name);
		return -1;
	}

	// read in data file header
	vector<PARA_entity*> &file_head_fmt = xfreader.format_file.file_head;
	ret = readin_entities(dfreader, file_head_fmt, data_file.head);
	printf("read in [%s] file head (%zu)\n", dat_file_name, data_file.head.size());
	if(ret < 0)
		return ret;

	// read in data file content
	vector<PARA_entity*> &file_content_fmt = xfreader.format_file.file_content;
	ret = readin_entities(dfreader, file_content_fmt, data_file.content);
	printf("read in [%s] file content (%zu)\n", dat_file_name, data_file.content.size());
	if(ret < 0)
		return ret;
	
	// read in left file contents
	if(!dfreader.eof())
	{
		data_file.left.ref = NULL;
		data_file.left.lenb = dfreader.capb - dfreader.ftellb();
		data_file.left.p = malloc((data_file.left.lenb >> 3) + 1);
		dfreader.readb(data_file.left.p, data_file.left.lenb);
		printf("read in left %d bits\n", data_file.left.lenb);
	}
	
	return ret;
}

// make a summay about parse result
void filereader::summary()
{
	printf("~~~summary~~~\n");
	printf("%s %s\n", fmt_file_name, dat_file_name);
	printf("head %zu\n", data_file.head.size());
	printf("content %zu\n", data_file.content.size());
	printf("~~~~~~~~~~~~~\n");
}

static void dump_dats(bitfile &ofile, const vector<data> dats)
{
	if(ofile.otype != WRITE)
		throw;
	for(size_t i = 0; i < dats.size(); ++i)
		ofile.writeb(dats[i].p, dats[i].lenb);
}

// dump all of log data to file
void filereader::dump_all_dat(const char *file)
{
	bitfile ofile;
	ofile.open(file, WRITE);

	dump_dats(ofile, data_file.head);
	dump_dats(ofile, data_file.content);
	ofile.writeb(data_file.left.p, data_file.left.lenb);

	ofile.write_out();
	printf("dumped all read in to data file [%s]\n", file);
	ofile.close();
}

// dump all format info read from XML file
void filereader::dump_fmt_info(const char *file)
{
	FILE *fout;
	fout = fopen(file, "w");
	if(fout == NULL)
	{
		printf("error in open %s to dump fmt info\n", file);
		return;
	}
	xfreader.printOut(fout);
	printf("dumped all fmt info to file [%s]\n", file);
	fclose(fout);
}

