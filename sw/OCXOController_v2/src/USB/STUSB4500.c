#include "STUSB4500.h"

// The sector values are calculated using the STSW-STUSB002 GUI (v2.00). 
// This configuration is for PD0 = 5V@1.5A, PD1 = 5V@2A and PD2 = 5V@3A.
// Lower and upper % levels are 5%. The USB_COMM_CAPABLE box is checked.
// POWER_ONLY_ABOVE_5V is also checked, this makes so that the MOSFET controling the VBUS only turns
// on when PD1 or PD2 are contracted between the chip and the source.
// The STUSB4500 will select PD2 over PD1 and PD1 over PD0. Depends on the capabilities of the 
// USB power source.
uint8_t Sector0[STUSB_SECTOR_SIZE] = {0x00,0x00,0xFF,0xAA,0x00,0x45,0x00,0x00};
uint8_t Sector1[STUSB_SECTOR_SIZE] = {0x00,0x40,0x9C,0x1C,0xF0,0x01,0x00,0xDF};
uint8_t Sector2[STUSB_SECTOR_SIZE] = {0x02,0x40,0x0F,0x00,0x32,0x00,0xFC,0xF1};
uint8_t Sector3[STUSB_SECTOR_SIZE] = {0x00,0x19,0x57,0x0F,0x07,0xB0,0x00,0x00};
uint8_t Sector4[STUSB_SECTOR_SIZE] = {0x00,0x19,0x64,0x20,0x43,0x00,0x48,0xFB};

uint8_t* sectors[STUSB_SECTOR_COUNT] = {Sector0, Sector1, Sector2, Sector3, Sector4};
I2C_HandleTypeDef* stusbI2CHandler = NULL;

int startSTUSB4500(I2C_HandleTypeDef* hI2C) {
    stusbI2CHandler = hI2C;

    // Check if the NVM sectors are the same as the sector arrays above.
    if(!checkNVMSectors()) {
        // The STUSB4500 has not been configured. This process writes the EEPROM sectors of the chip
        // so that on boot up the chip works only with the power profiles set above.
        if(!configureNVM()) return 0;

        // Do a RESET so that it starts supplying 5V.
        writeByte(0x51, 0x0D);
        writeByte(0x1a, 0x26);
        HAL_Delay(500);
    }

    // Check what the charger is currently supplying.
    uint8_t USBC_Attached = 0;
    readI2C(0x0e, sizeof(USBC_Attached), &USBC_Attached);
    USBC_Attached &= 0x01;

    uint32_t DPM_REQ = 0;
    readI2C(0x91, sizeof(DPM_REQ), (uint8_t*) &DPM_REQ);
    float operatingCurrent_A = ((DPM_REQ >> 10) & 0x3FF) / 100.0f;
    float maxCurrent_A = (DPM_REQ & 0x3FF) / 100.0f;
    
    uint8_t regx21 = 0;
    readI2C(0x21, sizeof(regx21), &regx21);
    float voltageRequested_V = regx21 / 10.0f;

    return (voltageRequested_V == STUSB4500_VOLTAGE_V) &&
    	   (operatingCurrent_A >= STUSB4500_CURRENT_A) &&
		   (maxCurrent_A >= STUSB4500_CURRENT_A);
}

