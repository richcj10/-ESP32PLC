#ifndef SENSORS_H
#define  SENSORS_H

void readDeviceClimate();
float getDeviceClimateHumidity();
float getDeviceClimateTemprature();
void printInfo(void);
void InitSensors(void);
void InitOneWire(void);
void ReadDS18B20OneWire(void);

#endif  /* SENSORS_H */
