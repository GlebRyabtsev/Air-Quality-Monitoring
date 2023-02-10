#include <SPS30.h>

SPS30::SPS30(uint8_t sda, uint8_t scl) : sw(sda, scl)
{
    sw.setTxBuffer(swTxBuffer, sizeof(swTxBuffer));
    sw.setRxBuffer(swRxBuffer, sizeof(swRxBuffer));
    sw.setDelay_us(5); // TODO What does this do exactly?
    sw.setTimeout(1000);
    sw.begin();
}

void SPS30::setPointer(uint8_t* pointerAddress)
{
    sw.beginTransmission(I2C_address);
    sw.write(pointerAddress, 2);
    sw.endTransmission();
}

void SPS30::setPointerWrite(uint8_t* pointerAddress, uint8_t* data, uint8_t size)
{
    uint8_t buffer[64];
    buffer[0] = pointerAddress[0];
    buffer[1] = pointerAddress[1];
    uint8_t bytesToWrite = size / 2 * 3 + 2;
    uint8_t k = 2;
    for (int i = 0; i < size; i += 2)
    {
        buffer[k] = data[i];
        buffer[k + 1] = data[i + 1];
        buffer[k + 2] = calcCrc(data + i);
        k += 3;
    }
    sw.beginTransmission(I2C_address);
    sw.write(buffer, bytesToWrite);
    sw.endTransmission();
}

uint8_t SPS30::setPointerRead(uint8_t* pointerAddress, uint8_t* data, uint8_t dataBytesToRead)
{
    uint8_t buffer[64];
    uint8_t bytesReceived = dataBytesToRead / 2 * 3;
    sw.beginTransmission(I2C_address);
    sw.write(pointerAddress, 2);
    sw.endTransmission();
    sw.requestFrom(I2C_address, bytesReceived);
    for (uint8_t i = 0; i < bytesReceived; i++)
    {
        buffer[i] = sw.read();
    }
    uint8_t k = 0;
    for (uint8_t i = 0; i < bytesReceived; i += 3)
    {
        if (calcCrc(buffer + i) != buffer[i + 2])
            return 0;
        data[k] = buffer[i];
        data[k + 1] = buffer[i + 1];
        k += 2;
    }
    return 1;
}

uint8_t SPS30::calcCrc(uint8_t data[2])
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < 2; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31u;
            }
            else
            {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

void SPS30::startMeasurement()
{
    uint8_t ptr_addr[] = {0x00, 0x10};
    uint8_t data[] = {0x03, 0x00};
    setPointerWrite(ptr_addr, data, 2);
}

void SPS30::stopMeasurement()
{
    uint8_t ptr_addr[] = {0x01, 0x04};
    setPointer(ptr_addr);
}

bool SPS30::readDataReadyFlag()
{
    uint8_t data[2];
    uint8_t ptr_addr[] = {0x02, 0x02};
    setPointerRead(ptr_addr, data, 2);
    return data[1];
}

void SPS30::readMeasuredValues(SPS30MeasuredValues* values)
{
    uint8_t data[40] = {0};
    float floatArray[10] = {0};
    uint8_t ptr_addr[] = {0x03, 0x00};
    setPointerRead(ptr_addr, data, 40); // TODO use crc check result
    for (int i = 0; i < 10; i++)
    {
        uint8_t* p = (uint8_t*)&floatArray[i];
        p[0] = data[i * 4 + 3];
        p[1] = data[i * 4 + 2];
        p[2] = data[i * 4 + 1];
        p[3] = data[i * 4];
    }
    values->MassConcentration.pm010 = floatArray[0];
    values->MassConcentration.pm025 = floatArray[1];
    values->MassConcentration.pm040 = floatArray[2];
    values->MassConcentration.pm100 = floatArray[3];
    values->NumberConcentration.pm005 = floatArray[4];
    values->NumberConcentration.pm010 = floatArray[5];
    values->NumberConcentration.pm025 = floatArray[6];
    values->NumberConcentration.pm040 = floatArray[7];
    values->NumberConcentration.pm100 = floatArray[8];
    values->typical_particle_size = floatArray[9];
}