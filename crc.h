#ifndef CRC_H
#define CRC_H
// -----------------------------------------------------------------------------
// Copyright telecnatron.com. 2014.
// $Id: $
// 
// Calculate CRC code as required by SMA Sunnyboy inverters' bluetooth protocol.
//
// Derived from on orignal code by Wim Hofman,
// See http://www.on4akh.be/SMA-read.html*/
// 
// This code is released to the public domain.
// -----------------------------------------------------------------------------
#include <stdint.h>

/* Initial FCS value    */
#define CRC_PPPINITFCS16 0xffff 

/** 
 * Calculate a new fcs given the current fcs and the new data.
 * This is the CRC as used by PPP (I think, possibly CRC CCITT 16)
 *
 * @param fcs The current value of the crc
 * @param cp  Pointer to the data on which crc will be calculated
 * @param len The length of the data
 * 
 * @return The new value of the crc
 */
uint16_t crc_calc_crc(uint16_t fcs, unsigned char *cp, int len);


#endif
