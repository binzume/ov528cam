// ov528 sample
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "ov528.h"

#define OV528_BAUD 115200 // 57600 115200
#define OV528_PKT_SZ     128
#define OV528_SIZE       OV528_SIZE_CIF
#define OV528_ADDR       (0<<5)

#define ACK_TIMEOUT 500
#define RETRY_LIMIT 200
#define TIMEOUT 500
#define SYNC_TIMEOUT 500


void delay_ms(uint16_t w){
    while (w-->0) _delay_ms(1);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
// SPI util
#define SPI_BUFSIZE 64 // 2^n size
volatile char spi_rxbuf[SPI_BUFSIZE];
volatile char spi_txbuf[SPI_BUFSIZE];
volatile uint8_t spi_rxr=0, spi_rxw=0;
volatile uint8_t spi_txr=0, spi_txw=0;

ISR(SPI_STC_vect) {
    uint8_t c=SPDR;
    if (spi_txr!=spi_txw) {
        SPDR = spi_txbuf[spi_txr];
        spi_txr=(spi_txr+1)&(SPI_BUFSIZE-1);
    } else {
        SPDR=0x00;
    }
    if (c) {
        spi_rxbuf[spi_rxw]=c;
        spi_rxw=(spi_rxw+1)&(SPI_BUFSIZE-1);
    }
}

void SPI_init_slave() {
    SPCR = (1<<SPE)|(1<<SPIE);
    DDRB |= (1<<PB4); // MISO
    DDRB &= ~((1<<PB2) | (1<<PB3) | (1<<PB5));  // SS, MOSI, SCK
}

uint8_t spi_recv()
{
    spi_rxr&=(SPI_BUFSIZE-1);
    while(spi_rxr==spi_rxw);
    return spi_rxbuf[spi_rxr++];
}

int spi_recv_chk(){
    return spi_rxr!=spi_rxw;
}

void spi_send(uint8_t d)
{
    while(((spi_txw+1)&(SPI_BUFSIZE-1))==spi_txr);
    spi_txbuf[spi_txw]=d;
    spi_txw=(spi_txw+1)&(SPI_BUFSIZE-1);
}


void spi_puts(char *s) {
    while(*s){
        spi_send(*s);
        s++;
    }
}



///////////////////////////////////////////////////////////////////////////////////////////////////////
// UART util

// uart recv buf
#define RX_BUF_SIZE 256
volatile uint8_t rx_buf[RX_BUF_SIZE];
volatile int rx_buf_pos = 0;
volatile int rx_buf_posr = 0;

ISR(USART_RX_vect) {
    if(bit_is_clear(UCSR0A,FE0)){// if no error.
        rx_buf[rx_buf_pos++] = UDR0;
        rx_buf_pos%=RX_BUF_SIZE;
    }
}

uint8_t recv_byte(void) {
    // while (bit_is_clear(UCSR0A, RXC0)); // Wait for data to be received.
    // return UDR0;
    while (rx_buf_posr == rx_buf_pos);
    uint8_t d = rx_buf[rx_buf_posr++];
    rx_buf_posr %= RX_BUF_SIZE;
    return d;
}

int recv_ready(void) {
    return rx_buf_pos - rx_buf_posr;
}
void recv_clear(void) {
    rx_buf_pos = rx_buf_posr = 0;
}

void send_byte(uint8_t d) {
    while (bit_is_clear(UCSR0A, UDRE0)); // wait to become writable.
    UDR0 = d;
}

void writeBytes(uint8_t buf[], uint8_t len) {
    for (uint8_t i = 0; i < len; i++) send_byte(buf[i]);
}


uint8_t readBytes_(uint8_t buf[], uint8_t len) {
    for (uint8_t i = 0; i < len; i++) buf[i] = recv_byte();
    return cmd_len;
}

uint8_t readBytes(uint8_t buf[], uint8_t len, uint16_t timeout_ms) {
    uint8_t i;
    uint8_t subms = 0;
    for (i = 0; i < len; i++) {
        while (recv_ready() == 0) {
            _delay_us(10);
            if (++subms >= 100) {
              if (timeout_ms == 0) {
                return i;
              }
              subms = 0;
              timeout_ms--;
            }
        }
        buf[i] = recv_byte();
    }
    return i;
}


void init_uart() {
    UBRR0 = 0; // stop UART
    UCSR0A |= (1<<U2X0); // 2x
    UCSR0B = _BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0); // enable Tx Rx Rx_INT
    UCSR0C = (1 << UCSZ00) |  (1 << UCSZ01); // 8bit
    UBRR0 = F_CPU/8/OV528_BAUD-1; // Baud rate
}



///////////////////////////////////////////////////////////////////////////////////////////////////////
// camera


