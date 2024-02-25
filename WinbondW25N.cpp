#include "WinbondW25N.h"

W25N::W25N(){};

#define CHANNEL 0

void W25N::sendData(unsigned char * buf, uint32_t len){
  digitalWrite(_cs, LOW);
  wiringPiSPIDataRW(CHANNEL, buf, len);
  digitalWrite(_cs, HIGH);
}

int W25N::begin(uint8_t cs, uint32_t speed){
  wiringPiSetup();
  wiringPiSPISetup(CHANNEL, speed);
  _cs = cs;
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);

  this->reset();

  unsigned char jedec[5] = {W25N_JEDEC_ID, 0x00, 0x00, 0x00, 0x00};
  this->sendData(jedec, sizeof(jedec));
  printf("JEDEC ID: Manufacter %2X Chip ID %2X%2X\n", jedec[2], jedec[3], jedec[4]);
  if(jedec[2] == WINBOND_MAN_ID){
    printf("Manufacter Windbond\n");
    if((uint16_t)(jedec[3] << 8 | jedec[4]) == W25N01GV_DEV_ID){
      this->setStatusReg(W25N_PROT_REG, 0x00);
      this->_model = W25N01GV;
      printf("Chip W25N01GV\n");
      return 0;
    }
    if((uint16_t)(jedec[3] << 8 | jedec[4]) == W25M02GV_DEV_ID){
      this->_model = W25M02GV;
      this->dieSelect(0);
      this->setStatusReg(W25N_PROT_REG, 0x00);
      this->dieSelect(1);
      this->setStatusReg(W25N_PROT_REG, 0x00);
      this->dieSelect(0);
      return 0;
    }
  }
  return 1;
}

void W25N::reset(){
  //TODO check WIP in case of reset during write
  unsigned char buf[] = {W25N_RESET};
  this->sendData(buf, sizeof(buf));
  delayMicroseconds(600);	//Trst max time 500uS
}

int W25N::dieSelect(char die){
  //TODO add some type of input validation
  unsigned char buf[2] = {W25M_DIE_SELECT, die};
  this->sendData(buf, sizeof(buf));
  this->_dieSelect = die;
  return 0;
}

int W25N::dieSelectOnAdd(uint32_t pageAdd){
  if(pageAdd > this->getMaxPage()) return 1;
  return this->dieSelect(pageAdd / W25N01GV_MAX_PAGE);
}

char W25N::getStatusReg(char reg){
  unsigned char buf[3] = {W25N_READ_STATUS_REG, reg, 0x00};
  this->sendData(buf, sizeof(buf));
  return buf[2];
}

void W25N::setStatusReg(char reg, char set){
  unsigned char buf[3] = {W25N_WRITE_STATUS_REG, reg, set};
  this->sendData(buf, sizeof(buf));
}

uint32_t W25N::getMaxPage(){
  if (_model == W25M02GV) return W25M02GV_MAX_PAGE;
  if (_model == W25N01GV) return W25N01GV_MAX_PAGE;
  return 0;
}

void W25N::writeEnable(){
  unsigned char buf[] = {W25N_WRITE_ENABLE};
  this->sendData(buf, sizeof(buf));
}

void W25N::writeDisable(){
  unsigned char buf[] = {W25N_WRITE_DISABLE};
  this->sendData(buf, sizeof(buf));
}

int W25N::blockErase(uint32_t pageAdd){
  if(pageAdd > this->getMaxPage()) return 1;
  this->dieSelectOnAdd(pageAdd);
  char pageHigh = (char)((pageAdd & 0xFF00) >> 8);
  char pageLow = (char)(pageAdd);
  unsigned char buf[4] = {W25N_BLOCK_ERASE, 0x00, pageHigh, pageLow};
  this->block_WIP();
  this->writeEnable();
  this->sendData(buf, sizeof(buf));
  return 0;
}

int W25N::bulkErase(){
  int error = 0;
  for(uint32_t i = 0; i < this->getMaxPage(); i+=64){
    if((error = this->blockErase(i)) != 0) return error;
  }
  return 0;
}