int configureNVM() {
    // The process of writing to EEPROM is recollected in "STUSB4500 NVM registers". 
    int status = 1;

    // 1. Unlock access to NVM.
    status &= writeByte(0x95, 0x47); // FTP_KEY = 0x47

    // 2. Init NVM.
    status &= writeByte(0x53, 0x00); // dummy write
    status &= writeByte(0x96, 0x00); // reset NVM
    HAL_Delay(1);
    status &= writeByte(0x96, 0x40); // power up NVM

    // 3. Full erase sequence.
    status &= writeByte(0x97, 0xFA); // shift in erase register
    status &= writeByte(0x96, 0x50);
    HAL_Delay(1);
    status &= writeByte(0x97, 0x07); // soft program array
    status &= writeByte(0x96, 0x50);
    HAL_Delay(5);
    status &= writeByte(0x97, 0x05); // erase array
    status &= writeByte(0x96, 0x50);
    HAL_Delay(5);

    // 4. Write sectors.
    status &= writeSector(0, Sector0);
    status &= writeSector(1, Sector1);
    status &= writeSector(2, Sector2);
    status &= writeSector(3, Sector3);
    status &= writeSector(4, Sector4);

    // 5. Exit test mode.
    uint8_t clear[] = {0x40, 0x00};
    status &= writeI2C(0x96, clear, 2); // FTP_CTRL_0 = 0x40, 0x00
    status &= writeByte(0x95, 0x00);    // FTP_KEY = 0

    return status;
}

int writeSector(uint8_t sector, uint8_t *data) {
    if(sector > STUSB_SECTOR_COUNT) return 0;

    int status = 1;

    // Load sector data from the data buffer.
    status &= writeI2C(0x53, data, STUSB_SECTOR_SIZE);

    // Opcode: Shift In Data on Program Load Register.
    status &= writeByte(0x97, 0x01);
    status &= writeByte(0x96, 0x50);
    HAL_Delay(1);

    // Opcode: Program word into EEPROM.
    status &= writeByte(0x97, 0x06);

    // Sector index (0x50 + sector).
    status &= writeByte(0x96, 0x50 + sector);
    HAL_Delay(2);
    return status;
}

int checkNVMSectors() {
    uint8_t readBuffer[STUSB_SECTOR_SIZE];
    uint8_t status = 1;
    
    // 1. Unlock access to NVM.
    status &= writeByte(0x95, 0x47); // FTP_KEY = 0x47

    // 2. Init NVM.
    status &= writeByte(0x53, 0x00); // dummy write
    status &= writeByte(0x96, 0x00); // reset NVM
    HAL_Delay(1);
    status &= writeByte(0x96, 0x40); // power up NVM

    // 3. Read sectors.
    for(uint8_t sectorNumber = 0; sectorNumber < sizeof(sectors)/sizeof(uint8_t*); sectorNumber++) {
        status &= readSector(sectorNumber, readBuffer);
        for(uint8_t i = 0; i < STUSB_SECTOR_SIZE; i++) {
            if(sectors[sectorNumber][i] != readBuffer[i]) {
                status = 0;
                break;
            }
        }
        if(!status) break;
    }

    // 4. Exit test mode.
    uint8_t clear[] = {0x40, 0x00};
    status &= writeI2C(0x96, clear, 2); // FTP_CTRL_0 = 0x40, 0x00
    status &= writeByte(0x95, 0x00);    // FTP_KEY = 0

    return status;
}

int readSector(uint8_t sector, uint8_t *data) {
    if(sector > STUSB_SECTOR_COUNT) return 0;

    int status = 1;

    status &= writeByte(0x97, 0x00); // set Read Sector opcode
    status &= writeByte(0x96, 0x50 + sector); // select sector
    HAL_Delay(1);

    status &= readI2C(0x53, STUSB_SECTOR_SIZE, data);
    return status;
}

int writeByte(uint8_t reg, uint8_t data) {
    return writeI2C(reg, &data, 1);
}

int writeI2C(uint8_t reg, uint8_t *data, uint8_t len) {
    if(stusbI2CHandler == NULL) return 0;

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(stusbI2CHandler, 
                      STUSB4500_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, len, HAL_MAX_DELAY);
    HAL_Delay(1);
    return status == HAL_OK;
}

int readI2C(uint8_t reg, uint8_t len, uint8_t *data) {
    if(stusbI2CHandler == NULL) return 0;

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(stusbI2CHandler, 
                     STUSB4500_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, len, HAL_MAX_DELAY);
    HAL_Delay(1);
    return status == HAL_OK;
}
