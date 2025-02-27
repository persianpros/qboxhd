/*******************************************************************************
 VLC Player Plugin by A. Lätsch 2007

 This is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free
 Software Foundation; either version 2, or (at your option) any later
 version.
********************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>
#include "servicets.h"
#include <lib/base/eerror.h>
#include <lib/base/object.h>
#include <lib/base/ebase.h>
#include <servicets.h>
#include <lib/service/service.h>
#include <lib/base/init_num.h>
#include <lib/base/init.h>
#include <lib/dvb/decoder.h>

#include <lib/dvb/pmt.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))


/********************************************************************/
/* eServiceFactoryTS                                                */
/********************************************************************/

eServiceFactoryTS::eServiceFactoryTS()
{
	ePtr<eServiceCenter> sc;

	eServiceCenter::getPrivInstance(sc);
	if (sc)
	{
		std::list<std::string> extensions;
		sc->addServiceFactory(eServiceFactoryTS::id, this, extensions);
	}
}

eServiceFactoryTS::~eServiceFactoryTS()
{
	ePtr<eServiceCenter> sc;

	eServiceCenter::getPrivInstance(sc);
	if (sc)
		sc->removeServiceFactory(eServiceFactoryTS::id);
}

DEFINE_REF(eServiceFactoryTS)

// iServiceHandler
RESULT eServiceFactoryTS::play(const eServiceReference &ref, ePtr<iPlayableService> &ptr)
{
	ptr = new eServiceTS(ref);
	return 0;
}

