#include "gsm_manager.h"
#include <Arduino.h>
#include <string.h>
#include <SoftwareSerial.h>

// SIM800L: 4V/2A external. TX divider 10k+20k to module RX.
// NOTE: don't drop HTTPDATA delay - broke uploads twice on SIM800L

static SoftwareSerial gsmSerial(PIN_GSM_TX, PIN_GSM_RX);

#define GSM_LINE_BUF_SIZE   256
#define GSM_HTTP_BODY_DELAY_MS  50

enum GsmState {
  GSM_IDLE,
  GSM_SEND_AT,
  GSM_WAIT_AT_OK,
  GSM_WAIT_SIM,
  GSM_WAIT_REG,
  GSM_WAIT_CGATT,
  GSM_WAIT_SAPBR_CONTYPE,
  GSM_WAIT_SAPBR_APN,
  GSM_SAPBR_OPEN,
  GSM_WAIT_SAPBR_OPEN,
  GSM_WAIT_SAPBR_CLOSE,
  GSM_HTTP_INIT,
  GSM_WAIT_HTTP_INIT,
  GSM_WAIT_HTTP_URL,
  GSM_WAIT_HTTP_DATA_PROMPT,
  GSM_WAIT_HTTP_BODY_DELAY,
  GSM_HTTP_ACTION,
  GSM_WAIT_HTTP_ACTION,
  GSM_WAIT_HTTP_TERM,
  GSM_READY,
  GSM_ERROR
};

static GsmState gsmState = GSM_IDLE;
static char lineBuffer[GSM_LINE_BUF_SIZE];
static char lastCmd[64];
static char pendingLine[GSM_LINE_BUF_SIZE];
static uint8_t linePos = 0;
static bool lineReady = false;
static unsigned long stateStartMs = 0;
static unsigned long lastPostMs = 0;
static bool gsmReady = false;
static bool vitalsQueued = false;
static bool sapbrCloseRetry = false;
static Vitals queuedVitals;
static char httpBody[128];

static void resetLineParser() {
  linePos = 0;
  lineReady = false;
  pendingLine[0] = '\0';
}

static void drainGsmSerial() {
  while (gsmSerial.available()) {
    gsmSerial.read();
  }
  resetLineParser();
}

static void sendAtCommand(const char* cmd) {
  strncpy(lastCmd, cmd, sizeof(lastCmd) - 1);
  lastCmd[sizeof(lastCmd) - 1] = '\0';
  gsmSerial.print(cmd);
  gsmSerial.print(F("\r\n"));
  Serial.print(F("[GSM] >> "));
  Serial.println(cmd);
}

static bool isEchoLine(const char* line) {
  if (strlen(line) == 0) {
    return true;
  }
  if (strncmp(line, lastCmd, strlen(lastCmd)) == 0) {
    return true;
  }
  return false;
}

static bool lineHasOk(const char* line) {
  return strstr(line, "OK") != nullptr;
}

static bool lineHasError(const char* line) {
  return strstr(line, "ERROR") != nullptr;
}

static bool isCregRegistered(const char* line) {
  const char* creg = strstr(line, "+CREG:");
  if (!creg) {
    return false;
  }
  return strstr(creg, ",1") != nullptr || strstr(creg, ",5") != nullptr;
}

static void onLineComplete(const char* line) {
  if (isEchoLine(line)) {
    return;
  }

  Serial.print(F("[GSM] << "));
  Serial.println(line);

  strncpy(pendingLine, line, GSM_LINE_BUF_SIZE - 1);
  pendingLine[GSM_LINE_BUF_SIZE - 1] = '\0';
  lineReady = true;
}

static void appendSerialChar(char c) {
  if (c == '\r') {
    return;
  }
  if (c == '\n') {
    if (linePos > 0) {
      lineBuffer[linePos] = '\0';
      onLineComplete(lineBuffer);
    }
    linePos = 0;
    return;
  }
  if (linePos < GSM_LINE_BUF_SIZE - 1) {
    lineBuffer[linePos++] = c;
  }
}

static void readGsmResponses() {
  while (gsmSerial.available()) {
    appendSerialChar((char)gsmSerial.read());
  }
}

static bool stateTimedOut(unsigned long timeoutMs) {
  return (millis() - stateStartMs) > timeoutMs;
}

static void enterState(GsmState next) {
  gsmState = next;
  stateStartMs = millis();
  lineReady = false;
}

