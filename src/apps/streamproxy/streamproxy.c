#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/dvb/dmx.h>
#include <stdarg.h>
#include <sys/stat.h>

#define MAX_PIDS 32
#define MAX_LINE_LENGTH 512

#define BSIZE                    188*88//1024*16

//#if defined(__sh__)
//#else
//#define HAVE_ADD_PID
//#endif

#define LOGOUTPUT

#ifdef HAVE_ADD_PID
#define DMX_ADD_PID              _IO('o', 51)
#define DMX_REMOVE_PID           _IO('o', 52)

typedef enum {
	DMX_TAP_TS = 0,
	DMX_TAP_PES = DMX_PES_OTHER, /* for backward binary compat. */
} dmx_tap_type_t;

#endif


char response_line[MAX_LINE_LENGTH];
int response_p;

int upstream;
int upstream_state, upstream_response_code;
		/*
		 0 - response
		 1 - options
		 2 - body
		 */
#ifdef HAVE_ADD_PID
int demux_fd = -1;
#else
int dvr_fd = -1;
int open_pids[MAX_PIDS];
#endif

char *reason = "";

int active_pids[MAX_PIDS];

int handle_upstream(void);
int handle_upstream_line(void);

char authorization[MAX_LINE_LENGTH]; /* the saved Authorization:-client-header which will be forwarded to the server */
char wwwauthenticate[MAX_LINE_LENGTH]; /* the saved WWW-Authenticate:-server-header, which will be forwarded to user client */

void logOutput(char *FormatStr, ...) {
#ifdef LOGOUTPUT
	char *cTimeFormat = "^D.^N.^YYY ^H:^M:^S:^C   "; /* Do not touch ! */
	va_list args;
	char lStr [200];
	int vStatus;
	struct stat vStatusBuffer;

	FILE * pFile;
	

	va_start (args, FormatStr);
	vsprintf (lStr, FormatStr, args);

	pFile = fopen ("/tmp/streamproxy.log","a");
	if (pFile!=NULL)
	{
		fputs (lStr,pFile);
		fclose (pFile);
	}

	va_end (args);
#endif
	return;
}

int main(int argc, char **argv)
{
	char request[MAX_LINE_LENGTH], upstream_request[256];
	char *c, *service_ref;

	logOutput("starting streamproxy\n");
#ifdef HAVE_ADD_PID
	logOutput("HAVE_ADD_PID\n");
#else
	logOutput("DONT HAVE_ADD_PID\n");
#endif
	logOutput("buffersize=%d\n", BSIZE);

	if (!fgets(request, MAX_LINE_LENGTH - 1, stdin))
		goto bad_request;

	if (strncmp(request, "GET /", 5)) {
		goto bad_request;
	}
	
	c = strchr(request + 5, ' ');
	if (!c || strncmp(c, " HTTP/1.", 7)) {
		goto bad_request;
	}
	
	*c++ = 0;
	
	c = request + 5;
	
	service_ref = c;
	
	int i;
	for (i=0; i<MAX_PIDS; ++i)
		active_pids[i] = -1;
	
	while (1)
	{
		char option[MAX_LINE_LENGTH];
		if (!fgets(option, MAX_LINE_LENGTH - 1, stdin))
			break;

		if (!strncasecmp(option, "Authorization: ", 15)) /* save authorization header */
			strcpy(authorization, option);
		
		if (option[1] && option[strlen(option)-2] == '\r')
			option[strlen(option)-2] = 0;
		else
			option[strlen(option)-1] = 0;

		if (!*option)
			break;
	}
	
		/* connect to enigma2 */
	upstream = socket(PF_INET, SOCK_STREAM, 0);
	
  	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(80);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(upstream, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)))
	{
		reason = "Upstream connect failed.";
		goto bad_gateway;
	}

	snprintf(upstream_request, sizeof(upstream_request), "GET /web/stream?StreamService=%s HTTP/1.0\r\n%s\r\n", service_ref, authorization);
	if (write(upstream, upstream_request, strlen(upstream_request)) != strlen(upstream_request))
		goto bad_gateway;
	while (1)
	{
		fd_set r;
		FD_ZERO(&r);
		FD_SET(upstream, &r);
		FD_SET(0, &r);
#ifdef HAVE_ADD_PID

		if (demux_fd != -1)
			FD_SET(demux_fd, &r);

		if (select(5, &r, 0, 0, 0) < 0)
			break;

		if (FD_ISSET(0, &r)) /* check for client disconnect */
			if (read(0, request, sizeof(request)) <= 0)
				break;

				/* handle enigma responses */
		if (FD_ISSET(upstream, &r))
			if (handle_upstream())
				break;
		if (demux_fd > 0 && FD_ISSET(demux_fd, &r))
		{
			static unsigned char buffer[BSIZE];
			int r = read(demux_fd, buffer, BSIZE);
			//logOutput("read %d bytes from demux0\n", r);
			if (r < 0)
				break;
			write(1, buffer, r);
		}
#else
		if (dvr_fd != -1)
			FD_SET(dvr_fd, &r);

		if (select(5, &r, 0, 0, 0) < 0)
			break;

		if (FD_ISSET(0, &r)) { /* check for client disconnect */
			if (read(0, request, sizeof(request)) <= 0)
				break;
				/* handle enigma responses */
		}

		if (FD_ISSET(upstream, &r)) {
			if (handle_upstream())
				break;
		}

		if (dvr_fd > 0 && FD_ISSET(dvr_fd, &r))
		{
			static unsigned char buffer[BSIZE];
			int r = read(dvr_fd, buffer, BSIZE);
			//logOutput("read %d bytes from dvr0\n", r);
			if (r < 0)
				break;
			write(1, buffer, r);
		}
#endif
	}
	
	if (upstream_state != 3)
		goto bad_gateway;
	
	return 0;
