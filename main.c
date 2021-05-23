#include <inttypes.h>
#include "ltc.h"


/** advances the LTC by one frame if it is running forward (resulting in the frame that **should** be decoded right now) */
void ltc_flywheel(struct LTC* ltc)
{
	uint8_t nFrames=0;

	//NTSC drop frame
	uint8_t dropFrame=0;//ltc->format & ltcFmtDF;

	switch(ltc->format & ltcFramesMask)
	{
		case ltcFmt24:
			nFrames=24;
			break;
		case ltcFmt25:
			nFrames=25;
			break;
			//		case ltcFmt29:
		case ltcFmt30:
			nFrames=30;
			break;
		default:
			//unknown format: do nothing
			return;
	}
	//
	//FIXME: Add reverse timecode support
	if (ltc->flags & ltcFlagReverse)
		return;

	ltc->frame++;

	if (ltc->frame==nFrames)
	{
		ltc->frame = 0;
		ltc->second++;
		if (ltc->second == 60)
		{
			ltc->second = 0;
			ltc->minute++;
		}

		if (ltc->minute== 60)
		{
			ltc->minute = 0;
			ltc->hour++;
		}

		if (ltc->hour==24)
			ltc->hour=0;


		//drop frame handling
		if (ltc->second)
			dropFrame = 0;

		if (ltc->minute % 10)
			dropFrame = 0;

		if (dropFrame)
			ltc->frame = 2;
	}
}

int main(void) {
    
    //buffer to decode current frame
    struct LTC ltc;
    struct LTC oldLtc;
    uint8_t ltcFormat = 0;
    
    uint8_t lastFrame, currentFrame;
	while(1)
	{
        
		if (ltc_ready)
		{
            ltc_ready=0;
			//save last frame number
			lastFrame = currentFrame;

			ltc.format = ltcFormat;
			unpackLTC(&ltc, ltcDecode);
			
            currentFrame = ltc.frame;
			
            uint8_t ltcContiguos = 0;
			//advance old ltc to see if we are receiving contigous codes
			ltc_flywheel(&oldLtc);
			//advance ltc by one frame in transport direction
			if ((ltc.frame == oldLtc.frame) &&
			        (ltc.second == oldLtc.second) &&
			        (ltc.minute == oldLtc.minute) &&
			        (ltc.hour   == oldLtc.hour))
			{
				ltcContiguos = 1;
			}
			memcpy(&oldLtc, &ltc, sizeof(struct LTC));
			if (ltcContiguos)
				ltc_flywheel(&ltc);

			//LTC is ready to be displayed
        }
    }
}
