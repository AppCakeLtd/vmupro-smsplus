/****************************************************************************
 *  config.c
 *
 *  SMS Plus GX configuration file support
 *
 *  code by Eke-Eke (2008)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ***************************************************************************/

#ifndef _CONFIG_H_
#define _CONFIG_H_

/****************************************************************************
 * Config Option
 *
 ****************************************************************************/
typedef struct {
  char version[15];
  int sndrate;
  int country;
  int console;
  int display;
  int fm;
  int codies;
  int16 xshift;
  int16 yshift;
  int16 xscale;
  int16 yscale;
  uint8_t tv_mode;
  uint8_t aspect;
  uint8_t overscan;
  uint8_t render;
  uint8_t ntsc;
  uint8_t bilinear;
  uint8_t sms_pal;
  uint8_t tms_pal;
  int8 autofreeze;
  uint8_t use_bios;
  uint8_t spritelimit;
  uint8_t extra_gg;
} t_option;

/* Global data */
extern t_option option;


extern void config_save();
extern void config_load();
extern void set_option_defaults();


#endif /* _CONFIG_H_ */
