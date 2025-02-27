#ifndef __decoder_h
#define __decoder_h

#include <lib/base/object.h>
#include <lib/dvb/demux.h>
#include <lib/dvb/feeder.h>
#include <lib/dvb/volume.h>

#define MAX_DVB_ADAPTER     4

class eSocketNotifier;

class eDVBAudio: public iObject
{
	DECLARE_REF(eDVBAudio);
private:
	ePtr<eDVBDemux> m_demux;
	int m_fd, m_fd_demux, m_dev, m_is_freezed;
#ifdef QBOXHD
	int m_adapter;
    eDVBVolumecontrol *m_volume;
    struct feeder_cmd_st m_feeder;
#endif
public:
	enum { aMPEG, aAC3, aDTS, aAAC, aAACHE, aLPCM };
	eDVBAudio(eDVBDemux *demux, int dev);
	enum { aMonoLeft, aStereo, aMonoRight };
	void setChannel(int channel);
	void stop();
#if HAVE_DVB_API_VERSION < 3
	int setPid(int pid, int type);
	int startPid();
	int start();
	int stopPid();
	int setAVSync(int val);
#else
	int startPid(int pid, int type);
#endif
	void flush();
	void freeze();
	void unfreeze();
	int getPTS(pts_t &now);
	virtual ~eDVBAudio();
};

class eDVBVideo: public iObject, public Object
{
	DECLARE_REF(eDVBVideo);
private:
	ePtr<eDVBDemux> m_demux;
	int m_fd, m_fd_demux, m_dev;
#ifdef QBOXHD
	int m_adapter;
	struct feeder_cmd_st m_feeder;
#endif
#if HAVE_DVB_API_VERSION < 3
	m_fd_video;
#endif
	int m_is_slow_motion, m_is_fast_forward, m_is_freezed;
	ePtr<eSocketNotifier> m_sn;
	void video_event(int what);
	Signal1<void, struct iTSMPEGDecoder::videoEvent> m_event;
	int m_width, m_height, m_framerate, m_aspect, m_progressive;
public:
	enum { MPEG2, MPEG4_H264, MPEG1, MPEG4_Part2, VC1, VC1_SM };
	eDVBVideo(eDVBDemux *demux, int dev);
	void stop();
#if HAVE_DVB_API_VERSION < 3
	int setPid(int pid);
	int startPid();
	int start();
	int stopPid();
#else
	int startPid(int pid, int type=MPEG2);
#endif
	void flush();
	void freeze();
	int setSlowMotion(int repeat);
	int setFastForward(int skip);
	void unfreeze();
	int getPTS(pts_t &now);
	virtual ~eDVBVideo();
	RESULT connectEvent(const Slot1<void, struct iTSMPEGDecoder::videoEvent> &event, ePtr<eConnection> &conn);
	int getWidth();
	int getHeight();
	int getProgressive();
	int getFrameRate();
	int getAspect();
};

class eDVBPCR: public iObject
{
	DECLARE_REF(eDVBPCR);
private:
	ePtr<eDVBDemux> m_demux;
	int m_fd_demux, m_dev;
public:
	eDVBPCR(eDVBDemux *demux, int dev);
#if HAVE_DVB_API_VERSION < 3
	int setPid(int pid);
	int startPid();
#else
	int startPid(int pid);
#endif
	void stop();
	virtual ~eDVBPCR();
};

class eDVBTText: public iObject
{
	DECLARE_REF(eDVBTText);
private:
	ePtr<eDVBDemux> m_demux;
	int m_fd_demux, m_dev;
#ifdef QBOXHD
    int m_adapter;
#endif
public:
	eDVBTText(eDVBDemux *demux, int dev);
	int startPid(int pid);
	void stop();
	virtual ~eDVBTText();
};

class eTSMPEGDecoder: public Object, public iTSMPEGDecoder
{
	DECLARE_REF(eTSMPEGDecoder);
private:
	static int m_pcm_delay;
	static int m_ac3_delay;
	static int m_audio_channel;
	std::string m_radio_pic;
	ePtr<eDVBDemux> m_demux;
	ePtr<eDVBAudio> m_audio;
	ePtr<eDVBVideo> m_video;
	ePtr<eDVBPCR> m_pcr;
	ePtr<eDVBTText> m_text;
	int m_vpid, m_vtype, m_apid, m_atype, m_pcrpid, m_textpid;
	enum
	{
		changeVideo = 1,
		changeAudio = 2,
		changePCR   = 4,
		changeText  = 8,
		changeState = 16,
	};
	int m_changed, m_decoder;
	int m_state;
	int m_ff_sm_ratio;
	int setState();
	ePtr<eConnection> m_demux_event_conn;
	ePtr<eConnection> m_video_event_conn;

	void demux_event(int event);
	void video_event(struct videoEvent);
	Signal1<void, struct videoEvent> m_video_event;
	int m_video_clip_fd;
	ePtr<eTimer> m_showSinglePicTimer;
	void finishShowSinglePic(); // called by timer
public:
	enum { pidNone = -1 };
	eTSMPEGDecoder(eDVBDemux *demux, int decoder);
	virtual ~eTSMPEGDecoder();
	RESULT setVideoPID(int vpid, int type);
	RESULT setAudioPID(int apid, int type);
	RESULT setAudioChannel(int channel);
	int getAudioChannel();
	RESULT setPCMDelay(int delay);
	int getPCMDelay() { return m_pcm_delay; }
	RESULT setAC3Delay(int delay);
	int getAC3Delay() { return m_ac3_delay; }
	RESULT setSyncPCR(int pcrpid);
	RESULT setTextPID(int textpid);
	RESULT setSyncMaster(int who);

		/*
		The following states exist:

		 - stop: data source closed, no playback
		 - pause: data source active, decoder paused
		 - play: data source active, decoder consuming
		 - decoder fast forward: data source linear, decoder drops frames
		 - trickmode, highspeed reverse: data source fast forwards / reverses, decoder just displays frames as fast as it can
		 - slow motion: decoder displays frames multiple times
		*/
	enum {
		stateStop,
		statePause,
		statePlay,
		stateDecoderFastForward,
		stateTrickmode,
		stateSlowMotion
	};
	RESULT set(); /* just apply settings, keep state */
	RESULT play(); /* -> play */
	RESULT pause(); /* -> pause */
	RESULT setFastForward(int frames_to_skip); /* -> decoder fast forward */
	RESULT setSlowMotion(int repeat); /* -> slow motion **/
	RESULT setTrickmode(); /* -> highspeed fast forward */

	RESULT flush();

	RESULT showSinglePic(const char *filename);


	RESULT setRadioPic(const std::string &filename);
		/* what 0=auto, 1=video, 2=audio. */
	RESULT getPTS(int what, pts_t &pts);
	RESULT connectVideoEvent(const Slot1<void, struct videoEvent> &event, ePtr<eConnection> &connection);
	int getVideoWidth();
	int getVideoHeight();
	int getVideoProgressive();
	int getVideoFrameRate();
	int getVideoAspect();
};

#endif
