/*
 * Copyright (C) 2015 SDN-WISE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *         Header file for SDN-WISE Packet Handler.
 * \author
 *         Sebastiano Milardo <s.milardo@hotmail.it>
 */

/**
 * \addtogroup sdn-wise
 * @{
 */

#ifndef PACKET_HANDLER_H_
#define PACKET_HANDLER_H_

#include "packet-buffer.h"

/* Address API. */
void handle_packet(packet_t*);
void test_handle_open_path(void);
void makeflowrule(uint8_t* mmd_array, int mmd_len);
int errorcheck(packet_t* p);
extern int mmd_count;
#endif /* PACKET_HANDLER_H_ */
/** @} */
