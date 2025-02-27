/*
 * stm_ioctls.h
 *
 * Copyright (C) STMicroelectronics Limited 2005. All rights reserved.
 *
 * Extensions to the LinuxDVB API (v3) implemented by the Havana implemenation.
 */

#ifndef H_STM_IOCTLS
#define H_STM_IOCTLS

#define DVB_SPEED_NORMAL_PLAY           1000
#define DVB_SPEED_STOPPED               0
#define DVB_SPEED_REVERSE_STOPPED       0x80000000
#define DVB_FRAME_RATE_MULTIPLIER       1000

#define VIDEO_FULL_SCREEN               (VIDEO_CENTER_CUT_OUT+1)

#define DMX_FILTER_BY_PRIORITY_LOW      0x00010000      /* These flags tell the transport pes filter whether to filter */
#define DMX_FILTER_BY_PRIORITY_HIGH     0x00020000      /* using the ts priority bit and, if so, whether to filter on */
#define DMX_FILTER_BY_PRIORITY_MASK     0x00030000      /* bit set or bit clear */


/*
 * Extra events
 */

#define VIDEO_EVENT_FIRST_FRAME_ON_DISPLAY      5               /*(VIDEO_EVENT_VSYNC+1)*/
#define VIDEO_EVENT_FRAME_DECODED_LATE          (VIDEO_EVENT_FIRST_FRAME_ON_DISPLAY+1)
#define VIDEO_EVENT_DATA_DELIVERED_LATE         (VIDEO_EVENT_FRAME_DECODED_LATE+1)
#define VIDEO_EVENT_STREAM_UNPLAYABLE           (VIDEO_EVENT_DATA_DELIVERED_LATE+1)
#define VIDEO_EVENT_TRICK_MODE_CHANGE           (VIDEO_EVENT_STREAM_UNPLAYABLE+1)
#define VIDEO_EVENT_FATAL_ERROR                 (VIDEO_EVENT_TRICK_MODE_CHANGE+1)

/*
 * List of possible container types - used to select demux..  If stream_source is VIDEO_SOURCE_DEMUX
 * then default is TRANSPORT, if stream_source is VIDEO_SOURCE_MEMORY then default is PES
 */
typedef enum {
	STREAM_TYPE_NONE,     /* Deprecated */
	STREAM_TYPE_TRANSPORT,/* Use latest PTI driver so it can be Deprecated */
	STREAM_TYPE_PES,
	STREAM_TYPE_ES,       /* Deprecated */
	STREAM_TYPE_PROGRAM,  /* Deprecated */
	STREAM_TYPE_SYSTEM,   /* Deprecated */
	STREAM_TYPE_SPU,      /* Deprecated */
	STREAM_TYPE_NAVI,     /* Deprecated */
	STREAM_TYPE_CSS,      /* Deprecated */
	STREAM_TYPE_AVI,      /* Deprecated */
	STREAM_TYPE_MP3,      /* Deprecated */
	STREAM_TYPE_H264,     /* Deprecated */
	STREAM_TYPE_ASF,      /* Needs work so it can be deprecated */
	STREAM_TYPE_MP4,      /* Deprecated */
	STREAM_TYPE_RAW,      /* Deprecated */
} stream_type_t;

/*
 * List of possible video encodings - used to select frame parser and codec.
 */
typedef enum {
	VIDEO_ENCODING_AUTO,
	VIDEO_ENCODING_MPEG1,
	VIDEO_ENCODING_MPEG2,
	VIDEO_ENCODING_MJPEG,
	VIDEO_ENCODING_DIVX3,
	VIDEO_ENCODING_DIVX4,
	VIDEO_ENCODING_DIVX5,
	VIDEO_ENCODING_MPEG4P2,
	VIDEO_ENCODING_H264,
	VIDEO_ENCODING_WMV,
	VIDEO_ENCODING_VC1,
	VIDEO_ENCODING_RAW,
	VIDEO_ENCODING_H263,
	VIDEO_ENCODING_FLV1,
	VIDEO_ENCODING_VP6,
	VIDEO_ENCODING_NONE,
	VIDEO_ENCODING_PRIVATE
} video_encoding_t;


/*
 * List of possible audio encodings - used to select frame parser and codec.
 */
