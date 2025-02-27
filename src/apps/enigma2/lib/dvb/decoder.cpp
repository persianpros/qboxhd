#include <lib/base/ebase.h>
#include <lib/base/eerror.h>
#include <lib/dvb/decoder.h>
#if HAVE_DVB_API_VERSION < 3
#define audioStatus audio_status
#define videoStatus video_status
#define pesType pes_type
#define playState play_state
#define audioStreamSource_t audio_stream_source_t
#define videoStreamSource_t video_stream_source_t
#define streamSource stream_source
#define dmxPesFilterParams dmx_pes_filter_params
#define DMX_PES_VIDEO0 DMX_PES_VIDEO
#define DMX_PES_AUDIO0 DMX_PES_AUDIO
#define DMX_PES_PCR0 DMX_PES_PCR
#define DMX_PES_TELETEXT0 DMX_PES_TELETEXT
#define DMX_PES_VIDEO1 DMX_PES_VIDEO
#define DMX_PES_AUDIO1 DMX_PES_AUDIO
#define DMX_PES_PCR1 DMX_PES_PCR
#define DMX_PES_TELETEXT1 DMX_PES_TELETEXT
#include <ost/dmx.h>
#include <ost/video.h>
#include <ost/audio.h>
#else
#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>
#include <linux/dvb/dmx.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef QBOXHD
extern bool radio_service;
#include <linux/dvb/stm_ioctls.h>
#include "../driver/avswitch.h"
#include "volume.h"
#endif

	/* these are quite new... */
#ifndef AUDIO_GET_PTS
#define AUDIO_GET_PTS              _IOR('o', 19, __u64)
#define VIDEO_GET_PTS              _IOR('o', 57, __u64)
#endif

DEFINE_REF(eDVBAudio);

#ifdef QBOXHD
int m_fd_video_support;
Format_video_t global_vf;
#endif

int last_adapter=-1;
eDVBAudio::eDVBAudio(eDVBDemux *demux, int dev)
	:m_demux(demux), m_dev(dev)
{
	char filename[128];
#if HAVE_DVB_API_VERSION < 3
	sprintf(filename, "/dev/dvb/card%d/audio%d", demux->adapter, dev);
#else
    sprintf(filename, "/dev/dvb/adapter%d/audio%d", demux->adapter, dev);
#endif
	m_fd = ::open(filename, O_RDWR);
	if (m_fd < 0)
		eWarning("%s: %m", filename);

#if HAVE_DVB_API_VERSION < 3
	sprintf(filename, "/dev/dvb/card%d/demux%d", demux->adapter, demux->demux);
#else
	sprintf(filename, "/dev/dvb/adapter%d/demux%d", demux->adapter, demux->demux);
#endif
	eDebug("eDVBAudio::eDVBAudio(): m_fd_demux of file %s",filename);
	m_fd_demux = ::open(filename, O_RDWR);
	if (m_fd_demux < 0)
		eWarning("%s: %m", filename);

#ifdef QBOXHD
	m_adapter = demux->adapter;

	if (!radio_service)
    {
		/* FIXME: It is a work around for teletext */
		if(last_adapter != m_adapter)
		{
			eDebug("Create link for teletext -> adapter: %d",m_adapter);
            unlink("/tmp/current_adapter");
			sprintf(filename,"ln -s /dev/dvb/adapter%d /tmp/current_adapter", m_adapter);
			system(filename);
			last_adapter=m_adapter;
		}
		else
		{
			eDebug("The same adapter: %d", m_adapter);
		}
	}
#endif // QBOXHD
}

