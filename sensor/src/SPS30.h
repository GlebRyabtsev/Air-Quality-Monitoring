#ifndef SPS30hpp
#define SPS30hpp

#include <SoftWire.h>

#define SPS30_UART 0
#define SPS30_I2C 1

typedef struct
{
    struct MassConcentration
    {
        float pm010, pm025, pm040, pm100;
    } MassConcentration;
    struct NumberConcentration
    {
        float pm005, pm010, pm025, pm040, pm100;
    } NumberConcentration;
    float typical_particle_size;
    uint8_t raw_data[60];
} SPS30MeasuredValues;

class SPS30
{
private:
    const uint8_t I2C_address = 0x69; // TODO Is this correct address
    SoftWire sw;
    uint8_t swTxBuffer[128]; // TODO: Buffer size can be reduced
    uint8_t swRxBuffer[128];

    void setPointer(uint8_t* pointerAddress);
    uint8_t setPointerRead(uint8_t* pointerAddress, uint8_t* data, uint8_t dataBytesToRead);
    void setPointerWrite(uint8_t* pointerAddress, uint8_t* data, uint8_t size);
    uint8_t calcCrc(uint8_t data[2]);

public:
    SPS30(uint8_t sda, uint8_t scl);
    void startMeasurement();
    void stopMeasurement();
    bool readDataReadyFlag();
    void readMeasuredValues(SPS30MeasuredValues* values);
    void startFanCleaning();
    void reset();
};

#endif