//BIBLIOTEKI
#include <DS3231.h> //MODUŁ RTC
#include <LiquidCrystal_I2C.h> //WYSWIETLACZ
#include <Key.h> //KLAWISZ
#include <Keypad.h> //Klawiatura
#include <Wire.h> //MAGISTRALA I2C
#include <EEPROM.h> //PAMIEC EEPROM
//Zdefiniowanie wyswietlacza na magistrali I2C
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
//Moduł RTC
DS3231 zegar;
RTCDateTime czas;
//Zdefiniowanie klawiatury
byte lastDay;
const byte rzedy = 4;
const byte kolumny = 4;
char klawisze[rzedy][kolumny] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pinyRzedy[rzedy] = {2, 3, 4, 5};
byte pinyKolumny[kolumny] = {A0, A1, A2, A3};
Keypad keypad = Keypad( makeKeymap(klawisze), pinyRzedy, pinyKolumny, rzedy, kolumny );
char klawisz;
//Zmienne wyswietlacza
boolean wylacz_podswietlenie = false;
unsigned int podswietlenie;
String output;
//Lokalne zmienne do kofniguracji
String rok = "";
String miesiac = "";
String dzien = "";
String godzina = "";
String minuta = "";
String sekunda = "";
String czasDzwonka =  "";
byte wybierzProgram =  0;
String iloscLekcji = "";
String calkowityCzas = "";
String czasPrzerwy = "";
String czasDzwonienia = "";
byte dzienTygodnia = 0;
int dzwonek_czas;
//Bierzace zmienne uzywane przez algorytm
int alarmTotal;
byte alarmRing;
byte alarmBreak;
byte obecnyProgram;
byte dzwonekTimeout ;
byte iloscDzwonkow;
int dzwonki[32];
int licznik;

void setup() {
  keypad.setDebounceTime(60);
  //Pobranie wartosci z pamieci EEPROM
  EEPROM.get(0, dzwonekTimeout);
  EEPROM.get(1, obecnyProgram);
  EEPROM.get(2, iloscDzwonkow);
  EEPROM.get(3, alarmRing);
  EEPROM.get(4, alarmBreak);
  EEPROM.get(5, alarmTotal);
  //Moduł zegara I2C RTC
  zegar.begin();
  zegar.armAlarm2(false);
  zegar.clearAlarm2();
  zegar.setAlarm2(0, 0, 0,  DS3231_MATCH_H_M);
  czas = zegar.getDateTime();
  //Zdefiniowanie pinów 
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  pinMode(11, OUTPUT);
  digitalWrite(11, LOW);
  //Wyswietlacz
  lcd.begin(16, 2);
  lcd.backlight();
  czas = zegar.getDateTime();
  //Pobranie informacji o dzwonkach
  zaladujDzwonki();
  licznik = ustawLicznik();
  wylacz_podswietlenie = true;
  podswietlenie = czas.hour * 60 + czas.minute + 1;
  if (podswietlenie > 1438) {
    podswietlenie = 0;
  }
  lcd.clear();
  lcd.setCursor(0, 0);
}

