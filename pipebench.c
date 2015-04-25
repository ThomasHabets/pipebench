/*
 *
 * Pipebench
 *
 * By Thomas Habets <thomas@habets.se>
 *
 * Measures the speed of stdin/stdout communication.
 *
 * TODO:
 *    -  Variable update time  (now just updates once a second)
 */
/*
 *  Copyright (C) 2002 Thomas Habets <thomas@habets.se>
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
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

#ifdef sun
#define u_int8_t uint8_t
#define u_int16_t uint16_t
#define u_int32_t uint32_t
#define u_int64_t uint64_t
#endif

static float version = 0.40;

static volatile int done = 0;

static void sigint(int n)
{
	done = 1;
}

/*
 * Turn a 64 int into SI or pseudo-SI units ("nunit" based).
 * Two decimal places.
 *
 * In:  64 bit int, a storage buffer, size of buffer, and unit base
 *      unit base squared must fit in unsigned long.
 *
 * Out: buffer is changed and a pointer is returned to it.
 *
 * NOTE: Bits are lost if
 *       1) Input > (2^64 / 100)
 *    *and*
 *       2) nunit < 100       (which it should never be)
 */
static char *unitify(u_int64_t _in, char *buf, int max, unsigned long nunit,
		     int dounit)
{
	int e = 0;
	u_int64_t in;
	double inf;
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
	int fra = 0;

	inf = in = _in;
	if (dounit) {
		if (in > (nunit*nunit)) {
			e++;
			in/=nunit;
		}
		in *= 100;
		while (in > (100*nunit)) {
			e++;
			in/=nunit;
		}
		/* Ha ha ha ha ha ha, oh my god... Yeah I wish I had the
		 * problem this part fixes.
		 */
		while (e && (e >= (sizeof(units)/sizeof(char*)))) {
		e--;
		in*=nunit;
		}
		unit = units[e];
		inf = in / 100.0;
		fra = 2;
	}
	snprintf(buf, max, "%7.*f %s",fra,inf,unit);
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
	buf[max-1] = 0;
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
	printf("Pipebench %1.2f, by Thomas Habets <thomas@habets.se>\n",
	       version);
	printf("usage: ... | pipebench [ -ehqQIoru ] [ -b <bufsize ] "
	       "[ -s <file> | -S <file> ]\\\n           | ...\n");
}

/*
 * main
 */
int main(int argc, char **argv)
{
	int c;
	u_int64_t datalen = 0,last_datalen = 0,speed = 0;
	struct timeval start,tv,tv2;
	char tdbuf[64];
	char speedbuf[64];
	char datalenbuf[64];
	unsigned int bufsize = 819200;
	int summary = 1;
	int errout = 0;
	int quiet = 0;
	int fancy = 1;
	int dounit = 1;
	FILE *statusf;
	int statusf_append = 0;
	const char *statusfn = 0;
	int unit = 1024;
	char *buffer;

	statusf = stderr;

	while (EOF != (c = getopt(argc, argv, "ehqQb:ros:S:Iu"))) {
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
		case 's':
			statusfn = optarg;
			statusf_append = 0;
			break;
		case 'S':
			statusfn = optarg;
			statusf_append = 1;
			break;
		case 'I':
			unit = 1000;
			break;
		case 'u':
			dounit = 0;
			break;
		default:
			usage();
			return 1;
		}
	}

	if (statusfn) {
		if (!(statusf = fopen(statusfn, statusf_append?"a":"w"))) {
			perror("pipebench: fopen(status file)");
			if (errout) {
				return 1;
			}
		}
	}

	if ((-1 == gettimeofday(&tv, NULL))
	    || (-1 == gettimeofday(&start, NULL))) {
		perror("pipebench: gettimeofday()");
		if (errout) {
			return 1;
		}
	}

	if ((SIG_ERR == signal(SIGINT, sigint))) {
		perror("pipebench: signal()");
		if (errout) {
			return 1;
		}
	}
	
	while (!(buffer = malloc(bufsize))) {
		perror("pipebench: malloc()");
		bufsize>>=1;
	}

	while (!feof(stdin) && !done) {
		int n;
		char ctimebuf[64];

		if (-1 == (n = fread(buffer, 1, bufsize, stdin))) {
			perror("pipebench: fread()");
			if (errout) {
				return 1;
			}
			continue;
		}
		datalen += n;
		while (-1 == fwrite(buffer, n, 1, stdout)) {
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
			fprintf(statusf, "%s: %sB %sB/second (%s)%c",
				time_diff(&start,&tv2,tdbuf,sizeof(tdbuf)),
				unitify(datalen,datalenbuf,sizeof(datalenbuf),
					unit,dounit),
				unitify(speed,speedbuf,sizeof(speedbuf),
					unit,dounit),
				ctimebuf,
				statusfn?'\n':'\r');
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
				fprintf(statusf, "%llu\n",speed);
			}
		}
	}
	free(buffer);
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
		fprintf(statusf,"                                     "
			"            "
			"                              "
			"%c"
			"Summary:\nPiped %sB in %s: %sB/second\n",
			statusfn?'\n':'\r',
			unitify(datalen,datalenbuf,sizeof(datalenbuf),
				unit,dounit),
			time_diff(&start,&tv2,tdbuf,sizeof(tdbuf)),
			unitify(n?datalen/n:0,
				speedbuf,sizeof(speedbuf),unit,dounit));
	}
	return 0;
}
