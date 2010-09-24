/*
 * acq_d: Discrete comedi acquisition.  Uses commands and read().
 */

#define _GNU_SOURCE 1

#include <stdbool.h>
#include "acq_d.h"

void cmd_init_parsed_options(struct parsed_options *);
int cmd_parse_options(struct parsed_options *, int, char * []);

int *verbose_flag, *version;
struct option long_options[] = {
	/* These options set a flag. */
/*	{"verbose", no_argument, &verbose_flag, 1},
	{"version", no_argument, &version, 1},*/
	/* These options don't set a flag.
	  We distinguish them by their indices. */
	{"devfile",   required_argument, 0, 'f'},
	{"monfile",   required_argument, 0, 'm'},
	{"outfile",   required_argument, 0, 'o'},
	{"subdev",    required_argument, 0, 'g'},
	{"channels",  required_argument, 0, 'C'},
	{"frequency", required_argument, 0, 'F'},
	{"freq",      required_argument, 0, 'F'},
	{"samples",   required_argument, 0, 'N'},
	{"n_chan",    required_argument, 0, 'n'},
	{"cadence",   required_argument, 0, 'd'},
	{"sets",      required_argument, 0, 'x'},
	{0, 0, 0, 0}
};
