/*
-----------------------------------------------------------------------------------------
Author:         Mark F (Cicero-MF) mark@cdelec.co.za
Remarks:        
Version:        15.05.2016
Description:    Ethernet device driver for the enc28j60 and an esp8266

  This code was modified for use with the ESP8266, based off a version by Harald Freudenberger 
  (haraldfreude@arcor.de) - 2009/03/16
  The base of this code was an enc28j60 device driver by Simon Schulz (avr@auctionant.de)
  and modifications done by eProfi (2007/08/19).

-----------------------------------------------------------------------------------------
License:
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.
  This program is distributed in the hope that it will be useful, but

  WITHOUT ANY WARRANTY;

  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE. See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 51
  Franklin St, Fifth Floor, Boston, MA 02110, USA

  http://www.gnu.de/gpl-ger.html
-----------------------------------------------------------------------------------------*/

#include "esp8266.h"
#include "globals.h"
#include "spi.h"
#include "enc28j60.h"
#include "gpio.h"
#include "io.h"

//-----------------------------------------------------------------------------

u8 mymac[6];
u8 enc_revid = 0;

//-----------------------------------------------------------------------------

static volatile u8 enc_cur_bank           = 0;
static volatile u16  enc_next_packet_ptr  = 0;

static const u8 enc_configdata[] = {

	// enc registers

	// tx buffer
	ENC_REG_ETXSTL, LO8(ENC_TX_BUFFER_START),
	ENC_REG_ETXSTH, HI8(ENC_TX_BUFFER_START),
	ENC_REG_ETXNDL, LO8(ENC_TX_BUFFER_END),
	ENC_REG_ETXNDH, HI8(ENC_TX_BUFFER_END),

	// rx buffer
	ENC_REG_ERXSTL, LO8(ENC_RX_BUFFER_START),
	ENC_REG_ERXSTH, HI8(ENC_RX_BUFFER_START),
	ENC_REG_ERXNDL, LO8(ENC_RX_BUFFER_END),
	ENC_REG_ERXNDH, HI8(ENC_RX_BUFFER_END),

	// push mac out of reset
	ENC_REG_MACON2, 0x00,

	// mac receive enable, rx and tx pause control frames enable
	ENC_REG_MACON1, (1<<ENC_BIT_MARXEN) | (1<<ENC_BIT_RXPAUS) | (1<<ENC_BIT_TXPAUS),

	#ifdef FULL_DUPLEX
	// mac auto padding of small packets, add crc, frame length check, full duplex
	ENC_REG_MACON3, (1<<ENC_BIT_PADCFG0) | (1<<ENC_BIT_TXCRCEN)
			 | (1<<ENC_BIT_FRMLNEN) | (1<<ENC_BIT_FULDPX),
	#else
	// mac auto padding of small packets, add crc, frame length check, half duplex
	ENC_REG_MACON3, (1<<ENC_BIT_PADCFG0) | (1<<ENC_BIT_TXCRCEN)
			 | (1<<ENC_BIT_FRMLNEN),
	#endif

	// max framelength 1518
	ENC_REG_MAMXFLL, LO8(1518),
	ENC_REG_MAMXFLH, HI8(1518),

	#ifdef FULL_DUPLEX
	// back-to-back inter packet gap delay time (0x15 for full duplex)
	ENC_REG_MABBIPG, 0x15,
	#else
	// back-to-back inter packet gap delay time (0x12 for half duplex)
	ENC_REG_MABBIPG, 0x12,
	#endif

	// non back-to-back inter packet gap delay time (should be 0x12)
	ENC_REG_MAIPGL, 0x12,
  
  // enable promiscuous mode
  //ENC_REG_ERXFCON, 0x00,

	#ifndef FULL_DUPLEX
	// non back-to-back inter packet gap delay time high (should be 0x0C for half duplex)
	ENC_REG_MAIPGH, 0x0C,
	#endif
  
  // disable CLKOUT pin
	ENC_REG_ECOCON, 0x00,

	// our mac address
	ENC_REG_MAADR5, MYMAC1,
	ENC_REG_MAADR4, MYMAC2,
	ENC_REG_MAADR3, MYMAC3,
	ENC_REG_MAADR2, MYMAC4,
	ENC_REG_MAADR1, MYMAC5,
	ENC_REG_MAADR0, MYMAC6,

  0xFF, 0xFF,
  
	// now the phy registers (with 2 bytes data each)

	#ifdef FULL_DUPLEX
	// set the PDPXMD full duplex mode bit on the phy
	#define ENC_REG_PHCON1_VALUE (0x0000 | (1 << ENC_BIT_PDPXMD))
	ENC_REG_PHCON1, HI8(ENC_REG_PHCON1_VALUE), LO8(ENC_REG_PHCON1_VALUE),
	#endif

	#ifndef FULL_DUPLEX
	// in half duplex do not loop back transmitted data
	#define ENC_REG_PHCON2_VALUE (0x0000 | (1 << ENC_BIT_HDLDIS))
	ENC_REG_PHCON2, HI8(ENC_REG_PHCON2_VALUE), LO8(ENC_REG_PHCON2_VALUE),
	#endif

	// leds: leda (yellow) rx and tx activity, stretch to 40ms
	//       ledb (green)  link status
	#define ENC_REG_PHCON_VALUE (0x0000 | (1 << ENC_BIT_STRCH) \
			     | (7 << ENC_BIT_LACFG0)               \
			     | (4 << ENC_BIT_LBCFG0))
	ENC_REG_PHLCON, HI8(ENC_REG_PHCON_VALUE), LO8(ENC_REG_PHCON_VALUE),

	// end of config data marker
	0xFF, 0xFF
};