void loop() {
  //Skrypt wykonywany raz na dobę o godz. 00:00
  lcd.clear();
  czas = zegar.getDateTime();
  zaladujDzwonki();
  licznik = ustawLicznik();
  lastDay = byte(czas.day);
  //Skrypt wykonywany w pętli w ciągu doby
  do {
    czas = zegar.getDateTime();
    output = "";
    //Weryfikacja czy wykonac dzwonek
    dzwonek_czas = czas.hour * 60 + czas.minute;
    if(dzwonki[licznik] == dzwonek_czas && licznik != -1){
      lcd.backlight();
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Dzwonek " + String(licznik+1));
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(12, LOW);
      delay(dzwonekTimeout*1000);
      digitalWrite(LED_BUILTIN, LOW);
      digitalWrite(12, HIGH);
      lcd.clear();
      lcd.setCursor(0,0);
      licznik = ustawLicznik();
      wylacz_podswietlenie = true;
      podswietlenie = czas.hour * 60 + czas.minute + 1;
      if (podswietlenie > 1438) {
    podswietlenie = 0;
  }
    }
    //Standardowe wyjscie na ekran LCD
    lcd.setCursor(0, 0);
    switch (czas.dayOfWeek) {
      case 1: lcd.print("Pon. "); break;
      case 2: lcd.print("Wt. "); break;
      case 3: lcd.print("Sr. "); break;
      case 4: lcd.print("Czw. "); break;
      case 5: lcd.print("Pt. "); break;
      case 6: lcd.print("Sob. "); break;
      case 7: lcd.print("Niedz. "); break;
      default: break;
    }
    if (czas.hour < 10) {
      lcd.print(String("0" + String(czas.hour) + ":"));
    } else {
      lcd.print(String(czas.hour) + ":");
    }
    if (czas.minute < 10) {
      lcd.print(String("0" + String(czas.minute) + ":"));
    } else {
      lcd.print(String(czas.minute) + ":");
    }
    if (czas.second < 10) {
      lcd.print(String("0" + String(czas.second)));
    } else {
      lcd.print(String(czas.second));
    }
    lcd.setCursor(0, 1);
    lcd.print("Temp. " + String(zegar.readTemperature()) + "C");

    //Modul klawiatury
    klawisz = keypad.getKey();
    if (klawisz == 'A') {
      //Alarm PPOZ
      digitalWrite(11, HIGH);
      lcd.backlight();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Aby wylaczyc");
      lcd.setCursor(0, 1);
      lcd.print("odlacz zasilanie");
      czas = zegar.getDateTime();
      unsigned long timeOut = czas.unixtime + alarmTotal;
      do {
        digitalWrite(LED_BUILTIN, HIGH);
        digitalWrite(12, LOW);
        delay(alarmRing * 1000);
        digitalWrite(LED_BUILTIN, LOW);
        digitalWrite(12, HIGH);
        delay(alarmBreak * 1000);
        czas = zegar.getDateTime();
      } while (timeOut > czas.unixtime);
      lcd.clear();
      digitalWrite(11, LOW);
      wylacz_podswietlenie = true;
      podswietlenie = czas.hour * 60 + czas.minute + 1;
      if (podswietlenie > 1438) {
        podswietlenie = 0;
      }
    } else if (klawisz == '#') {
      //MODUL KONFIGURACJI
      lcd.clear();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("Wybierz opcje");
      lcd.setCursor(0, 1);
      lcd.cursor();
      lcd.blink();
      klawisz = keypad.waitForKey();
      switch (klawisz) {
        //Konfiguracja czasu
        case '1':
          konfiguratorCzasu();
          czas = zegar.getDateTime();
          wylacz_podswietlenie = true;
          podswietlenie = czas.hour * 60 + czas.minute + 1;
          if (podswietlenie > 1438) {
            podswietlenie = 0;
          }
          break;
        //Konfiguracja dlugosci dzownka
        case '2':
          konfiguratorCzasuDzwonka();
          czas = zegar.getDateTime();
          wylacz_podswietlenie = true;
          podswietlenie = czas.hour * 60 + czas.minute + 1;
          if (podswietlenie > 1438) {
            podswietlenie = 0;
          }
          break;
        //Konfiguracja wyboru programu
        case '3':
          konfiguratorWyboruProgramu();
          czas = zegar.getDateTime();
          wylacz_podswietlenie = true;
          podswietlenie = czas.hour * 60 + czas.minute + 1;
          if (podswietlenie > 1438) {
            podswietlenie = 0;
          }
          break;
        //Konfiguracja ilosci lekcji
        case '4':
          konfiguratorIlosciLekcji();
          czas = zegar.getDateTime();
          wylacz_podswietlenie = true;
          podswietlenie = czas.hour * 60 + czas.minute + 1;
          if (podswietlenie > 1438) {
            podswietlenie = 0;
          }
          break;
        //Konfiguracja alarmu
        case '5':
          konfiguratorAlarmu();
          czas = zegar.getDateTime();
          wylacz_podswietlenie = true;
          podswietlenie = czas.hour * 60 + czas.minute + 1;
          if (podswietlenie > 1438) {
            podswietlenie = 0;
          }
          break;
        //Konfiguracja dzownkow
        case '6':
          konfiguratorDzwonkow();
          czas = zegar.getDateTime();
          wylacz_podswietlenie = true;
          podswietlenie = czas.hour * 60 + czas.minute + 1;
          if (podswietlenie > 1438) {
            podswietlenie = 0;
          }
          break;
        //Podswietlenie
        default:
          wylacz_podswietlenie = true;
          czas = zegar.getDateTime();
          podswietlenie = czas.hour * 60 + czas.minute + 1;
          if (podswietlenie > 1438) {
            podswietlenie = 0;
          }
          break;
      }
      lcd.noBlink();
      lcd.noCursor();
      lcd.clear();
    } else if (klawisz != NO_KEY && klawisz != 'A' && klawisz != '#') {
      lcd.backlight();
      wylacz_podswietlenie = true;
      podswietlenie = czas.hour * 60 + czas.minute + 1;
      if (podswietlenie > 1438) {
        podswietlenie = 0;
      }
    }

    //Modul wygaszenia podswietlenia
    if (wylacz_podswietlenie) {
      if (podswietlenie == 0) {
        if ((czas.hour * 60 + czas.minute) == 1) {
          lcd.noBacklight();
          wylacz_podswietlenie = false;
          podswietlenie = 0;
        }
      } else if ((czas.hour * 60 + czas.minute) > podswietlenie) {
        lcd.noBacklight();
        wylacz_podswietlenie = false;
        podswietlenie = 0;
      }
    }
  } while (!zegar.isAlarm2() && byte(czas.day) == lastDay);

}