static void buildHttpJson(const Vitals& v, char* out, size_t len) {
  snprintf(out, len,
           "{\"hr\":%d,\"spo2\":%d,\"temp\":%.1f,\"ecg\":%d,\"timestamp\":%lu}",
           v.hrValid ? v.hr : -1,
           v.spo2Valid ? v.spo2 : -1,
           v.tempValid ? v.tempC : -1.0f,
           v.ecgValid ? v.ecg : -1,
           millis() / 1000UL);
}

static void resetGsm() {
  gsmReady = false;
  sapbrCloseRetry = false;
  enterState(GSM_SEND_AT);
}

static bool consumeLine() {
  if (!lineReady) {
    return false;
  }
  strncpy(lineBuffer, pendingLine, GSM_LINE_BUF_SIZE - 1);
  lineBuffer[GSM_LINE_BUF_SIZE - 1] = '\0';
  lineReady = false;
  return true;
}

void gsmManagerInit() {
  gsmSerial.begin(GSM_BAUD);
  delay(1000);
  drainGsmSerial();
  gsmReady = false;
  vitalsQueued = false;
  lastCmd[0] = '\0';
  enterState(GSM_SEND_AT);
  Serial.println(F("[GSM] Initializing SIM800L..."));
}

bool gsmManagerIsReady() {
  return gsmReady;
}

void gsmManagerQueueVitals(const Vitals& vitals) {
  if (!gsmReady || gsmState == GSM_HTTP_ACTION ||
      gsmState == GSM_WAIT_HTTP_ACTION ||
      gsmState == GSM_WAIT_HTTP_BODY_DELAY) {
    return;
  }
  queuedVitals = vitals;
  vitalsQueued = true;
}