u16 ICACHE_FLASH_ATTR enc_linkup (void) {
  u16 u;
  u = enc_read_phyreg(ENC_REG_PHSTAT2);  //ENC_DEBUG("PHSTAT2 %4x\n", u);  
  if (u & ETHERNET_LINK_UP) {
    return ETHERNET_LINK_UP;
  } else {
    return 0;
  }
}



//-----------------------------------------------------------------------------

static void ICACHE_FLASH_ATTR usdelay( u16 us )
{
  os_delay_us(us);
}

static inline void spi_put( u8 value )
{
	//ENC_DEBUG("spi_put(%2x)\n", (unsigned) value );  
  spi_tx8(SPI_USED, value);
}

static inline u8 spi_get(void)
{
  return (u8)spi_rx8(SPI_USED);
}

//-----------------------------------------------------------------------------
static inline void enc_reset(void)
{
	enc_select();
	spi_put( ENC_SPI_OP_SC );
	enc_deselect();

	// errata #2: wait for at least 300 us
	usdelay( 1000 );
}

static void ICACHE_FLASH_ATTR enc_clrbits_reg( u8 reg, u8 bits )
{
	// no automatic bank switching in this function !!!

	u8 addr = reg & ENC_REG_ADDR_MASK;

	enc_select();
  spi_transaction(SPI_USED, 8, (ENC_SPI_OP_BFC | addr), 0, 0, 8, bits, 0, 0);
	enc_deselect();
}

static void ICACHE_FLASH_ATTR enc_setbits_reg( u8 reg, u8 bits )
{
	// no automatic bank switching in this function !!!

	u8 addr = reg & ENC_REG_ADDR_MASK;

	enc_select();
  spi_transaction(SPI_USED, 8, (ENC_SPI_OP_BFS | addr), 0, 0, 8, bits, 0, 0);
	enc_deselect();
}

static u8 ICACHE_FLASH_ATTR enc_read_reg( u8 reg )
{
	u8 value;
	u8 addr = reg & ENC_REG_ADDR_MASK;

	if( addr < 0x1A ) {
		u8 bank = (reg & ENC_REG_BANK_MASK) >> ENC_REG_BANK_SHIFT;
		if( bank != enc_cur_bank ) {
			// need to switch bank first
			enc_clrbits_reg( ENC_REG_ECON1, 0x03 << ENC_BIT_BSEL0 );
			if( bank ) {
				enc_setbits_reg( ENC_REG_ECON1, bank << ENC_BIT_BSEL0 );
			}
			enc_cur_bank = bank;
		}
	}

	enc_select();
  if( reg & ENC_REG_WAIT_MASK ) {
    value = spi_transaction(SPI_USED, 8, (ENC_SPI_OP_RCR | addr), 0, 0, 8, 0, 8, 0);  
  } else {
    value = spi_transaction(SPI_USED, 8, (ENC_SPI_OP_RCR | addr), 0, 0, 0, 0, 8, 0);
  }
  
	enc_deselect();

	return value;
}

