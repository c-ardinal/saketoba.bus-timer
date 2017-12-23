#ifndef TIMETABLE_H
#define TIMETABLE_H

#define JST 3600*9
#define JSON_BUFFER 4096

typedef struct NowTime_t {
  uint8_t Hour;
  uint8_t Min;
  uint8_t Sec;
};

typedef struct NextTime_t {
  uint8_t nextHour;
  uint8_t nextMin;
  uint8_t remMin;
  uint8_t remSec;
};

typedef struct TimetableInfo_t {
  uint8_t hour;
  uint8_t *minutes;
  uint8_t minutesSize;
};

typedef struct Timetable_t {
  String fromName;
  String toName;
  TimetableInfo_t *weekday;
  uint8_t weekdaySize;
  TimetableInfo_t *saturday;
  uint8_t saturdaySize;
  TimetableInfo_t *holiday;
  uint8_t holidaySize;
};

class Timetable {
  public:
    Timetable();
    NowTime_t getNowTime();
    NextTime_t getNextTimetable();
    void setJson(String *);
    boolean isSetJson();
  private:
    boolean isNextTimetable(uint8_t, uint8_t, uint8_t, uint8_t);
    Timetable_t timetable;
    boolean isSetJsonFlag = false;
};

#endif
