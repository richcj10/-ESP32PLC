#ifndef SENSORS_H
#define  SENSORS_H

void  InitSensors(void);
void  UpdateSensors(void);
float getDeviceClimateHumidity();
float getDeviceClimateTemprature();   // degrees F

#endif  /* SENSORS_H */
