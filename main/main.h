/*
 * main.h
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */


#ifndef MAIN_H_
#define MAIN_H_

typedef struct {
  unsigned char device_id;
  unsigned char program_id;
  unsigned char message_id;
  unsigned char message[8];
} program_data_t;

#endif /* MAIN_H_ */