void sendCmd(uint8_t cmd, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4) {
    uint8_t buf[6] = { 0xaa };
    buf[1] = cmd | OV528_ADDR;
    buf[2] = p1;
    buf[3] = p2;
    buf[4] = p3;
    buf[5] = p4;
    writeBytes(buf, 6);
}

uint8_t chkAck(uint8_t cmd) {
    uint8_t buf[6];
    if (readBytes(buf, 6, ACK_TIMEOUT) != 6) {
        return 0;
    }
    if (buf[0] == 0xaa && buf[1] == (OV528_CMD_ACK | OV528_ADDR) && buf[2] == cmd && buf[4] == 0 && buf[5] == 0) {
        return 1;
    }
    return 0;
}

inline uint8_t send_cmd_with_ack(uint8_t cmd, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4) {
    while (1) {
        recv_clear();
        sendCmd(cmd, p1, p2, p3, p4);
        if (chkAck(cmd)) {
            break;
        }
    }
    return 1;
}


void camera_sync() {
    uint8_t resp[6];
    recv_clear();
    while (1) {
        sendCmd(OV528_CMD_SYNC, 0,0,0,0);
        if (!chkAck(OV528_CMD_SYNC)) {
            continue;
        }

        if (readBytes(resp, 6, TIMEOUT) != 6) continue;
        if (resp[0] == 0xaa && resp[1] == (OV528_CMD_SYNC | OV528_ADDR) && resp[2] == 0 && resp[3] == 0 && resp[4] == 0 && resp[5] == 0) break;
    }

    sendCmd(OV528_CMD_ACK, OV528_CMD_SYNC,0,0,0);
}

void camera_init(void) {
    send_cmd_with_ack(OV528_CMD_INIT, 0,7,0,OV528_SIZE);
}



void camera_snapshot(void) {
    send_cmd_with_ack(OV528_CMD_PKT_SZ, 0x08, OV528_PKT_SZ & 0xff, (OV528_PKT_SZ>>8) & 0xff ,0);
    send_cmd_with_ack(OV528_CMD_SNAPSHOT, 0,0,0,0);
}

uint8_t camera_get_data(void) {
    uint32_t data_size;
    uint8_t buf1[6];
    uint8_t buf[OV528_PKT_SZ];

    while (1) {
        send_cmd_with_ack(OV528_CMD_GET_PIC, OV528_PIC_TYPE_SNAPSHOT,0,0,0);

        PORTD ^= 0x40;

        if (readBytes(buf1, 6, 1000) != 6) {
            continue;
        }
        if (buf1[0] == 0xaa && buf1[1] == (OV528_CMD_DATA | OV528_ADDR) && buf1[2] == 0x01) {
            data_size = (buf1[3]) | (buf1[4] << 8) | ((uint32_t)buf1[5] << 16);
            break;
        }
    }

    uint16_t num_packet = (data_size + (OV528_PKT_SZ-6 -1)) / (OV528_PKT_SZ - 6);

    for (uint16_t i = 0; i < num_packet; i++) {
        uint8_t retry_cnt = 0;

        retry:
        recv_clear();
        sendCmd(OV528_CMD_ACK, 0,0,  i & 0xff, (i >> 8) & 0xff);

        // recv data : 0xaa, OV528_CMD_DATA, len16, data..., sum?
        uint16_t len = readBytes(buf, OV528_PKT_SZ, 200);

        // checksum
        uint8_t sum = 0;
        for (uint16_t y = 0; y < len - 2; y++) {
            sum += buf[y];
        }

        if (sum != buf[len-2]) {
            if (++retry_cnt < RETRY_LIMIT) {
                delay_ms(100);
                goto retry;
            } else {
                sendCmd(OV528_CMD_ACK, 0,0, 0xf0, 0xf0);
                return 0;
            }
        }

        // Send data to SPI master.
        for (int p = 4; p < len - 2; p++) {
            spi_send(buf[p]);
        }

        PORTD ^= 0x40;
    }
    sendCmd(OV528_CMD_ACK, 0,0, 0xf0, 0xf0);
    return 1;
}

int main(void) {
    DDRC = 0x00;
    PORTC = 0x20; // pull up PC2
    DDRD = 0xe0; // LEDs
    PORTD = 0x00;

    init_uart();
    SPI_init_slave();
    sei();

    camera_sync();

    camera_init();

    uint8_t count;
    for (count = 0;;count ++) {
        delay_ms(500);
        PORTD ^= 0x80; // LED1

        if (PINC & 0x20) {
            delay_ms(100);
        } else {
            send_byte(0xa5);
            camera_snapshot();
            camera_get_data();

            PORTD &= ~0x80; // LED1
            delay_ms(200);
            PORTD |= 0x80; // LED1
            delay_ms(200);
        }
        PORTD ^= 0x80; // LED1
    }
    return 0;
}
