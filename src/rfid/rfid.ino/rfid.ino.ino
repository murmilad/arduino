

/* Дмитрий Осипов. http://www.youtube.com/user/d36073?feature=watch
 
 v.01 Arduino Метро Единый билет RC522 Card Read Module RFID
 Version 0.1 2014/06/08
 ====================================
 бесконтактные билеты для прохода в метро.
 читаем данные с бумажного билета "Единый" – 
 бумажный (билет / карта / проездной) на метро,
 который теперь действует и на наземном транспорте.
 
 ----------------
 Печатаем в serial монитор информацию о карте:
 
 1)  тип карты
 
 2) Card UID — уникальный идентификатор карты (суть серийный номер).
 
 3) Номер билета (который отпечатан на нем).
 
 4) Дата выдачи билета.
 
 5) Срок действия в днях.
 
 6) количество оставшихся поездок.
 ------------------ 
 Базовый принцип действия В карточке и в считывателе есть антенны,
 при этом сигнал считывающего устройства (суть электромагнитное поле)
 одновременно служит источником
 питания для карточки. Т.е. и в части энергетики, и в части передачи 
 данных это очень похоже на беспроводные зарядки.
 --------------------
 что нам понадобится:
 1) Arduino.
 
 2) RC522 Card Read Module
 Mifare RC522 Card Read Module Tags SPI Interface Read and Write.
 http://www.ebay.com/itm/321297386758
 — Поддерживаемые типы карт: MIFARE S50, MIFARE S70, 
 MIFARE UltraLight, MIFARE Pro, MIFARE DESfire.
 ! Напряжение / питание: 3.3 v.
 
 3) Библиотека "RFID", для RC522.
 Скачать http://yadi.sk/d/6XLMLCuxSjdGa
 -----------------------------
 
 Скачать sketch.
 v.01 Arduino Метро Единый билет RC522 Card Read Module RFID
 
 ----------------------------
 Подробную видео инструкцию выложу здесь.
 v.01 Arduino Метро Единый билет RC522 Card Read Module RFID
 
 
 ============================================
 Благодарю человека (имя не знаю), за статью, 
 Считыватель карточек RFID RC522 в домашнем хозяйстве.
 http://mysku.ru/blog/aliexpress/23114.html
 
 */



#include <SPI.h>
#include <MFRC522.h> // это скачанная библиотека "RFID".

/*
подключение для Arduino Uno и Mega, производится к разным Pin!
 ----------------------------------------------------- Nicola Coppola
 * Pin layout should be as follows:
 * Signal     Pin              Pin               Pin
 *            Arduino Uno      Arduino Mega      MFRC522 board
 * ------------------------------------------------------------
 * Reset      9                5                 RST
 * SPI SS     10               53                SDA
 * SPI MOSI   11               51                MOSI
 * SPI MISO   12               50                MISO
 * SPI SCK    13               52                SCK
 
 */

// два Pin (SS и RST) допускают произвольное подключение и конфигурируются в коде.
// !(SS - он же - SDA).

#define SS_PIN 53
#define RST_PIN 5

MFRC522 mfrc522(SS_PIN, RST_PIN);       // объект MFRC522
unsigned long uidDec, uidDecTemp; // для отображения номера карточки в десятичном формате.
byte bCounter, readBit;
unsigned long ticketNumber;


void setup() {
  Serial.begin(9600);     
  SPI.begin();            // инициализация SPI.
  mfrc522.PCD_Init();     // инициализация MFRC522.
  Serial.println("Prilozhite kartu / Waiting for card...");
}