bad_request:
	printf("HTTP/1.0 400 Bad Request\r\n\r\n");
	return 1;
bad_gateway:
	printf("HTTP/1.0 %s\r\n%s\r\n%s\r\n", 
		upstream_response_code == 401 ? "401 Unauthorized" : "502 Bad Gateway",
		wwwauthenticate, reason);
	return 1;
}

int handle_upstream(void)
{
	char buffer[MAX_LINE_LENGTH];
	int n = read(upstream, buffer, MAX_LINE_LENGTH);
	if (n == 0)
		return 1;

	if (n < 0)
	{
		perror("read");
		return 1;
	}
	
	char *c = buffer;
	while (n)
	{
		char *next_line;
		int valid;

		next_line = memchr(c, '\n', n);
		if (!next_line)
			next_line = c + n;
		else
			next_line++;
		
		valid = next_line - c;
		if (valid > sizeof(response_line)-response_p) {
			return 1;
		}
		
		memcpy(response_line + response_p, c, valid);
		c += valid;
		response_p += valid;
		n -= valid;
		
				/* line received? */
		if (response_line[response_p - 1] == '\n')
		{
			response_line[response_p-1] = 0;
			
			if (response_p >= 2 && response_line[response_p - 2] == '\r')
				response_line[response_p-2] = 0;
			response_p = 0;
		
			if (handle_upstream_line()) {
				return 1;
			}
		}
	}
	return 0;
}

