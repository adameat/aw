#pragma once

#include "ArduinoWorkflow.h"
#include <MPU9250.h>
#include <quaternionFilters.h>

namespace AW {

template <typename Env = TDefaultEnvironment>
class TSensorMPU9250 : public TActor, public TSensorSource {
public:
    uint8_t AddressMPU6500 = 0x68;
    uint8_t AddressAK8963 = 0x0c;
    TActor* Owner;

    struct TSensor3DValues {
        TSensorValue X;
        TSensorValue Y;
        TSensorValue Z;
    };

    TSensor3DValues Accelerometer;
    TSensor3DValues Gyroscope;
    TSensor3DValues Magnetometer;
    TSensorValue Temperature;

    TSensorMPU9250(uint8_t addressMPU6500, uint8_t addressAK9063, TActor* owner, StringBuf name = "mpu9250")
        : AddressMPU6500(addressMPU6500)
        , AddressAK8963(addressAK9063)
        , Owner(owner)
        , IMU(addressMPU6500, Wire, 400000) {
        Name = name;
        Accelerometer.X.Name = "accelerometer.X";
        Accelerometer.Y.Name = "accelerometer.Y";
        Accelerometer.Z.Name = "accelerometer.Z";
        Gyroscope.X.Name = "gyroscope.X";
        Gyroscope.Y.Name = "gyroscope.Y";
        Gyroscope.Z.Name = "gyroscope.Z";
        Magnetometer.X.Name = "magnetometer.X";
        Magnetometer.Y.Name = "magnetometer.Y";
        Magnetometer.Z.Name = "magnetometer.Z";
        Temperature.Name = "temperature";
    }

protected:
    MPU9250 IMU;

    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventBootstrap::EventID:
            return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
        case TEventReceive::EventID:
            return OnReceive(static_cast<TEventReceive*>(event.Release()), context);
        default:
            break;
        }
    }

    void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
        uint8_t chipId1 = IMU.readByte(AddressMPU6500, WHO_AM_I_MPU9250);
        if (chipId1 == 0x71) {
            if (Env::Diagnostics) {
                context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "MPU6500 on " << String(AddressMPU6500, 16)));
            }
            IMU.MPU9250SelfTest(IMU.selfTest);
            IMU.calibrateMPU9250(IMU.gyroBias, IMU.accelBias);
            IMU.initMPU9250();
            uint8_t chipId2 = IMU.readByte(AddressAK8963, WHO_AM_I_AK8963);
            if (chipId2 == 0x48) {
                if (Env::Diagnostics) {
                    context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "AK8963 on " << String(AddressAK8963, 16)));
                }
                IMU.initAK8963(IMU.factoryMagCalibration);
                IMU.getAres();
                IMU.getGres();
                IMU.getMres();
                //IMU.magCalMPU9250(IMU.magBias, IMU.magScale);
                IMU.magBias[0] = -987.25;
                IMU.magBias[1] = 1498.18;
                IMU.magBias[2] = 828.88;
                IMU.magScale[0] = 1.18;
                IMU.magScale[1] = 0.66;
                IMU.magScale[1] = 1.55;
                context.Send(this, this, new AW::TEventReceive());
            } else {
                if (Env::Diagnostics) {
                    context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "Failed to initialize AK8963"));
                }
            }
        } else {
            if (Env::Diagnostics) {
                context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "Failed to initialize MPU9250"));
            }
        }
    }

    void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        if (IMU.readByte(AddressMPU6500, INT_STATUS) & 0x01) {
            IMU.readAccelData(IMU.accelCount); // Read the x/y/z adc values
            // Now we'll calculate the accleration value into actual g's
            // This depends on scale being set
            IMU.ax = (float)IMU.accelCount[0] * IMU.aRes; // - IMU.accelBias[0];
            IMU.ay = (float)IMU.accelCount[1] * IMU.aRes; // - IMU.accelBias[1];
            IMU.az = (float)IMU.accelCount[2] * IMU.aRes; // - IMU.accelBias[2];

            IMU.readGyroData(IMU.gyroCount);  // Read the x/y/z adc values
            // Calculate the gyro value into actual degrees per second
            // This depends on scale being set
            IMU.gx = (float)IMU.gyroCount[0] * IMU.gRes;
            IMU.gy = (float)IMU.gyroCount[1] * IMU.gRes;
            IMU.gz = (float)IMU.gyroCount[2] * IMU.gRes;

            IMU.readMagData(IMU.magCount);  // Read the x/y/z adc values
            // Calculate the magnetometer values in milliGauss
            // Include factory calibration per data sheet and user environmental
            // corrections
            // Get actual magnetometer value, this depends on scale being set
            IMU.mx = (float)IMU.magCount[0] * IMU.mRes * IMU.factoryMagCalibration[0] - IMU.magBias[0];
            IMU.my = (float)IMU.magCount[1] * IMU.mRes * IMU.factoryMagCalibration[1] - IMU.magBias[1];
            IMU.mz = (float)IMU.magCount[2] * IMU.mRes * IMU.factoryMagCalibration[2] - IMU.magBias[2];

            IMU.updateTime();

            // Sensors x (y)-axis of the accelerometer is aligned with the y (x)-axis of
            // the magnetometer; the magnetometer z-axis (+ down) is opposite to z-axis
            // (+ up) of accelerometer and gyro! We have to make some allowance for this
            // orientationmismatch in feeding the output to the quaternion filter. For the
            // MPU-9250, we have chosen a magnetic rotation that keeps the sensor forward
            // along the x-axis just like in the LSM9DS0 sensor. This rotation can be
            // modified to allow any convenient orientation convention. This is ok by
            // aircraft orientation standards! Pass gyro rate as rad/s
            MahonyQuaternionUpdate(IMU.ax, IMU.ay, IMU.az, IMU.gx * DEG_TO_RAD, IMU.gy * DEG_TO_RAD, IMU.gz * DEG_TO_RAD, IMU.my, IMU.mx, IMU.mz, IMU.deltat);

            Accelerometer.X = 1000 * IMU.ax;
            Accelerometer.Y = 1000 * IMU.ay;
            Accelerometer.Z = 1000 * IMU.az;
            Gyroscope.X = IMU.gx;
            Gyroscope.Y = IMU.gy;
            Gyroscope.Z = IMU.gz;
            Magnetometer.X = IMU.mx;
            Magnetometer.Y = IMU.my;
            Magnetometer.Z = IMU.mz;

            IMU.tempCount = IMU.readTempData();
            IMU.temperature = ((float)IMU.tempCount) / 333.87 + 21.0;
            Temperature = IMU.temperature;

            Updated = context.Now;

            if (Env::SensorsSendValues) {
                context.Send(this, Owner, new AW::TEventSensorData(*this, Accelerometer.X));
                context.Send(this, Owner, new AW::TEventSensorData(*this, Accelerometer.Y));
                context.Send(this, Owner, new AW::TEventSensorData(*this, Accelerometer.Z));
                context.Send(this, Owner, new AW::TEventSensorData(*this, Gyroscope.X));
                context.Send(this, Owner, new AW::TEventSensorData(*this, Gyroscope.Y));
                context.Send(this, Owner, new AW::TEventSensorData(*this, Gyroscope.Z));
                context.Send(this, Owner, new AW::TEventSensorData(*this, Magnetometer.X));
                context.Send(this, Owner, new AW::TEventSensorData(*this, Magnetometer.Y));
                context.Send(this, Owner, new AW::TEventSensorData(*this, Magnetometer.Z));
                context.Send(this, Owner, new AW::TEventSensorData(*this, Temperature));
            }
        }

        event->NotBefore = context.Now + Env::SensorsPeriod;
        context.Resend(this, event.Release());
    }
};

}
