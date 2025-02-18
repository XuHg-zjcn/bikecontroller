/*
 * Copyright (C) 2025  徐瑞骏(科技骏马)
 *
 * This code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */
#ifndef MISC_H
#define MISC_H

#define READ_UINT8(x)          (*(uint8_t *)(x))

#define READ_UINT16_UNALIGN(x) ((((uint8_t *)(x))[0]) |		\
				((((uint8_t *)(x))[1])<<8))

#define READ_UINT32_UNALIGN(x) ((((uint8_t *)(x))[0]) |		\
				((((uint8_t *)(x))[1])<<8) |	\
				((((uint8_t *)(x))[2])<<16) |	\
				((((uint8_t *)(x))[3])<<24))

#define READ_INT8(x)          ((int8_t)READ_UINT8(x))
#define READ_INT16_UNALIGN(x) ((int16_t)READ_UINT16_UNALIGN(x))
#define READ_INT32_UNALIGN(x) ((int16_t)READ_UINT32_UNALIGN(x))

#endif
