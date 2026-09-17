#ifndef __R60ABD1_H
#define __R60ABD1_H

#include <stdio.h>
void Parse_R60ABD1data(uint8_t *data, uint16_t length);
uint8_t get_hex_breathe_data(void);
uint8_t get_hex_heart_data(void);
void R60ABD1_Init(void);

#endif
