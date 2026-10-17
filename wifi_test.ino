#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLECharacteristic *pTX;
bool deviceConnected = false;
String rxload = "";

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("手機已連線");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("手機已斷線");

    // 斷線後重新廣播，讓 LightBlue 可以再次找到
    pServer->getAdvertising()->start();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {

    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      rxload = "";

      for (int i = 0; i < rxValue.length(); i++) {
        rxload += (char)rxValue[i];
      }

      Serial.print("手機傳來：");
      Serial.println(rxload);
    }
  }
};

void setupBLE(String BLEName) {

  BLEDevice::init(BLEName.c_str());

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService =
      pServer->createService(SERVICE_UUID);

  // ESP32 → 手機
  pTX = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_NOTIFY
  );

  pTX->addDescriptor(new BLE2902());

  // 手機 → ESP32
  BLECharacteristic *pRX =
      pService->createCharacteristic(
          CHARACTERISTIC_UUID_RX,
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_WRITE_NR
      );

  pRX->setCallbacks(new MyCallbacks());

  pService->start();

  pServer->getAdvertising()->start();

  Serial.println("BLE 已啟動");
}

void setup() {

  Serial.begin(115200);

  // 教室很多人時，改成自己的名稱
  setupBLE("ESP32_KaiYu");
}

void loop() {

  // 收到手機傳來的資料
  if (deviceConnected && rxload.length() > 0) {

    Serial.print("收到：");
    Serial.println(rxload);

    rxload = "";
  }

  // 每兩秒傳一次訊息給手機
  if (deviceConnected) {

    if (Serial.available() > 0) {

      String message = Serial.readString();
    
      pTX->setValue(message.c_str());
      pTX->notify();
    
      Serial.print("已傳給手機：");
      Serial.println(message);
    }
  }

  delay(2000);
}
