/* $Id: pipebench.c,v 1.6 2002/12/15 19:58:37 marvin Exp $
 *
 * Pipebench
 *
 * By Thomas Habets <thomas@habets.pp.se>
 *
 * Measures the speed of stdin/stdout communication.
 *
 * TODO:
 *    -  Variable update time  (now just updates once a second)
 */
/*
 *  Copyright (C) 2002 Thomas Habets <thomas@habets.pp.se>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

static float version = 0.10;

/*
 * Turn a 64 int into pseudo-SI units (1024 based). Two decimal places.
 *
 * In:  64 bit int, a storage buffer and size of it.
 * Out: buffer is changed and a pointer is returned to it.
 */
static char *unitify(u_int64_t _in, char *buf, int max)
{
	int e = 0;
	u_int64_t in;
	char *unit = "";
	char *units[] = {
		"",
		"k",
		"M",
		"G",
		"T",
		"P",
		"E",
	};
	in = _in;
	in *= 100;
	while (in > 102400UL) {
		e++;
		in>>=10;
	}
	/* Ha ha ha ha ha ha, oh my god... Yeah I wish I had the problem this
	 * part fixes.
	 */
	while (e && (e >= (sizeof(units)/sizeof(char*)))) {
		e--;
		in<<=10;
	}
	unit = units[e];
	snprintf(buf, max, "%7.2f %s",in/100.0,unit);
	return buf;
}

/*
 * Return a string representation of time differance.
 *
 * In:  Start and end time, a storage buffer and size of it.
 * Out: buffer is changed and a pointer is returned to it.
 */
static char *time_diff(struct timeval *start, struct timeval *end, char *buf,
		       int max)
{
	struct timeval diff;
	diff.tv_usec = end->tv_usec - start->tv_usec;
	diff.tv_sec = end->tv_sec - start->tv_sec;
	if (diff.tv_usec < 0) {
		diff.tv_usec += 1000000;
		diff.tv_sec--;
	}
	
	snprintf(buf,max,"%.2dh%.2dm%.2d.%.2ds",
		 diff.tv_sec / 3600,
		 (diff.tv_sec / 60) % 60,
		 diff.tv_sec % 60,
		 diff.tv_usec/10000);
	return buf;
}

/* spoon?
 *
 */
static void usage(void)
{
	printf("Pipebench %1.2f, by Thomas Habets <thomas@habets.pp.se>\n",
	       version);
	printf("usage: <cmd> | pipebench [ -ehqQ ] [ -b <bufsize ] | <cmd>\n");
}

/*
 * main
 */
int main(int argc, char **argv)
{
	int c;
	u_int64_t datalen = 0,last_datalen = 0,speed = 0;
	struct timeval start,tv,tv2;
	char tdbuf[32];
	char speedbuf[32];
	unsigned int bufsize = 819200;
	int summary = 1;
	int errout = 0;
	int quiet = 0;
	int fancy = 1;

	while (EOF != (c = getopt(argc, argv, "ehqQb:ro"))) {
		switch(c) {
		case 'e':
			errout = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'Q':
			quiet = 1;
			summary = 0;
			break;
		case 'o':
			summary = 0;
			break;
		case 'b':
			bufsize = atoi(optarg);
			break;
		case 'h':
			usage();
			return 0;
		case 'r':
			fancy = 0;
			summary = 0;
			break;
		default:
			usage();
			return 1;
		}
	}


	if ((-1 == gettimeofday(&tv, NULL))
	    || (-1 == gettimeofday(&start, NULL))) {
		perror("pipebench: gettimeofday()");
		if (errout) {
			return 1;
		}
	}

	while (!feof(stdin)) {
		int n;
		char buffer[819200];
		char ctimebuf[64];
		char datalenbuf[32];

		if (-1 == (n = fread(buffer, 1, sizeof(buffer), stdin))) {
			perror("pipebench: fread()");
			if (errout) {
				return 1;
			}
		}
		datalen += n;
		if (-1 == fwrite(buffer, n, 1, stdout)) {
			perror("pipebench: fwrite()");
			if (errout) {
				return 1;
			}
		}
		if (0) {
			fflush(stdout);
		}

		if (-1 == gettimeofday(&tv2,NULL)) {
			perror("pipebench(): gettimeofday()");
			if (errout) {
				return 1;
			}
		}
		strcpy(ctimebuf,ctime(&tv2.tv_sec));
		if ((n=strlen(ctimebuf)) && ctimebuf[n-1] == '\n') {
			ctimebuf[n-1] = 0;
		}
		if (fancy && !quiet) {
			fprintf(stderr, "%s: %sbytes %sB/second (%s)\r",
				time_diff(&start,&tv2,tdbuf,sizeof(tdbuf)),
				unitify(datalen,datalenbuf,sizeof(datalenbuf)),
				unitify(speed,speedbuf,sizeof(speedbuf)),
				ctimebuf);
		}
		if (tv.tv_sec != tv2.tv_sec) {
			speed = (datalen - last_datalen);
			last_datalen = datalen;
			if (-1 == gettimeofday(&tv,NULL)) {
				perror("pipebench(): gettimeofday()");
				if (errout) {
					return 1;
				}
			}
			if (!fancy) {
				fprintf(stderr, "%llu\n",speed);
			}
		}
	}
	
	if (summary) {
		float n;

		if (-1 == gettimeofday(&tv,NULL)) {
			perror("pipebench(): gettimeofday()");
			if (errout) {
				return 1;
			}
		}

		n = (tv2.tv_sec - start.tv_sec)
			+ (tv2.tv_usec - start.tv_usec) / 1000000.0;
		fprintf(stderr,"                                     "
			"            "
			"                              "
			"\r"
			"Summary:\nPiped %llu bytes in %s: %sB/second\n",
			datalen,time_diff(&start,&tv2,tdbuf,sizeof(tdbuf)),
			unitify(n?datalen/n:0,
				speedbuf,sizeof(speedbuf)));
	}
	return 0;
}
