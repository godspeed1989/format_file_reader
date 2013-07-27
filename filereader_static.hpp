#ifndef __FILEREADER_STATIC_HPP__
#define __FILEHEADER_STATIC_HPP__

// check wether a value fix into the range
static bool range_equal(const range &rng, const long value)
{
	if(rng.type == T_ANY)
		return true;
	if(rng.type == T_VALUE)
		return (rng.low == value);
	if(rng.type == T_RANGE)
		return (value >= rng.low && value <= rng.high);
	return false;
}

#if 0
static void dump_data(const data& d)
{
	u32 i;
	printf("\tdebug: %s %p %u = ", d.ref->name, d.p, d.lenb);
	for(i = 0; i < (d.lenb>>3) && i < 10; ++i)
		printf("%02x ", *((u8*)d.p+i));
	if(i < (d.lenb>>3))
		printf("...");
	printf("\n");
}
#endif

static const data& get_data_by_name(const vector<data> &data_set, const xmlChar* name)
{
	size_t i;
	static const data d;
	for(i = 0; i < data_set.size(); ++i)
	{
		if(xmlStrncasecmp(data_set[i].ref->a.name, name, MLEN) == 0)
			break;
	}
	if(i == data_set.size())
	{
		printf("warning: can't find data of [%s]\n", name);
		return d;
	}
	return data_set[i];
}

// get a value by its name from a data set
static const void* get_valuep_by_name(const vector<data> &data_set, const xmlChar* name)
{
	return get_data_by_name(data_set, name).p;
}

// get data length by its name from a data set
static int get_lenB_by_name(const vector<data> &data_set, const xmlChar* name)
{
	u32 lenb = get_data_by_name(data_set, name).lenb;
	return  (lenb >> 3) + ((lenb & 7) != 0);
}

// get a data ref by its reference pointer
static const data& get_data_by_ref(const vector<data> &data_set, const PARA_entity* ref)
{
	size_t i;
	for(i = 0; i < data_set.size(); ++i)
	{
		if(data_set[i].ref == ref)
			break;
	}
	if(i == data_set.size())
	{
		printf("error: can't find [%s]\n", ref->a.name);
		throw;
	}
	return data_set[i];
}

// get a value by data ref
static int get_value_by_data(const data &dat)
{
	int lenB, value;
	lenB = (dat.lenb >> 3) + ((dat.lenb & 7) != 0);
	switch(lenB)
	{
		case 1:  value = *((char*)dat.p);	break;
		case 2:  value = *((short*)dat.p);	break;
		case 4:  value = *((int*)dat.p);	break;
		default:
			printf("%s: not suppported value length\n", dat.ref->a.name);
			throw;
	}
	return value;
}

// get a value by its reference pointer
static int get_value_by_ref(const vector<data> &data_set, const PARA_entity* ref)
{
	
	const data &dat = get_data_by_ref(data_set, ref);
	return get_value_by_data(dat);
}

// read in one entity to the container
static int readin_entity(bitfile &reader, const PARA_entity* e, vector<data> &container)
{
	data d;
	d.ref = e;
	// check the dependent parameter
	if(e->refer && (get_value_by_ref(container, e->refer) == 0))
	{
		return 0;
	}
	// calculate the length by type
	switch(e->a.type)
	{
		// really exist PARA
		T_BIT_CASE: T_BYTE_CASE:
			d.lenb = e->a.len.lb;
			break;
		T_BIT_REF_CASE:
			if(e->a.len.lb == 0)
				d.lenb = get_value_by_ref(container, e->a.len.le);
			else
				d.lenb = e->a.len.lb;
			break;
		T_BYTE_REF_CASE:
			if(e->a.len.lb == 0)
				d.lenb = get_value_by_ref(container, e->a.len.le) << 3;
			else
				d.lenb = e->a.len.lb;
			break;
		// logic exist PARA
		T_COND_BLK_CASE:
			return 0;
		T_BLK_CASE: T_NULL_CASE:
			return 0;
		default:
			printf("unkonw attr type %d of %s\n", e->a.type, e->a.name);
			return -1;
	}
	if(d.lenb == 0)
	{
		printf("warning: the length of %s is zero\n", e->a.name);
		return 0;
	}
	// read the data from file
	if(reader.eof())
	{
		printf("read in para entity [%s] reached EOF\n", e->a.name);
		return -1;
	}
	d.p = malloc((d.lenb >> 3) + 1);
	d.lenb = reader.readb(d.p, d.lenb);
#if 0
	dump_data(d);
#endif
	container.push_back(d);
	return 0;
}

// read in set of entites to a container
static int readin_entities(bitfile &reader, const vector<PARA_entity*> es, vector<data> &container)
{
	for(size_t i = 0; i < es.size(); ++i)
	{
		if(es[i]->type == T_PARA)
		{
			if(readin_entity(reader, es[i], container))
			{
				printf("read in para entity [%s] error\n", es[i]->a.name);
				return -1;
			}
		}
		else if(es[i]->type == T_PARACHOICE)
		{
			size_t j;
			vector<size_t> choices;
			choices.push_back(i);
			// find all the parallel PARACHOICE
			j = i + 1;
			while(j < es.size() && es[j]->depth >= es[i]->depth)
			{
				if(es[j]->type == T_PARACHOICE &&
				   es[j]->depth == es[i]->depth)
					choices.push_back(j);
			}
			// find the first matched choice
			for(j = 0; j < choices.size(); ++j)
			{
				if(es[j]->a.rng.type == T_NULL)
				{
					assert(choices.size() == 1);
					break;
				}
				else
				{
					long val;
					val = get_value_by_ref(container, es[i]->refer);
					if(es[j]->a.rng.type == T_BOOL)
						if(val)
							break;
					else
						if(range_equal(es[j]->a.rng, val))
							break;
				}
			}
			if(j >= choices.size())
			{
				printf("Warning: %d: no matched parachoice\n", j);
				// skip the following entities with the higher depth
				j = choices.back() + 1;
				while(j < es.size() && es[j]->depth > es[i]->depth)
					++j;
				i = j-1;
			}
			else
			{
				i = choices[j];
				// TODO  skip following not matched parachoice
			}	
		}
	}
	return 0;
}

#endif

