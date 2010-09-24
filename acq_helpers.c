/*
 * This is a little helper function to parse options that
 * are common to most of the examples.
 */
#define COMMON_C

#include <stdio.h>
#include <comedilib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include "acq_helpers.h"

void cmd_init_parsed_options(struct parsed_options *options) {
	/* Command-line option initialization.  Defaults set here. */
	memset(options, 0, sizeof(struct parsed_options));
	options->devfile = "/dev/comedi0";
	options->monfile = "";
	options->outfile = "";

	options->subdevice = 0;
	options->channels = "1111";
	options->ranges = "1111";

	options->freq = 10000000.0;
	options->samples = 65536;
	options->n_chan = 4;
	options->cadence = 2.0;

	options->sets = -1;

	options->value = 0.;
}

/* Command-line option parsing */
int cmd_parse_options(struct parsed_options *options, int argc, char *argv[])
{
	int c;

	while (-1 != (c = getopt(argc, argv, "f:g:m:o:C:r:n:N:F:d:x:Vvh"))) {
		switch (c) {
		case 'f':
			options->devfile = optarg;
			break;
		case 'g':
			options->subdevice = strtoul(optarg, NULL, 0);
			break;
		case 'm':
			options->monfile = optarg;
			break;
		case 'o':
			options->outfile = optarg;
			break;
		case 'C':
			options->channels = optarg;
			break;
		case 'r':
			options->ranges = optarg;
			break;
		case 'n':
			options->n_chan = strtoul(optarg, NULL, 0);
			break;
		case 'N':
			options->samples = strtoul(optarg, NULL, 0);
			break;
		case 'F':
			options->freq = strtod(optarg, NULL);
			break;
		case 'd':
			options->cadence = strtod(optarg, NULL);
			break;
		case 'x':
			options->sets = strtoul(optarg, NULL, 0);
			break;
		case 'v':
			options->verbose = true;
			break;
		case 'V':
			printf("acq_d: Discrete Comedi Acquisition\n");
			printf("Version %3g.\n\n",current_code_version);
			exit(1);
		case 'h':
		default:
			printf("acq_d: Discrete Comedi Acquisition\n\n Options:\n");
			printf("\t-f <file>\tDevice File [default: /dev/comedi0]\n");
			printf("\t-g <#>\t\tSubdevice # [0]\n");
			printf("\t-m <file>\tMonitor File [none]\n");
			printf("\t-o <file>\tOutput File [none]\n");
			printf("\t-C <#>\t\tChannel mask [1111]\n");
			printf("\t-r <#>\t\tRange mask [1111]\n");
			printf("\t-n <#>\t\t# Channels [4]\n");
			printf("\t-N <#>\t\t# Samples per set [65536]\n");
			printf("\t-F <#>\t\tFrequency [10MHz]\n");
			printf("\t-d <#>\t\tCadence (Time Between Acqs) [2s]\n");
			printf("\t-x <#>\t\tNumber of times to execute [inf]\n");
			printf("\t-V\t\tPrint version\n");
			printf("\t-v\t\tBe Verbose\n");
			printf("\n");
			exit(1);
		}
	}
	if (optind < argc) {
		/* data value */
		options->value = strtod(argv[optind++], NULL);
	}

	return argc;
}

char *cmd_src(int src,char *buf)
{
	buf[0]=0;

	if(src&TRIG_NONE)strcat(buf,"none|");
	if(src&TRIG_NOW)strcat(buf,"now|");
	if(src&TRIG_FOLLOW)strcat(buf, "follow|");
	if(src&TRIG_TIME)strcat(buf, "time|");
	if(src&TRIG_TIMER)strcat(buf, "timer|");
	if(src&TRIG_COUNT)strcat(buf, "count|");
	if(src&TRIG_EXT)strcat(buf, "ext|");
	if(src&TRIG_INT)strcat(buf, "int|");
#ifdef TRIG_OTHER
	if(src&TRIG_OTHER)strcat(buf, "other|");
#endif

	if(strlen(buf)==0){
		sprintf(buf,"unknown(0x%08x)",src);
	}else{
		buf[strlen(buf)-1]=0;
	}

	return buf;
}

void dump_cmd(FILE *out,comedi_cmd *cmd)
{
	char buf[100];

	fprintf(out,"subdevice:      %d\n",
		cmd->subdev);

	fprintf(out,"start:      %-8s %d\n",
		cmd_src(cmd->start_src,buf),
		cmd->start_arg);

	fprintf(out,"scan_begin: %-8s %d\n",
		cmd_src(cmd->scan_begin_src,buf),
		cmd->scan_begin_arg);

	fprintf(out,"convert:    %-8s %d\n",
		cmd_src(cmd->convert_src,buf),
		cmd->convert_arg);

	fprintf(out,"scan_end:   %-8s %d\n",
		cmd_src(cmd->scan_end_src,buf),
		cmd->scan_end_arg);

	fprintf(out,"stop:       %-8s %d\n",
		cmd_src(cmd->stop_src,buf),
		cmd->stop_arg);
}
