
#include "ov528.h"

// ov528 protocol.
#define OV528_BAUD 115200 // 57600 115200
#define OV528_PKT_SZ     500
#define OV528_SIZE       OV528_SIZE_QVGA
#define OV528_ADDR       (0<<5)

#define ACK_TIMEOUT 500
#define RETRY_LIMIT 200
#define TIMEOUT 500

void writeBytes(uint8_t buf[], uint16_t len) {
  Serial.write(buf, len);
}

uint16_t readBytes(uint8_t buf[], uint16_t len, uint16_t timeout_ms) {
  uint16_t i;
  uint8_t subms = 0;
  for (i = 0; i < len; i++) {
    while (Serial.available() == 0) {
      delayMicroseconds(10);
      if (++subms >= 100) {
        if (timeout_ms == 0) {
          return i;
        }
        subms = 0;
        timeout_ms--;
      }
    }
    buf[i] = Serial.read();
  }
  return i;
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

inline void recv_clear(void) {
  Serial.flush();
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


void sendCmd(uint8_t cmd, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4) {
  uint8_t buf[6] = { 0xaa };
  buf[1] = cmd | OV528_ADDR;
  buf[2] = p1;
  buf[3] = p2;
  buf[4] = p3;
  buf[5] = p4;
  writeBytes(buf, 6);
}

void camera_sync() {
  uint8_t resp[6];
  recv_clear();
  while (1) {
    sendCmd(OV528_CMD_SYNC, 0, 0, 0, 0);
    if (!chkAck(OV528_CMD_SYNC)) {
      continue;
    }

    if (readBytes(resp, 6, TIMEOUT) != 6) continue;
    if (resp[0] == 0xaa && resp[1] == (OV528_CMD_SYNC | OV528_ADDR) && resp[2] == 0 && resp[3] == 0 && resp[4] == 0 && resp[5] == 0) break;
  }

  sendCmd(OV528_CMD_ACK, OV528_CMD_SYNC, 0, 0, 0);
}

void camera_init(void) {
  send_cmd_with_ack(OV528_CMD_INIT, 0, 7, 0, OV528_SIZE);
}

void camera_snapshot(void) {
  send_cmd_with_ack(OV528_CMD_PKT_SZ, 0x08, OV528_PKT_SZ & 0xff, (OV528_PKT_SZ >> 8) & 0xff , 0);
  send_cmd_with_ack(OV528_CMD_SNAPSHOT, 0, 0, 0, 0);
}

uint8_t camera_get_data(void *ctx, SIZE_FUN size_fun, WRITE_FUN write_fun) {
  uint32_t data_size;
  uint8_t buf1[6];
  uint8_t buf[OV528_PKT_SZ];

  while (1) {
    send_cmd_with_ack(OV528_CMD_GET_PIC, OV528_PIC_TYPE_SNAPSHOT, 0, 0, 0);

    if (readBytes(buf1, 6, 1000) != 6) {
      continue;
    }
    if (buf1[0] == 0xaa && buf1[1] == (OV528_CMD_DATA | OV528_ADDR) && buf1[2] == 0x01) {
      data_size = (buf1[3]) | (buf1[4] << 8) | ((uint32_t)buf1[5] << 16);
      break;
    }
  }

  uint16_t num_packet = (data_size + (OV528_PKT_SZ - 6 - 1)) / (OV528_PKT_SZ - 6);

  size_fun(ctx, data_size);

  for (uint16_t i = 0; i < num_packet; i++) {
    uint8_t retry_cnt = 0;

retry:
    recv_clear();
    sendCmd(OV528_CMD_ACK, 0, 0,  i & 0xff, (i >> 8) & 0xff);

    // recv data : 0xaa, OV528_CMD_DATA, len16, data..., sum?
    uint16_t len = readBytes(buf, OV528_PKT_SZ, 200);

    // checksum
    uint8_t sum = 0;
    for (uint16_t y = 0; y < len - 2; y++) {
      sum += buf[y];
    }

    if (sum != buf[len - 2]) {
      if (++retry_cnt < RETRY_LIMIT) {
        delay(100);
        goto retry;
      } else {
        sendCmd(OV528_CMD_ACK, 0, 0, 0xf0, 0xf0);
        return 0;
      }
    }

    // Send image data.
    write_fun(ctx, buf+4, len-6);
  }
  sendCmd(OV528_CMD_ACK, 0, 0, 0xf0, 0xf0);
  return 1;
}