void konfiguratorDzwonkow() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wybierz program:");
  lcd.setCursor(0, 1);
  wybierzProgram = 0;
  do {
    klawisz = keypad.waitForKey();
    if (klawisz == '1' || klawisz == '2') {
      wybierzProgram = byte(String(klawisz).toInt());
      lcd.setCursor(0, 1);
      lcd.print(wybierzProgram);
      lcd.setCursor(0, 1);
    } else if (klawisz == '#') {
      if (wybierzProgram == 1 || wybierzProgram == 2) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Dzien tygodnia:");
        lcd.setCursor(0, 1);
        dzienTygodnia = 0;
        do {
          klawisz = keypad.waitForKey();
          if (isDigit(klawisz)) {
            dzienTygodnia = byte(String(klawisz).toInt());
            lcd.setCursor(0, 1);
            lcd.print(dzienTygodnia);
            lcd.setCursor(0, 1);
          } else if (klawisz == '#') {
            if (dzienTygodnia >= 1 && dzienTygodnia <= 7) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Ilosc lekcji:");
              lcd.setCursor(0, 1);
              iloscLekcji = "";
              do {
                klawisz = keypad.waitForKey();
                if (isDigit(klawisz)) {
                  iloscLekcji += klawisz;
                  lcd.setCursor(0, 1);
                  lcd.print(iloscLekcji);
                } else if (klawisz == '#') {
                  if (iloscLekcji.toInt() <= 16  && iloscLekcji.toInt() >= 0) {
                    byte pozycja;
                    for (int x = 1; x <= iloscLekcji.toInt(); x++) {
                      lcd.clear();
                      lcd.setCursor(0, 0);
                      lcd.print("[" + String(x) + "] Poczatek:");
                      lcd.setCursor(0, 1);
                      lcd.print("Godzina: ");
                      godzina = "";
                      do {
                        klawisz = keypad.waitForKey();
                        if (isDigit(klawisz)) {
                          godzina += klawisz;
                          lcd.setCursor(9, 1);
                          lcd.print(godzina);
                        } else if (klawisz == '#') {
                          if (godzina.toInt() >= 0 && godzina.toInt() <= 23) {
                            break;
                          } else {
                            lcd.noBlink();
                            lcd.noCursor();
                            lcd.clear();
                            lcd.setCursor(0, 0);
                            lcd.print("Bledne dane!");
                            delay(2000);
                            lcd.clear();
                            lcd.print("[" + String(x) + "] Poczatek:");
                            lcd.setCursor(0, 1);
                            lcd.print("Godzina: ");
                            lcd.blink();
                            lcd.cursor();
                            lcd.setCursor(9, 1);
                            godzina = "";
                          }
                        } else if (klawisz == 'C') {
                          lcd.clear();
                          lcd.setCursor(0, 0);
                          lcd.print("[" + String(x) + "] Poczatek:");
                          lcd.setCursor(0, 1);
                          lcd.print("Godzina: ");
                          lcd.setCursor(9, 1);
                          godzina.remove(godzina.length() - 1);
                          lcd.print(godzina);
                        }
                      } while (true);
                      lcd.clear();
                      lcd.setCursor(0, 0);
                      lcd.print("[" + String(x) + "] Poczatek:");
                      lcd.setCursor(0, 1);
                      lcd.print("Minuta: ");
                      minuta = "";
                      do {
                        klawisz = keypad.waitForKey();
                        if (isDigit(klawisz)) {
                          minuta += klawisz;
                          lcd.setCursor(8, 1);
                          lcd.print(minuta);
                        } else if (klawisz == '#') {
                          if (minuta.toInt() >= 0 && minuta.toInt() <= 59) {
                            break;
                          } else {
                            lcd.noBlink();
                            lcd.noCursor();
                            lcd.clear();
                            lcd.setCursor(0, 0);
                            lcd.print("Bledne dane!");
                            delay(2000);
                            lcd.clear();
                            lcd.print("[" + String(x) + "] Poczatek:");
                            lcd.setCursor(0, 1);
                            lcd.print("Minuta: ");
                            lcd.blink();
                            lcd.cursor();
                            lcd.setCursor(8, 1);
                            minuta = "";
                          }
                        } else if (klawisz == 'C') {
                          lcd.clear();
                          lcd.setCursor(0, 0);
                          lcd.print("[" + String(x) + "] Poczatek:");
                          lcd.setCursor(0, 1);
                          lcd.print("Minuta: ");
                          lcd.setCursor(8, 1);
                          minuta.remove(minuta.length() - 1);
                          lcd.print(minuta);
                        }
                      } while (true);
                      pozycja = x * 2 - 2;
                      dzwonki[pozycja] = godzina.toInt() * 60 + minuta.toInt();
                      lcd.clear();
                      lcd.setCursor(0, 0);
                      lcd.print("[" + String(x) + "] Koniec:");
                      lcd.setCursor(0, 1);
                      lcd.print("Godzina: ");
                      godzina = "";
                      do {
                        klawisz = keypad.waitForKey();
                        if (isDigit(klawisz)) {
                          godzina += klawisz;
                          lcd.setCursor(9, 1);
                          lcd.print(godzina);
                        } else if (klawisz == '#') {
                          if (godzina.toInt() >= 0 && godzina.toInt() <= 23) {
                            break;
                          } else {
                            lcd.noBlink();
                            lcd.noCursor();
                            lcd.clear();
                            lcd.setCursor(0, 0);
                            lcd.print("Bledne dane!");
                            delay(2000);
                            lcd.clear();
                            lcd.print("[" + String(x) + "] Koniec:");
                            lcd.setCursor(0, 1);
                            lcd.print("Godzina: ");
                            lcd.blink();
                            lcd.cursor();
                            lcd.setCursor(9, 1);
                            godzina = "";
                          }
                        } else if (klawisz == 'C') {
                          lcd.clear();
                          lcd.setCursor(0, 0);
                          lcd.print("[" + String(x) + "] Koniec:");
                          lcd.setCursor(0, 1);
                          lcd.print("Godzina: ");
                          lcd.setCursor(9, 1);
                          godzina.remove(godzina.length() - 1);
                          lcd.print(godzina);
                        }
                      } while (true);
                      lcd.clear();
                      lcd.setCursor(0, 0);
                      lcd.print("[" + String(x) + "] Koniec:");
                      lcd.setCursor(0, 1);
                      lcd.print("Minuta: ");
                      minuta = "";
                      do {
                        klawisz = keypad.waitForKey();
                        if (isDigit(klawisz)) {
                          minuta += klawisz;
                          lcd.setCursor(8, 1);
                          lcd.print(minuta);
                        } else if (klawisz == '#') {
                          if (minuta.toInt() >= 0 && minuta.toInt() <= 59) {
                            break;
                          } else {
                            lcd.noBlink();
                            lcd.noCursor();
                            lcd.clear();
                            lcd.setCursor(0, 0);
                            lcd.print("Bledne dane!");
                            delay(2000);
                            lcd.clear();
                            lcd.print("[" + String(x) + "] Koniec:");
                            lcd.setCursor(0, 1);
                            lcd.print("Minuta: ");
                            lcd.blink();
                            lcd.cursor();
                            lcd.setCursor(8, 1);
                            minuta = "";
                          }
                        } else if (klawisz == 'C') {
                          lcd.clear();
                          lcd.setCursor(0, 0);
                          lcd.print("[" + String(x) + "] Koniec:");
                          lcd.setCursor(0, 1);
                          lcd.print("Minuta: ");
                          lcd.setCursor(8, 1);
                          minuta.remove(minuta.length() - 1);
                          lcd.print(minuta);
                        }
                      } while (true);
                      pozycja = x * 2 - 1;
                      dzwonki[pozycja] = godzina.toInt() * 60 + minuta.toInt();
                    }
                    for (int y = iloscLekcji.toInt() * 2; y < 32; y++) {
                      dzwonki[y] = -1;
                    }
                    zapiszDzwonki(wybierzProgram, dzienTygodnia);
                    zaladujDzwonki();
                    licznik = ustawLicznik();
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.noBlink();
                    lcd.noCursor();
                    lcd.print("Zapisano!!!");
                    delay(3000);
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    godzina = "";
                    minuta = "";
                    wybierzProgram = 0;
                    iloscLekcji = "";
                    dzienTygodnia = 0;
                    break;
                  } else {
                    lcd.noBlink();
                    lcd.noCursor();
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Bledne dane!");
                    delay(2000);
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Ilosc lekcji: ");
                    lcd.blink();
                    lcd.cursor();
                    lcd.setCursor(0, 1);
                    iloscLekcji = "";
                  }
                } else if (klawisz == 'C') {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Ilosc lekcji:");
                  lcd.setCursor(0, 1);
                  iloscLekcji.remove(iloscLekcji.length() - 1);
                  lcd.print(iloscLekcji);
                } else if (klawisz == '*') {
                  iloscLekcji = "";
                  dzienTygodnia = 0;
                  wybierzProgram = 0;
                  break;
                }
              } while (true);
              break;
            } else {
              lcd.noBlink();
              lcd.noCursor();
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Bledne dane!");
              delay(2000);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Dzien tygodnia:");
              lcd.blink();
              lcd.cursor();
              lcd.setCursor(0, 1);
              dzienTygodnia = 0;
            }
          } else if (klawisz == '*') {
            dzienTygodnia = 0;
            wybierzProgram = 0;
            break;
          }
        } while (true);
        break;
      }
    } else if (klawisz == '*') {
      wybierzProgram = 0;
      break;
    }
  } while (true);
}