typedef enum {
	AUDIO_ENCODING_AUTO,
	AUDIO_ENCODING_PCM,
	AUDIO_ENCODING_LPCM,
	AUDIO_ENCODING_MPEG1,
	AUDIO_ENCODING_MPEG2,
	AUDIO_ENCODING_MP3,
	AUDIO_ENCODING_AC3,
	AUDIO_ENCODING_DTS,
	AUDIO_ENCODING_AAC,
	AUDIO_ENCODING_WMA,
	AUDIO_ENCODING_RAW,
	AUDIO_ENCODING_LPCMA,
	AUDIO_ENCODING_LPCMH,
	AUDIO_ENCODING_LPCMB,
	AUDIO_ENCODING_SPDIF, /*<! Data coming through SPDIF link :: compressed or PCM data */
	AUDIO_ENCODING_DTS_LBR,
	AUDIO_ENCODING_MLP,
	AUDIO_ENCODING_NONE,
	AUDIO_ENCODING_PRIVATE
} audio_encoding_t;

/*
 * List of possible sources for SP/DIF output.
 */
typedef enum audio_spdif_source {
	AUDIO_SPDIF_SOURCE_PP,  /*<! normal decoder output */
	AUDIO_SPDIF_SOURCE_DEC, /*<! decoder output w/o post-proc */
	AUDIO_SPDIF_SOURCE_ES,  /*<! raw elementary stream data */
} audio_spdif_source_t;

typedef struct {
	int x;
	int y;
	int width;
	int height;
} video_window_t;

typedef enum
{
    DVB_DISCONTINUITY_SKIP                = 0x01,
    DVB_DISCONTINUITY_CONTINUOUS_REVERSE  = 0x02,
    DVB_DISCONTINUITY_SURPLUS_DATA        = 0x04
} dvb_discontinuity_t;

/*
 * audio discontinuity
 */
typedef enum {
	AUDIO_DISCONTINUITY_SKIP                = DVB_DISCONTINUITY_SKIP,
	AUDIO_DISCONTINUITY_CONTINUOUS_REVERSE  = DVB_DISCONTINUITY_CONTINUOUS_REVERSE,
	AUDIO_DISCONTINUITY_SURPLUS_DATA        = DVB_DISCONTINUITY_SURPLUS_DATA,
} audio_discontinuity_t;

/*
 * video discontinuity
 */
typedef enum {
	VIDEO_DISCONTINUITY_SKIP                = DVB_DISCONTINUITY_SKIP,
	VIDEO_DISCONTINUITY_CONTINUOUS_REVERSE  = DVB_DISCONTINUITY_CONTINUOUS_REVERSE,
	VIDEO_DISCONTINUITY_SURPLUS_DATA        = DVB_DISCONTINUITY_SURPLUS_DATA,
} video_discontinuity_t;

#define DVB_TIME_NOT_BOUNDED            0xfedcba9876543210ULL

typedef struct dvb_play_interval_s {
	unsigned long long              start;
	unsigned long long              end;
}dvb_play_interval_t;

typedef dvb_play_interval_t             video_play_interval_t;
typedef dvb_play_interval_t             audio_play_interval_t;

typedef struct dvb_play_time_s {
	unsigned long long              system_time;
	unsigned long long              presentation_time;
	unsigned long long              pts;
}dvb_play_time_t;

typedef dvb_play_time_t                 video_play_time_t;
typedef dvb_play_time_t                 audio_play_time_t;


