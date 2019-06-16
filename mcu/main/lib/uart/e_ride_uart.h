#ifndef _E_RIDE_UART_H
#define _E_RIDE_UART_H

#include <e_ride_err.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>


#define E_RIDE_UART_CONFIG_DEFAULT_ESP32()      \
{                                               \
    .baud_rate = 115200,                        \
    .data_bits = UART_DATA_8_BITS,              \
    .parity = UART_PARITY_DISABLE,              \
    .stop_bits = UART_STOP_BITS_1,              \
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE       \
}


#define E_RIDE_MSG_PKT_HEADER {0x5A, 0xA5}
#define E_RIDE_MSG_HEADER_SIZE 5
#define E_RIDE_MSG_MIN_SIZE 9


typedef struct e_ride_uart_msg_t
{
    uint8_t  pld_length;
    uint8_t  src_address;
    uint8_t  dst_address;
    uint8_t  cmd_command;
    uint8_t  cmd_argment;
    uint8_t* payload;
    uint8_t  chk_sum[2];
} e_ride_uart_msg_t;


typedef enum e_ride_uart_reg_t
{
    /* Status */
    E_RIDE_REG_BMS_CAPACITY    = 0x32,
    E_RIDE_REG_BMS_CURRENT     = 0x33,
    E_RIDE_REG_BMS_VOLTAGE     = 0x34,
    E_RIDE_REG_BMS_TEMPRTR     = 0x35,
    E_RIDE_REG_BMS_HEALTH      = 0x3B,

    /* Deep Status */
    E_RIDE_REG_BMS_CELL0_V     = 0x40,
    E_RIDE_REG_BMS_CELL1_V     = 0x41,
    E_RIDE_REG_BMS_CELL2_V     = 0x42,
    E_RIDE_REG_BMS_CELL3_V     = 0x43,
    E_RIDE_REG_BMS_CELL4_V     = 0x44,
    E_RIDE_REG_BMS_CELL5_V     = 0x45,
    E_RIDE_REG_BMS_CELL6_V     = 0x46,
    E_RIDE_REG_BMS_CELL7_V     = 0x47,
    E_RIDE_REG_BMS_CELL8_V     = 0x48,
    E_RIDE_REG_BMS_CELL9_V     = 0x49,
    E_RIDE_REG_BMS_MANUFACTURE_DATE    = 0x10,
    E_RIDE_REG_BMS_SERIAL_NUMBER       = 0x10,
    E_RIDE_REG_BMS_FW_VERSION          = 0x17,
    E_RIDE_REG_BMS_FACTORY_CAPACITY    = 0x18,
    E_RIDE_REG_BMS_ACTUAL_CAPACITY     = 0x19,
    E_RIDE_REG_BMS_CHARGE_FULL_CYCLES  = 0x1B,
    E_RIDE_REG_BMS_CHARGE_COUNT       = 0x1C,
    E_RIDE_REG_BMS_CAPACITY_mAh        = 0x31,
} e_ride_uart_reg_t;


typedef enum e_ride_uart_addr_t
{
    E_RIDE_ADDR_ESC = 0x20,
    E_RIDE_ADDR_BLE = 0x21,
    E_RIDE_ADDR_BMS = 0x22,
    E_RIDE_ADDR_APP = 0x3e,
} e_ride_uart_addr_t;


/**
 * Finds the first msg header in `buffer`.
 * If no header is found, returns -1.
 **/
int e_ride_uart_msg_find_header(
    
    uint8_t* buffer,
    int buf_length

);


/**
 * Creates a new message from a
 * `uint8_t` buffer.
 * Finds the header, and the returned
 * msg has a copy of the valid part of the
 * buffer, if the message is valid.
 * Allocates payload resources.
 * Returns `E_RIDE_UART_MSG_SUCCESS`
 * if the message is valid and was parsed
 * properly. Any error does not allocate
 * resources.
 **/
e_ride_err_t e_ride_uart_msg_parse(

    uint8_t* buffer,
    size_t buf_length,
    e_ride_uart_msg_t* outMsg

);


/**
 * Gets the length of the buffer needed
 * to hold the entire serialized message,
 * including the initial pkt header of
 * size 2, and the checksum of size 2.
 **/
size_t e_ride_uart_msg_get_serialized_length(

    e_ride_uart_msg_t msg

);


/**
 * Serializes `msg` into `buff`.
 * The length of the buffer needed
 * can be obtained with
 * `e_ride_uart_msg_get_serialized_length`.
 **/
void e_ride_uart_msg_serialize(

    e_ride_uart_msg_t msg,
    uint8_t *buff

);


/**
 * Creates a new message for reading
 * the specified register of the device
 * with `dstAddr` address.
 * The new message asks for `readLen`
 * bytes from the specified register.
 **/
e_ride_err_t e_ride_uart_regread_msg_new(

    e_ride_uart_addr_t dstAddr,
    e_ride_uart_reg_t reg,
    uint8_t readLen,
    e_ride_uart_msg_t *outMsg

);


/**
 * Cleans the resources of a message.
 **/
void e_ride_uart_msg_free(

    e_ride_uart_msg_t msg

);


/**
 * Calculates and writes the
 * buffer checksum to `chkSumBuf`.
 * This out buffer must have space
 * for at least 2 elements.
 **/
void e_ride_uart_buff_chk_calc(

    uint8_t* buffer,
    size_t   bufferLen,
    uint8_t  chkSumBuf[2]

);


/**
 * Calculates the checksum for
 * a message.
 **/
void e_ride_uart_msg_chk_calc(

    e_ride_uart_msg_t msg,
    uint8_t chkSum[2]

);


/**
 * Calculates the checksum of the supplied
 * Overrides the `msg_chsum` field.
 **/
void e_ride_uart_msg_set_chk(

    e_ride_uart_msg_t* msg

);


/**
 * Verifies the integrity of the message.
 * Returns `E_RIDE_UART_MSG_SUCCESS` if
 * valid, any of the errors if not valid.
 **/
e_ride_err_t e_ride_uart_msg_chk(

    e_ride_uart_msg_t msg

);


#endif /* _E_RIDE_UART_H */