void konfiguratorAlarmu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Czas trwania:");
  lcd.setCursor(1, 1);
  lcd.print("s");
  lcd.setCursor(0, 1);
  calkowityCzas = "";
  do {
    klawisz = keypad.waitForKey();
    if (isDigit(klawisz)) {
      calkowityCzas += klawisz;
      lcd.setCursor(0, 1);
      lcd.print(calkowityCzas + "s");
      lcd.setCursor(calkowityCzas.length() - 1, 1);
    } else if (klawisz == '#') {
      if (calkowityCzas.toInt() > 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Czas dzwonienia:");
        lcd.setCursor(1, 1);
        lcd.print("s");
        lcd.setCursor(0, 1);
        czasDzwonienia = "";
        do {
          klawisz = keypad.waitForKey();
          if (isDigit(klawisz)) {
            czasDzwonienia += klawisz;
            lcd.setCursor(0, 1);
            lcd.print(czasDzwonienia + "s");
            lcd.setCursor(czasDzwonienia.length() - 1, 1);
          } else if (klawisz == '#') {
            if (czasDzwonienia.toInt() > 0) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Czas przerwy:");
              lcd.setCursor(1, 1);
              lcd.print("s");
              lcd.setCursor(0, 1);
              czasPrzerwy = "";
              do {
                klawisz = keypad.waitForKey();
                if (isDigit(klawisz)) {
                  czasPrzerwy += klawisz;
                  lcd.setCursor(0, 1);
                  lcd.print(czasPrzerwy + "s");
                  lcd.setCursor(czasPrzerwy.length() - 1, 1);
                } else if (klawisz == '#') {
                  if (czasPrzerwy.toInt() > 0) {
                    alarmTotal = calkowityCzas.toInt();
                    alarmRing = czasDzwonienia.toInt();
                    alarmBreak = czasPrzerwy.toInt();
                    EEPROM.put(3, alarmRing);
                    EEPROM.put(4, alarmBreak);
                    EEPROM.put(5, alarmTotal);
                    lcd.noCursor();
                    lcd.noBlink();
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Zapisano!!!");
                    delay(3000);
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    calkowityCzas = "";
                    czasDzwonienia = "";
                    czasPrzerwy = "";
                    break;
                  } else {
                    lcd.noBlink();
                    lcd.noCursor();
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Bledne dane!");
                    delay(2000);
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Czas przerwy:");
                    lcd.blink();
                    lcd.cursor();
                    lcd.setCursor(1, 1);
                    lcd.print("s");
                    lcd.setCursor(0, 1);
                    czasPrzerwy = "";
                  }
                } else if (klawisz == 'C') {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Czas Przerwy:");
                  lcd.setCursor(0, 1);
                  czasPrzerwy.remove(czasPrzerwy.length() - 1);
                  lcd.print(czasPrzerwy + "s");
                  lcd.setCursor(czasPrzerwy.length() - 1, 1);
                } else if (klawisz == '*') {
                  calkowityCzas = "";
                  czasDzwonienia = "";
                  czasPrzerwy = "";
                  break;
                }
              } while (true);
              break;
            } else {
              lcd.noBlink();
              lcd.noCursor();
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Bledne dane!");
              delay(2000);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Czas dzwonienia:");
              lcd.blink();
              lcd.cursor();
              lcd.setCursor(1, 1);
              lcd.print("s");
              lcd.setCursor(0, 1);
              czasDzwonienia = "";
            }
          } else if (klawisz == 'C') {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Czas dzwonienia:");
            lcd.setCursor(0, 1);
            czasDzwonienia.remove(czasDzwonienia.length() - 1);
            lcd.print(czasDzwonienia + "s");
            lcd.setCursor(czasDzwonienia.length() - 1, 1);
          } else if (klawisz == '*') {
            calkowityCzas = "";
            czasDzwonienia = "";
            break;
          }
        } while (true);
        break;
      } else {
        lcd.noBlink();
        lcd.noCursor();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Bledne dane!");
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Czas trwania:");
        lcd.blink();
        lcd.cursor();
        lcd.setCursor(1, 1);
        lcd.print("s");
        lcd.setCursor(0, 1);
        calkowityCzas = "";
      }
    } else if (klawisz == 'C') {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Czas trwania:");
      lcd.setCursor(0, 1);
      calkowityCzas.remove(calkowityCzas.length() - 1);
      lcd.print(calkowityCzas + "s");
      lcd.setCursor(calkowityCzas.length() - 1, 1);
    } else if (klawisz == '*') {
      calkowityCzas = "";
      break;
    }
  } while (true);
}