RESULT eServiceFactoryTS::record(const eServiceReference &ref, ePtr<iRecordableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryTS::list(const eServiceReference &, ePtr<iListableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryTS::info(const eServiceReference &ref, ePtr<iStaticServiceInformation> &ptr)
{
	ptr = 0;
	return -1;
}

RESULT eServiceFactoryTS::offlineOperations(const eServiceReference &, ePtr<iServiceOfflineOperations> &ptr)
{
	ptr = 0;
	return -1;
}


/********************************************************************/
/* TSAudioInfo                                            */
/********************************************************************/
DEFINE_REF(TSAudioInfo);

void TSAudioInfo::addAudio(int pid, std::string lang, std::string desc, int type) {
	StreamInfo as;
	as.description = desc;
	as.language = lang;
	as.pid = pid;
	as.type = type;
	audioStreams.push_back(as);
}


/********************************************************************/
/* eServiceTS                                                       */
/********************************************************************/

eServiceTS::eServiceTS(const eServiceReference &url): m_pump(eApp, 1)
{
	eDebug("ServiceTS construct!");
	m_filename = url.path.c_str();
	m_vpid = url.getData(0) == 0 ? 0x44 : url.getData(0);
	m_apid = url.getData(1) == 0 ? 0x45 : url.getData(1);
	m_state = stIdle;
	m_audioInfo = 0;
	m_destfd = -1;
}

eServiceTS::~eServiceTS()
{
	eDebug("ServiceTS destruct!");
#ifdef QBOXHD
	if(m_state == stRunning)
	  stop();
#else
	stop();
#endif
}

DEFINE_REF(eServiceTS);

size_t crop(char *buf)
{
	size_t len = strlen(buf) - 1;
	while (len > 0 && (buf[len] == '\r' || buf[len] == '\n')) {
		buf[len--] = '\0';
	}
	return len;
}

static int getline(char** pbuffer, size_t* pbufsize, int fd)
{
	size_t i = 0;
	int rc;
	while (true) {
		if (i >= *pbufsize) {
			char *newbuf = (char*)realloc(*pbuffer, (*pbufsize)+1024);
			if (newbuf == NULL)
				return -ENOMEM;
			*pbuffer = newbuf;
			*pbufsize = (*pbufsize)+1024;
		}
		rc = ::read(fd, (*pbuffer)+i, 1);
		if (rc <= 0 || (*pbuffer)[i] == '\n')
		{
			(*pbuffer)[i] = '\0';
			return rc <= 0 ? -1 : i;
		}
		if ((*pbuffer)[i] != '\r') i++;
	}
}

int eServiceTS::openHttpConnection(std::string url)
{
	std::string host;
	int port = 80;
	std::string uri;

	int slash = url.find("/", 7);
	if (slash > 0) {
		host = url.substr(7, slash-7);
		uri = url.substr(slash, url.length()-slash);
	} else {
		host = url.substr(7, url.length()-7);
		uri = "";
	}
	int dp = host.find(":");
	if (dp == 0) {
		port = atoi(host.substr(1, host.length()-1).c_str());
		host = "localhost";
	} else if (dp > 0) {
		port = atoi(host.substr(dp+1, host.length()-dp-1).c_str());
		host = host.substr(0, dp);
	}

	struct hostent* h = gethostbyname(host.c_str());
	if (h == NULL || h->h_addr_list == NULL)
		return -1;
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		return -1;

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = *((in_addr_t*)h->h_addr_list[0]);
	addr.sin_port = htons(port);

	eDebug("connecting to %s", url.c_str());

	if (connect(fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
		std::string msg = "connect failed for: " + url;
		eDebug(msg.c_str());
		return -1;
	}

	std::string request = "GET ";
	request.append(uri).append(" HTTP/1.1\n");
	request.append("Host: ").append(host).append("\n");
	request.append("Accept: */*\n");
	request.append("Connection: close\n");
	request.append("\n");
	//eDebug(request.c_str());
	write(fd, request.c_str(), request.length());

	int rc;
	size_t buflen = 1000;
	char* linebuf = (char*)malloc(1000);

	rc = getline(&linebuf, &buflen, fd);
	eDebug("RECV(%d): %s", rc, linebuf);
	if (rc <= 0)
	{
		close(fd);
		free(linebuf);
		return -1;
	}

	char proto[100];
	int statuscode = 0;
	char statusmsg[100];
	rc = sscanf(linebuf, "%99s %d %99s", proto, &statuscode, statusmsg);
	if (rc != 3 || statuscode != 200) {
		eDebug("wrong response: \"200 OK\" expected.");
		free(linebuf);
		close(fd);
		return -1;
	}
	eDebug("proto=%s, code=%d, msg=%s", proto, statuscode, statusmsg);
	while (rc > 0)
	{
		rc = getline(&linebuf, &buflen, fd);
		eDebug("RECV(%d): %s", rc, linebuf);
	}
	free(linebuf);

	return fd;
}

RESULT eServiceTS::connectEvent(const Slot2<void,iPlayableService*,int> &event, ePtr<eConnection> &connection)
{
	connection = new eConnection((iPlayableService*)this, m_event.connect(event));
	return 0;
}

RESULT eServiceTS::start()
{
	ePtr<eDVBResourceManager> rmgr;
	eDVBResourceManager::getInstance(rmgr);
	eDVBChannel dvbChannel(rmgr, 0);
	if (m_destfd == -1)
	{
		m_destfd = ::open("/dev/misc/pvr", O_WRONLY);
		if (m_destfd < 0)
		{
			eDebug("Cannot open /dev/misc/pvr");
			return -1;
		}
	}
	if (dvbChannel.getDemux(m_decodedemux, iDVBChannel::capDecode) != 0) {
		eDebug("Cannot allocate decode-demux");
		return -1;
	}
	if (m_decodedemux->getMPEGDecoder(m_decoder, 1) != 0) {
		eDebug("Cannot allocate MPEGDecoder");
		return -1;
	}
	m_decoder->setVideoPID(m_vpid, eDVBVideo::MPEG2);
	m_decoder->setAudioPID(m_apid, eDVBAudio::aMPEG);

	m_streamthread = new eStreamThread();
	CONNECT(m_streamthread->m_event, eServiceTS::recv_event);
	m_decoder->pause();
	if (unpause() != 0)
       return -1;
	m_state = stRunning;
	m_event(this, evStart);
	return 0;
}

RESULT eServiceTS::stop()
{
	if (m_state != stRunning)
		return -1;

	if(m_destfd>=0)
	{
		::close(m_destfd);
		m_destfd = -1;
	}
	printf("TS: %s stop\n", m_filename.c_str());
	m_streamthread->stop();
	m_decodedemux->flush();
	m_state = stStopped;
	m_audioInfo = 0;

	return 0;
}

void eServiceTS::recv_event(int evt)
{
	eDebug("eServiceTS::recv_event: %d", evt);
	switch (evt) {
	case eStreamThread::evtEOS:
		m_decodedemux->flush();
		m_state = stStopped;
		m_event((iPlayableService*)this, evEOF);
		break;
	case eStreamThread::evtReadError:
	case eStreamThread::evtWriteError:
		m_decoder->pause();
		m_state = stStopped;
		m_event((iPlayableService*)this, evEOF);
		break;
	case eStreamThread::evtSOS:
		m_event((iPlayableService*)this, evSOF);
		break;
	case eStreamThread::evtStreamInfo:
		bool wasnull = !m_audioInfo;
		m_streamthread->getAudioInfo(m_audioInfo);
		if (m_audioInfo)
			eDebug("[servicets] %d audiostreams found", m_audioInfo->audioStreams.size());
		if (m_audioInfo && wasnull) {
			int sel = getCurrentTrack();
			if (sel < 0)
				selectTrack(0);
			else if (m_audioInfo->audioStreams[sel].type != eDVBAudio::aMPEG)
				selectTrack(sel);
		}
		break;
	}
}

RESULT eServiceTS::pause(ePtr<iPauseableService> &ptr)
{
	ptr = this;
	return 0;
}

// iPausableService
RESULT eServiceTS::pause()
{
	m_streamthread->stop();
	m_decoder->pause();
	return 0;
}

RESULT eServiceTS::unpause()
{

#ifdef E2_LAST_MOD
    if(!m_streamthread->running())
    {
      int is_streaming = !strncmp(m_filename.c_str(), "http://", 7);
      int srcfd = -1;

      if (is_streaming)
        srcfd = openHttpConnection(m_filename);
      else
        srcfd = ::open(m_filename.c_str(), O_RDONLY);

      if (srcfd < 0) {
        eDebug("Cannot open source stream: %s", m_filename.c_str());
        return 1;
      }

      m_decodedemux->flush();
      m_streamthread->start(srcfd, m_destfd);
      m_decoder->play();

    }
    else
      eDebug("unpause but thread already running!");
#else ///!E2_LAST_MOD
    int is_streaming = !strncmp(m_filename.c_str(), "http://", 7);
    int srcfd = -1;

	if (is_streaming) {
		srcfd = openHttpConnection(m_filename);
	} else {
		srcfd = ::open(m_filename.c_str(), O_RDONLY);
	}

	if (srcfd < 0) {
		eDebug("Cannot open source stream: %s", m_filename.c_str());
		return 1;
	}

#if 0
	char dvbDevice[30]="";
	int adapterIndex = m_decodedemux->getAdapter();
	int dmxIndex= m_decodedemux->getDemux();
	int dvrIndex=dmxIndex;

	///This will be adapterX/dvrY
	eDebug("eServiceTS::unpause() adapter=%d demux=%d dvr=%d",adapterIndex,dmxIndex,dvrIndex);
	sprintf(dvbDevice, "/dev/dvb/adapter%d/dvr%d", adapterIndex,dvrIndex);
#else
	char dvbDevice[30]="/dev/misc/pvr";
#endif
	eDebug("eServiceTS::unpause() dvbDevice=%s",dvbDevice);
	int destfd = ::open(dvbDevice, O_WRONLY);
// 	int destfd = ::open("/dev/dvb/adapter1/dvr0", O_WRONLY);
	if (destfd < 0) {
// 		eDebug("Cannot open /dev/dvb/adapter1/dvr0");
		eDebug("Cannot open %s",dvbDevice);
		::close(srcfd);
		return 1;
	}

	m_decodedemux->flush();
	m_streamthread->start(srcfd, destfd);
	m_decoder->play();
#endif ///E2_LAST_MOD
	return 0;
}

// iSeekableService
RESULT eServiceTS::seek(ePtr<iSeekableService> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceTS::getLength(pts_t &pts)
{
	return 0;
}

RESULT eServiceTS::seekTo(pts_t to)
{
	return 0;
}

RESULT eServiceTS::seekRelative(int direction, pts_t to)
{
	return 0;
}

RESULT eServiceTS::getPlayPosition(pts_t &pts)
{
	return 0;
}

RESULT eServiceTS::setTrickmode(int trick)
{
	return -1;
}

RESULT eServiceTS::isCurrentlySeekable()
{
	return 1;
}

RESULT eServiceTS::info(ePtr<iServiceInformation>&i)
{
	i = this;
	return 0;
}

RESULT eServiceTS::getName(std::string &name)
{
	name = m_filename;
	size_t n = name.rfind('/');
	if (n != std::string::npos)
		name = name.substr(n + 1);
	return 0;
}

int eServiceTS::getInfo(int w)
{
	return resNA;
}

std::string eServiceTS::getInfoString(int w)
{
	return "";
}

int eServiceTS::getNumberOfTracks() {
	if (m_audioInfo)
		return (int)m_audioInfo->audioStreams.size();
	else
		return 0;
}

RESULT eServiceTS::selectTrack(unsigned int i) {
	if (m_audioInfo) {
		m_apid = m_audioInfo->audioStreams[i].pid;
		eDebug("[servicets] audio track %d PID 0x%02x type %d\n", i, m_apid, m_audioInfo->audioStreams[i].type);
		m_decoder->setAudioPID(m_apid, m_audioInfo->audioStreams[i].type);

#ifdef E2_LAST_MOD
        m_decoder->set();
#else ///!E2_LAST_MOD
		if (m_state == stRunning)
			m_decoder->set();
#endif ///E2_LAST_MOD

		return 0;
	} else {
		return -1;
	}
}

RESULT eServiceTS::getTrackInfo(struct iAudioTrackInfo &info, unsigned int n) {
	if (m_audioInfo) {
		info.m_pid = m_audioInfo->audioStreams[n].pid;
		info.m_description = m_audioInfo->audioStreams[n].description;
		info.m_language = m_audioInfo->audioStreams[n].language;
		return 0;
	} else {
		return -1;
	}
}

int eServiceTS::getCurrentTrack() {
	if (m_audioInfo) {
		for (size_t i = 0; i < m_audioInfo->audioStreams.size(); i++) {
			if (m_apid == m_audioInfo->audioStreams[i].pid) {
				return i;
			}
		}
	}
	return -1;
}

/********************************************************************/
/* eStreamThread                                                       */
/********************************************************************/

DEFINE_REF(eStreamThread)

eStreamThread::eStreamThread(): m_messagepump(eApp, 0) {
	CONNECT(m_messagepump.recv_msg, eStreamThread::recvEvent);
}
eStreamThread::~eStreamThread() {
// 	eDebug("eStreamThread deinit");
}

void eStreamThread::start(int srcfd, int destfd) {
	m_srcfd = srcfd;
	m_destfd = destfd;
	m_stop = m_running = false;
	m_audioInfo = 0;
	run(IOPRIO_CLASS_RT);
}

void eStreamThread::stop() {
	m_stop = true;
	kill();
}

void eStreamThread::recvEvent(const int &evt)
{
	m_event(evt);
}

RESULT eStreamThread::getAudioInfo(ePtr<TSAudioInfo> &ptr)
{
	ptr = m_audioInfo;
	return 0;
}

#define REGISTRATION_DESCRIPTOR 5
#define LANGUAGE_DESCRIPTOR 10

std::string eStreamThread::getDescriptor(unsigned char buf[], int buflen, int type)
{
	int desc_len;
	while (buflen > 1) {
		desc_len = buf[1];
		if (buf[0] == type) {
			char str[21];
			if (desc_len > 20) desc_len = 20;
			strncpy(str, (char*)buf+2, desc_len);
			str[desc_len] = '\0';
			return std::string(str);
		} else {
			buflen -= desc_len+2;
			buf += desc_len+2;
		}
	}
	return "";
}

bool eStreamThread::scanAudioInfo(unsigned char buf[], int len)
{
	if (len < 1880)
		return false;

	int adaptfield, pmtpid, offset;
	unsigned char pmt[1188];
	int pmtsize = 0;

	for (int a=0; a < len - 188*4; a++) {
		if ( buf[a] != 0x47 || buf[a + 188] != 0x47 || buf[a + 376] != 0x47 )
			continue; // TS Header

		if ((0x40 & buf[a + 1]) == 0) // start
			continue;

		if ((0xC0 & buf[a + 3]) != 0) // scrambling
			continue;

		adaptfield = (0x30 & buf[a + 3]) >> 4;

		if ((adaptfield & 1) == 0) // adapt - no payload
			continue;

		offset = adaptfield == 3 ? 1 + (0xFF & buf[a + 4]) : 0; //adaptlength

		if (buf[a + offset + 4] != 0 || buf[a + offset + 5] != 2 || (0xF0 & buf[a + offset + 6]) != 0xB0)
		{
			a += 187;
			continue;
		}

		pmtpid = (0x1F & buf[a + 1])<<8 | (0xFF & buf[a + 2]);
		memcpy(pmt + pmtsize, buf + a + 4 + offset, 184 - offset);
		pmtsize += 184 - offset;

		if (pmtsize >= 1000)
			break;
	}

	if (pmtsize == 0) return false;

	int pmtlen = (0x0F & pmt[2]) << 8 | (0xFF & pmt[3]);
	std::string lang;
	std::string pd_type;
	ePtr<TSAudioInfo> ainfo = new TSAudioInfo();

	for (int b=8; b < pmtlen-4 && b < pmtsize-6; b++)
	{
		if ( (0xe0 & pmt[b+1]) != 0xe0 )
			continue;

		int pid = (0x1F & pmt[b+1])<<8 | (0xFF & pmt[b+2]);

		switch(pmt[b])
		{
		case 1:
		case 2: // MPEG Video
			//addVideo(pid, "MPEG2");
			break;

		case 0x1B: // H.264 Video
			//addVideo(pid, "H.264");
			break;

		case 3:
		case 4: // MPEG Audio
			lang = getDescriptor(pmt+b+5, pmt[b+4], LANGUAGE_DESCRIPTOR);
			ainfo->addAudio(pid, lang, "MPEG", eDVBAudio::aMPEG);
			break;

		case 0x80:
		case 0x81:  //private data of AC3 in ATSC
		case 0x82:
		case 0x83:
		case 6:
			lang = getDescriptor(pmt+b+5, pmt[b+4], LANGUAGE_DESCRIPTOR);
			pd_type = getDescriptor(pmt+b+5, pmt[b+4], REGISTRATION_DESCRIPTOR);
			if (pd_type == "AC-3")
				ainfo->addAudio(pid, lang, pd_type, eDVBAudio::aAC3);
			break;
		}
		b += 4 + pmt[b+4];
	}
	if (ainfo->audioStreams.size() > 0) {
		m_audioInfo = ainfo;
		return true;
	} else {
		return false;
	}
}


void eStreamThread::thread() {
	//const int bufsize = 40000;
#ifdef QBOXHD
	const int bufsize = 37412*8;
#else
	const int bufsize = 37412*4;
#endif
	unsigned char buf[bufsize];
	bool eof = false;
	fd_set rfds;
	fd_set wfds;
	struct timeval timeout;
	int rc,r,w,maxfd;
	time_t next_scantime = 0;
	bool sosSend = false;
	m_running = true;

	r = w = 0;
	hasStarted();
	eDebug("eStreamThread started");
	while (!m_stop) {
		pthread_testcancel();
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		maxfd = 0;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if (r < bufsize) {
			FD_SET(m_srcfd, &rfds);
			maxfd = MAX(maxfd, m_srcfd);
		}
		if (w < r) {
			FD_SET(m_destfd, &wfds);
			maxfd = MAX(maxfd, m_destfd);
		}
		rc = select(maxfd+1, &rfds, &wfds, NULL, &timeout);
		if (rc == 0) {
			eDebug("eStreamThread::thread: timeout!");
			continue;
		}
		if (rc < 0) {
			eDebug("eStreamThread::thread: error in select (%d)", errno);
			break;
		}

		///READ FROM SOURCE (SOCKET)
		if (FD_ISSET(m_srcfd, &rfds)) {

			rc = ::read(m_srcfd, buf+r, bufsize - r);

// 			eDebug("eStreamThread::thread: READ %d bytes from source        <----------------",rc);
			if (rc < 0) {
				eDebug("eStreamThread::thread: error in read (%d)", errno);
				m_messagepump.send(evtReadError);
				break;
			} else if (rc == 0) {
				eof = true;
			} else {
 				if (!sosSend) {
 					sosSend = true;
 					m_messagepump.send(evtSOS);
 				}
				r += rc;
				//if (r == bufsize) eDebug("eStreamThread::thread: buffer full");
			}
		}

		///WRITE TO DESTINATION (PVR)
		if (FD_ISSET(m_destfd, &wfds) && (w < r) && (((r-w) > 0) || eof)) {

			unsigned int nbyte_remaing = 0;
			unsigned int pkt_complete = 0;
			unsigned int errcode = 0;
// 			eDebug("eStreamThread::thread: will WRITE to destination    <----------------");
			while( 1 ) {
#ifdef QBOXHD
				pkt_complete = (r-w)/(bufsize/8);
				nbyte_remaing = (r-w) % (bufsize/8);
#else
				pkt_complete = (r-w)/(bufsize/4);
				nbyte_remaing = (r-w) % (bufsize/4);
#endif
// 				eDebug("eStreamThread::thread: WRITE pkt_complete=%d (r=%d,w=%d,bufsize=%d)        <----------------",pkt_complete,r,w,bufsize);
				if (pkt_complete>0) {
#define MY_PATCH
#ifdef MY_PATCH
					{
						int w1=w;
						while(*(buf+w)!=0x47 && (w-w1)<188)
							w++;

						if(w1!=w)
							eDebug("eStreamThread::thread: moved write %d ahead to align stream to packet's start-byte <==================", w-w1);
					}
#endif
					rc = ::write(m_destfd, buf+w, bufsize/8);

// 					eDebug("eStreamThread::thread: write complete packet %d <----------------", rc);

					if (rc < 0) {
						eDebug("eStreamThread::thread: error in write (%d)", errno);
						m_messagepump.send(evtWriteError);
						errcode = 1;
						break;
					}
					w += rc;
				}
				else {
					if ( (nbyte_remaing > 0) && eof ) {
						rc = ::write(m_destfd, buf+w, nbyte_remaing);
						if (rc < 0) {
							eDebug("eStreamThread::thread: error in write (%d)", errno);
							m_messagepump.send(evtWriteError);
							errcode = 1;
							break;
						}
						w += rc;
// 						eDebug("eStreamThread::thread: write remain bytes for eof %d", rc);
					}

					break;	/* End of packet complete trasmission*/
				}
			}

			if (errcode > 0) break;

//			eDebug("eStreamThread::thread: buffer r=%d w=%d",r,w);
			if (w == r) {
 				if (time(0) >= next_scantime) {
					if (scanAudioInfo(buf, r)) {
						m_messagepump.send(evtStreamInfo);
						next_scantime = time(0) + 1;
					}
 				}
				w = r = 0;
			}
		}

/*
		if (FD_ISSET(m_destfd, &wfds) && (w < r) && ((r > bufsize/8) || eof)) {
			rc = ::write(m_destfd, buf+w, r-w);
			if (rc < 0) {
				eDebug("eStreamThread::thread: error in write (%d)", errno);
				m_messagepump.send(evtWriteError);
				break;
			}
			w += rc;
			eDebug("eStreamThread::thread: buffer r=%d w=%d",r,w);
			if (w == r) {
 				if (time(0) >= next_scantime) {
 					if (scanAudioInfo(buf, r)) {
 						m_messagepump.send(evtStreamInfo);
 						next_scantime = time(0) + 1;
 					}
 				}
				w = r = 0;
			}

		}
*/
		if (eof && (r==w)) {
			::close(m_destfd);
			m_destfd = -1;
			::close(m_srcfd);
			m_srcfd = -1;
			m_messagepump.send(evtEOS);
			break;
		}
	}

	eDebug("eStreamThread end");
}

void eStreamThread::thread_finished() {
	if (m_srcfd >= 0)
      ::close(m_srcfd);
	if (m_destfd >= 0)
      ::close(m_destfd);
	eDebug("eStreamThread closed");
	m_running = false;
}

eAutoInitPtr<eServiceFactoryTS> init_eServiceFactoryTS(eAutoInitNumbers::service+1, "eServiceFactoryTS");

PyMODINIT_FUNC
initservicets(void)
{
	Py_InitModule("servicets", NULL);
}
