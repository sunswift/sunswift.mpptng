/* Copyright (C) David Snowdon, 2009 <scandal@snowdon.id.au> */ 

/* 
 * This file is part of the UNSWMPPTNG firmware.
 * 
 * The UNSWMPPTNG firmware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * The UNSWMPPTNG firmware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with the UNSWMPPTNG firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

/* config.c 
 * David Snowdon, 28 March, 2008
 */
#include <io.h>
#include <signal.h>
#include <iomacros.h>

#include <scandal/devices.h>
#include <scandal/eeprom.h>
#include <scandal/types.h>

#include <project/mpptng_error.h>
#include <project/config.h>

/* Magic number to make sure EEPROM has been programmed */ 
#define MPPTNG_CONFIG_MAGIC 0xAA

static inline void
calculate_checksum(mpptng_config_t config, uint8_t *sum, uint8_t *xor){
    uint8_t* array; 
    uint16_t i; 
    
    *sum = *xor = 0;
    array = (uint8_t*)(&config); 
    for(i=0; i<sizeof(config); i++){
        *sum += *array; 
        *xor ^= *array; 
        array++; 
    }
}

void
config_read(void){
  uint8_t sum, xor; 
  uint8_t insum, inxor; 

  sc_user_eeprom_read_block(0, (uint8_t*)&config, sizeof(config)); 
    
  insum = config.checksum; 
  inxor = config.checkxor; 
    
  config.checksum = config.checkxor = 0; 

  calculate_checksum(config, &sum, &xor); 

  if( (insum != sum) || (inxor != xor) ){
    mpptng_fatal_error(UNSWMPPTNG_ERROR_EEPROM); 
  }
}

int config_write(void){
  uint8_t sum, xor; 

  config.checksum = config.checkxor = 0; 
  
  calculate_checksum(config, &sum, &xor); 
  
  config.checksum = sum; 
  config.checkxor = xor; 

  sc_user_eeprom_write_block(0, (u08*)&config, sizeof(config)); 

  return 0; 
}