void konfiguratorIlosciLekcji() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ilosc lekcji:");
  lcd.setCursor(0, 1);
  iloscLekcji = "";
  do {
    klawisz = keypad.waitForKey();
    if (isDigit(klawisz)) {
      iloscLekcji += klawisz;
      lcd.setCursor(0, 1);
      lcd.print(iloscLekcji);
    } else if (klawisz == 'C') {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Ilosc lekcji:");
      lcd.setCursor(0, 1);
      iloscLekcji.remove(iloscLekcji.length() - 1);
      lcd.print(iloscLekcji);
    } else if (klawisz == '#') {
      if (iloscLekcji.toInt() <= 16  && iloscLekcji.toInt() >= 0) {
        iloscDzwonkow = iloscLekcji.toInt() * 2;
        EEPROM.put(2, iloscDzwonkow);
        lcd.clear();
        lcd.noBlink();
        lcd.noCursor();
        lcd.print("Zapisano!!!");
        delay(3000);
        zaladujDzwonki();
        licznik = ustawLicznik();
        lcd.clear();
        lcd.setCursor(0, 0);
        iloscLekcji = "";
        break;
      } else {
        lcd.noBlink();
        lcd.noCursor();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Bledne dane!");
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Ilosc lekcji: ");
        lcd.blink();
        lcd.cursor();
        lcd.setCursor(0, 1);
        iloscLekcji = "";
      }
    } else if (klawisz == '*') {
      iloscLekcji = "";
      break;
    }
  } while (true);
}

