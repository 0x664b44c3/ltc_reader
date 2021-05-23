#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <string.h>
#include <segments.h>
uint16_t bitsize;
uint16_t lastCapture=0;

static uint8_t ltc_status=0;
uint16_t captureData=0;
uint16_t longBit = 6400;
uint16_t shortBit = 3200;

const uint16_t min_short = 2400;
uint16_t max_short = 4500;

uint16_t min_long = 6000;
const uint16_t max_long = 9000;
uint16_t threshold = (min_short + max_long)/2;
uint8_t ltc_reversed=0;
struct timeCode{
};

enum ltcStatus
{
	ltcFindEdges=0,
	ltcFindSync=1,
	ltcRunning=2,
	ltcLocked=3
};


void init_ltc(void)
{
	DDRD|=_BV(2) | _BV(1);
	TCCR1A = 0;
	TCCR1B = _BV(ICNC1) | _BV(CS10);
	TIMSK1 = _BV(ICIE1);
	DDRD|=_BV(5); // sync led
	DDRD|=_BV(6); // search led
	DDRD|=_BV(7); // frame out
	PORTD|=0xe0;
	ltc_status = ltcFindSync;
}

uint8_t shortCount=0;
uint8_t onesCount=0;
uint8_t __ltc_bitCtr = 0;
uint8_t __ltc_currentByte= 0;
uint8_t __ltcData[10];
uint8_t ltcDecode[10];
uint8_t ltc_ready=0;


inline void shiftBits(uint8_t nextBit)
{
	if (ltc_reversed)
	{
		__ltcData[9]<<=1;
		for(uint8_t i=9;i>0;--i)
		{
			if (__ltcData[i-1] & 0x80)
				__ltcData[i]|=0x01;
			__ltcData[i-1]<<=1;
		}
		if (nextBit)
			__ltcData[0]|=0x01;
	}
	else
	{
		__ltcData[0]>>=1;
		for(uint8_t i=1;i<10;++i)
		{
			if (__ltcData[i] & 0x01)
				__ltcData[i-1]|=0x80;
			__ltcData[i]>>=1;
		}
		if (nextBit)
			__ltcData[9]|=0x80;
	}
}


volatile uint8_t __ltc_BitBuffer=0;
inline void pushBit(uint8_t bit)
{
	PORTD&=~_BV(2);
	if (bit)
		PORTD|=_BV(1);
	else
		PORTD&=~_BV(1);
	shiftBits(bit);

	switch(ltc_status)
	{
		case ltcFindSync:
			PORTD|=_BV(5);
			PORTD&=~_BV(6);
			__ltc_bitCtr=0;
			if (
			        (   __ltcData[8] == 0xfc)
			        && (__ltcData[9] == 0xbf))
			{
				ltc_status = ltcRunning;
				__ltc_bitCtr = 80;
				break;
			}
			//reverse start code
			if (
			        (   __ltcData[8] == 0xfd)
			        && (__ltcData[9] == 0x3f))
			{
				ltc_reversed = 1-ltc_reversed;
//				ltc_status = ltcRunning;
//				__ltc_bitCtr = 80;
				break;
			}

			break;
		case ltcRunning:
			PORTD|=_BV(6);
			PORTD&=~_BV(5);
			++__ltc_bitCtr;
			if (__ltc_bitCtr==80)
			{
				if (
				        (   __ltcData[8] != 0xfc)
				        || (__ltcData[9] != 0xbf))
				{
					ltc_status = ltcFindSync;
					__ltc_bitCtr=0;
				}
			}
			break;
		default:
			ltc_status = ltcFindSync;
	}

	if (__ltc_bitCtr==80)
	{
		__ltc_bitCtr=0;
		memcpy(ltcDecode, __ltcData, 10);
		ltc_ready=1;
		PORTD^=_BV(7);
	}
}

ISR(TIMER1_CAPT_vect) {
	register uint16_t icr = ICR1;
	TCCR1B ^= _BV(ICES1);
	register uint16_t delta = icr - lastCapture;
	lastCapture = icr;
	captureData = delta;
//	if ((delta > min_short) && (delta < max_short))
//	{
//		shortCount++;
//		if (shortCount==2)
//		{
//			shortCount = 0;
//			pushBit(1);
//		}
//	}
//	else
//		shortCount=0;

//	if ((delta > min_long) && (delta < max_long))
//	{
//		pushBit(0);
//	}
	if (delta<threshold)
	{
		shortCount++;
		if (shortCount==2)
		{
			shortCount = 0;
			pushBit(1);
		}
	}
	else
	{
		pushBit(0);
	}
	PORTD|=_BV(2);
}


/*** decoding */
void unpackLTC(struct LTC * ltc, uint8_t *captureBuffer)
{
	if (ltc->format & ltcBCD)
	{
		ltc->hour   = (captureBuffer[7] & 0x03) * 0x10
		        | (captureBuffer[6] & 0x0f);
		ltc->minute = (captureBuffer[5] & 0x07) * 0x10
		        | (captureBuffer[4] & 0x0f);
		ltc->second = (captureBuffer[3] & 0x07) * 0x10
		        | (captureBuffer[2] & 0x0f);
		ltc->frame  = (captureBuffer[1] & 0x03) * 0x10
		        | (captureBuffer[0] & 0x0f);
	} else {
		ltc->hour   = (captureBuffer[7] & 0x03) * 10
		        + (captureBuffer[6] & 0x0f);
		ltc->minute = (captureBuffer[5] & 0x07) * 10
		        + (captureBuffer[4] & 0x0f);
		ltc->second = (captureBuffer[3] & 0x07) * 10
		        + (captureBuffer[2] & 0x0f);
		ltc->frame  = (captureBuffer[1] & 0x03) * 10
		        + (captureBuffer[0] & 0x0f);
	}
	ltc->userbits[0] = ((captureBuffer[0] & 0xf0) >> 4)
	        | (captureBuffer[1] & 0xf0);
	ltc->userbits[1] = ((captureBuffer[2] & 0xf0) >> 4)
	        | (captureBuffer[3] & 0xf0);
	ltc->userbits[2] = ((captureBuffer[4] & 0xf0) >> 4)
	        | (captureBuffer[5] & 0xf0);
	ltc->userbits[3] = ((captureBuffer[6] & 0xf0) >> 4)
	        | (captureBuffer[7] & 0xf0);
	uint8_t flags=0;
	if (captureBuffer[1] & 0x04)
		flags|=ltcFlagDropFrame;
	if (captureBuffer[1] & 0x08)
		flags|=ltcFlagColorFrame;
	if (captureBuffer[7]&0x04)
		flags|=ltcFlagBGF1;
	if ((ltc->format & ltcFramesMask) == ltcFmt25)
	{
		if (captureBuffer[7] & 0x08)
			flags|=ltcFlagPolarity;
		if (captureBuffer[3] & 0x08)
			flags|=ltcFlagBGF0;
		if (captureBuffer[5] & 0x08)
			flags|=ltcFlagBGF2;
	}
	else
	{
		if (captureBuffer[3] & 0x08)
			flags|=ltcFlagPolarity;
		if (captureBuffer[5] & 0x08)
			flags|=ltcFlagBGF0;
		if (captureBuffer[7] & 0x08)
			flags|=ltcFlagBGF2;
	}
	if (ltc_reversed)
		flags|=ltcFlagReverse;
	ltc->flags = flags;
}

