/*
 *  project-conf.h
 *
 *  Created on: 27 jan 2016
 *      Author: Sebastiano Milardo
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/*************************************************************************/
#define CFS_ENABLED   0
#define ELF_ENABLED   0
#define BATTERY_ENABLED 0
/*************************************************************************/
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     contikimac_driver /*enable contikimac_driver for energy efficiency,
 * otherwise set to nullrdc_driver*/

#endif // PROJECT_CONF_H_