#if HAVE_DVB_API_VERSION < 3
int eDVBAudio::setPid(int pid, int type)
{
	if ((m_fd < 0) || (m_fd_demux < 0))
		return -1;

	int bypass = 0;

	switch (type)
	{
	case aMPEG:
		bypass = 1;
		break;
	case aAC3:
		bypass = 0;
		break;
		/*
	case aDTS:
		bypass = 2;
		break;
		*/
	}

	if (::ioctl(m_fd, AUDIO_SET_BYPASS_MODE, bypass) < 0)
		eDebug("failed (%m)");

	dmx_pes_filter_params pes;

	pes.pid      = pid;
	pes.input    = DMX_IN_FRONTEND;
	pes.output   = DMX_OUT_DECODER;
	pes.pes_type = m_dev ? DMX_PES_AUDIO1 : DMX_PES_AUDIO0; /* FIXME */
	pes.flags    = 0;
 	eDebugNoNewLine("DMX_SET_PES_FILTER(0x%02x) - audio - ", pid);
	if (::ioctl(m_fd_demux, DMX_SET_PES_FILTER, &pes) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");

	return 0;
}

int eDVBAudio::startPid()
{
 	eDebugNoNewLine("DEMUX_START - audio - ");
	if (::ioctl(m_fd_demux, DMX_START) < 0)
	{
		eDebug("DEMUX_START - audio - failed (%m)");
		return -errno;
	}
	eDebug("DEMUX_START - audio - ok");
	return 0;
}

int eDVBAudio::start()
{
 	eDebugNoNewLine("AUDIO_PLAY - ");
	if (::ioctl(m_fd, AUDIO_PLAY) < 0)
	{
		eDebug("AUDIO_PLAY - failed (%m)");
		return -errno;
	}
	eDebug("AUDIO_PLAY - ok");
	return 0;
}

int eDVBAudio::stopPid()
{
 	eDebugNoNewLine("DEMUX_STOP - audio - ");
	if (::ioctl(m_fd_demux, DMX_STOP) < 0)
	{
		eDebug("DEMUX_STOP - audio - failed (%m)");
		return -errno;
	}
	eDebug("DEMUX_STOP - audio - ok");
	return 0;
}

int eDVBAudio::setAVSync(int val)
{
 	eDebugNoNewLine("AUDIO_SET_AV_SYNC - ");
	if (::ioctl(m_fd, AUDIO_SET_AV_SYNC, val) < 0)
	{
		eDebug("AUDIO_SET_AV_SYNC - failed (%m)");
		return -errno;
	}
	eDebug("AUDIO_SET_AV_SYNC - ok");
	return 0;
}
#else
int eDVBAudio::startPid(int pid, int type)
{
    if ((m_fd < 0) || (m_fd_demux < 0))
        return -1;
    dmx_pes_filter_params pes;

	pes.pid      = pid;
	pes.input    = DMX_IN_FRONTEND;
	pes.output   = DMX_OUT_DECODER;
	pes.pes_type = m_dev ? DMX_PES_AUDIO1 : DMX_PES_AUDIO0; /* FIXME */
	pes.flags    = 0;
	eDebugNoNewLine("DMX_SET_PES_FILTER(0x%02x) - audio - ", pid);
	if (::ioctl(m_fd_demux, DMX_SET_PES_FILTER, &pes) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");
	eDebugNoNewLine("DEMUX_START - audio - ");
	if (::ioctl(m_fd_demux, DMX_START) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");

#ifdef QBOXHD
    if (::ioctl(m_fd, AUDIO_SELECT_SOURCE, AUDIO_SOURCE_DEMUX) < 0)
        eDebug("AUDIO_SELECT_SOURCE failed (%m)");
    else
        eDebug("AUDIO_SELECT_SOURCE ok");
#endif // QBOXHD

	int bypass = 0;

	switch (type)
	{
	case aMPEG:
		bypass = 1;

        setVolumeAudioType(MPEG_TYPE_VOLUME);
        amixer_control_set(AMIXER_SPDIF_ENCODING, 0);
        amixer_control_set(AMIXER_SPDIF_BYPASS, 0);
		break;
	case aAC3:
		bypass = 0;

        setVolumeAudioType(AC3_TYPE_VOLUME);
        amixer_control_set(AMIXER_SPDIF_ENCODING, 1);
        amixer_control_set(AMIXER_SPDIF_BYPASS, 1);
		break;
	case aDTS:
		bypass = 2;

        amixer_control_set(AMIXER_SPDIF_ENCODING, 2);
        amixer_control_set(AMIXER_SPDIF_BYPASS, 1);
		break;
	case aAAC:
		bypass = 8;

        amixer_control_set(AMIXER_SPDIF_ENCODING, 1);
        amixer_control_set(AMIXER_SPDIF_BYPASS, 1);
		break;
	case aAACHE:
		bypass = 9;
		break;
	case aLPCM:
		bypass = 6;
		break;
	}

#ifdef QBOXHD
	if(radio_service==1)
	{
		/* FIXME: work arounf for radio volume */
		eDebug("Set the max volume for radio channels");
		setVolumeAudioType(RADIO_TYPE_VOLUME);
	}
#endif ///QBOXHD


	eDebugNoNewLine("AUDIO_SET_BYPASS(%d) - ", bypass);
	if (::ioctl(m_fd, AUDIO_SET_BYPASS_MODE, bypass) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
	freeze();  // why freeze here?!? this is a problem when only a pid change is requested... because of the unfreeze logic in Decoder::setState
	eDebugNoNewLine("AUDIO_PLAY - ");
	if (::ioctl(m_fd, AUDIO_PLAY) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");

#ifdef __sh__
/* Dagobert 03.10.2009: setting the volume from the py source
 * when they are initted is not good for us because there is
 * no stream and the player is not intialized. so we set it
 * here directly after the player is started fully.
 *
 * see ticket #42
 */

   eDVBVolumecontrol* vol = eDVBVolumecontrol::getInstance();
   vol->setVolume(vol->getVolume(), vol->getVolume());
#endif

	return 0;
}
#endif

void eDVBAudio::stop()
{
#if HAVE_DVB_API_VERSION > 2
	flush();
#endif
	eDebugNoNewLine("AUDIO_STOP - ");
	if (::ioctl(m_fd, AUDIO_STOP) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
#if HAVE_DVB_API_VERSION > 2
	eDebugNoNewLine("DEMUX_STOP - audio - ");
	if (::ioctl(m_fd_demux, DMX_STOP) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
#endif
}

void eDVBAudio::flush()
{
	eDebugNoNewLine("AUDIO_CLEAR_BUFFER - ");
	if (::ioctl(m_fd, AUDIO_CLEAR_BUFFER) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
}

void eDVBAudio::freeze()
{
	eDebugNoNewLine("AUDIO_PAUSE - ");
	if (::ioctl(m_fd, AUDIO_PAUSE) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
}

void eDVBAudio::unfreeze()
{
	eDebugNoNewLine("AUDIO_CONTINUE - ");
	if (::ioctl(m_fd, AUDIO_CONTINUE) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
}

void eDVBAudio::setChannel(int channel)
{
	int val = AUDIO_STEREO;
	switch (channel)
	{
	case aMonoLeft: val = AUDIO_MONO_LEFT; break;
	case aMonoRight: val = AUDIO_MONO_RIGHT; break;
	default: break;
	}
	eDebugNoNewLine("AUDIO_CHANNEL_SELECT(%d) - ", val);
	if (::ioctl(m_fd, AUDIO_CHANNEL_SELECT, val) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
}

int eDVBAudio::getPTS(pts_t &now)
{
	if (::ioctl(m_fd, AUDIO_GET_PTS, &now) < 0)
		eDebug("AUDIO_GET_PTS failed (%m)");
	return 0;
}

eDVBAudio::~eDVBAudio()
{
	unfreeze();  // why unfreeze here... but not unfreeze video in ~eDVBVideo ?!?
	if (m_fd >= 0)
		::close(m_fd);
	if (m_fd_demux >= 0)
		::close(m_fd_demux);
}

DEFINE_REF(eDVBVideo);

eDVBVideo::eDVBVideo(eDVBDemux *demux, int dev)
	: m_demux(demux), m_dev(dev),
	m_width(-1), m_height(-1), m_framerate(-1), m_aspect(-1), m_progressive(-1)
{
	char filename[128];
#if HAVE_DVB_API_VERSION < 3
	sprintf(filename, "/dev/dvb/card%d/video%d", demux->adapter, dev);
	m_fd_video = ::open("/dev/video", O_RDWR);
	if (m_fd_video < 0)
		eWarning("/dev/video: %m");
#else
	sprintf(filename, "/dev/dvb/adapter%d/video%d", demux->adapter, dev);
#endif
	m_fd = ::open(filename, O_RDWR);
	if (m_fd < 0)
		eWarning("%s: %m", filename);
	else
	{
		m_sn = eSocketNotifier::create(eApp, m_fd, eSocketNotifier::Priority);
		CONNECT(m_sn->activated, eDVBVideo::video_event);
	}
	eDebug("Video Device: %s", filename);
#if HAVE_DVB_API_VERSION < 3
	sprintf(filename, "/dev/dvb/card%d/demux%d", demux->adapter, demux->demux);
#else
	sprintf(filename, "/dev/dvb/adapter%d/demux%d", demux->adapter, demux->demux);
#endif
	eDebug("eDVBVideo::eDVBVideo(): m_fd_demux of file %s",filename);
	m_fd_demux = ::open(filename, O_RDWR);
	if (m_fd_demux < 0)
		eWarning("%s: %m", filename);
	eDebug("demux device: %s", filename);
}

// not finally values i think.. !!
#define VIDEO_STREAMTYPE_MPEG2        0
#define VIDEO_STREAMTYPE_MPEG4_H264   1
#define VIDEO_STREAMTYPE_VC1          3
#define VIDEO_STREAMTYPE_MPEG4_Part2  4
#define VIDEO_STREAMTYPE_VC1_SM       5
#define VIDEO_STREAMTYPE_MPEG1        6

#if HAVE_DVB_API_VERSION < 3
int eDVBVideo::setPid(int pid)
{
	if ((m_fd < 0) || (m_fd_demux < 0))
		return -1;
	dmx_pes_filter_params pes;

	pes.pid      = pid;
	pes.input    = DMX_IN_FRONTEND;
	pes.output   = DMX_OUT_DECODER;
	pes.pes_type = m_dev ? DMX_PES_VIDEO1 : DMX_PES_VIDEO0; /* FIXME */
	pes.flags    = 0;
	eDebugNoNewLine("DMX_SET_PES_FILTER(0x%02x) - video - ", pid);
	if (::ioctl(m_fd_demux, DMX_SET_PES_FILTER, &pes) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");
	return 0;
}

int eDVBVideo::startPid()
{
	eDebugNoNewLine("DEMUX_START - video - ");
	if (::ioctl(m_fd_demux, DMX_START) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");
	return 0;
}

int eDVBVideo::start()
{
	eDebugNoNewLine("VIDEO_PLAY - ");
	if (::ioctl(m_fd, VIDEO_PLAY) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");
	return 0;
}

int eDVBVideo::stopPid()
{
	eDebugNoNewLine("DEMUX_STOP - video - ");
	if (::ioctl(m_fd_demux, DMX_STOP) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");
	return 0;
}
#else
int eDVBVideo::startPid(int pid, int type)
{
	int streamtype = VIDEO_STREAMTYPE_MPEG2;

	if ((m_fd < 0) || (m_fd_demux < 0))
		return -1;
	dmx_pes_filter_params pes;

	switch(type)
	{
	default:
	case MPEG2:
		break;
	case MPEG4_H264:
		streamtype = VIDEO_STREAMTYPE_MPEG4_H264;
		break;
	case MPEG1:
		streamtype = VIDEO_STREAMTYPE_MPEG1;
		break;
	case MPEG4_Part2:
		streamtype = VIDEO_STREAMTYPE_MPEG4_Part2;
		break;
	case VC1:
		streamtype = VIDEO_STREAMTYPE_VC1;
		break;
	case VC1_SM:
		streamtype = VIDEO_STREAMTYPE_VC1_SM;
		break;
	}

	eDebugNoNewLine("VIDEO_SET_STREAMTYPE %d - ", streamtype);
	if (::ioctl(m_fd, VIDEO_SET_STREAMTYPE, streamtype) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");

	pes.pid      = pid;
	pes.input    = DMX_IN_FRONTEND;
	pes.output   = DMX_OUT_DECODER;
	pes.pes_type = m_dev ? DMX_PES_VIDEO1 : DMX_PES_VIDEO0; /* FIXME */
	pes.flags    = 0;
	eDebugNoNewLine("DMX_SET_PES_FILTER(0x%02x) - video - ", pid);
	if (::ioctl(m_fd_demux, DMX_SET_PES_FILTER, &pes) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");
	eDebugNoNewLine("DEMUX_START - video - ");
	if (::ioctl(m_fd_demux, DMX_START) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");
	freeze();  // why freeze here?!? this is a problem when only a pid change is requested... because of the unfreeze logic in Decoder::setState
	eDebugNoNewLine("VIDEO_PLAY - ");
	if (::ioctl(m_fd, VIDEO_PLAY) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
	return 0;
}
#endif

void eDVBVideo::stop()
{
#if HAVE_DVB_API_VERSION > 2
	eDebugNoNewLine("DEMUX_STOP - video - ");
	if (::ioctl(m_fd_demux, DMX_STOP) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
#endif
	eDebugNoNewLine("VIDEO_STOP - ");
	if (::ioctl(m_fd, VIDEO_STOP, 1) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
}

void eDVBVideo::flush()
{
	eDebugNoNewLine("VIDEO_CLEAR_BUFFER - ");
	if (::ioctl(m_fd, VIDEO_CLEAR_BUFFER) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
}

void eDVBVideo::freeze()
{
	eDebugNoNewLine("----------> VIDEO_FREEZE - ");
	if (::ioctl(m_fd, VIDEO_FREEZE) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
}

void eDVBVideo::unfreeze()
{
	eDebugNoNewLine("----------> VIDEO_CONTINUE - ");
	if (::ioctl(m_fd, VIDEO_CONTINUE) < 0)
		eDebug("failed (%m)");
	else
		eDebug("ok");
}

int eDVBVideo::setSlowMotion(int repeat)
{
	eDebugNoNewLine("----------> VIDEO_SLOWMOTION(%d) - ", repeat);
	int ret = ::ioctl(m_fd, VIDEO_SLOWMOTION, repeat);
	if (ret < 0)
		eDebug("failed(%m)");
	else
		eDebug("ok");
	return ret;
}

int eDVBVideo::setFastForward(int skip)
{
	eDebugNoNewLine("----------> VIDEO_FAST_FORWARD(%d) - ", skip);
	int ret = ::ioctl(m_fd, VIDEO_FAST_FORWARD, skip);
	if (ret < 0)
		eDebug("failed(%m)");
	else
		eDebug("ok");
	return ret;
}

int eDVBVideo::getPTS(pts_t &now)
{
#if HAVE_DVB_API_VERSION < 3
	#define VIDEO_GET_PTS_OLD           _IOR('o', 1, unsigned int*)
	unsigned int pts;
	int ret = ::ioctl(m_fd_video, VIDEO_GET_PTS_OLD, &pts);
	now = pts;
	now *= 2;
#else
	int ret = ::ioctl(m_fd, VIDEO_GET_PTS, &now);
#endif
// 	if (ret < 0)
// 		eDebug("VIDEO_GET_PTS failed(%m)");
	return ret;
}

eDVBVideo::~eDVBVideo()
{
	if (m_fd >= 0)
		::close(m_fd);
	if (m_fd_demux >= 0)
		::close(m_fd_demux);
#if HAVE_DVB_API_VERSION < 3
	if (m_fd_video >= 0)
		::close(m_fd_video);
#endif
}

void eDVBVideo::video_event(int)
{
#if HAVE_DVB_API_VERSION >= 3
	struct video_event evt;
	eDebugNoNewLine("VIDEO_GET_EVENT - ");
	if (::ioctl(m_fd, VIDEO_GET_EVENT, &evt) < 0)
		eDebug("failed (%m)");
	else
	{
		eDebug("ok");
		if (evt.type == VIDEO_EVENT_SIZE_CHANGED)
		{
			eDebug("VIDEO_GET_EVENT: VIDEO_EVENT_SIZE_CHANGED - ok");
			struct iTSMPEGDecoder::videoEvent event;
			event.type = iTSMPEGDecoder::videoEvent::eventSizeChanged;
			m_aspect = event.aspect = evt.u.size.aspect_ratio == 0 ? 2 : 3;  // convert dvb api to etsi
			m_height = event.height = evt.u.size.h;
			m_width = event.width = evt.u.size.w;
			/* emit */ m_event(event);
		}
		else if (evt.type == VIDEO_EVENT_FRAME_RATE_CHANGED)
		{
			eDebug("VIDEO_GET_EVENT: VIDEO_EVENT_FRAME_RATE_CHANGED - ok");
			struct iTSMPEGDecoder::videoEvent event;
			event.type = iTSMPEGDecoder::videoEvent::eventFrameRateChanged;
			m_framerate = event.framerate = evt.u.frame_rate;
			/* emit */ m_event(event);
		}
		else if (evt.type == 16 /*VIDEO_EVENT_PROGRESSIVE_CHANGED*/)
		{
			eDebug("VIDEO_GET_EVENT: VIDEO_EVENT_PROGRESSIVE_CHANGED - ok");
			struct iTSMPEGDecoder::videoEvent event;
			event.type = iTSMPEGDecoder::videoEvent::eventProgressiveChanged;
			m_progressive = event.progressive = evt.u.frame_rate;
			/* emit */ m_event(event);
		}
		else
			eDebug("unhandled DVBAPI Video Event %d", evt.type);
	}
#else
#warning "FIXMEE!! Video Events not implemented for old api"
#endif
}

RESULT eDVBVideo::connectEvent(const Slot1<void, struct iTSMPEGDecoder::videoEvent> &event, ePtr<eConnection> &conn)
{
	conn = new eConnection(this, m_event.connect(event));
	return 0;
}

static int readMpegProc(const char *str, int decoder)
{
	int val = -1;
	char tmp[64];
	sprintf(tmp, "/proc/stb/vmpeg/%d/%s", decoder, str);
	FILE *f = fopen(tmp, "r");
	if (f)
	{
		fscanf(f, "%x", &val);
		fclose(f);
	}
	return val;
}

static int readApiSize(int fd, int &xres, int &yres, int &aspect)
{
#if HAVE_DVB_API_VERSION >= 3
	video_size_t size;
	if (!::ioctl(fd, VIDEO_GET_SIZE, &size))
	{
		xres = size.w;
		yres = size.h;
		aspect = size.aspect_ratio == 0 ? 2 : 3;  // convert dvb api to etsi
		return 0;
	}
//	eDebug("VIDEO_GET_SIZE failed (%m)");
#endif
	return -1;
}

static int readApiFrameRate(int fd, int &framerate)
{
#if HAVE_DVB_API_VERSION >= 3
	unsigned int frate;
	if (!::ioctl(fd, VIDEO_GET_FRAME_RATE, &frate))
	{
		framerate = frate;
		return 0;
	}
//	eDebug("VIDEO_GET_FRAME_RATE failed (%m)");
#endif
	return -1;
}

int eDVBVideo::getWidth()
{
	if (m_width == -1)
		readApiSize(m_fd, m_width, m_height, m_aspect);
	if (m_width == -1)
		m_width = readMpegProc("xres", m_dev);
	return m_width;
}

int eDVBVideo::getHeight()
{
	if (m_height == -1)
		readApiSize(m_fd, m_width, m_height, m_aspect);
	if (m_height == -1)
		m_height = readMpegProc("yres", m_dev);
	return m_height;
}

int eDVBVideo::getAspect()
{
	if (m_aspect == -1)
		readApiSize(m_fd, m_width, m_height, m_aspect);
	if (m_aspect == -1)
		m_aspect = readMpegProc("aspect", m_dev);
	return m_aspect;
}

int eDVBVideo::getProgressive()
{
	if (m_progressive == -1)
		m_progressive = readMpegProc("progressive", m_dev);
	return m_progressive;
}

int eDVBVideo::getFrameRate()
{
	if (m_framerate == -1)
		readApiFrameRate(m_fd, m_framerate);
	if (m_framerate == -1)
		m_framerate = readMpegProc("framerate", m_dev);
	return m_framerate;
}

DEFINE_REF(eDVBPCR);

eDVBPCR::eDVBPCR(eDVBDemux *demux, int dev): m_demux(demux), m_dev(dev)
{
	char filename[128];
#if HAVE_DVB_API_VERSION < 3
	sprintf(filename, "/dev/dvb/card%d/demux%d", demux->adapter, demux->demux);
#else
	sprintf(filename, "/dev/dvb/adapter%d/demux%d", demux->adapter, demux->demux);
#endif
	eDebug("eDVBPCR::eDVBPCR(): m_fd_demux of file %s",filename);
	m_fd_demux = ::open(filename, O_RDWR);
	if (m_fd_demux < 0)
		eWarning("%s: %m", filename);
}

#if HAVE_DVB_API_VERSION < 3
int eDVBPCR::setPid(int pid)
{
	if (m_fd_demux < 0)
		return -1;
	dmx_pes_filter_params pes;

	pes.pid      = pid;
	pes.input    = DMX_IN_FRONTEND;
	pes.output   = DMX_OUT_DECODER;
	pes.pes_type = DMX_PES_PCR;
	pes.flags    = 0;

	eDebugNoNewLine("DMX_SET_PES_FILTER(0x%02x) - pcr - ", pid);
	if (::ioctl(m_fd_demux, DMX_SET_PES_FILTER, &pes) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");
	return 0;
}

int eDVBPCR::startPid()
{
	if (m_fd_demux < 0)
		return -1;
	eDebugNoNewLine("DEMUX_START - pcr - ");
	if (::ioctl(m_fd_demux, DMX_START) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");
	return 0;
}
#else
int eDVBPCR::startPid(int pid)
{
	if (m_fd_demux < 0)
		return -1;
	dmx_pes_filter_params pes;

	pes.pid      = pid;
	pes.input    = DMX_IN_FRONTEND;
	pes.output   = DMX_OUT_DECODER;
	pes.pes_type = m_dev ? DMX_PES_PCR1 : DMX_PES_PCR0; /* FIXME */
	pes.flags    = 0;
	eDebugNoNewLine("DMX_SET_PES_FILTER(0x%02x) - pcr - ", pid);
	if (::ioctl(m_fd_demux, DMX_SET_PES_FILTER, &pes) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");
	eDebugNoNewLine("DEMUX_START - pcr - ");
	if (::ioctl(m_fd_demux, DMX_START) < 0)
	{
		eDebug("failed (%m)");
		return -errno;
	}
	eDebug("ok");
	return 0;
}
#endif

void eDVBPCR::stop()
{
	eDebugNoNewLine("DEMUX_STOP - pcr - ");
	if (::ioctl(m_fd_demux, DMX_STOP) < 0)
		eDebug("failed(%m)");
	else
		eDebug("ok");
}

eDVBPCR::~eDVBPCR()
{
	if (m_fd_demux >= 0)
		::close(m_fd_demux);
}

DEFINE_REF(eDVBTText);

eDVBTText::eDVBTText(eDVBDemux *demux, int dev)
    :m_demux(demux), m_dev(dev)
{
	char filename[128];
#if HAVE_DVB_API_VERSION < 3
	sprintf(filename, "/dev/dvb/card%d/demux%d", demux->adapter, demux->demux);
#else
	sprintf(filename, "/dev/dvb/adapter%d/demux%d", demux->adapter, demux->demux);
#endif
	m_fd_demux = ::open(filename, O_RDWR);
	if (m_fd_demux < 0)
		eWarning("%s: %m", filename);
}

int eDVBTText::startPid(int pid)
{
	if (m_fd_demux < 0)
		return -1;
	dmx_pes_filter_params pes;

	pes.pid      = pid;
	pes.input    = DMX_IN_FRONTEND;
	pes.output   = DMX_OUT_DECODER;
	pes.pes_type = m_dev ? DMX_PES_TELETEXT1 : DMX_PES_TELETEXT0; // FIXME
	pes.flags    = 0;

	eDebugNoNewLine("DMX_SET_PES_FILTER(0x%02x) - ttx - ", pid);
	if (::ioctl(m_fd_demux, DMX_SET_PES_FILTER, &pes) < 0)
	{
		eDebug("failed(%m)");
		return -errno;
	}
	eDebug("ok");
	eDebugNoNewLine("DEMUX_START - ttx - ");
	if (::ioctl(m_fd_demux, DMX_START) < 0)
	{
		eDebug("failed(%m)");
		return -errno;
	}
	eDebug("ok");
	return 0;
}

void eDVBTText::stop()
{
	eDebugNoNewLine("DEMUX_STOP - ttx - ");
	if (::ioctl(m_fd_demux, DMX_STOP) < 0)
		eDebug("failed(%m)");
	else
		eDebug("ok");
}

eDVBTText::~eDVBTText()
{
	if (m_fd_demux >= 0)
		::close(m_fd_demux);
}

DEFINE_REF(eTSMPEGDecoder);

int eTSMPEGDecoder::setState()
{
	int res = 0;

	int noaudio = (m_state != statePlay) && (m_state != statePause);
	int nott = noaudio; /* actually same conditions */

	if ((noaudio && m_audio) || (!m_audio && !noaudio))
		m_changed |= changeAudio | changeState;

	if ((nott && m_text) || (!m_text && !nott))
		m_changed |= changeText | changeState;

#ifdef QBOXHD
	const char *decoder_states[] = {"stop", "pause", "play", "decoderfastforward", "trickmode", "slowmotion"};
	eDebug("eTSMPEGDecoder::%s(): %s, dev='/dev/dvb/adapter%d/demux%d' vpid 0x%02x, apid 0x%02x", __func__, decoder_states[m_state], m_demux->getAdapter(), m_demux->getDemux(),m_vpid, m_apid);
#endif

	bool changed = m_changed;
#if HAVE_DVB_API_VERSION < 3
	bool checkAVSync = m_changed & (changeAudio|changeVideo|changePCR);
	if (m_changed & changeAudio && m_audio)
		m_audio->stopPid();
	if (m_changed & changeVideo && m_video)
		m_video->stopPid();
	if (m_changed & changePCR && m_pcr)
	{
		m_pcr->stop();
		m_pcr=0;
		if (!(m_pcrpid >= 0 && m_pcrpid < 0x1ff))
			m_changed &= ~changePCR;
	}
	if (m_changed & changeAudio && m_audio)
	{
		m_audio->stop();
		m_audio=0;
		if (!(m_apid >= 0 && m_apid < 0x1ff))
			m_changed &= ~changeAudio;
	}
	if (m_changed & changeVideo && m_video)
	{
		m_video->stop();
		m_video=0;
		m_video_event_conn=0;
		if (!(m_vpid >= 0 && m_vpid < 0x1ff))
			m_changed &= ~changeVideo;
	}
	if (m_changed & changeVideo)
	{
		m_video = new eDVBVideo(m_demux, m_decoder);
		m_video->connectEvent(slot(*this, &eTSMPEGDecoder::video_event), m_video_event_conn);
		if (m_video->setPid(m_vpid))
			res -1;
	}
	if (m_changed & changePCR)
	{
		m_pcr = new eDVBPCR(m_demux);
		if (m_pcr->setPid(m_pcrpid))
			res = -1;
	}
	if (m_changed & changeAudio)
	{
		m_audio = new eDVBAudio(m_demux, m_decoder);
		if (m_audio->setPid(m_apid, m_atype))
			res = -1;
	}
	if (m_changed & changePCR)
	{
		if (m_pcr->startPid())
			res = -1;
		m_changed &= ~changePCR;
	}
	else if (checkAVSync && m_audio && m_video)
	{
		if (m_audio->setAVSync(1))
			res = -1;
	}
	if (m_changed & changeVideo)
	{
		if (m_video->startPid() || m_video->start())
			res = -1;
		m_changed &= ~changeVideo;
	}
	if (m_changed & changeAudio)
	{
		if (m_audio->start() || m_audio->startPid())
			res = -1;
		m_changed &= ~changeAudio;
	}
#else ///!HAVE_DVB_API_VERSION < 3

	if (m_changed & changePCR)
	{
		if (m_pcr)
			m_pcr->stop();
		m_pcr = 0;
	}
	if (m_changed & changeVideo)
	{
		if (m_video)
		{
			m_video->stop();
			m_video = 0;
			m_video_event_conn = 0;
		}
	}
	if (m_changed & changeAudio)
	{
		if (m_audio)
			m_audio->stop();
		m_audio = 0;
	}
	if (m_changed & changeText)
	{
		if (m_text)
			m_text->stop();
		m_text = 0;
	}
	if (m_changed & changePCR)
	{
		if ((m_pcrpid >= 0) && (m_pcrpid < 0x1FFF))
		{
			m_pcr = new eDVBPCR(m_demux, m_decoder);
			if (m_pcr->startPid(m_pcrpid))
				res = -1;
		}
		m_changed &= ~changePCR;
	}
	if (m_changed & changeAudio)
	{
		if ((m_apid >= 0) && (m_apid < 0x1FFF) && !noaudio)
		{
			m_audio = new eDVBAudio(m_demux, m_decoder);
			if (m_audio->startPid(m_apid, m_atype))
				res = -1;
		}
		m_changed &= ~changeAudio;
	}
	if (m_changed & changeVideo)
	{
		if ((m_vpid >= 0) && (m_vpid < 0x1FFF))
		{
			m_video = new eDVBVideo(m_demux, m_decoder);
			m_video->connectEvent(slot(*this, &eTSMPEGDecoder::video_event), m_video_event_conn);
			if (m_video->startPid(m_vpid, m_vtype))
				res = -1;
		}
		m_changed &= ~changeVideo;
	}
	if (m_changed & changeText)
	{
		if ((m_textpid >= 0) && (m_textpid < 0x1FFF) && !nott)
		{
			m_text = new eDVBTText(m_demux, m_decoder);
			if (m_text->startPid(m_textpid))
				res = -1;
		}
		m_changed &= ~changeText;
	}
// eDebug("eTSMPEGDecoder::%s(): %s 1.--------------",__func__, decoder_states[m_state]);


#endif ///HAVE_DVB_API_VERSION < 3

	if (changed & (changeState|changeVideo|changeAudio))
	{
					/* play, slowmotion, fast-forward */
		int state_table[6][4] =
			{
				/* [stateStop] =                 */ {0, 0, 0},
				/* [statePause] =                */ {0, 0, 0},
				/* [statePlay] =                 */ {1, 0, 0},
				/* [stateDecoderFastForward] =   */ {1, 0, m_ff_sm_ratio},
				/* [stateHighspeedFastForward] = */ {1, 0, 1},
				/* [stateSlowMotion] =           */ {1, m_ff_sm_ratio, 0}
			};
		int *s = state_table[m_state];

		eDebug("eTSMPEGDecoder::%s(): m_state=%d - changed=%d, changeState=%d,changeVideo=%d,changeAudio=%d.",
        		 __func__,m_state,changed,changeState,changeVideo,changeAudio);

		if (changed & (changeState|changeVideo) && m_video)
		{
 			eDebug("eTSMPEGDecoder::%s(): changeState|changeVideo", __func__);
#if not defined QBOXHD && not defined QBOXHD_MINI
#if not defined(__sh__)
			m_video->setSlowMotion(s[1]);
			m_video->setFastForward(s[2]);
#endif
			if (s[0])
				m_video->unfreeze();
			else
				m_video->freeze();
#if defined(__sh__)
			/* the VIDEO_CONTINUE would reset the FASTFORWARD  command so we
			execute the FASTFORWARD after the VIDEO_CONTINUE */
			m_video->setSlowMotion(s[1]);
			m_video->setFastForward(s[2]);
#endif
#else  // QBOXHD
			if (s[0])
			{
				m_video->unfreeze();
				m_video->setSlowMotion(s[1]);
				m_video->setFastForward(s[2]);
			}
			else
			{
				m_video->setSlowMotion(s[1]);
				m_video->setFastForward(s[2]);
				m_video->freeze();
			}
#endif  // QBOXHD
		}

		if (changed & (changeState|changeAudio) && m_audio)
		{
 			eDebug("eTSMPEGDecoder::%s(): changeState|changeAudio", __func__);
			if (s[0])
				m_audio->unfreeze();
			else
				m_audio->freeze();
		}

		m_changed &= ~changeState;
	}
	eDebug("eTSMPEGDecoder::%s(): - freezed.", __func__);

	if (changed && !m_video && m_audio && m_radio_pic.length())
		showSinglePic(m_radio_pic.c_str());

	eDebug("eTSMPEGDecoder::%s(): - DONE.", __func__);
	return res;
}

int eTSMPEGDecoder::m_pcm_delay=-1,
	eTSMPEGDecoder::m_ac3_delay=-1;

RESULT eTSMPEGDecoder::setPCMDelay(int delay)
{
	if (m_decoder == 0 && delay != m_pcm_delay )
	{
		FILE *fp = fopen("/proc/stb/audio/audio_delay_pcm", "w");
		if (fp)
		{
			fprintf(fp, "%x", delay*90);
			fclose(fp);
			m_pcm_delay = delay;
			return 0;
		}
	}
	return -1;
}

RESULT eTSMPEGDecoder::setAC3Delay(int delay)
{
	if ( m_decoder == 0 && delay != m_ac3_delay )
	{
		FILE *fp = fopen("/proc/stb/audio/audio_delay_bitstream", "w");
		if (fp)
		{
			fprintf(fp, "%x", delay*90);
			fclose(fp);
			m_ac3_delay = delay;
			return 0;
		}
	}
	return -1;
}

eTSMPEGDecoder::eTSMPEGDecoder(eDVBDemux *demux, int decoder)
	: m_demux(demux),
		m_vpid(-1), m_vtype(-1), m_apid(-1), m_atype(-1), m_pcrpid(-1), m_textpid(-1),
		m_changed(0), m_decoder(decoder), m_video_clip_fd(-1), m_showSinglePicTimer(eTimer::create(eApp))
{
	demux->connectEvent(slot(*this, &eTSMPEGDecoder::demux_event), m_demux_event_conn);
	CONNECT(m_showSinglePicTimer->timeout, eTSMPEGDecoder::finishShowSinglePic);
	m_state = stateStop;
}

eTSMPEGDecoder::~eTSMPEGDecoder()
{
	finishShowSinglePic();
	m_vpid = m_apid = m_pcrpid = m_textpid = pidNone;
	m_changed = -1;
	setState();
}

RESULT eTSMPEGDecoder::setVideoPID(int vpid, int type)
{
	if ((m_vpid != vpid) || (m_vtype != type))
	{
		m_changed |= changeVideo;
		m_vpid = vpid;
		m_vtype = type;
	}
	return 0;
}

RESULT eTSMPEGDecoder::setAudioPID(int apid, int type)
{
	if ((m_apid != apid) || (m_atype != type))
	{
		m_changed |= changeAudio;
		m_atype = type;
		m_apid = apid;
	}
	return 0;
}

int eTSMPEGDecoder::m_audio_channel = -1;

RESULT eTSMPEGDecoder::setAudioChannel(int channel)
{
	if (channel == -1)
		channel = ac_stereo;
	if (m_decoder == 0 && m_audio_channel != channel)
	{
		if (m_audio)
		{
			m_audio->setChannel(channel);
			m_audio_channel=channel;
		}
		else
			eDebug("eTSMPEGDecoder::setAudioChannel but no audio decoder exist");
	}
	return 0;
}

int eTSMPEGDecoder::getAudioChannel()
{
	return m_audio_channel == -1 ? ac_stereo : m_audio_channel;
}

RESULT eTSMPEGDecoder::setSyncPCR(int pcrpid)
{
	if (m_pcrpid != pcrpid)
	{
		m_changed |= changePCR;
		m_pcrpid = pcrpid;
	}
	return 0;
}

RESULT eTSMPEGDecoder::setTextPID(int textpid)
{
	if (m_textpid != textpid)
	{
		m_changed |= changeText;
		m_textpid = textpid;
	}
	return 0;
}

RESULT eTSMPEGDecoder::setSyncMaster(int who)
{
	return -1;
}

RESULT eTSMPEGDecoder::set()
{
	return setState();
}

RESULT eTSMPEGDecoder::play()
{
	if (m_state == statePlay)
	{
		if (!m_changed)
			return 0;
	}
	else
	{
		m_state = statePlay;
		m_changed |= changeState;
	}
//     eDebug("eTSMPEGDecoder::play() calling setState()");
	return setState();
}

RESULT eTSMPEGDecoder::pause()
{
// eDebug("eTSMPEGDecoder::pause() called m_state=%d",m_state);
	if (m_state == statePause)
	{
// eDebug("eTSMPEGDecoder::pause() still in statePause nothing DONE.");
		return 0;
	}
	m_state = statePause;
	m_changed |= changeState;

// eDebug("eTSMPEGDecoder::pause() SETTING NEW PAUSE STATE - m_state=%d",m_state);
	return setState();
}

RESULT eTSMPEGDecoder::setFastForward(int frames_to_skip)
{
eDebug("eTSMPEGDecoder::setFastForward(): frames_to_skip=%d",frames_to_skip);
	if ((m_state == stateDecoderFastForward) && (m_ff_sm_ratio == frames_to_skip))
		return 0;

	m_state = stateDecoderFastForward;
	m_ff_sm_ratio = frames_to_skip;
	m_changed |= changeState;

eDebug("eTSMPEGDecoder::setFastForward(): m_state=%d, m_ff_sm_ratio=%d, m_changed=%d",m_state, m_ff_sm_ratio, m_changed);
	return setState();

//		return m_video->setFastForward(frames_to_skip);
}

RESULT eTSMPEGDecoder::setSlowMotion(int repeat)
{
	if ((m_state == stateSlowMotion) && (m_ff_sm_ratio == repeat))
		return 0;

	m_state = stateSlowMotion;
	m_ff_sm_ratio = repeat;
	m_changed |= changeState;
	return setState();
}

RESULT eTSMPEGDecoder::setTrickmode()
{
	if (m_state == stateTrickmode)
		return 0;

	m_state = stateTrickmode;
	m_changed |= changeState;
	return setState();
}

RESULT eTSMPEGDecoder::flush()
{
	if (m_audio)
		m_audio->flush();
	if (m_video)
		m_video->flush();
	return 0;
}

void eTSMPEGDecoder::demux_event(int event)
{
	switch (event)
	{
		case eDVBDemux::evtFlush:
			flush();
			break;
		default:
			break;
	}
}

RESULT eTSMPEGDecoder::getPTS(int what, pts_t &pts)
{
	if (what == 0) /* auto */
		what = m_video ? 1 : 2;

	if (what == 1) /* video */
	{
		if (m_video)
			return m_video->getPTS(pts);
		else
			return -1;
	}

	if (what == 2) /* audio */
	{
		if (m_audio)
			return m_audio->getPTS(pts);
		else
			return -1;
	}

	return -1;
}

RESULT eTSMPEGDecoder::setRadioPic(const std::string &filename)
{
	m_radio_pic = filename;
	return 0;
}

RESULT eTSMPEGDecoder::showSinglePic(const char *filename)
{
	if (m_decoder == 0)
	{
		eDebug("showSinglePic %s", filename);
		int f = open(filename, O_RDONLY);
		if (f >= 0)
		{
			struct stat s;
			fstat(f, &s);
			if (m_video_clip_fd == -1)
				m_video_clip_fd = open("/dev/dvb/adapter0/video0", O_WRONLY|O_NONBLOCK);
			if (m_video_clip_fd >= 0)
			{
				bool seq_end_avail = false;
				size_t pos=0;
				unsigned char pes_header[] = { 0x00, 0x00, 0x01, 0xE0, 0x00, 0x00, 0x80, 0x00, 0x00 };
				unsigned char seq_end[] = { 0x00, 0x00, 0x01, 0xB7 };
				unsigned char iframe[s.st_size];
				unsigned char stuffing[8192];
				int streamtype = VIDEO_STREAMTYPE_MPEG2;
				memset(stuffing, 0, 8192);
				read(f, iframe, s.st_size);
				if (ioctl(m_video_clip_fd, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_MEMORY) < 0)
					eDebug("VIDEO_SELECT_SOURCE MEMORY failed (%m)");
				if (ioctl(m_video_clip_fd, VIDEO_SET_STREAMTYPE, streamtype) < 0)
					eDebug("VIDEO_SET_STREAMTYPE failed(%m)");
				if (ioctl(m_video_clip_fd, VIDEO_PLAY) < 0)
					eDebug("VIDEO_PLAY failed (%m)");
				if (ioctl(m_video_clip_fd, VIDEO_CONTINUE) < 0)
					eDebug("video: VIDEO_CONTINUE: %m");
				if (ioctl(m_video_clip_fd, VIDEO_CLEAR_BUFFER) < 0)
					eDebug("video: VIDEO_CLEAR_BUFFER: %m");
				while(pos <= (s.st_size-4) && !(seq_end_avail = (!iframe[pos] && !iframe[pos+1] && iframe[pos+2] == 1 && iframe[pos+3] == 0xB7)))
					++pos;
				if ((iframe[3] >> 4) != 0xE) // no pes header
					write(m_video_clip_fd, pes_header, sizeof(pes_header));
				else
					iframe[4] = iframe[5] = 0x00;
				write(m_video_clip_fd, iframe, s.st_size);
				if (!seq_end_avail)
					write(m_video_clip_fd, seq_end, sizeof(seq_end));
				write(m_video_clip_fd, stuffing, 8192);

#ifdef QBOXHD
				/* If the radio mode is active, no set timer */
				if(!(!m_video && m_audio && m_radio_pic.length()))
				{
					m_showSinglePicTimer->start(150, true);
				}
#else
				m_showSinglePicTimer->start(150, true);
#endif
			}
			close(f);
		}
		else
		{
			eDebug("couldnt open %s", filename);
			return -1;
		}
	}
	else
	{
		eDebug("only show single pics on first decoder");
		return -1;
	}
	return 0;
}

void eTSMPEGDecoder::finishShowSinglePic()
{
	if (m_video_clip_fd >= 0)
	{
		if (ioctl(m_video_clip_fd, VIDEO_STOP, 0) < 0)
			eDebug("VIDEO_STOP failed (%m)");
		if (ioctl(m_video_clip_fd, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_DEMUX) < 0)
				eDebug("VIDEO_SELECT_SOURCE DEMUX failed (%m)");
		close(m_video_clip_fd);
		m_video_clip_fd = -1;
	}
}

RESULT eTSMPEGDecoder::connectVideoEvent(const Slot1<void, struct videoEvent> &event, ePtr<eConnection> &conn)
{
	conn = new eConnection(this, m_video_event.connect(event));
	return 0;
}

void eTSMPEGDecoder::video_event(struct videoEvent event)
{
	/* emit */ m_video_event(event);
}

int eTSMPEGDecoder::getVideoWidth()
{
	if (m_video)
		return m_video->getWidth();
	return -1;
}

int eTSMPEGDecoder::getVideoHeight()
{
	if (m_video)
		return m_video->getHeight();
	return -1;
}

int eTSMPEGDecoder::getVideoProgressive()
{
	if (m_video)
		return m_video->getProgressive();
	return -1;
}

int eTSMPEGDecoder::getVideoFrameRate()
{
	if (m_video)
		return m_video->getFrameRate();
	return -1;
}

int eTSMPEGDecoder::getVideoAspect()
{
	if (m_video)
		return m_video->getAspect();
	return -1;
}
