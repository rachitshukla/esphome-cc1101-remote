#include <Arduino.h>
#include <SPI.h>

namespace {
constexpr int sck = 18;
constexpr int mosi = 23;
constexpr int miso = 19;
constexpr int cs = 5;
constexpr int gdo0 = 4;
constexpr int gdo2 = 27;
constexpr uint8_t sres = 0x30;
constexpr uint8_t partnum = 0x30;
constexpr uint8_t version = 0x31;
constexpr uint8_t packetAddress = 0x09;
constexpr uint8_t stateRegister = 0x35;
const SPISettings settings(1000000, MSBFIRST, SPI_MODE0);

bool waitReady() {
  const uint32_t start = micros();
  while (digitalRead(miso) == HIGH) {
    if (micros() - start >= 10000) {
      return false;
    }
  }
  return true;
}

void deselect() {
  digitalWrite(cs, HIGH);
  SPI.endTransaction();
}

bool select() {
  SPI.beginTransaction(settings);
  digitalWrite(cs, LOW);
  if (!waitReady()) {
    deselect();
    return false;
  }
  return true;
}

bool resetRadio() {
  SPI.end();
  pinMode(sck, OUTPUT);
  pinMode(mosi, OUTPUT);
  digitalWrite(sck, HIGH);
  digitalWrite(mosi, LOW);
  digitalWrite(cs, HIGH);
  delayMicroseconds(10);
  digitalWrite(cs, LOW);
  delayMicroseconds(10);
  digitalWrite(cs, HIGH);
  delayMicroseconds(50);
  SPI.begin(sck, miso, mosi, -1);
  pinMode(miso, INPUT_PULLUP);
  if (!select()) {
    Serial.println("FAIL: SO stayed high before SRES");
    return false;
  }
  SPI.transfer(sres);
  const bool ready = waitReady();
  deselect();
  if (!ready) {
    Serial.println("FAIL: SO stayed high after SRES");
  }
  return ready;
}

bool readRegister(uint8_t address, bool status, uint8_t &value) {
  if (!select()) {
    Serial.printf("FAIL: SO timeout reading 0x%02X\n", address);
    return false;
  }
  SPI.transfer(address | (status ? 0xC0 : 0x80));
  value = SPI.transfer(0);
  deselect();
  return true;
}

bool writeAddress(uint8_t value) {
  if (!select()) {
    Serial.println("FAIL: SO timeout writing ADDR");
    return false;
  }
  SPI.transfer(packetAddress);
  SPI.transfer(value);
  deselect();
  return true;
}

bool checkAddress(uint8_t expected) {
  uint8_t observed = 0;
  if (!readRegister(packetAddress, false, observed)) {
    return false;
  }
  Serial.printf("ADDR expected=0x%02X readback=0x%02X\n", expected, observed);
  return observed == expected;
}

struct Register {
  const char *name;
  uint8_t address;
  bool status;
};

constexpr Register registers[] = {
    {"PARTNUM", partnum, true}, {"VERSION", version, true},
    {"MARCSTATE", stateRegister, true}, {"PKTSTATUS", 0x38, true},
    {"RXBYTES", 0x3B, true}, {"TXBYTES", 0x3A, true},
    {"IOCFG2", 0x00, false}, {"IOCFG0", 0x02, false},
    {"FREQ2", 0x0D, false}, {"FREQ1", 0x0E, false},
    {"FREQ0", 0x0F, false}, {"MDMCFG4", 0x10, false},
    {"MDMCFG2", 0x12, false}, {"PKTCTRL0", 0x08, false},
};

bool diagnose() {
  if (!resetRadio()) {
    return false;
  }

  uint8_t values[sizeof(registers) / sizeof(registers[0])] = {};
  for (size_t i = 0; i < sizeof(values); ++i) {
    const Register &reg = registers[i];
    if (!readRegister(reg.address, reg.status, values[i])) {
      return false;
    }
    Serial.printf("%-10s 0x%02X\n", reg.name, values[i]);
  }

  const uint8_t expectedPart = values[0];
  const uint8_t expectedVersion = values[1];
  if (expectedPart != 0 || expectedVersion == 0 || expectedVersion == 0xFF ||
      (values[2] & 0x1F) != 0x01) {
    Serial.println("FAIL: unexpected CC1101 identity or post-reset state");
    return false;
  }

  for (unsigned i = 0; i < 32; ++i) {
    uint8_t part = 0;
    uint8_t revision = 0;
    if (!readRegister(partnum, true, part) ||
        !readRegister(version, true, revision)) {
      return false;
    }
    if (part != expectedPart || revision != expectedVersion) {
      Serial.printf("FAIL: identity read %u changed to %02X/%02X\n",
                    i + 1, part, revision);
      return false;
    }
  }
  Serial.println("Identity stable across 32 repeated reads");

  uint8_t original = 0;
  if (!readRegister(packetAddress, false, original)) {
    return false;
  }
  const bool first = writeAddress(0xA5) && checkAddress(0xA5);
  const bool second = writeAddress(0x5A) && checkAddress(0x5A);
  const bool restored = writeAddress(original) && checkAddress(original);
  if (!restored) {
    resetRadio();
  }
  return first && second && restored;
}
}  // namespace

void setup() {
  Serial.begin(115200);
  digitalWrite(cs, HIGH);
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  pinMode(gdo0, INPUT);
  pinMode(gdo2, INPUT);
  delay(100);
}

void loop() {
  Serial.println("\nCC1101 SPI diagnostic: SCK=18 MOSI=23 MISO=19 CS=5");
  Serial.println(diagnose() ? "PASS: CC1101 SPI reads and writes verified"
                           : "FAIL: CC1101 SPI diagnostic");
  delay(5000);
}