static void ICACHE_FLASH_ATTR enc_write_reg( u8 reg, u8 value )
{
	u8 addr = reg & ENC_REG_ADDR_MASK;

	if( addr < 0x1A ) {
		u8 bank = (reg & ENC_REG_BANK_MASK) >> ENC_REG_BANK_SHIFT;
		if( bank != enc_cur_bank ) {
			// need to switch bank first
			enc_clrbits_reg( ENC_REG_ECON1, 0x03 << ENC_BIT_BSEL0 );
			if( bank ) {
				enc_setbits_reg( ENC_REG_ECON1, bank << ENC_BIT_BSEL0 );
			}
			enc_cur_bank = bank;
		}
	}
  enc_select();
  spi_transaction(SPI_USED, 8, (ENC_SPI_OP_WCR | addr), 0, 0, 8, value, 0, 0);
	enc_deselect();
}

#if 1
u16 ICACHE_FLASH_ATTR enc_read_phyreg( u8 phyreg )
{
	u16 value;

	enc_write_reg( ENC_REG_MIREGADR, phyreg );
	enc_write_reg( ENC_REG_MICMD, (1<<ENC_BIT_MIIRD) );
	usdelay(10);
	
  while( enc_read_reg( ENC_REG_MISTAT ) & (1<<ENC_BIT_BUSY) ) ;
	enc_write_reg( ENC_REG_MICMD, 0x00 );
	value = (((u16) enc_read_reg( ENC_REG_MIRDH )) << 8);
	value |= ((u16) enc_read_reg( ENC_REG_MIRDL ));

	return value;
}
#endif

static void ICACHE_FLASH_ATTR enc_write_phyreg( u8 phyreg, u16 value )
{
  u8 rec;
	enc_write_reg( ENC_REG_MIREGADR, phyreg );
	enc_write_reg( ENC_REG_MIWRL, LO8(value) );
	enc_write_reg( ENC_REG_MIWRH, HI8(value) );
	usdelay(10);	
  
  rec = enc_read_reg( ENC_REG_MISTAT );
	while( rec & (1<<ENC_BIT_BUSY) ) {
    rec = enc_read_reg( ENC_REG_MISTAT );   
    ENC_DEBUG("phy: ENC_REG_MISTAT=%2x\n", rec);
  }
}

static void ICACHE_FLASH_ATTR enc_read_buf( u8 *buf, u16 len )
{
	//ENC_DEBUG("enc_read_buf: %u bytes\n", len);
  enc_select();
	
	for(; len > 0; len--, buf++ ) {
		*buf = spi_transaction(SPI_USED, 8, (ENC_SPI_OP_RBM), 0, 0, 0, 0, 8, 0);
	}
	enc_deselect();
}

static void ICACHE_FLASH_ATTR enc_write_buf( u8 *buf, u16 len )
{
	enc_select();
  ENC_DEBUG("enc_wbuf:%u\n", len);
  spi_transaction(SPI_USED, 8, (ENC_SPI_OP_WBM), 0, 0, 8, 0, 0, 0);
	for(; len > 0; len--, buf++ ) {
    spi_transaction(SPI_USED, 8, (ENC_SPI_OP_WBM), 0, 0, 8, *buf, 0, 0);
	}
	enc_deselect();
}

//-----------------------------------------------------------------------------

