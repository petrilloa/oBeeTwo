#include "DroneDigital.h"

DroneDigital::DroneDigital()
{
    }

void DroneDigital::SetUpSensor(sensor oSensor)
{
    _sensor.droneID = oSensor.droneID;
    _sensor.type = oSensor.type;
    _sensor.pin = oSensor.pin;
    _sensor.pin2 = oSensor.pin2;
    _sensor.buzzerNotificationID = oSensor.buzzerNotificationID;
    _sensor.rgbNotificationID = oSensor.rgbNotificationID;
    _sensor.notificationElapsedTime = oSensor.notificationElapsedTime;
    _sensor.mode = oSensor.mode;
    _sensor.workerID = oSensor.workerID;
    _sensor.workerElapsedTime = oSensor.workerElapsedTime;
    _sensor.notificationFieldID = oSensor.notificationFieldID;
    _sensor.inverted = oSensor.inverted;

    pinMode(_sensor.pin, INPUT);

    _sensor_event.timestamp = millis();
    _sensor_event.lastChange = _sensor_event.timestamp;
    _sensor_event.value = _sensor.inverted ? 1:0;
    _sensor_event.lastValue = _sensor_event.value;
    _sensor_event.changed = false;

    _sensor_event.acumulatedNotification = 0;

    _sensor_event.triggerPublish = false;

}

void DroneDigital::GetSensor(sensor *oSensor)
{
    /* Clear the event */
    //Serial.println("Digital Get Sensor 1");
    memset(oSensor, 0, sizeof(sensor));

    oSensor->droneID = _sensor.droneID;
    oSensor->type = _sensor.type;
    oSensor->pin = _sensor.pin;
    oSensor->pin2 = _sensor.pin2;
    oSensor->buzzerNotificationID = _sensor.buzzerNotificationID;
    oSensor->rgbNotificationID = _sensor.rgbNotificationID;
    oSensor->notificationElapsedTime = _sensor.notificationElapsedTime;
    oSensor->fieldID = _sensor.fieldID;
    oSensor->mode = _sensor.mode;
    oSensor->workerID = _sensor.workerID;
    oSensor->workerElapsedTime = _sensor.workerElapsedTime;
    oSensor->notificationFieldID = _sensor.notificationFieldID;
    oSensor->inverted = _sensor.inverted;

    //Serial.println("Digital Get Sensor 2");

}

void DroneDigital::GetEvent(sensor_event *oEvent)
{
    /* Clear the event */
    memset(oEvent, 0, sizeof(sensor_event));

    //Read the sensor to get Value and Times
    read();

    //Check for acumulated
    if (wasActive())
        _sensor_event.acumulatedValue = _sensor_event.acumulatedValue + 1;

    //Check notification
    if(activeFor(_sensor.notificationElapsedTime))
    {
      _sensor_event.triggerNotification = true;

      if(_sensor_event.firstTriggerPublish)
      {
          //Solo se PUBLICA ante la primera NOTIFICACION, Luego se van acumulando las notificaciones y permanece encendido para poder NOTIFICAR por RGG, BZZR
          _sensor_event.triggerPublish = true;
          Log.info("Drone Trigger Publish");
          _sensor_event.firstTriggerPublish = false;
      }
      else
      {
          _sensor_event.triggerPublish = false;
      }

      //Para digitales, si se produce una NOTIFICACION, es NECESARIO Publicar!
      if (!_sensor_event.changed)
      {
        _sensor_event.acumulatedNotification = 1;
      }
      else
      {
        _sensor_event.acumulatedNotification = _sensor_event.acumulatedNotification + 1;
          //Solo se PUBLICA ante la primera NOTIFICACION, Luego se van acumulando las notificaciones y permanece encendido para poder NOTIFICAR por RGG, BZZR
        _sensor_event.triggerPublish = true;
      }
    }

    else
    {
      _sensor_event.triggerNotification = false;
      _sensor_event.triggerPublish = false;
      _sensor_event.firstTriggerPublish = true;
    }


    //Check worker
    if(_sensor.workerID != -1 && activeFor(_sensor.workerElapsedTime))
        _sensor_event.triggerWorker = true;
    else
        _sensor_event.triggerWorker = false;

    //Copy event
    /* Copy object to return */
    oEvent->value = _sensor_event.value;
    oEvent->lastValue = _sensor_event.lastValue;
    oEvent->acumulatedValue = _sensor_event.acumulatedValue;
    oEvent->changed = _sensor_event.changed;
    oEvent->timestamp = _sensor_event.timestamp;
    oEvent->lastChange = _sensor_event.lastChange;
    oEvent->triggerNotification = _sensor_event.triggerNotification;
    oEvent->triggerWorker = _sensor_event.triggerWorker;
    oEvent->acumulatedNotification = _sensor_event.acumulatedNotification;
    oEvent->triggerPublish = _sensor_event.triggerPublish;

    //Serial.println("_Event value: " + String(_sensor_event.value));
    //Serial.println("_Event notification " + String (_sensor_event.triggerNotification));
    //delay(1000);
    //Serial.println("_Event acumulated: " + String(_sensor_event.acumulatedValue));


}

float DroneDigital::read()
{
    static int ms;
    static int pinVal;

    ms = millis();
    pinVal = digitalRead(_sensor.pin);

    //Serial.println("Digital: " + String(_sensor.droneID));

    if(ms-_sensor_event.lastChange < _dbTime) //if TRUE is en Debounce time
    {
        _sensor_event.timestamp = ms;
        _sensor_event.changed = false;

        return _sensor_event.value; //Return value (is en Debounce time)
    }
    else //is FALSE -> read value to return
    {
        _sensor_event.lastValue = _sensor_event.value;
        if(pinVal == HIGH )
        {
            _sensor_event.value = _sensor.inverted ? 0:1;
            //Serial.println("Value HIGH ");
            //Serial.println("Inverted: " + String(_sensor.inverted));
            //delay(500);
        }
        else
        {
          _sensor_event.value = _sensor.inverted ? 1:0;
          //Serial.println("Value LOW ");
          //Serial.println("Inverted: " + String(_sensor.inverted));
          //delay(500);
        }

        _sensor_event.timestamp = ms;

        if(_sensor_event.lastValue != _sensor_event.value)
        {
            _sensor_event.lastChange = ms;
            _sensor_event.changed = true;
        }
        else
        {
            _sensor_event.changed = false;
        }

        //Serial.println("Value: " + String(_sensor_event.value));

        return _sensor_event.value;
    }

}

int DroneDigital::wasActive(void)
{
    return _sensor_event.value == 1 && _sensor_event.changed;
}

int DroneDigital::activeFor(int ms)
{
    return (_sensor_event.value == 1 && _sensor_event.timestamp - _sensor_event.lastChange >= ms) ? 1 : 0;
}

void DroneDigital::Publish(sensor_event *oEvent)
{
    //TODO: CHECK!!!
    if(_sensor.mode == SENSOR_MODE_PULSE)
        oEvent->value = _sensor_event.acumulatedValue;
    else
        oEvent->value = _sensor_event.value;

    oEvent->acumulatedNotification = _sensor_event.acumulatedNotification;

    //Clean acumulatedValue - Only if is not Active
    if(_sensor_event.value == 0)
        _sensor_event.acumulatedValue = 0;

    //Clean acumulatedNotification
    _sensor_event.acumulatedNotification = 0;
}
