/*
 * acq_d: Discrete comedi acquisition.  Uses commands and read().
 */

#define _GNU_SOURCE 1 // Includes c99 macros

#include <sys/time.h>
#include <stdbool.h>

#define site_str "Dartmouth"
#define current_code_version 0.01

#define ACQERR_BADINPUT 1	// User provided bad options
#define ACQERR_BADDEV 2		// Bad (sub?)device
#define ACQERR_BADCMD 3		// Error in generated comedi command
#define ACQERR_BADMEM 4		// Error in memory allocation
#define ACQERR_BADOUT 5

extern comedi_t *device;

struct parsed_options {
	char *devfile;
	char *monfile;
	char *outfile;

	unsigned short int subdevice;
	char *channels;
	char *ranges;

	double freq;
	unsigned long int samples;
	unsigned short int n_chan;
	double cadence;

	long long int sets;

	bool verbose;
	double value;
};

/* define the header structure to be stored with each chunk of data */
struct header_info {
	char site_id[12];
	int num_channels;
	char channel_flags;
	unsigned int num_samples;
	unsigned int num_read;
	float sample_frequency;
	float time_between_acquisitions;
	int byte_packing;
	time_t start_time;
	struct timeval start_timeval;
	float code_version;
};

extern char *cmd_src(int src,char *buf);
extern void dump_cmd(FILE *file,comedi_cmd *cmd);
extern void cmd_init_parsed_options(struct parsed_options *);
extern int cmd_parse_options(struct parsed_options *, int, char * []);

void strtary (int *, char *);
void do_depart(int signum);
int prepare_cmd_lib(comedi_t *dev,struct parsed_options options, comedi_cmd *cmd);

#ifndef COMMON_C
char *cmdtest_messages[]={
	"success",
	"invalid source",
	"source conflict",
	"invalid argument",
	"argument conflict",
	"invalid chanlist",
};
#endif