void konfiguratorWyboruProgramu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wybierz program");
  lcd.setCursor(0, 1);
  wybierzProgram = 0;
  do {
    klawisz = keypad.waitForKey();
    if (klawisz == '1' || klawisz == '2') {
      wybierzProgram = byte(String(klawisz).toInt());
      lcd.setCursor(0, 1);
      lcd.print(wybierzProgram);
      lcd.setCursor(0, 1);
    } else if (klawisz == '#') {
      if (wybierzProgram == 1 || wybierzProgram == 2) {
        obecnyProgram = wybierzProgram;
        EEPROM.put(1, obecnyProgram);
        lcd.clear();
        lcd.noBlink();
        lcd.noCursor();
        lcd.print("Program wybrano!");
        delay(3000);
        zaladujDzwonki();
        licznik = ustawLicznik();
        lcd.clear();
        lcd.setCursor(0, 0);
        wybierzProgram = 0;
        break;
      }
    } else if (klawisz == '*') {
      wybierzProgram = 0;
      break;
    }
  } while (true);
}

void konfiguratorCzasuDzwonka() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Czas dzwonka:");
  lcd.setCursor(1, 1);
  lcd.print("s");
  lcd.setCursor(0, 1);
  czasDzwonka =  "";
  do {
    klawisz = keypad.waitForKey();
    if (isDigit(klawisz)) {
      czasDzwonka += klawisz;
      lcd.setCursor(0, 1);
      lcd.print(czasDzwonka + "s");
      lcd.setCursor(czasDzwonka.length() - 1, 1);
    } else if (klawisz == 'C') {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Czas dzwonka:");
      lcd.setCursor(0, 1);
      czasDzwonka.remove(czasDzwonka.length() - 1);
      lcd.print(czasDzwonka + "s");
      lcd.setCursor(czasDzwonka.length() - 1, 1);
    } else if (klawisz == '#') {
      if (czasDzwonka.toInt() < 100 && czasDzwonka.toInt() > 0) {
        dzwonekTimeout = byte(czasDzwonka.toInt());
        EEPROM.put(0, dzwonekTimeout);
        lcd.noBlink();
        lcd.noCursor();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Zapisano!!!");
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0);
        czasDzwonka = "";
        break;
      } else {
        lcd.noBlink();
        lcd.noCursor();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Bledne dane!");
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Czas dzwonka:");
        lcd.blink();
        lcd.cursor();
        lcd.setCursor(1, 1);
        lcd.print("s");
        lcd.setCursor(0, 1);
        czasDzwonka = "";
      }
    } else if (klawisz == '*') {
      czasDzwonka = "";
      break;
    }
  } while (true);
}

