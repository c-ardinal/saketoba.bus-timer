#include "Arduino.h"
#include <time.h>
#include <ArduinoJson.h>
#include "Timetable.h"


/* コンストラクタ */
Timetable::Timetable() {
  configTime( JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
}


/* JSONのセット */
void Timetable::setJson(String *json) {
  // JSONのパース
  DynamicJsonBuffer jsonBuf(JSON_BUFFER);
  JsonObject& rootObj = jsonBuf.parseObject(*json);
  if (!rootObj.success()) {
    Serial.println("[ERROR]: JSON parse failed.");
    return;
  }

  isSetJsonFlag = true;
  String fromName = rootObj["from"];
  this->timetable.fromName = fromName;
  String toName = rootObj["to"];
  this->timetable.toName = toName;
  Serial.println("[TIMETABLE]: " + fromName + "->" + toName);

  String day[3] = {"weekday", "saturday", "holiday"};

  for (int dayId = 0; dayId < 3; dayId++) {
    uint8_t timetablesSize = (uint8_t)rootObj["time-table"][day[dayId]].size();
    TimetableInfo_t *targetDay = new TimetableInfo_t[timetablesSize];

    for (uint8_t i = 0; i < timetablesSize; i++) {
      uint8_t minutesSize = (uint8_t)rootObj["time-table"][day[dayId]][i]["minutes"].size();
      targetDay[i].minutes = new uint8_t[timetablesSize];

      for (uint8_t j = 0; j < minutesSize; j++) {
        targetDay[i].minutes[j] = (uint8_t)rootObj["time-table"][day[dayId]][i]["minutes"][j];
      }

      targetDay[i].minutesSize = minutesSize;
      targetDay[i].hour = (uint8_t)rootObj["time-table"][day[dayId]][i]["hour"];

      switch (dayId) {
        case 0:
          this->timetable.weekdaySize = timetablesSize;
          this->timetable.weekday = targetDay;
          break;
        case 1:
          this->timetable.saturdaySize = timetablesSize;
          this->timetable.saturday = targetDay;
          break;
        case 2:
          this->timetable.holidaySize = timetablesSize;
          this->timetable.holiday = targetDay;
          break;
      }
    }
  }
}


/* 現在の時間を取得 */
NowTime_t Timetable::getNowTime() {
  time_t t;
  struct tm *tm;
  t = time(NULL);
  tm = localtime(&t);

  NowTime_t nt;
  nt.Hour = tm->tm_hour;
  nt.Min = tm->tm_min;
  nt.Sec = tm->tm_sec;
  return nt;
}


/* 次の時刻を取得 */
NextTime_t Timetable::getNextTimetable() {
  NextTime_t tt = {0, 0, 0, 0};

  if (isSetJsonFlag == true) {
    time_t t;
    struct tm *tm;
    t = time(NULL);
    tm = localtime(&t);

    TimetableInfo_t *targetDay;
    uint8_t targetSize;
    switch (tm->tm_wday) {
      case 0:
        // 休日
        targetDay = this->timetable.holiday;
        targetSize = this->timetable.holidaySize;
        break;
      case 6:
        // 土曜
        targetDay = this->timetable.saturday;
        targetSize = this->timetable.saturdaySize;
        break;
      default:
        // 平日
        targetDay = this->timetable.weekday;
        targetSize = this->timetable.weekdaySize;
        break;
    }

    for (uint8_t i = 0; i < targetSize; i++) {
      for (uint8_t j = 0; j < targetDay[i].minutesSize; j++) {
        uint8_t targetHour = targetDay[i].hour;
        uint8_t targetMinute = targetDay[i].minutes[j];
        if (isNextTimetable(tm->tm_hour, tm->tm_min, targetHour, targetMinute)) {
          tt.nextHour = targetHour;
          tt.nextMin = targetMinute;
          if (((((int)tt.nextHour - tm->tm_hour) * 60) + ((int)tt.nextMin - tm->tm_min) - 1) > 99) {
            tt.remMin = 99;
            tt.remSec = 99;
          } else {
            tt.remMin = (((tt.nextHour - (uint8_t)tm->tm_hour) * 60) + (tt.nextMin - (uint8_t)tm->tm_min) - 1);
            tt.remSec = 59 - tm->tm_sec;
          }
          return tt;
        }
      }
    }

    // 次の時刻表にマッチするものがなければ始発を表示
    tt.nextHour = targetDay[0].hour;
    tt.nextMin = targetDay[0].minutes[0];
    if ((((((int)tt.nextHour + 24) - tm->tm_hour) * 60) + ((int)tt.nextMin - tm->tm_min) - 1) > 99) {
      tt.remMin = 99;
      tt.remSec = 99;
    } else {
      tt.remMin = ((((tt.nextHour + 24) - (uint8_t)tm->tm_hour) * 60) + (tt.nextMin - (uint8_t)tm->tm_min) - 1);
      tt.remSec = 59 - tm->tm_sec;
    }
  } 
  return tt;
}


/* 指定時刻が現在時刻より後か判定する */
boolean Timetable::isNextTimetable(uint8_t nowHour, uint8_t nowMin, uint8_t targetHour, uint8_t targetMin) {
  if (nowHour == targetHour) {
    if (nowMin < targetMin) {
      return true;
    }
    else {
      return false;
    }
  } else if (nowHour < targetHour) {
    return true;
  } else {
    return false;
  }
}


/* JSONファイルがセットされているか返す */
boolean Timetable::isSetJson(){
  return isSetJsonFlag;
}