int W25N::loadProgData(uint16_t columnAdd, unsigned char* buf, uint32_t dataLen){
  if(columnAdd > (uint32_t)W25N_MAX_COLUMN) return 1;
  if(dataLen > (uint32_t)W25N_MAX_COLUMN - columnAdd) return 1;
  char columnHigh = (columnAdd & 0xFF00) >> 8;
  char columnLow = columnAdd & 0xff;
  unsigned char cmdbuf[3] = {W25N_PROG_DATA_LOAD, columnHigh, columnLow};
  this->block_WIP();
  this->writeEnable();

  digitalWrite(_cs, LOW);
  wiringPiSPIDataRW(CHANNEL, cmdbuf, sizeof(cmdbuf));
  wiringPiSPIDataRW(CHANNEL, buf, dataLen);
  digitalWrite(_cs, HIGH);

  return 0;
}

int W25N::loadProgData(uint16_t columnAdd, unsigned char* buf, uint32_t dataLen, uint32_t pageAdd){
  if(this->dieSelectOnAdd(pageAdd)) return 1;
  return this->loadProgData(columnAdd, buf, dataLen);
}

int W25N::loadRandProgData(uint16_t columnAdd, unsigned char* buf, uint32_t dataLen){
  if(columnAdd > (uint32_t)W25N_MAX_COLUMN) return 1;
  if(dataLen > (uint32_t)W25N_MAX_COLUMN - columnAdd) return 1;
  char columnHigh = (columnAdd & 0xFF00) >> 8;
  char columnLow = columnAdd & 0xff;
  unsigned char cmdbuf[3] = {W25N_RAND_PROG_DATA_LOAD, columnHigh, columnLow};
  this->block_WIP();
  this->writeEnable();

  digitalWrite(_cs, LOW);
  wiringPiSPIDataRW(CHANNEL, cmdbuf, sizeof(cmdbuf));
  wiringPiSPIDataRW(CHANNEL, buf, dataLen);
  digitalWrite(_cs, HIGH);

  return 0;
}

int W25N::loadRandProgData(uint16_t columnAdd, unsigned char* buf, uint32_t dataLen, uint32_t pageAdd){
  if(this->dieSelectOnAdd(pageAdd)) return 1;
  return this->loadRandProgData(columnAdd, buf, dataLen);
}

int W25N::ProgramExecute(uint32_t pageAdd){
  if(pageAdd > this->getMaxPage()) return 1;
  this->dieSelectOnAdd(pageAdd);
  char pageHigh = (char)((pageAdd & 0xFF00) >> 8);
  char pageLow = (char)(pageAdd);
  this->writeEnable();
  unsigned char buf[4] = {W25N_PROG_EXECUTE, 0x00, pageHigh, pageLow};
  this->sendData(buf, sizeof(buf));
  return 0;
}

int W25N::pageDataRead(uint32_t pageAdd){
  if(pageAdd > this->getMaxPage()) return 1;
  this->dieSelectOnAdd(pageAdd);
  char pageHigh = (char)((pageAdd & 0xFF00) >> 8);
  char pageLow = (char)(pageAdd);
  unsigned char buf[4] = {W25N_PAGE_DATA_READ, 0x00, pageHigh, pageLow};
  this->block_WIP();
  this->sendData(buf, sizeof(buf));
  return 0;
}

int W25N::read(uint16_t columnAdd, unsigned char* buf, uint32_t dataLen){
  if(columnAdd > (uint32_t)W25N_MAX_COLUMN) return 1;
  if(dataLen > (uint32_t)W25N_MAX_COLUMN - columnAdd) return 1;
  char columnHigh = (columnAdd & 0xFF00) >> 8;
  char columnLow = columnAdd & 0xff;
  unsigned char cmdbuf[4] = {W25N_READ, columnHigh, columnLow, 0x00};
  this->block_WIP();

  digitalWrite(_cs, LOW);
  wiringPiSPIDataRW(CHANNEL, cmdbuf, sizeof(cmdbuf));
  wiringPiSPIDataRW(CHANNEL, buf, dataLen);
  digitalWrite(_cs, HIGH);

  return 0;
}
//Returns the Write In Progress bit from flash.
int W25N::check_WIP(){
  char status = this->getStatusReg(W25N_STAT_REG);
  if(status & 0x01){
    return 1;
  }
  return 0;
}                      

int W25N::block_WIP(){
  //Max WIP time is 10ms for block erase so 15 should be a max.
  unsigned long tstamp = millis();
  while(this->check_WIP()){
    delay(1);
    if (millis() > tstamp + 15) return 1;
  }
  return 0;
}

int W25N::check_status(){
  return(this->getStatusReg(W25N_STAT_REG));
}
