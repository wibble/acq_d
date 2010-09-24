/*
 * acq_d: Discrete comedi acquisition.  Uses commands and read().
 */

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <comedilib.h>
#include <stdbool.h>

#include "acq_d.h"

//#define DMA_BUFFER_SIZE 4096
#define DMA_OVERSCAN_MULT 1
#define USLEEP_GRAN 10
#define AREF AREF_DIFF

static comedi_t *dev;
static int out;
static bool running = true;

void serial_toggle(void);

int main(int argc, char *argv[])
{
	comedi_cmd c,*cmd=&c;
	struct header_info header;
	long int t_samples, t_bytes, t_sleep;
	long int ret;
	int dt_usec, usec_elapsed;
//	struct timeval start,end;
	struct parsed_options options;
	unsigned short *samples; //, *junk;
	struct timeval now,then;
	long long unsigned int exec = 0;
	time_t t;

	umask(000);

	signal(SIGINT,do_depart);

	if (geteuid() != 0) {
		fprintf(stderr,"Must be setuid root to renice self.\n");
		exit(0);
	}

	cmd_init_parsed_options(&options);
	cmd_parse_options(&options, argc, argv);

	t_samples = options.n_chan * options.samples;
	t_bytes = t_samples * 2;
	t_sleep = (long int) DMA_OVERSCAN_MULT*(1e6/options.freq)*options.samples;
	printf("freq = %f, tsamp = %li, tby = %li, tsleep = %li.\n",options.freq,t_samples,t_bytes,t_sleep);


	/* Test for failful options */
	if (options.freq <= 1/(options.cadence + 0.00005)) {
		fprintf(stderr,"Acquisition frequency is faster than sampling frequency!  FAIL!\n");
		exit(ACQERR_BADINPUT);
	}

	if (options.samples > 16777216) {
		fprintf(stderr,"acq_d can't take more than 2^24 samples per set!  Try acq_c.\n");
		exit(ACQERR_BADINPUT);
	}

	ret = nice(-20);
	fprintf(stderr,"I've been niced to %li.\n",ret);
	if (ret != -20) printf("  WARNING!! Data collection could not set nice=-20. Data loss is probable at high speed.");

	dt_usec = options.cadence * 1e6;
	gettimeofday(&then, NULL);

	/* open the device */
	dev = comedi_open(options.devfile);
	if (!dev) {
		comedi_perror(options.devfile);
		exit(ACQERR_BADDEV);
	}

//	printf("%li samples in %i-byte buffer.\n\n",t_samples,comedi_get_buffer_size(dev,options.subdevice));

	prepare_cmd_lib(dev,options,cmd);

	printf("Testing command...");
	ret = comedi_command_test(dev, cmd);
	if (ret < 0) {
		comedi_perror("comedi_command_test");
		if(errno == EIO){
			fprintf(stderr,"Ummm... this subdevice doesn't support commands\n");
		}
		exit(ACQERR_BADCMD);
	}
	printf("%s...",cmdtest_messages[ret]);
	ret = comedi_command_test(dev, cmd);
	printf("%s...",cmdtest_messages[ret]);
	ret = comedi_command_test(dev, cmd);
	printf("%s...\n",cmdtest_messages[ret]);
	if (ret < 0) {
		fprintf(stderr,"Bad command, and unable to fix.\n");
		dump_cmd(stderr, cmd);
		exit(ACQERR_BADCMD);
	}
	dump_cmd(stdout,cmd);
	comedi_close(dev);

	//fprintf(stderr,"%i scan -- %i chan -- %li samples -- %li bytes\n",options.samples,options.n_chan,t_samples,t_bytes);

	/* Print data file header */
//	sprintf(dataheader,	"[system]\n"
//				"whoami = \"%s\"")

	samples = (unsigned short*) malloc(t_bytes);
//	junk = (unsigned short*) malloc(t_bytes);
	if ((samples == NULL)) { // | (junk == NULL)) {
		fprintf(stderr,"Error allocating sample memory!\n");
		exit(ACQERR_BADMEM);
	}

	printf("Starting acquisition...\n");
	/*Open the serial port*/
	/*serial = (FILE*)fopen("/dev/ttyS0","w");*/
	

	/* Do what we're here to do */
	while (running) {
		if (exec == options.sets) {
			break;
		}
		exec++;

       serial_toggle();

		/* open the device */
		dev = comedi_open(options.devfile);
		if (!dev) {
			comedi_perror(options.devfile);
			exit(ACQERR_BADDEV);
		}

		gettimeofday(&now, NULL);
		usec_elapsed = (now.tv_sec - then.tv_sec) * 1e6 + (now.tv_usec - then.tv_usec);
		while (usec_elapsed < dt_usec) {
			usleep(USLEEP_GRAN);
			gettimeofday(&now, NULL);
			usec_elapsed = (now.tv_sec - then.tv_sec) * 1e6 + (now.tv_usec - then.tv_usec);
		}
		/* start the command */
		ret = comedi_command(dev, cmd);
		if (ret < 0) {
			comedi_perror("comedi_command");
			exit(ACQERR_BADCMD);
		}

		then = now;

		/*
		 * Insert enough waiting to get through most of the sampling
		 */
		/*serial_toggle();*/
		usleep(t_sleep);

		/*
		 * Wait until the buffer has our data.
		 */

		int exsleeps = 0;
		do {
			usleep(USLEEP_GRAN);
			comedi_poll(dev,options.subdevice);
			ret = comedi_get_buffer_contents(dev,options.subdevice);
			exsleeps++;
		} while(ret < t_bytes);

//		printf("Waited %ius for additional data.\n",exsleeps);

		/*
		 * Get teh dataz.
		 */
		ret = read(comedi_fileno(dev), samples, t_bytes);
		if (ret < t_bytes) {
			fprintf(stderr,"Read mismatch!  Got %li of %li bytes (%li/%li samples).\n",ret,t_bytes,ret/2,t_samples);
		}
		header.num_read = ret/2;

		// collect & discard overscan
/*		ret = comedi_get_buffer_contents(dev,options.subdevice);
		if (realloc(junk,ret) == NULL) {
			fprintf(stderr,"Error in oversample realloc?\n");
			exit(ACQERR_BADMEM);
		}
		if (ret > 0) {
			ret = read(comedi_fileno(dev),junk,ret);
		}*/

		comedi_close(dev);

		// Prepare header
		t = time(NULL);
		sprintf(header.site_id,"%s",site_str);
		header.start_time = t;
		header.start_timeval = now;
		header.num_channels=options.n_chan;
		header.channel_flags=0x0F;
		header.num_samples=options.samples;
		header.sample_frequency=options.freq;
		header.time_between_acquisitions=options.cadence;
		header.byte_packing=0;
		header.code_version=current_code_version;

		// Save latest to monitor file
		if (strcmp(options.monfile,"") != 0) {
			out = open(options.monfile,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
			ret = errno;
			if (out > 0) {
				ret = write(out,&header,sizeof(header));
				ret = write(out,samples,2*header.num_read);
				close(out);
			}
			else {
				fprintf(stderr,"Unable to open monitor file %s: %s.\n",options.monfile,strerror(ret));
				exit(ACQERR_BADOUT);
			}
		}

		if (options.cadence >= 1.0 && options.verbose) printf("\tSaved latest to: %s\n",options.monfile);

		// Append to output file
		if (strcmp(options.outfile,"") != 0) {
			out = open(options.outfile,O_WRONLY|O_APPEND|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
			ret = errno;
			if (out > 0) {
				ret = write(out,samples,2*header.num_read);
				close(out);
				if (options.cadence >= 1.0 && options.verbose) printf("\tAppended to: %s\n",options.outfile);
			}
			else {
				fprintf(stderr,"Unable to open output file %s: %s.\n",options.outfile,strerror(ret));
				exit(ACQERR_BADOUT);
			}
		}
	}

	if (dev != NULL)
		comedi_close(dev);
	if (out != 0)
		close(out);

	printf("\nEnd.\n");

	exit(0);
}

/*
 * This prepares a command in a pretty generic way.  We ask the
 * library to create a stock command that supports periodic
 * sampling of data, then modify the parts we want. */
int prepare_cmd_lib(comedi_t *dev,struct parsed_options opt, comedi_cmd *cmd) {
	static unsigned int chanlist[256];
	int ranges[256];
	int ret;

	memset(cmd,0,sizeof(*cmd));

	/* This comedilib function will get us a generic timed
	 * command for a particular board.  If it returns -1,
	 * that's bad. */
	ret = comedi_get_cmd_generic_timed(dev,opt.subdevice, cmd,opt.n_chan,1e9/opt.freq);

	if(ret<0){
		printf("comedi_get_cmd_generic_timed failed\n");
		return ret;
	}

	/* Modify parts of the command */

	strtary(ranges,opt.ranges);
	if (opt.verbose) printf("Current range tokens: %i %i %i %i.\n",ranges[0],ranges[1],ranges[2],ranges[3]);

	for (unsigned short int i = 0; i < opt.n_chan; i++) {
		chanlist[i] = CR_PACK(i,ranges[i],AREF);
	}

	cmd->chanlist = chanlist;
	cmd->chanlist_len = opt.n_chan;
	cmd->stop_arg = opt.samples*DMA_OVERSCAN_MULT;

	return 0;
}

void strtary (int * array, char * string) {
  int i=0;

  while (*string)
    array[i++] = *(string++)-'0';
}

void do_depart(int signum) {
	printf("Exiting...");

	running = false;
}

void serial_toggle(void) {
    static int sfile = -1;
    static bool dtr = false;
    struct termios ComParams;
    int iFlags;

    if (sfile < 0) {
        sfile = open("/dev/ttyS0", O_RDWR|O_NONBLOCK);

        tcgetattr(sfile, &ComParams);
        ComParams.c_cflag &= ~CBAUD; // baud rate = 9600 bd
        ComParams.c_cflag |= B9600;
        tcsetattr(sfile, TCSANOW, &ComParams );
    }

    if (dtr) {
        iFlags = TIOCM_DTR;
        ioctl(sfile, TIOCMBIC, &iFlags);
        dtr = false;
    }
    else {
        iFlags = TIOCM_DTR;
        ioctl(sfile, TIOCMBIS, &iFlags);
        dtr = true;
    }
}
