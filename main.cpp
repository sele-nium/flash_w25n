#include "WinbondW25N.h"
#include <cerrno>

W25N flash = W25N();

uint8_t buf[2048];

#define CS_PIN 10
#define SPEED 32000000

void usage(char *sProg)
{
    printf("Usage: %s <r|w|e> <out|in file only r|w>\n", sProg);
}

int main(int iArg, char *asArg[]) 
{
  if (iArg < 2) {
      usage(asArg[0]);
      return 1;
  }
  if(flash.begin(CS_PIN, SPEED) == 0){
    printf("Register Protection: 0x%.2X\nRegister Config: 0x%.2X\nRegister Status: 0x%.2X\n", flash.getStatusReg(W25N_PROT_REG), flash.getStatusReg(W25N_CONFIG_REG), flash.getStatusReg(W25N_STAT_REG));
    printf("Flash init successful\n");
  } else {
    printf("Flash init Failed\n");
    return 1;
  }
  if (!strcmp(asArg[1], "r")) {
      if (iArg != 3) {
          usage(asArg[0]);
          return 1;
      }
      FILE *fOut = fopen(asArg[2], "w");
      if (fOut == NULL) {
          printf("Error file: %s\n", strerror(errno));
          return 1;
      }
      uint32_t iMaxPage = flash.getMaxPage();
      printf("Max page %d\n", iMaxPage);
      for (uint32_t iNdx = 0; iNdx < iMaxPage; iNdx++) {
          flash.pageDataRead(iNdx);
          memset(buf, 0, sizeof(buf));
          flash.read(0, buf, sizeof(buf));
          fwrite(buf, sizeof(buf), 1, fOut);
          printf("%d of %d\r", iNdx, iMaxPage);
      }
      fclose(fOut);
  } else if (!strcmp(asArg[1], "w")) {
      if (iArg != 3) {
          usage(asArg[0]);
          return 1;
      }
      FILE *fIn = fopen(asArg[2], "r");
      if (fIn == NULL) {
          printf("Error file: %s\n", strerror(errno));
          return 1;
      }
      flash.bulkErase();

      uint32_t iMaxPage = flash.getMaxPage();
      printf("Max page %d\n", iMaxPage);
      for (uint32_t iNdx = 0; iNdx < iMaxPage; iNdx++) {
          memset(buf, 0, sizeof(buf));
          if (fread(buf, sizeof(buf), 1, fIn) != 1) {
              printf("fread failed\n");
              break;
          }
          flash.loadProgData(0, buf, sizeof(buf));
          flash.ProgramExecute(iNdx);
          printf("%d of %d\r", iNdx, iMaxPage);
      }
      fclose(fIn);
  } else if (!strcmp(asArg[1], "e")) {
      flash.bulkErase();
  } else {
      printf("Operation not supported\n");
  }
}
