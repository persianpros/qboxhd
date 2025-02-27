/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : frame_parser.h
Author :           Nick

Definition of the frame parser class module for player 2.


Date        Modification                                    Name
----        ------------                                    --------
02-Nov-06   Created                                         Nick

************************************************************************/

#ifndef H_FRAME_PARSER
#define H_FRAME_PARSER

#include "player.h"

// ---------------------------------------------------------------------
//
// Local type definitions
//

enum
{
    FrameParserNoError				= PlayerNoError,
    FrameParserError				= PlayerError,

    FrameParserNoStreamParameters		= BASE_FRAME_PARSER,
    FrameParserPartialFrameParameters,
    FrameParserUnhandledHeader,
    FrameParserHeaderSyntaxError,
    FrameParserHeaderUnplayable,
    FrameParserStreamSyntaxError,

    FrameParserFailedToCreateReversePlayStacks,

    FrameParserFailedToAllocateBuffer,

    FrameParserReferenceListConstructionDeferred,
#if defined(CONFIG_SH_QBOXHD_1_0) || defined(CONFIG_SH_QBOXHD_MINI_1_0)
	FrameParserInsufficientReferenceFrames,
	FrameParserCodecError
#else
 	FrameParserInsufficientReferenceFrames
#endif
};

typedef PlayerStatus_t	FrameParserStatus_t;

//

enum
{
    FrameParserFnRegisterOutputBufferRing	= BASE_FRAME_PARSER,
    FrameParserFnInput,
    FrameParserFnTranslatePlaybackTimeNativeToNormalized,
    FrameParserFnTranslatePlaybackTimeNormalizedToNative,
    FrameParserFnApplyCorrectiveNativeTimeWrap,

    FrameParserFnSetModuleParameters
};

// ---------------------------------------------------------------------
//
// Class definition
//

class FrameParser_c : public BaseComponentClass_c
{
public:

    virtual FrameParserStatus_t   RegisterOutputBufferRing(	Ring_t			  Ring ) = 0;

    virtual FrameParserStatus_t   Input(			Buffer_t		  CodedBuffer ) = 0;

    virtual FrameParserStatus_t   TranslatePlaybackTimeNativeToNormalized(
								unsigned long long	  NativeTime,
								unsigned long long	 *NormalizedTime ) = 0;

    virtual FrameParserStatus_t   TranslatePlaybackTimeNormalizedToNative(
								unsigned long long	  NormalizedTime,
								unsigned long long	 *NativeTime ) = 0;

    virtual FrameParserStatus_t   ApplyCorrectiveNativeTimeWrap( void ) = 0;
};

// ---------------------------------------------------------------------
//
// Docuentation
//

/*! \class FrameParser_c
\brief Responsible for parsing coded frames extract metadata from the frame.

The frame parser class is responsible for taking individual coded
frames parsing them to extract parameters controlling the decode and
manifestation of the frame, and passing the coded frame on to the
output ring for passing to the codec for decoding. This is a list of
its entrypoints, and a partial list of the calls it makes, and the data
structures it accesses, these are in addition to the standard component
class entrypoints, and the complete list of support entrypoints in the
Player class.

The partial list of entrypoints used by this class:

- Empty list.

The partial list of meta data types used by this class:

- Attached to input buffers:
  - <b>CodedFrameParameters</b>, Describes the coded frame output.
  - <b>StartCodeList</b>, Optional output attachment for those collators generating a start code scan.

- Added to output buffers:
  - <b>ParsedFrameParameters</b>, Contains the parsed frame/stream parameters pointers, and the assigned frame index.
  - <b>Parsed[Video|Audio|Data]Parameters</b>, Optional output attachment containing the descriptive parameters for a frame used in manifestation/timing.

*/

/*! \fn FrameParserStatus_t FrameParser_c::RegisterOutputBufferRing(Ring_t Ring)
\brief Register the ring on which parsed frame buffers are to be placed.

This function is used to register the ring on which parsed frame buffers are to be placed.

\param Ring A pointer to a Ring_c instance.

\return Frame parser status code, FrameParserNoError indicates success.
*/

/*! \fn FrameParserStatus_t FrameParser_c::Input(Buffer_t CodedBuffer)
\brief Accept input from a collator.

This function accepts a coded frame buffer, as output from a collator.

\param CodedBuffer A pointer to a Buffer_c instance.

\return Frame parser status code, FrameParserNoError indicates success.
*/

/*! \fn FrameParserStatus_t FrameParser_c::TranslatePlaybackTimeNativeToNormalized(unsigned long long NativeTime, unsigned long long *NormalizedTime)
\brief Translate a stream time value to player intenal value.

This function translates a 'native' stream time (e.g. in STC/PTS units for PES)
to 'normalized' player internal time value (in microseconds).  See
\ref time for more information.

\param NativeTime     Native stream time.
\param NormalizedTime Pointer to a variable to hold the normalized time.

\return Frame parser status code, FrameParserNoError indicates success.
*/

/*! \fn FrameParserStatus_t FrameParser_c::TranslatePlaybackTimeNormalizedToNative(unsigned long long NormalizedTime, unsigned long long *NativeTime)
\brief

Takes as parameters a long long normalized playback time and a pointer to a long long variable to hold the native time.
\brief Translate a stream time value to player intenal value.

This function translates a 'normalized' player internal time value (in
microseconds) to 'native' stream time (e.g. in STC/PTS units for PES).
See \ref time for more information.


\param NormalizedTime Normalized player internal time.
\param NativeTime     Pointer to a variable to hold the native time.

\return Frame parser status code, FrameParserNoError indicates success.
*/

/*! \fn FrameParserStatus_t FrameParser_c::ApplyCorrectiveNativeTimeWrap()
\brief Pretend there has been a wrap of the PTS.

This function pretends there has been a wrap of the PTS, used to
correct wrap state when some streams in a playback start off on one
side of a PTS wrap point, while others start off on the opposite side.

\return Frame parser status code, FrameParserNoError indicates success.
*/

#endif
