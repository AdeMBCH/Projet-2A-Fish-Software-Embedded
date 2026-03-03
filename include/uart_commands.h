#ifndef UART_COMMANDS_H
#define UART_COMMANDS_H

#include <stdint.h>

#define UART_PROTO_VERSION   0x0001

/* Espace de codes 16 bits
 * 0x0000            : invalide
 * 0x0001..0x00FF    : transport/diagnostic
 * 0x0100..0x7FFF    : applicatif (Phase 2+)
 * 0x8000..0xFFFF    : réservés/expérimental
 */

typedef enum {
    UART_CMD_INVALID   = 0x0000,

    /* Transport / diagnostic */
    UART_CMD_PING      = 0x0001,   /* payload: vide -> réponse PONG */
    UART_CMD_PONG      = 0x0002,   /* payload: "pong" optionnel    */
    UART_CMD_TEXT      = 0x0003,   /* payload: UTF-8 bytes          */
    UART_CMD_ACK       = 0x0004,   /* payload: [2B func][2B len][1B status] */
    UART_CMD_NACK      = 0x0005,   /* payload: [1B err_code][opt]   */

    /* Démo/générique tests Phase 1 */
    UART_CMD_ACTION    = 0x0010,

    /* Événements internes (pas envoyés sur le fil) */
    UART_EVT_DECOD_OK  = 0x00F0,
    UART_EVT_DECOD_ERR = 0x00F1,

    /* Commandes applicatives robot */
    UART_CMD_SERVO_STOP   = 0x0100, /* payload: vide -> stop (servo à 0 / neutre) */
    UART_CMD_SERVO_ENABLE = 0x0101, /* payload: vide -> reprise fonctionnement normal (avance) */
    UART_CMD_SERVO_LEFT   = 0x0102, /* payload: vide -> virage gauche (centre décalé) */
    UART_CMD_SERVO_RIGHT  = 0x0103  /* payload: vide -> virage droite (centre décalé) */
    

} uart_cmd_t;

/* Codes d’erreur pour NACK */
typedef enum {
    UART_ERR_LEN_OVERSIZE  = 0x02,
    UART_ERR_UNKNOWN_FUNC  = 0x03,
    UART_ERR_BUSY          = 0x04
} uart_err_t;

/* Helpers ACK status */
static inline uint8_t uart_ack_status_ok(void)   { 
    return 0x00; 
}
static inline uint8_t uart_ack_status_fail(void) { 
    return 0xFF; 
}

/* Endianness helpers (big-endian sur le fil) */
static inline void u16_be_write(uint8_t *p, uint16_t v)
{ 
    p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)(v); 
}

static inline uint16_t u16_be_read(const uint8_t *p)
{ 
    return (uint16_t)(((uint16_t)p[0]<<8) | (uint16_t)p[1]); 
}

/* Conventions payloads:
 * - ACK:  [FUNC_H][FUNC_L][LEN_H][LEN_L][STATUS]
 * - NACK: [ERR_CODE][...optionnel]
 * - TEXT: octets UTF-8
 * - PING/PONG: payload vide
 */

 #endif