void konfiguratorCzasu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wpisz rok:");
  rok = "";
  lcd.setCursor(0, 1);
  do {
    klawisz = keypad.waitForKey();
    if (isDigit(klawisz)) {
      rok += klawisz;
      lcd.setCursor(0, 1);
      lcd.print(rok);
    } else if (klawisz == 'C') {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wpisz rok:");
      lcd.setCursor(0, 1);
      rok.remove(rok.length() - 1);
      lcd.print(rok);
    } else if (klawisz == '#') {
      if (rok.toInt() > 2016 && rok.length() == 4) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wpisz miesiac:");
        lcd.setCursor(0, 1);
        miesiac = "";
        do {
          klawisz = keypad.waitForKey();
          if (isDigit(klawisz)) {
            miesiac += klawisz;
            lcd.setCursor(0, 1);
            lcd.print(miesiac);
          } else if (klawisz == 'C') {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Wpisz miesiac:");
            lcd.setCursor(0, 1);
            miesiac.remove(miesiac.length() - 1);
            lcd.print(miesiac);
          } else if (klawisz == '#') {
            if (miesiac.toInt() > 0 && miesiac.toInt() <= 12 && (miesiac.length() == 2 || miesiac.length() == 1)) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Wpisz dzien:");
              lcd.setCursor(0, 1);
              dzien = "";
              do {
                klawisz = keypad.waitForKey();
                if (isDigit(klawisz)) {
                  dzien += klawisz;
                  lcd.setCursor(0, 1);
                  lcd.print(dzien);
                } else if (klawisz == 'C') {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Wpisz dzien:");
                  lcd.setCursor(0, 1);
                  dzien.remove(dzien.length() - 1);
                  lcd.print(dzien);
                } else if (klawisz == '#') {
                  if (dzien.toInt() > 0 && dzien.toInt() <= 31 && (dzien.length() == 1 || dzien.length() == 2)) {
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Wpisz godzine:");
                    lcd.setCursor(0, 1);
                    godzina = "";
                    do {
                      klawisz = keypad.waitForKey();
                      if (isDigit(klawisz)) {
                        godzina += klawisz;
                        lcd.setCursor(0, 1);
                        lcd.print(godzina);
                      } else if (klawisz == 'C') {
                        lcd.clear();
                        lcd.setCursor(0, 0);
                        lcd.print("Wpisz godzine:");
                        lcd.setCursor(0, 1);
                        godzina.remove(godzina.length() - 1);
                        lcd.print(godzina);
                      } else if (klawisz == '#') {
                        if (godzina.toInt() >= 0 && godzina.toInt() <= 23 && (godzina.length() == 1 || godzina.length() == 2)) {
                          lcd.clear();
                          lcd.setCursor(0, 0);
                          lcd.print("Wpisz minute:");
                          lcd.setCursor(0, 1);
                          minuta = "";
                          do {
                            klawisz = keypad.waitForKey();
                            if (isDigit(klawisz)) {
                              minuta += klawisz;
                              lcd.setCursor(0, 1);
                              lcd.print(minuta);
                            } else if (klawisz == 'C') {
                              lcd.clear();
                              lcd.setCursor(0, 0);
                              lcd.print("Wpisz minute:");
                              lcd.setCursor(0, 1);
                              minuta.remove(minuta.length() - 1);
                              lcd.print(minuta);
                            } else if (klawisz == '#') {
                              if (minuta.toInt() >= 0 && minuta.toInt() <= 59 && (minuta.length() == 1 || minuta.length() == 2)) {
                                lcd.clear();
                                lcd.setCursor(0, 0);
                                lcd.print("Wpisz sekunde:");
                                lcd.setCursor(0, 1);
                                sekunda = "";
                                do {
                                  klawisz = keypad.waitForKey();
                                  if (isDigit(klawisz)) {
                                    sekunda += klawisz;
                                    lcd.setCursor(0, 1);
                                    lcd.print(sekunda);
                                  } else if (klawisz == 'C') {
                                    lcd.clear();
                                    lcd.setCursor(0, 0);
                                    lcd.print("Wpisz sekunde:");
                                    lcd.setCursor(0, 1);
                                    sekunda.remove(sekunda.length() - 1);
                                    lcd.print(sekunda);
                                  } else if (klawisz == '#') {
                                    if (sekunda.toInt() >= 0 && sekunda.toInt() <= 59 && (sekunda.length() == 1 || sekunda.length() == 2)) {
                                      zegar.setDateTime(rok.toInt(), miesiac.toInt(), dzien.toInt(), godzina.toInt(), minuta.toInt(), sekunda.toInt());
                                      lcd.noBlink();
                                      lcd.noCursor();
                                      lcd.clear();
                                      lcd.setCursor(0, 0);
                                      lcd.print("Zapisano!!!");
                                      delay(3000);
                                      zaladujDzwonki();
                                      licznik = ustawLicznik();
                                      lcd.clear();
                                      lcd.setCursor(0, 0);
                                      sekunda = "";
                                      minuta = "";
                                      godzina = "";
                                      dzien = "";
                                      miesiac = "";
                                      rok = "";
                                      break;
                                    } else {
                                      lcd.noBlink();
                                      lcd.noCursor();
                                      lcd.clear();
                                      lcd.setCursor(0, 0);
                                      lcd.print("Bledne dane!");
                                      delay(2000);
                                      lcd.clear();
                                      lcd.setCursor(0, 0);
                                      lcd.print("Wpisz sekunde:");
                                      lcd.blink();
                                      lcd.cursor();
                                      lcd.setCursor(0, 1);
                                      sekunda = "";
                                    }
                                  } else if (klawisz == '*') {
                                    sekunda = "";
                                    minuta = "";
                                    godzina = "";
                                    dzien = "";
                                    miesiac = "";
                                    rok = "";
                                    break;
                                  }
                                } while (true);
                                break;
                              } else {
                                lcd.noBlink();
                                lcd.noCursor();
                                lcd.clear();
                                lcd.setCursor(0, 0);
                                lcd.print("Bledne dane!");
                                delay(2000);
                                lcd.clear();
                                lcd.setCursor(0, 0);
                                lcd.print("Wpisz minute:");
                                lcd.blink();
                                lcd.cursor();
                                lcd.setCursor(0, 1);
                                minuta = "";
                              }
                            } else if (klawisz == '*') {
                              minuta = "";
                              godzina = "";
                              dzien = "";
                              miesiac = "";
                              rok = "";
                              break;
                            }
                          } while (true);
                          break;
                        } else {
                          lcd.noBlink();
                          lcd.noCursor();
                          lcd.clear();
                          lcd.setCursor(0, 0);
                          lcd.print("Bledne dane!");
                          delay(2000);
                          lcd.clear();
                          lcd.setCursor(0, 0);
                          lcd.print("Wpisz godzine:");
                          lcd.blink();
                          lcd.cursor();
                          lcd.setCursor(0, 1);
                          godzina = "";
                        }
                      } else if (klawisz == '*') {
                        godzina = "";
                        dzien = "";
                        miesiac = "";
                        rok = "";
                        break;
                      }
                    } while (true);
                    break;
                  } else {
                    lcd.noBlink();
                    lcd.noCursor();
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Bledne dane!");
                    delay(2000);
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Wpisz dzien:");
                    lcd.blink();
                    lcd.cursor();
                    lcd.setCursor(0, 1);
                    dzien = "";
                  }
                } else if (klawisz == '*') {
                  dzien = "";
                  miesiac = "";
                  rok = "";
                  break;
                }
              } while (true);
              break;
            } else {
              lcd.noBlink();
              lcd.noCursor();
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Bledne dane!");
              delay(2000);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Wpisz miesiac:");
              lcd.blink();
              lcd.cursor();
              lcd.setCursor(0, 1);
              miesiac = "";
            }
          } else if (klawisz == '*') {
            rok = "";
            miesiac = "";
            break;
          }
        } while (true);
        break;
      } else {
        lcd.noBlink();
        lcd.noCursor();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Bledne dane!");
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wpisz rok: ");
        lcd.blink();
        lcd.cursor();
        lcd.setCursor(0, 1);
        rok = "";
      }
    } else if (klawisz == '*') {
      rok = "";
      break;
    }
  } while (true);
}