int handle_upstream_line(void)
{
	switch (upstream_state)
	{
	case 0:
		if (strncmp(response_line, "HTTP/1.", 7) || strlen(response_line) < 9)
		{
			reason = "Invalid upstream response.";
			return 1;
		}
		upstream_response_code = atoi(response_line + 9);
		reason = strdup(response_line + 9);
		upstream_state++;
		break;
	case 1:
		if (!*response_line)
		{
			if (upstream_response_code == 200)
				upstream_state = 2;
			else {
				return 1; /* reason was already set in state 0, but we need all header lines for potential WWW-Authenticate */
			}
		} else if (!strncasecmp(response_line, "WWW-Authenticate: ", 18))
			snprintf(wwwauthenticate, MAX_LINE_LENGTH, "%s\r\n", response_line);
		break;
	case 2:
	case 3:
		if (response_line[0] == '+')
		{
#ifndef HAVE_ADD_PID


			int dvr = atoi(response_line + 1);
			int adapter = atoi(response_line + 3);
	        
	        	if (dvr_fd < 0)
	        	{
					char dvrfn[32];
					sprintf(dvrfn, "/dev/dvb/adapter%d/dvr0", adapter);         //  <<<<<<<<<<<<<<<<<<<
					logOutput ("Open dvr0");
					dvr_fd = open(dvrfn, O_RDONLY);
					if (dvr_fd < 0)
					{
						logOutput (" - failed\n");
						reason = "DVR OPEN FAILED";
						return 2;
					}
					logOutput (" - succeeded\n");
	        	}

#else
					/* parse (and possibly open) demux */
			int demux = atoi(response_line + 1);
			
			if (demux_fd < 0)
			{
			  	struct dmx_pes_filter_params flt; 
				char demuxfn[32];
				sprintf(demuxfn, "/dev/dvb/adapter0/demux%d", demux);
				demux_fd = open(demuxfn, O_RDWR);
				if (demux_fd < 0)
				{
					reason = "DEMUX OPEN FAILED";
					return 2;
				}

		    		flt.pid = -1;
		    		flt.input = DMX_IN_FRONTEND;
		    		flt.output = DMX_OUT_TAP;
		    		flt.pes_type = DMX_TAP_TS;
		    		flt.flags = 0;

		   		 if (ioctl(demux_fd, DMX_SET_PES_FILTER, &flt) < 0)
		    		{
		    			reason = "DEMUX PES FILTER SET FAILED";
		    			return 2;
				}

				ioctl(demux_fd, DMX_SET_BUFFER_SIZE, 1024*1024);
				fcntl(demux_fd, F_SETFL, O_NONBLOCK);

		    		if (ioctl(demux_fd, DMX_START, 0) < 0)
		    		{
		    			reason = "DMX_START FAILED";
		    			return 2;
				}

			}
#endif

					/* parse new pids */
			const char *p = strchr(response_line, ':');
			int old_active_pids[MAX_PIDS];
			
			memcpy(old_active_pids, active_pids, sizeof(active_pids));
			
			int nr_pids = 0, i, j;
			while (p)
			{
				++p;
				int pid = strtoul(p, 0, 0x10);
				p = strchr(p, ',');
				
					/* do not add pids twice */
				for (i = 0; i < nr_pids; ++i)
					if (active_pids[i] == pid)
						break;

				if (i != nr_pids)
					continue;

				active_pids[nr_pids++] = pid;
				
				if (nr_pids == MAX_PIDS)
					break;
			}
			
			for (i = nr_pids; i < MAX_PIDS; ++i)
				active_pids[i] = -1;
				
					/* check for added pids */
			for (i = 0; i < nr_pids; ++i)
			{
				for (j = 0; j < MAX_PIDS; ++j)
					if (active_pids[i] == old_active_pids[j])
						break;

#ifndef HAVE_ADD_PID
				logOutput ("ADD PID %d - j=%d == MAX_PIDS=%d\n", active_pids[i], j, MAX_PIDS);
				if (j == MAX_PIDS) {
					struct dmx_pes_filter_params flt;
					char demuxfn[32];
	        	
        			flt.pes_type = DMX_PES_OTHER;
					flt.pid     = active_pids[i];
					flt.input   = DMX_IN_FRONTEND;
					flt.output  = DMX_OUT_TS_TAP;
					flt.flags   = DMX_IMMEDIATE_START;

					sprintf(demuxfn, "/dev/dvb/adapter%d/demux0", adapter);
				
					logOutput ("Open demux0");
					int fd = open(demuxfn, O_RDWR); // <<<<<<<<<<<<<<<
	        			if (fd < 0)
	        			{
	               				logOutput(" - failed\n");
	               				return -1;
	        			}
					logOutput (" - succedded fd=%d\n", fd);

					int res = ioctl(fd, DMX_SET_PES_FILTER, &flt);
					if (res < 0)
					{
						logOutput("set pes filter failed!\n");
						close(fd);
						break;
					}
					open_pids[i] = fd;
				}
#else

#if 1
				if (j == MAX_PIDS)
					ioctl(demux_fd, DMX_ADD_PID, active_pids[i]);
#endif

#endif
			}
			
					/* check for removed pids */
			for (i = 0; i < MAX_PIDS; ++i)
			{
				if (old_active_pids[i] == -1)
					continue;
				for (j = 0; j < nr_pids; ++j)
					if (old_active_pids[i] == active_pids[j])
						break;
#ifndef HAVE_ADD_PID
				logOutput ("REMOVE PID %d, j=%d == nr_pids=%d\n", old_active_pids[i], j, nr_pids);

				if (j == nr_pids) {
					logOutput ("close fd=%d\n", open_pids[i]);
					close(open_pids[i]);
				}
#else		

#if 1
				if (j == nr_pids)
					ioctl(demux_fd, DMX_REMOVE_PID, old_active_pids[i]);
#endif

#endif
			}
			if (upstream_state == 2)
			{
				char *c = "HTTP/1.0 200 OK\r\nConnection: Close\r\nContent-Type: video/mpeg\r\nServer: stream_enigma2\r\n\r\n";
				write(1, c, strlen(c));
				upstream_state = 3; /* HTTP response sent */
			}
		} else if (response_line[0] == '-')
		{
			reason = strdup(response_line + 1);
			return 1;
		}
				/* ignore everything not starting with + or - */
		break;
	}
	return 0;
}