void ICACHE_FLASH_ATTR enc_send_packet( u16 len, u8 *buf )
{
	u16 ms = 100;

	//ENC_DEBUG("enc_send: %u bytes\n", len);

	// wait up to 100 ms for the previos tx to finish
	while( ms-- ) {
		if( !(enc_read_reg( ENC_REG_ECON1 ) & (1<<ENC_BIT_TXRTS)) ) break;
		usdelay( 1000 );
	}

	#ifdef FULL_DUPLEX
	// reset tx logic if TXRTS bit is still on
	if( enc_read_reg( ENC_REG_ECON1 ) & (1<<ENC_BIT_TXRTS) ) {
	#else
	// errata #12: reset tx logic
	if( 1 ) {
	#endif
		//ENC_DEBUG("enc_send: reset tx logic\n");
		enc_setbits_reg( ENC_REG_ECON1, (1<<ENC_BIT_TXRST) );
		enc_clrbits_reg( ENC_REG_ECON1, (1<<ENC_BIT_TXRST) );
	}

	// setup write pointer
	enc_write_reg( ENC_REG_EWRPTL, LO8(ENC_TX_BUFFER_START) );
	enc_write_reg( ENC_REG_EWRPTH, HI8(ENC_TX_BUFFER_START) );

	// end pointer (points to last byte) to start + len
	enc_write_reg( ENC_REG_ETXNDL, LO8(ENC_TX_BUFFER_START+len) );
	enc_write_reg( ENC_REG_ETXNDH, HI8(ENC_TX_BUFFER_START+len) );

	// copy packet to enc buffer
	enc_write_buf( buf, len );

	// clear TXIF flag
	enc_clrbits_reg( ENC_REG_EIR, (1<<ENC_BIT_TXIF) );

	// start transmission by setting the TXRTS bit
	enc_setbits_reg( ENC_REG_ECON1, (1<<ENC_BIT_TXRTS) );
}

u16 ICACHE_FLASH_ATTR enc_receive_packet( u16 bufsize, u8 *buf )
{
	u8 rxheader[6];
	u16 len, status;
	u8 u;

	// check rx packet counter
	u = enc_read_reg( ENC_REG_EPKTCNT );
	//ENC_DEBUG("enc_receive: EPKTCNT=%u\n", (int) u);
	if( u == 0 ) {
		// packetcounter is 0, there is nothing to receive, go back
		return 0;
	}

	//set read pointer to next packet
	enc_write_reg( ENC_REG_ERDPTL, LO8(enc_next_packet_ptr) );
	enc_write_reg( ENC_REG_ERDPTH, HI8(enc_next_packet_ptr) );

	// read enc rx packet header
	enc_read_buf( rxheader, sizeof(rxheader) );
	enc_next_packet_ptr  =             rxheader[0];
	enc_next_packet_ptr |= (rxheader[1] << 8);
	len                  =             rxheader[2];
	len                 |= (rxheader[3] << 8);
	status               =             rxheader[4];
	status              |= (rxheader[5] << 8);

	// added by Sjors: reset the ENC when needed
	// If the receive OK bit is not 1 or the zero bit is not zero, or the packet is larger then the buffer, reset the enc chip and SPI
	if ((!(status & (1<<7))) || (status & 0x8000))
	{ 
    ENC_DEBUG("resetting enc @ enc_receive_packet\r\n");
		enc_init();
	}
	//ENC_DEBUG("enc_receive: status=%4x, %2x,%2x,%2x,%2x,%2x,%2x,\n", status, rxheader[0],rxheader[1],rxheader[2],rxheader[3],rxheader[4],rxheader[5]);

	// skip the checksum (4 bytes) at the end of the buffer
	len -= 4;

	// if the application buffer is to small, we just truncate
	if( len > bufsize ) len = bufsize;

	// now read the packet data into buffer
	enc_read_buf( buf, len );

	// adjust the ERXRDPT pointer (= free this packet in rx buffer)
	if(    enc_next_packet_ptr-1 > ENC_RX_BUFFER_END
	    || enc_next_packet_ptr-1 < ENC_RX_BUFFER_START ) {
		enc_write_reg( ENC_REG_ERXRDPTL, LO8(ENC_RX_BUFFER_END) );
		enc_write_reg( ENC_REG_ERXRDPTH, HI8(ENC_RX_BUFFER_END) );
	} else {
		enc_write_reg( ENC_REG_ERXRDPTL, LO8(enc_next_packet_ptr-1) );
		enc_write_reg( ENC_REG_ERXRDPTH, HI8(enc_next_packet_ptr-1) );
	}

	// trigger a decrement of the rx packet counter
	// this will clear PKTIF if EPKTCNT reaches 0
	enc_setbits_reg( ENC_REG_ECON2, (1<<ENC_BIT_PKTDEC) );

	// return number of bytes written to the buffer
	//ENC_DEBUG("enc_receive: %u bytes\n", len);
	return len;
}



//-----------------------------------------------------------------------------

#if 0
static void ICACHE_FLASH_ATTR enc_regdump(void)
{
	u8 v;
	u16  u;

	v = enc_read_reg(ENC_REG_EIE);         ENC_DEBUG("EIE %2x\n", (unsigned)v);
	v = enc_read_reg(ENC_REG_EIR);         ENC_DEBUG("EIR %2x\n", (unsigned)v);
	v = enc_read_reg(ENC_REG_ESTAT);       ENC_DEBUG("ESTAT %2x\n", (unsigned)v);
	v = enc_read_reg(ENC_REG_ECON2);       ENC_DEBUG("ECON2 %2x\n", (unsigned)v);
	v = enc_read_reg(ENC_REG_ECON1);       ENC_DEBUG("ECON1 %2x\n", (unsigned)v);

	v = enc_read_reg(ENC_REG_ERDPTH);      u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_ERDPTL);     ENC_DEBUG("ERDPT %4x\n", u);
	v = enc_read_reg(ENC_REG_EWRPTH);      u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_EWRPTL);     ENC_DEBUG("EWRPT %4x\n", u);
	v = enc_read_reg(ENC_REG_ETXSTH);      u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_ETXSTL);     ENC_DEBUG("ETXST %4x\n", u);
	v = enc_read_reg(ENC_REG_ETXNDH);      u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_ETXNDL);     ENC_DEBUG("ETXND %4x\n", u);
	v = enc_read_reg(ENC_REG_ERXSTH);      u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_ERXSTL);     ENC_DEBUG("ERXST %4x\n", u);
	v = enc_read_reg(ENC_REG_ERXNDH);      u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_ERXNDL);     ENC_DEBUG("ERXND %4x\n", u);
	v = enc_read_reg(ENC_REG_ERXRDPTH);    u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_ERXRDPTL);   ENC_DEBUG("ERXRDPT %4x\n", u);
	v = enc_read_reg(ENC_REG_ERXWRPTH);    u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_ERXWRPTL);   ENC_DEBUG("ERXWRPT %4x\n", u);

	//v = enc_read_reg(ENC_REG_EDMASTH);     u = (((unsigned)(v & 0x1F)) << 8);
	//u |= enc_read_reg(ENC_REG_EDMASTL);    ENC_DEBUG("EDMAST %4x\n", u);
	//v = enc_read_reg(ENC_REG_EDMANDH);     u = (((unsigned)(v & 0x1F)) << 8);
	//u |= enc_read_reg(ENC_REG_EDMANDL);    ENC_DEBUG("EDMAND %4x\n", u);
	//v = enc_read_reg(ENC_REG_EDMADSTH);    u = (((unsigned)(v & 0x1F)) << 8);
	//u |= enc_read_reg(ENC_REG_EDMADSTL);   ENC_DEBUG("EDMADST %4x\n", u);
	//v = enc_read_reg(ENC_REG_EDMACSH);     u = (((unsigned)(v & 0x1F)) << 8);
	//u |= enc_read_reg(ENC_REG_EDMACSL);    ENC_DEBUG("EDMACS %4x\n", u);

	//v = enc_read_reg(ENC_REG_EHT0);        ENC_DEBUG("EHT0 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EHT1);        ENC_DEBUG("EHT1 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EHT2);        ENC_DEBUG("EHT2 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EHT3);        ENC_DEBUG("EHT3 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EHT4);        ENC_DEBUG("EHT4 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EHT5);        ENC_DEBUG("EHT5 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EHT6);        ENC_DEBUG("EHT6 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EHT7);        ENC_DEBUG("EHT7 %2x\n", (unsigned) v);

	//v = enc_read_reg(ENC_REG_EPMM0);       ENC_DEBUG("EPMM0 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EPMM1);       ENC_DEBUG("EPMM1 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EPMM2);       ENC_DEBUG("EPMM2 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EPMM3);       ENC_DEBUG("EPMM3 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EPMM4);       ENC_DEBUG("EPMM4 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EPMM5);       ENC_DEBUG("EPMM5 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EPMM6);       ENC_DEBUG("EPMM6 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EPMM7);       ENC_DEBUG("EPMM7 %2x\n", (unsigned) v);

	//v = enc_read_reg(ENC_REG_EPMCSH);      u = (((unsigned)(v & 0x1F)) << 8);
	//u |= enc_read_reg(ENC_REG_EPMCSL);     ENC_DEBUG("EPMCS %4x\n", u);
	//v = enc_read_reg(ENC_REG_EPMOH);       u = (((unsigned)(v & 0x1F)) << 8);
	//u |= enc_read_reg(ENC_REG_EPMOL);      ENC_DEBUG("EPMO %4x\n", u);

	//v = enc_read_reg(ENC_REG_EWOLIE);      ENC_DEBUG("EWOLIE %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_EWOLIR);      ENC_DEBUG("EWOLIR %2x\n", (unsigned) v);

	v = enc_read_reg(ENC_REG_ERXFCON);     ENC_DEBUG("ERXFCON %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_EPKTCNT);     ENC_DEBUG("EPKTCNT %2x\n", (unsigned) v);

	v = enc_read_reg(ENC_REG_MACON1);      ENC_DEBUG("MACON1 %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_MACON2);      ENC_DEBUG("MACON2 %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_MACON3);      ENC_DEBUG("MACON3 %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_MACON4);      ENC_DEBUG("MACON4 %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_MABBIPG);     ENC_DEBUG("MABBIPG %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_MAIPGH);      u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_MAIPGL);     ENC_DEBUG("MAIPG %4x\n", u);
	v = enc_read_reg(ENC_REG_MACLCON1);    ENC_DEBUG("MACLCON1 %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_MACLCON2);    ENC_DEBUG("MACLCON2 %2x\n", (unsigned) v);

	v = enc_read_reg(ENC_REG_MAMXFLH);     u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_MAMXFLL);    ENC_DEBUG("MAMXFL %4x\n", u);
	v = enc_read_reg(ENC_REG_MAPHSUP);     ENC_DEBUG("MAPHSUP %2x\n", (unsigned) v);

	v = enc_read_reg(ENC_REG_MICON);       ENC_DEBUG("MICON %2x\n", (unsigned) v);

	//v = enc_read_reg(ENC_REG_MICMD);       ENC_DEBUG("MICMD %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_MIREGADR);    ENC_DEBUG("MIREGADR %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_MIWRH);       u = (((unsigned)(v & 0x1F)) << 8);
	//u |= enc_read_reg(ENC_REG_MIWRL);      ENC_DEBUG("MIWR %4x\n", u);
	//v = enc_read_reg(ENC_REG_MIRDH);       u = (((unsigned)(v & 0x1F)) << 8);
	//u |= enc_read_reg(ENC_REG_MIRDL);      ENC_DEBUG("MIRD %4x\n", u);

	//v = enc_read_reg(ENC_REG_MAADR5);      ENC_DEBUG("MAADR5 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_MAADR4);      ENC_DEBUG("MAADR4 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_MAADR3);      ENC_DEBUG("MAADR3 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_MAADR2);      ENC_DEBUG("MAADR2 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_MAADR1);      ENC_DEBUG("MAADR1 %2x\n", (unsigned) v);
	//v = enc_read_reg(ENC_REG_MAADR0);      ENC_DEBUG("MAADR0 %2x\n", (unsigned) v);

	v = enc_read_reg(ENC_REG_EBSTSD);      ENC_DEBUG("EBSTSD %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_EBSTCON);     ENC_DEBUG("EBSTCON %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_EBSTCSH);     u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_EBSTCSL);    ENC_DEBUG("EBSTCS %4x\n", u);

	v = enc_read_reg(ENC_REG_MISTAT);      ENC_DEBUG("MISTAT %2x\n", (unsigned) v);

	v = enc_read_reg(ENC_REG_EREVID);      ENC_DEBUG("EREVID %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_ECOCON);      ENC_DEBUG("ECOCON %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_EFLOCON);     ENC_DEBUG("EFLOCON %2x\n", (unsigned) v);
	v = enc_read_reg(ENC_REG_EPAUSH);      u = (((unsigned)(v & 0x1F)) << 8);
	u |= enc_read_reg(ENC_REG_EPAUSL);     ENC_DEBUG("EPAUS %4x\n", u);

	u = enc_read_phyreg(ENC_REG_PHCON1);   ENC_DEBUG("PHCON1 %4x\n", u);
	u = enc_read_phyreg(ENC_REG_PHSTAT1);  ENC_DEBUG("PHSTAT1 %4x\n", u);
	u = enc_read_phyreg(ENC_REG_PHID1);    ENC_DEBUG("PHID1 %4x\n", u);
	u = enc_read_phyreg(ENC_REG_PHID2);    ENC_DEBUG("PHID2 %4x\n", u);
	u = enc_read_phyreg(ENC_REG_PHCON2);   ENC_DEBUG("PHCON2 %4x\n", u);
	u = enc_read_phyreg(ENC_REG_PHSTAT2);  ENC_DEBUG("PHSTAT2 %4x\n", u);
	u = enc_read_phyreg(ENC_REG_PHIE);     ENC_DEBUG("PHIE %4x\n", u);
	u = enc_read_phyreg(ENC_REG_PHIR);     ENC_DEBUG("PHIR %4x\n", u);
	u = enc_read_phyreg(ENC_REG_PHLCON);   ENC_DEBUG("PHLCON %4x\n", u);
}
#endif

