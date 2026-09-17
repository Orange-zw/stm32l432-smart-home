#ifndef __HX711_H
#define __HX711_H

#include "sys.h"

#define HX711_SCK PBout(5) // PB5
#define HX711_DOUT PBin(6) // PB6
extern s32 Weight_Shiwu;

void Init_HX711pin(void);
u32 HX711_Read(void);
void Get_Maopi(void);
uint32_t Get_Weight(void);

#endif
