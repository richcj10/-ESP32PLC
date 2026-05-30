#ifndef MBBP_H
#define MBBP_H

/*
 * MBBP — ModBus Bootloader Protocol
 *
 * Frame format (both directions):
 *   [ADDR 1B][CMD 1B][LEN_H 1B][LEN_L 1B][DATA 0-130B][CRC_H 1B][CRC_L 1B]
 *
 * CRC is Modbus CRC-16 (poly 0xA001, init 0xFFFF) over ADDR through end of DATA.
 * ADDR byte is the device's Modbus slave ID.
 */

/* ── FC65 trigger (app → bootloader handoff) ────────────────────────────── */
#define MBBP_FC_TRIGGER         0x41
#define MBBP_MAGIC_H            0xB0
#define MBBP_MAGIC_L            0x07

/* ── Commands (master → slave) ──────────────────────────────────────────── */
#define MBBP_CMD_HELLO          0x41
#define MBBP_CMD_START          0x42
#define MBBP_CMD_WRITE_PAGE     0x43
#define MBBP_CMD_VERIFY         0x44
#define MBBP_CMD_JUMP           0x45

/* ── Responses (slave → master) — CMD | 0x80 ────────────────────────────── */
#define MBBP_RSP_HELLO          0xC1
#define MBBP_RSP_START          0xC2
#define MBBP_RSP_WRITE_PAGE     0xC3
#define MBBP_RSP_VERIFY_OK      0xC4
#define MBBP_RSP_JUMP           0xC5
#define MBBP_RSP_ERROR          0xFF

/* ── Error codes ─────────────────────────────────────────────────────────── */
#define MBBP_ERR_BAD_CRC        0x01
#define MBBP_ERR_WRONG_MAGIC    0x02
#define MBBP_ERR_PAGE_OVERFLOW  0x03
#define MBBP_ERR_FLASH_FAIL     0x04
#define MBBP_ERR_VERIFY_FAIL    0x05
#define MBBP_ERR_WRONG_STATE    0x06
#define MBBP_ERR_TIMEOUT        0x07

/* ── Flash geometry (ATmega328P) ─────────────────────────────────────────── */
#define MBBP_PAGE_SIZE          128
#define MBBP_APP_START          0x0000
#define MBBP_APP_SIZE_MAX       30720
#define MBBP_MAX_PAGES          (MBBP_APP_SIZE_MAX / MBBP_PAGE_SIZE)

/* ── EEPROM layout ───────────────────────────────────────────────────────── */
#define MBBP_EE_BOOT_FLAG       0x00
#define MBBP_EE_SLAVE_ID        0x01
#define MBBP_EE_APP_VALID       0x02

#define MBBP_BOOT_FLAG_VAL      0xB0
#define MBBP_APP_VALID_VAL      0xA5

#ifndef MBBP_SLAVE_ADDR
#define MBBP_SLAVE_ADDR         1
#endif

/* ── Frame size constants ────────────────────────────────────────────────── */
#define MBBP_HEADER_SIZE        4
#define MBBP_CRC_SIZE           2
#define MBBP_OVERHEAD           (MBBP_HEADER_SIZE + MBBP_CRC_SIZE)
#define MBBP_MAX_PAYLOAD        (2 + MBBP_PAGE_SIZE)
#define MBBP_MAX_FRAME          (MBBP_OVERHEAD + MBBP_MAX_PAYLOAD)

/* ── RS-485 default DIR pin ──────────────────────────────────────────────── */
#ifndef MBBP_DIR_PIN
#define MBBP_DIR_PIN            20
#endif

#endif /* MBBP_H */