//-----------------------------------------------------------------------------

void ICACHE_FLASH_ATTR enc_init(void)
{
	int i=0, j=0;
	u16 u;
	u8 r, d;
	
  //ENC Hardware Reset
  
  gpio_output_set(0, (1<<ENCRESETGPIO), (1<<ENCRESETGPIO), 0);  
  os_delay_us(200000);
  gpio_output_set((1<<ENCRESETGPIO), 0, (1<<ENCRESETGPIO), 0);  
	os_delay_us(200000);
  
	// config enc chip select as output and deselect enc
  // chip select is configured in spi_init
  
	// init spi
	spi_init(SPI_USED);

	// send a reset command via spi to the enc
	enc_reset();

	// wait for the CLKRDY bit
	while( !(enc_read_reg( ENC_REG_ESTAT ) & (1<<ENC_BIT_CLKRDY)) ) ;

	// get enc revision id
	//enc_revid = enc_read_reg( ENC_REG_EREVID );
	//ENC_DEBUG("enc revid %x\n", (int) enc_revid);
  
  //MAC Address
  #ifdef USE_SEPARATE_ENC_MAC
    //MAC Adresse setzen
    mymac[0] = MYMAC1;
    mymac[1] = MYMAC2;
    mymac[2] = MYMAC3;
    mymac[3] = MYMAC4;
    mymac[4] = MYMAC5;
    mymac[5] = MYMAC6;
  #else
    wifi_get_macaddr(STATION_IF, mymac);
    ENC_DEBUG("\""MACSTR"\"\r\n", MAC2STR(mymac));
	#endif

	// setup enc registers according to the enc_configdata struct
	while(i < sizeof(enc_configdata)) {
		r = enc_configdata[i++];    
		d = enc_configdata[i++];
		if(r == ENC_REG_MAADR5) break;
		enc_write_reg( r, d );
	}
  
  // load mac 
  i-=2;  // to keep with the loading method, ease of understanding
  for (j=0;j<6;j++) {
    r = enc_configdata[i]; 
    d = mymac[j];
    i+=2;
    enc_write_reg( r, d );   
  }
  
  //ENC_DEBUG("finished enc registers, cu bank = %u\r\n", enc_cur_bank);
  
  //enc_regdump(); 
  
	// now the phy registers
	while(i < sizeof(enc_configdata)) {
		r = enc_configdata[i++];
		d = enc_configdata[i++];
		if( r == 0xFF && d == 0xFF ) break;
		u = (((u16)d) << 8);
		d = enc_configdata[i++];
		u |= d;
		enc_write_phyreg( r, u );
	}
  
	// setup receive next packet pointer
	enc_next_packet_ptr = ENC_RX_BUFFER_START;

	// configure the enc interrupt sources
	enc_write_reg( ENC_REG_EIE, (1 << ENC_BIT_INTIE)  | (1 << ENC_BIT_PKTIE)
				  | (0 << ENC_BIT_DMAIE)  | (0 << ENC_BIT_LINKIE)
				  | (0 << ENC_BIT_TXIE)   | (0 << ENC_BIT_WOLIE)
				  | (0 << ENC_BIT_TXERIE) | (0 << ENC_BIT_RXERIE));

	// enable receive
	enc_setbits_reg( ENC_REG_ECON1, (1<<ENC_BIT_RXEN) );
}

//-----------------------------------------------------------------------------

//LED blink test
void ICACHE_FLASH_ATTR enc28j60_led_blink (u8 a)
{
	if(a)
	{
		//set up leds: LEDA: link status, LEDB: RX&TX activity, stretch 40ms, stretch enable
		enc_write_phyreg(ENC_REG_PHLCON, 0x321A);
	}
	else
	{
		//set up leds: LEDA: link status, LEDB: RX&TX activity, stretch 40ms, stretch enable
		enc_write_phyreg(ENC_REG_PHLCON, 0x347A); //cave: Table3-3: reset value is 0x3422, do not modify the reserved "3"!!
		//RevA Datasheet page 9: write as '0000', see RevB Datasheet: write 0011!
	}
}