void zapiszDzwonki(byte program, byte dzienTyg) {
  int komorka;
  int tempDzwonek;
  if (program == 1) {
    komorka = 10 + (dzienTyg - 1)  * 32 * 2 ;
  } else if (program == 2) {
    komorka = 458 + (dzienTyg - 1)  * 32 * 2;
  }
  for (int x = 0; x < 32; x++) {
    tempDzwonek = dzwonki[x];
    EEPROM.put(komorka, tempDzwonek);
    komorka += 2;
    tempDzwonek = 0;
  }
}

void zaladujDzwonki() {
  czas = zegar.getDateTime();
  int komorka;
  if (obecnyProgram == 1) {
    komorka = 10 + (czas.dayOfWeek - 1)  * 32 * 2 ;
    int tempDzwonek;
    for (int x = 0; x < iloscDzwonkow; x++) {
      EEPROM.get(komorka, tempDzwonek);
      dzwonki[x] = tempDzwonek;
      komorka += 2;
      tempDzwonek = 0;
    }
  } else if (obecnyProgram == 2) {
    komorka = 458 + (czas.dayOfWeek - 1)  * 32 * 2 ;
    int tempDzwonek;
    for (int x = 0; x < iloscDzwonkow; x++) {
      EEPROM.get(komorka, tempDzwonek);
      dzwonki[x] = tempDzwonek;
      komorka += 2;
      tempDzwonek = 0;
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
}

int ustawLicznik() {
  czas = zegar.getDateTime();
  int tempData;
  byte tempLicznik = 0;
  do {
    tempData = czas.hour * 60 + czas.minute;
    if (tempData >= dzwonki[tempLicznik] && tempData < dzwonki[tempLicznik + 1] && tempLicznik < iloscDzwonkow) {
      return tempLicznik + 1;
      break;
    } else if (tempData < dzwonki[0]) {
      return 0;
      break;
    } else if (tempLicznik >= iloscDzwonkow) {
      return -1;
      break;
    } else {
      tempLicznik++;
    }
  } while (true);
}