typedef enum {
#define DVB_OPTION_VALUE_DISABLE                                                    0
#define DVB_OPTION_VALUE_ENABLE                                                     1

    DVB_OPTION_TRICK_MODE_AUDIO               = 0,
    DVB_OPTION_PLAY_24FPS_VIDEO_AT_25FPS,

#define DVB_OPTION_VALUE_VIDEO_CLOCK_MASTER                                         0
#define DVB_OPTION_VALUE_AUDIO_CLOCK_MASTER                                         1
#define DVB_OPTION_VALUE_SYSTEM_CLOCK_MASTER                                        2
    DVB_OPTION_MASTER_CLOCK,

    DVB_OPTION_EXTERNAL_TIME_MAPPING,
    DVB_OPTION_AV_SYNC,
    DVB_OPTION_DISPLAY_FIRST_FRAME_EARLY,
    DVB_OPTION_VIDEO_BLANK,
    DVB_OPTION_STREAM_ONLY_KEY_FRAMES,
    DVB_OPTION_STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES,
    DVB_OPTION_CLAMP_PLAYBACK_INTERVAL_ON_PLAYBACK_DIRECTION_CHANGE,

#define DVB_OPTION_VALUE_PLAYOUT                                                    0
#define DVB_OPTION_VALUE_DISCARD                                                    1
    DVB_OPTION_PLAYOUT_ON_TERMINATE,
    DVB_OPTION_PLAYOUT_ON_SWITCH,
    DVB_OPTION_PLAYOUT_ON_DRAIN,

    DVB_OPTION_VIDEO_ASPECT_RATIO,
    DVB_OPTION_VIDEO_DISPLAY_FORMAT,

#define DVB_OPTION_VALUE_TRICK_MODE_AUTO                                            0
#define DVB_OPTION_VALUE_TRICK_MODE_DECODE_ALL                                      1
#define DVB_OPTION_VALUE_TRICK_MODE_DECODE_ALL_DEGRADE_NON_REFERENCE_FRAMES         2
#define DVB_OPTION_VALUE_TRICK_MODE_START_DISCARDING_NON_REFERENCE_FRAMES           3
#define DVB_OPTION_VALUE_TRICK_MODE_DECODE_REFERENCE_FRAMES_DEGRADE_NON_KEY_FRAMES  4
#define DVB_OPTION_VALUE_TRICK_MODE_DECODE_KEY_FRAMES                               5
#define DVB_OPTION_VALUE_TRICK_MODE_DISCONTINUOUS_KEY_FRAMES                        6
    DVB_OPTION_TRICK_MODE_DOMAIN,

#define DVB_OPTION_VALUE_DISCARD_LATE_FRAMES_NEVER                                  0
#define DVB_OPTION_VALUE_DISCARD_LATE_FRAMES_ALWAYS                                 1
#define DVB_OPTION_VALUE_DISCARD_LATE_FRAMES_AFTER_SYNCHRONIZE                      2
    DVB_OPTION_DISCARD_LATE_FRAMES,
    DVB_OPTION_VIDEO_START_IMMEDIATE,
    DVB_OPTION_REBASE_ON_DATA_DELIVERY_LATE,
    DVB_OPTION_REBASE_ON_FRAME_DECODE_LATE,
    DVB_OPTION_LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE,
    DVB_OPTION_H264_ALLOW_NON_IDR_RESYNCHRONIZATION,
    DVB_OPTION_MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG,
    DVB_OPTION_AUDIO_SPDIF_SOURCE,

    DVB_OPTION_H264_ALLOW_BAD_PREPROCESSED_FRAMES,
    DVB_OPTION_CLOCK_RATE_ADJUSTMENT_LIMIT_2_TO_THE_N_PARTS_PER_MILLION,                /* Value = N */
    DVB_OPTION_LIMIT_INPUT_INJECT_AHEAD,

    DVB_OPTION_MAX
} dvb_option_t;

typedef dvb_option_t                    video_option_t;

/* Decoder commands */
#define VIDEO_CMD_PLAY                  (0)
#define VIDEO_CMD_STOP                  (1)
#define VIDEO_CMD_FREEZE                (2)
#define VIDEO_CMD_SET_OPTION            (3)
#define VIDEO_CMD_GET_OPTION            (4)

/* Flags for VIDEO_CMD_FREEZE */
#define VIDEO_CMD_FREEZE_TO_BLACK       (1 << 0)

/* Flags for VIDEO_CMD_STOP */
#define VIDEO_CMD_STOP_TO_BLACK         (1 << 0)
#define VIDEO_CMD_STOP_IMMEDIATELY      (1 << 1)

/* Play input formats: */
/* The decoder has no special format requirements */
#define VIDEO_PLAY_FMT_NONE         (0)
/* The decoder requires full GOPs */
#define VIDEO_PLAY_FMT_GOP          (1)

/* ST specific video ioctls */
#define VIDEO_SET_ENCODING              _IO('o',  81)
#define VIDEO_FLUSH                     _IO('o',  82)
#define VIDEO_SET_SPEED                 _IO('o',  83)
#define VIDEO_DISCONTINUITY             _IO('o',  84)
#define VIDEO_STEP                      _IO('o',  85)
#define VIDEO_SET_PLAY_INTERVAL         _IOW('o', 86, video_play_interval_t)
#define VIDEO_SET_SYNC_GROUP            _IO('o',  87)
#define VIDEO_GET_PLAY_TIME             _IOR('o', 88, video_play_time_t)

/* ST specific audio ioctls */
#define AUDIO_SET_ENCODING              _IO('o',  70)
#define AUDIO_FLUSH                     _IO('o',  71)
#define AUDIO_SET_SPDIF_SOURCE          _IO('o',  72)
#define AUDIO_SET_SPEED                 _IO('o',  73)
#define AUDIO_DISCONTINUITY             _IO('o',  74)
#define AUDIO_SET_PLAY_INTERVAL         _IOW('o', 75, audio_play_interval_t)
#define AUDIO_SET_SYNC_GROUP            _IO('o',  76)
#define AUDIO_GET_PLAY_TIME             _IOR('o', 77, audio_play_time_t)

#endif /* H_DVB_STM_H */

