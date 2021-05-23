#ifndef LTC_H
#define LTC_H
#include <inttypes.h>

void init_ltc(void);
extern volatile uint8_t __ltcData[];
extern volatile uint8_t ltcDecode[];
extern volatile uint8_t ltc_ready;

enum LTC_Flags
{
	ltcFlagBGF0       = 0x01,
	ltcFlagBGF1       = 0x02,
	ltcFlagBGF2       = 0x04,
	ltcFlagBGF_Mask   = 0x07,
	ltcFlagDropFrame  = 0x10,
	ltcFlagColorFrame = 0x20,
	ltcFlagReverse    = 0x40,
	ltcFlagPolarity   = 0x80,
};

enum {
	ltcFmtUnknown = 0x00,
	ltcFmt24      = 0x01,
	ltcFmt25      = 0x02,
	ltcFmt29      = 0x03,
	ltcFmt30      = 0x03,
	ltcFramesMask = 0x07,
	ltcFmtDF = 0x10,
	ltcBCD= 0x80 ///< not actually part of LTC
};

struct LTC
{
	/// LTC format is not updated while unpacking
	uint8_t format;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t frame;
	uint8_t userbits[4];
	uint8_t flags;
};

extern uint8_t ltc_reversed;

/* to be called from non-interrupt context */
void unpackLTC(struct LTC * ltc, uint8_t *captureBuffer);




#endif // LTC_H