void loop() {
  // Поиск новой карточки.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Выбор карточки.
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  uidDec = 0;
  Serial.println("MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");
  Serial.println("================================================");

  // Выдача серийного номера карточки.
  Serial.println("Serijnyj nomer karty / Card UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidDecTemp = mfrc522.uid.uidByte[i];
    uidDec = uidDec*256+uidDecTemp;
  } 
  Serial.print("            [");
  Serial.print(uidDec);
  Serial.println("]");
  Serial.println("================================================");

  // Выдача типа карты.
  byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak); // запрос типа.
  Serial.println("Tip karty / Card type: ");

  Serial.print(" [");
  Serial.print(mfrc522.PICC_GetTypeName(piccType)); // трансляция типа в читаемый вид.
  Serial.println("]");
  Serial.println("================================================");
  if (piccType != MFRC522::PICC_TYPE_MIFARE_UL) { // если не билетная карта.

    Serial.println("Neizvestnaja karta / Not a valid card: Type"); 
    Serial.print("            [");
    Serial.print(piccType); 
    Serial.println("]");  
    Serial.println("================================================");
    Serial.println("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"); 

    // Halt PICC
    mfrc522.PICC_HaltA();  // остановка чипа.
    return;
    delay(1);
  }

  // сюда мы приедем, если чип правильный.
  byte status;
  byte byteCount;
  byte buffer[18]; // длина массива (16 байт + 2 байта контрольная сумма). 
  byte pages[2] = {
    4, 8                      }; // страницы с данными.

  byte pageByte; // счетчик байтов страницы

  byteCount = sizeof(buffer);
  byte bCount = 0;


  for (byte i = 0; i<2; i++) { // начинаем читать страницы.
    status = mfrc522.MIFARE_Read(pages[i], buffer, &byteCount);

    if (status != MFRC522::STATUS_OK) {
      Serial.print("Read error: ");
      Serial.println(mfrc522.GetStatusCodeName(status));
    }
    else {
      if (pages[i] == 4) {
        bCounter = 0; // 32-битный счетчик для номера.

        // биты 0-3.
        for (bCount = 0; bCount < 4; bCount++) {
          readBit = bitRead(buffer[6], (bCount+4));
          setBitsForGood(readBit);
        }

        // биты 4 - 27.
        for (pageByte = 5; pageByte > 2; pageByte--) {
          for (bCount = 0; bCount<8; bCount++) {
            readBit = bitRead(buffer[pageByte], bCount);
            setBitsForGood(readBit);
          }
        }

        // биты 28-31.
        for (bCount = 0; bCount < 4; bCount++) {
          readBit = bitRead(buffer[2], bCount);
          setBitsForGood(readBit);
        }

        Serial.println("Napechatannyj nomer na karte / Ticket number: ");
        Serial.print("            [");
        Serial.print(ticketNumber, DEC);
        Serial.println("]");
        Serial.println("================================================");
      }

      if (pages[i] == 8) { // читаем дату выдачи.

        Serial.println("Data pokupki karty / Issued: ");
        Serial.println("    Chislo Mesjac God");
        // количество дней с 01.01.1992 в десятичном формате, 256 - сдвиг на 8 бит.
        unsigned int issueDate = buffer[0] * 256 + buffer[1];
        printIssueDate(issueDate);
        Serial.println("================================================");

        Serial.println("Srok dejstvija karty / Good for (days): "); // срок действия.
        Serial.print("            [");
        Serial.print(buffer[2], DEC);                                                           
        Serial.print("]");  
        Serial.println(" Dnej");        
        Serial.println("================================================");
        Serial.println("Kolichestvo ostavshihsja poezdok / Trip reminder: ");
        Serial.print("            [");
        // количество оставшихся поездок.
        Serial.print(buffer[5], DEC);
        if (buffer[5] == 0) {
          digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
          delay(1000);                       // wait for a second
          digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        }                                                           
        Serial.println("]");  

        Serial.println("================================================");
        Serial.println("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");      
      }
    }
  }



  // Halt PICC
  mfrc522.PICC_HaltA();                       

}

void printIssueDate(unsigned int incoming) {

  boolean isLeap = true; // признак високосного года.
  int days[]={
    // последний по порядку день месяца для обычного года.
    0,31,59,90,120,151,181,212,243,273,304,334                      };
  byte dayOfMonth, monthCounter;
  unsigned int yearCount;

  // подогнал под ответ, но возможно это как раз необходимая. 
  // коррекция, потому что начало отсчета - 01.01.1992, а не 00.01.1992.
  //  incoming = incoming+1; 

  // считаем год и количество дней, прошедших с выдачи билета.
  for (yearCount = 1992; incoming >366; yearCount++) { 

    if ((yearCount % 4 == 0 && yearCount % 100 != 0) ||  yearCount % 400 == 0) {
      incoming = incoming - 366;
      isLeap = true;
    } 
    else {
      incoming = incoming - 365;
      isLeap = false;
    }
  }
  // узнаем номер месяца.
  for (monthCounter = 0; incoming > days[monthCounter]; monthCounter++) {
  }

  // считаем день месяца.

  if (isLeap == true) { // если високосный год.

    // если не первый месяц, то добавляем к последнему дню месяца единицы.
    if (days[monthCounter-1]>31) { 
      dayOfMonth = incoming - (days[monthCounter-1]+ 1);
    } 
    else {
      dayOfMonth = incoming - (days[monthCounter-1]); 
    }
  }
  // если первый - ничего не добавляем, потому что сдвиг начинается с февраля. 
  else {
    dayOfMonth = incoming - (days[monthCounter-1]); // если не високосный год.
  }
  Serial.print("            [");
  Serial.print(dayOfMonth);
  Serial.print(".");
  Serial.print(monthCounter);
  Serial.print(".");
  Serial.print(yearCount);
  Serial.println("]"); 
}

void setBitsForGood(byte daBeat) {
  if (daBeat == 1) {
    bitSet(ticketNumber, bCounter);
    bCounter=bCounter+1;
  }
  else {
    bitClear(ticketNumber, bCounter);
    bCounter=bCounter+1;
  }
}








