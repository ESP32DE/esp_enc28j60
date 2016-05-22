#ifndef _ENC28J60_H_
#define _ENC28J60_H_

#include <stdio.h>
#include "globals.h"
#include "user_interface.h"
#include "config.h"
	

  
  #define ETHERNET_LINK_UP 0x0400

	// this variable is set during enc_init to the values above.
	extern u8 mymac[6];

  // Dummy functs if CS line is done manually and not via spi.c 
	#define enc_select()            /*ENC_PORT &= ~(1<<ENC_CS)*/
	#define enc_deselect()          /*ENC_PORT |= (1<<ENC_CS)*/

	#define LO8(x) ( (u8) ((x) & 0xFF) )
	#define HI8(x) ( (u8) (((x) >> 8) & 0xFF) )

	// EREVID value, filled with the enc_init() function.
	extern u8 enc_revid;

  u16 ICACHE_FLASH_ATTR enc_linkup (void);
	void      enc_init(void);
	void		  enc28j60_led_blink (u8 a);
	void      enc_send_packet( u16 len, u8 *buf );
	u16       enc_receive_packet( u16 bufsize, u8 *buf );
  u16 ICACHE_FLASH_ATTR enc_read_phyreg( u8 phyreg );

	#define ETH_INIT                enc_init
	#define ETH_PACKET_RECEIVE      enc_receive_packet
	#define ETH_PACKET_SEND         enc_send_packet
	#define enc28j60_revision       enc_revid

	// define for forcing full duplex mode, undefine for half duplex
	// both modes are tested and supported.
	#define FULL_DUPLEX

	// rx buffer 0x1A00 = 6656 bytes
	#define ENC_RX_BUFFER_START  0x0000
	#define ENC_RX_BUFFER_END    0x19FF
	// tx buffer 0x0600 = 1536 bytes
	#define ENC_TX_BUFFER_START  0x1A00
	#define ENC_TX_BUFFER_END    0x1FFF
	
	/* ENC registers and bit definitions */

	// Bit 7 is used to flag if there is one dummy read needed
	// before the register value can be read (see the Read Control
	// Register command in the enc spec).
	// Bit 6...5 are used to code in the bank value where this
	// register is located. The register read and register
	// functions do automatic bank switching based in these bits.

	#define ENC_REG_WAIT_MASK  0x80
	#define ENC_REG_BANK_MASK  0x60
	#define ENC_REG_BANK_SHIFT 5
	#define ENC_REG_ADDR_MASK  0x1F

	/* enc registers bank 0 */
	#define ENC_REG_ERDPTL   (0x00 | 0x00)
	#define ENC_REG_ERDPTH   (0x00 | 0x01)
	#define ENC_REG_EWRPTL   (0x00 | 0x02)
	#define ENC_REG_EWRPTH   (0x00 | 0x03)
	#define ENC_REG_ETXSTL   (0x00 | 0x04)
	#define ENC_REG_ETXSTH   (0x00 | 0x05)
	#define ENC_REG_ETXNDL   (0x00 | 0x06)
	#define ENC_REG_ETXNDH   (0x00 | 0x07)
	#define ENC_REG_ERXSTL   (0x00 | 0x08)
	#define ENC_REG_ERXSTH   (0x00 | 0x09)
	#define ENC_REG_ERXNDL   (0x00 | 0x0A)
	#define ENC_REG_ERXNDH   (0x00 | 0x0B)
	#define ENC_REG_ERXRDPTL (0x00 | 0x0C)
	#define ENC_REG_ERXRDPTH (0x00 | 0x0D)
	#define ENC_REG_ERXWRPTL (0x00 | 0x0E)
	#define ENC_REG_ERXWRPTH (0x00 | 0x0F)
	#define ENC_REG_EDMASTL  (0x00 | 0x10)
	#define ENC_REG_EDMASTH  (0x00 | 0x11)
	#define ENC_REG_EDMANDL  (0x00 | 0x12)
	#define ENC_REG_EDMANDH  (0x00 | 0x13)
	#define ENC_REG_EDMADSTL (0x00 | 0x14)
	#define ENC_REG_EDMADSTH (0x00 | 0x15)
	#define ENC_REG_EDMACSL  (0x00 | 0x16)
	#define ENC_REG_EDMACSH  (0x00 | 0x17)

	/* enc registers bank 1 */
	#define ENC_REG_EHT0     (0x20 | 0x00)
	#define ENC_REG_EHT1     (0x20 | 0x01)
	#define ENC_REG_EHT2     (0x20 | 0x02)
	#define ENC_REG_EHT3     (0x20 | 0x03)
	#define ENC_REG_EHT4     (0x20 | 0x04)
	#define ENC_REG_EHT5     (0x20 | 0x05)
	#define ENC_REG_EHT6     (0x20 | 0x06)
	#define ENC_REG_EHT7     (0x20 | 0x07)
	#define ENC_REG_EPMM0    (0x20 | 0x08)
	#define ENC_REG_EPMM1    (0x20 | 0x09)
	#define ENC_REG_EPMM2    (0x20 | 0x0A)
	#define ENC_REG_EPMM3    (0x20 | 0x0B)
	#define ENC_REG_EPMM4    (0x20 | 0x0C)
	#define ENC_REG_EPMM5    (0x20 | 0x0D)
	#define ENC_REG_EPMM6    (0x20 | 0x0E)
	#define ENC_REG_EPMM7    (0x20 | 0x0F)
	#define ENC_REG_EPMCSL   (0x20 | 0x10)
	#define ENC_REG_EPMCSH   (0x20 | 0x11)
	#define ENC_REG_EPMOL    (0x20 | 0x14)
	#define ENC_REG_EPMOH    (0x20 | 0x15)
	#define ENC_REG_EWOLIE   (0x20 | 0x16)
	 #define ENC_BIT_UCWOLIE 7
	 #define ENC_BIT_AWOLIE  6
	 #define ENC_BIT_PMWOLIE 4
	 #define ENC_BIT_MPWOLIE 3
	 #define ENC_BIT_HTWOLIE 2
	 #define ENC_BIT_MCWOLIE 1
	 #define ENC_BIT_BCWOLIE 0
	#define ENC_REG_EWOLIR   (0x20 | 0x17)
	 #define ENC_BIT_UCWOLIF 7
	 #define ENC_BIT_AWOLIF  6
	 #define ENC_BIT_PMWOLIF 4
	 #define ENC_BIT_MPWOLIF 3
	 #define ENC_BIT_HTWOLIF 2
	 #define ENC_BIT_MCWOLIF 1
	 #define ENC_BIT_BCWOLIF 0
	#define ENC_REG_ERXFCON  (0x20 | 0x18)
	 #define ENC_BIT_UCEN    7
	 #define ENC_BIT_ANDOR   6
	 #define ENC_BIT_CRCEN   5
	 #define ENC_BIT_PMEN    4
	 #define ENC_BIT_MPEN    3
	 #define ENC_BIT_HTEN    2
	 #define ENC_BIT_MCEN    1
	 #define ENC_BIT_BCEN    0
	#define ENC_REG_EPKTCNT  (0x20 | 0x19)

	/* enc registers bank 2 */
	#define ENC_REG_MACON1   (0x80 | 0x40 | 0x00)
	 #define ENC_BIT_LOOPBK  4
	 #define ENC_BIT_TXPAUS  3
	 #define ENC_BIT_RXPAUS  2
	 #define ENC_BIT_PASSALL 1
	 #define ENC_BIT_MARXEN  0
	#define ENC_REG_MACON2   (0x80 | 0x40 | 0x01)
	 #define ENC_BIT_MARST   7
	 #define ENC_BIT_RNDRST  6
	 #define ENC_BIT_MARXRST 3
	 #define ENC_BIT_RFUNRST 2
	 #define ENC_BIT_MATXRST 1
	 #define ENC_BIT_TFUNRST 0
	#define ENC_REG_MACON3   (0x80 | 0x40 | 0x02)
	 #define ENC_BIT_PADCFG2 7
	 #define ENC_BIT_PADCFG1 6
	 #define ENC_BIT_PADCFG0 5
	 #define ENC_BIT_TXCRCEN 4
	 #define ENC_BIT_PHDRLEN 3
	 #define ENC_BIT_HFRMEN  2
	 #define ENC_BIT_FRMLNEN 1
	 #define ENC_BIT_FULDPX  0
	#define ENC_REG_MACON4   (0x80 | 0x40 | 0x03)
	 #define ENC_BIT_DEFER   6
	 #define ENC_BIT_BPEN    5
	 #define ENC_BIT_NOBKOFF 4
	 #define ENC_BIT_LONGPRE 1
	 #define ENC_BIT_PUREPRE 0
	#define ENC_REG_MABBIPG  (0x80 | 0x40 | 0x04)
	#define ENC_REG_MAIPGL   (0x80 | 0x40 | 0x06)
	#define ENC_REG_MAIPGH   (0x80 | 0x40 | 0x07)
	#define ENC_REG_MACLCON1 (0x80 | 0x40 | 0x08)
	#define ENC_REG_MACLCON2 (0x80 | 0x40 | 0x09)
	#define ENC_REG_MAMXFLL  (0x80 | 0x40 | 0x0A)
	#define ENC_REG_MAMXFLH  (0x80 | 0x40 | 0x0B)
	#define ENC_REG_MAPHSUP  (0x80 | 0x40 | 0x0D)
	 #define ENC_BIT_RSTINTFC 7
	 #define ENC_BIT_RSTRMII  3
	#define ENC_REG_MICON    (0x80 | 0x40 | 0x11)
	 #define ENC_BIT_RSTMII  7
	#define ENC_REG_MICMD    (0x80 | 0x40 | 0x12)
	 #define ENC_BIT_MIISCAN 1
	 #define ENC_BIT_MIIRD   0
	#define ENC_REG_MIREGADR (0x80 | 0x40 | 0x14)
	#define ENC_REG_MIWRL    (0x80 | 0x40 | 0x16)
	#define ENC_REG_MIWRH    (0x80 | 0x40 | 0x17)
	#define ENC_REG_MIRDL    (0x80 | 0x40 | 0x18)
	#define ENC_REG_MIRDH    (0x80 | 0x40 | 0x19)

	/* enc registers bank 3 */
	#define ENC_REG_MAADR1   (0x80 | 0x60 | 0x00)
	#define ENC_REG_MAADR0   (0x80 | 0x60 | 0x01)
	#define ENC_REG_MAADR3   (0x80 | 0x60 | 0x02)
	#define ENC_REG_MAADR2   (0x80 | 0x60 | 0x03)
	#define ENC_REG_MAADR5   (0x80 | 0x60 | 0x04)
	#define ENC_REG_MAADR4   (0x80 | 0x60 | 0x05)
	#define ENC_REG_EBSTSD   (0x60 | 0x06)
	#define ENC_REG_EBSTCON  (0x60 | 0x07)
	 #define ENC_BIT_PSV2    7
	 #define ENC_BIT_PSV1    6
	 #define ENC_BIT_PSV0    5
	 #define ENC_BIT_PSEL    4
	 #define ENC_BIT_TMSEL1  3
	 #define ENC_BIT_TMSEL0  2
	 #define ENC_BIT_TME     1
	 #define ENC_BIT_BISTST  0
	#define ENC_REG_EBSTCSL  (0x60 | 0x08)
	#define ENC_REG_EBSTCSH  (0x60 | 0x09)
	#define ENC_REG_MISTAT   (0x80 | 0x60 | 0x0A)
	 #define ENC_BIT_NVALID  2
	 #define ENC_BIT_SCAN    1
	 #define ENC_BIT_BUSY    0
	#define ENC_REG_EREVID   (0x60 | 0x12)
	#define ENC_REG_ECOCON   (0x60 | 0x15)
	 #define ENC_BIT_COCON2  2
	 #define ENC_BIT_COCON1  1
	 #define ENC_BIT_COCON0  0
	#define ENC_REG_EFLOCON  (0x60 | 0x17)
	 #define ENC_BIT_FULDPXS 2
	 #define ENC_BIT_FCEN1   1
	 #define ENC_BIT_FCEN0   0
	#define ENC_REG_EPAUSL   (0x60 | 0x18)
	#define ENC_REG_EPAUSH   (0x60 | 0x19)

	/* enc registers common in all banks */
	#define ENC_REG_EIE      (0x00 | 0x1B)
	 #define ENC_BIT_INTIE   7
	 #define ENC_BIT_PKTIE   6
	 #define ENC_BIT_DMAIE   5
	 #define ENC_BIT_LINKIE  4
	 #define ENC_BIT_TXIE    3
	 #define ENC_BIT_WOLIE   2
	 #define ENC_BIT_TXERIE  1
	 #define ENC_BIT_RXERIE  0
	#define ENC_REG_EIR      (0x00 | 0x1C)
	 #define ENC_BIT_PKTIF   6
	 #define ENC_BIT_DMAIF   5
	 #define ENC_BIT_LINKIF  4
	 #define ENC_BIT_TXIF    3
	 #define ENC_BIT_WOLIF   2
	 #define ENC_BIT_TXERIF  1
	 #define ENC_BIT_RXERIF  0
	#define ENC_REG_ESTAT    (0x00 | 0x1D)
	 #define ENC_BIT_INT     7
	 #define ENC_BIT_LATECOL 4
	 #define ENC_BIT_RXBUSY  2
	 #define ENC_BIT_TXABRT  1
	 #define ENC_BIT_CLKRDY  0
	#define ENC_REG_ECON2    (0x00 | 0x1E)
	 #define ENC_BIT_AUTOINC 7
	 #define ENC_BIT_PKTDEC  6
	 #define ENC_BIT_PWRSV   5
	 #define ENC_BIT_VRPS    3
	#define ENC_REG_ECON1    (0x00 | 0x1F)
	 #define ENC_BIT_TXRST   7
	 #define ENC_BIT_RXRST   6
	 #define ENC_BIT_DMAST   5
	 #define ENC_BIT_CSUMEN  4
	 #define ENC_BIT_TXRTS   3
	 #define ENC_BIT_RXEN    2
	 #define ENC_BIT_BSEL1   1
	 #define ENC_BIT_BSEL0   0

	/* phy registers */
	#define ENC_REG_PHCON1   0x00
	 #define ENC_BIT_PRST    15
	 #define ENC_BIT_PLOOPBK 14
	 #define ENC_BIT_PPWRSV  11
	 #define ENC_BIT_PDPXMD  8
	#define ENC_REG_PHSTAT1  0x01
	 #define ENC_BIT_PFDPX   12
	 #define ENC_BIT_PHDPX   11
	 #define ENC_BIT_LLSTAT  2
	 #define ENC_BIT_JBSTAT  1
	#define ENC_REG_PHID1    0x02
	#define ENC_REG_PHID2    0x03
	#define ENC_REG_PHCON2   0x10
	 #define ENC_BIT_FRCLNK  14
	 #define ENC_BIT_TXDIS   13
	 #define ENC_BIT_JABBER  10
	 #define ENC_BIT_HDLDIS  8
	#define ENC_REG_PHSTAT2  0x11
	 #define ENC_BIT_TXSTAT  13
	 #define ENC_BIT_RXSTAT  12
	 #define ENC_BIT_COLSTAT 11
	 #define ENC_BIT_LSTAT   10
	 #define ENC_BIT_DPXSTAT 9
	 #define ENC_BIT_PLRITY  4
	#define ENC_REG_PHIE     0x12
	 #define ENC_BIT_PLNKIE  4
	 #define ENC_BIT_PGEIE   1
	#define ENC_REG_PHIR     0x13
	 #define ENC_BIT_PLNKIF  4
	 #define ENC_BIT_PGIF    2
	#define ENC_REG_PHLCON   0x14
	 #define ENC_BIT_LACFG0  8
	 #define ENC_BIT_LBCFG0  4
	 #define ENC_BIT_LFRQ0   2
	 #define ENC_BIT_STRCH   1

	/* SPI commands known by the ENC */

	#define ENC_SPI_OP_RCR    0x00
	#define ENC_SPI_OP_RBM    0x3A
	#define ENC_SPI_OP_WCR    0x40
	#define ENC_SPI_OP_WBM    0x7A
	#define ENC_SPI_OP_BFS    0x80
	#define ENC_SPI_OP_BFC    0xA0
	#define ENC_SPI_OP_SC     0xFF

#endif // #ifndef _ENC28j60_H_