void gsmManagerUpdate() {
  readGsmResponses();

  switch (gsmState) {
    case GSM_IDLE:
      break;

    case GSM_SEND_AT:
      drainGsmSerial();
      sendAtCommand("AT");
      enterState(GSM_WAIT_AT_OK);
      break;

    case GSM_WAIT_AT_OK:
      if (consumeLine()) {
        if (lineHasOk(lineBuffer)) {
          sendAtCommand("AT+CPIN?");
          enterState(GSM_WAIT_SIM);
        }
      } else if (stateTimedOut(GSM_CMD_TIMEOUT_MS)) {
        Serial.println(F("[GSM] AT timeout - retry"));
        resetGsm();
      }
      break;

    case GSM_WAIT_SIM:
      if (consumeLine()) {
        if (strstr(lineBuffer, "READY") || lineHasOk(lineBuffer)) {
          sendAtCommand("AT+CREG?");
          enterState(GSM_WAIT_REG);
        } else if (lineHasError(lineBuffer)) {
          resetGsm();
        }
      } else if (stateTimedOut(GSM_CMD_TIMEOUT_MS)) {
        resetGsm();
      }
      break;

    case GSM_WAIT_REG:
      if (consumeLine()) {
        if (isCregRegistered(lineBuffer)) {
          sendAtCommand("AT+CGATT=1");
          enterState(GSM_WAIT_CGATT);
        }
      } else if (stateTimedOut(GSM_REG_TIMEOUT_MS)) {
        Serial.println(F("[GSM] Network registration timeout"));
        resetGsm();
      }
      break;

    case GSM_WAIT_CGATT:
      if (consumeLine() && lineHasOk(lineBuffer)) {
        sendAtCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
        enterState(GSM_WAIT_SAPBR_CONTYPE);
      } else if (stateTimedOut(GSM_CMD_TIMEOUT_MS)) {
        resetGsm();
      }
      break;

    case GSM_WAIT_SAPBR_CONTYPE:
      if (consumeLine() && lineHasOk(lineBuffer)) {
        char apnCmd[64];
        snprintf(apnCmd, sizeof(apnCmd), "AT+SAPBR=3,1,\"APN\",\"%s\"", GSM_APN);
        sendAtCommand(apnCmd);
        enterState(GSM_WAIT_SAPBR_APN);
      } else if (stateTimedOut(GSM_CMD_TIMEOUT_MS)) {
        resetGsm();
      }
      break;

    case GSM_WAIT_SAPBR_APN:
      if (consumeLine() && lineHasOk(lineBuffer)) {
        sendAtCommand("AT+SAPBR=1,1");
        enterState(GSM_WAIT_SAPBR_OPEN);
      } else if (stateTimedOut(GSM_CMD_TIMEOUT_MS)) {
        resetGsm();
      }
      break;

    case GSM_WAIT_SAPBR_OPEN:
      if (consumeLine()) {
        if (lineHasOk(lineBuffer)) {
          gsmReady = true;
          enterState(GSM_READY);
          sapbrCloseRetry = false;
          Serial.println(F("[GSM] Ready for HTTP"));
        } else if (lineHasError(lineBuffer) && !sapbrCloseRetry) {
          sapbrCloseRetry = true;
          sendAtCommand("AT+SAPBR=0,1");
          enterState(GSM_WAIT_SAPBR_CLOSE);
        } else if (lineHasError(lineBuffer)) {
          resetGsm();
        }
      } else if (stateTimedOut(GSM_CMD_TIMEOUT_MS)) {
        resetGsm();
      }
      break;

    case GSM_WAIT_SAPBR_CLOSE:
      if (consumeLine() && (lineHasOk(lineBuffer) || lineHasError(lineBuffer))) {
        sendAtCommand("AT+SAPBR=1,1");
        enterState(GSM_WAIT_SAPBR_OPEN);
      } else if (stateTimedOut(GSM_CMD_TIMEOUT_MS)) {
        resetGsm();
      }
      break;

    case GSM_READY:
      if (vitalsQueued && (millis() - lastPostMs >= (unsigned long)GSM_POST_INTERVAL_MS)) {
        buildHttpJson(queuedVitals, httpBody, sizeof(httpBody));
        vitalsQueued = false;
        sendAtCommand("AT+HTTPINIT");
        enterState(GSM_WAIT_HTTP_INIT);
      }
      break;

    case GSM_WAIT_HTTP_INIT:
      if (consumeLine() && lineHasOk(lineBuffer)) {
        char urlCmd[96];
        snprintf(urlCmd, sizeof(urlCmd), "AT+HTTPPARA=\"URL\",\"%s\"", GSM_SERVER_URL);
        sendAtCommand(urlCmd);
        enterState(GSM_WAIT_HTTP_URL);
      } else if (stateTimedOut(GSM_CMD_TIMEOUT_MS)) {
        resetGsm();
      }
      break;

    case GSM_WAIT_HTTP_URL:
      if (consumeLine() && lineHasOk(lineBuffer)) {
        char dataCmd[32];
        snprintf(dataCmd, sizeof(dataCmd), "AT+HTTPDATA=%d,10000", (int)strlen(httpBody));
        sendAtCommand(dataCmd);
        enterState(GSM_WAIT_HTTP_DATA_PROMPT);
      } else if (stateTimedOut(GSM_CMD_TIMEOUT_MS)) {
        resetGsm();
      }
      break;

    case GSM_WAIT_HTTP_DATA_PROMPT:
      if (consumeLine() && strstr(lineBuffer, "DOWNLOAD")) {
        enterState(GSM_WAIT_HTTP_BODY_DELAY);
      } else if (stateTimedOut(GSM_CMD_TIMEOUT_MS)) {
        resetGsm();
      }
      break;

    case GSM_WAIT_HTTP_BODY_DELAY:
      if (stateTimedOut(GSM_HTTP_BODY_DELAY_MS)) {
        gsmSerial.print(httpBody);
        enterState(GSM_HTTP_ACTION);
      }
      break;

    case GSM_HTTP_ACTION:
      sendAtCommand("AT+HTTPACTION=1");
      enterState(GSM_WAIT_HTTP_ACTION);
      break;

    case GSM_WAIT_HTTP_ACTION:
      if (consumeLine() && strstr(lineBuffer, "+HTTPACTION:")) {
        sendAtCommand("AT+HTTPTERM");
        enterState(GSM_WAIT_HTTP_TERM);
        lastPostMs = millis();
        Serial.println(F("[GSM] HTTP POST complete"));
      } else if (stateTimedOut(GSM_HTTP_TIMEOUT_MS)) {
        Serial.println(F("[GSM] HTTP timeout"));
        sendAtCommand("AT+HTTPTERM");
        enterState(GSM_WAIT_HTTP_TERM);
      }
      break;

    case GSM_WAIT_HTTP_TERM:
      if (consumeLine() && lineHasOk(lineBuffer)) {
        enterState(GSM_READY);
      } else if (stateTimedOut(GSM_CMD_TIMEOUT_MS)) {
        enterState(GSM_READY);
      }
      break;

    case GSM_ERROR:
    default:
      resetGsm();
      break;
  }
}
