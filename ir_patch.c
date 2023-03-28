/*
 * IR - Lightweight JIT Compilation Framework
 * (Native code patcher)
 * Copyright (C) 2022 Zend by Perforce.
 * Authors: Dmitry Stogov <dmitry@php.net>
 *
 * Based on Mike Pall's implementation for LuaJIT.
 */

#include "ir.h"
#include "ir_private.h"

#if defined(IR_TARGET_X86) || defined(IR_TARGET_X64)
static uint32_t _asm_x86_inslen(const uint8_t* p)
{
	static const uint8_t map_op1[256] = {
		0x92,0x92,0x92,0x92,0x52,0x45,0x51,0x51,0x92,0x92,0x92,0x92,0x52,0x45,0x51,0x20,
		0x92,0x92,0x92,0x92,0x52,0x45,0x51,0x51,0x92,0x92,0x92,0x92,0x52,0x45,0x51,0x51,
		0x92,0x92,0x92,0x92,0x52,0x45,0x10,0x51,0x92,0x92,0x92,0x92,0x52,0x45,0x10,0x51,
		0x92,0x92,0x92,0x92,0x52,0x45,0x10,0x51,0x92,0x92,0x92,0x92,0x52,0x45,0x10,0x51,
#ifdef IR_TARGET_X64
		0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,
#else
		0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
#endif
		0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
		0x51,0x51,0x92,0x92,0x10,0x10,0x12,0x11,0x45,0x86,0x52,0x93,0x51,0x51,0x51,0x51,
		0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
		0x93,0x86,0x93,0x93,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,
		0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x47,0x51,0x51,0x51,0x51,0x51,
#ifdef IR_TARGET_X64
		0x59,0x59,0x59,0x59,0x51,0x51,0x51,0x51,0x52,0x45,0x51,0x51,0x51,0x51,0x51,0x51,
#else
		0x55,0x55,0x55,0x55,0x51,0x51,0x51,0x51,0x52,0x45,0x51,0x51,0x51,0x51,0x51,0x51,
#endif
		0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,
		0x93,0x93,0x53,0x51,0x70,0x71,0x93,0x86,0x54,0x51,0x53,0x51,0x51,0x52,0x51,0x51,
		0x92,0x92,0x92,0x92,0x52,0x52,0x51,0x51,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,
		0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x45,0x45,0x47,0x52,0x51,0x51,0x51,0x51,
		0x10,0x51,0x10,0x10,0x51,0x51,0x63,0x66,0x51,0x51,0x51,0x51,0x51,0x51,0x92,0x92
	};
	static const uint8_t map_op2[256] = {
		0x93,0x93,0x93,0x93,0x52,0x52,0x52,0x52,0x52,0x52,0x51,0x52,0x51,0x93,0x52,0x94,
		0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
		0x53,0x53,0x53,0x53,0x53,0x53,0x53,0x53,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
		0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x34,0x51,0x35,0x51,0x51,0x51,0x51,0x51,
		0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
		0x53,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
		0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
		0x94,0x54,0x54,0x54,0x93,0x93,0x93,0x52,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
		0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,
		0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
		0x52,0x52,0x52,0x93,0x94,0x93,0x51,0x51,0x52,0x52,0x52,0x93,0x94,0x93,0x93,0x93,
		0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x94,0x93,0x93,0x93,0x93,0x93,
		0x93,0x93,0x94,0x93,0x94,0x94,0x94,0x93,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
		0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
		0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
		0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x52
	};
	uint32_t result = 0;
	uint32_t prefixes = 0;
	uint32_t x = map_op1[*p];

	for (;;) {
		switch (x >> 4) {
			case 0:
				return result + x + (prefixes & 4);
			case 1:
				prefixes |= x;
				x = map_op1[*++p];
				result++;
				break;
			case 2:
				x = map_op2[*++p];
				break;
			case 3:
				p++;
				goto mrm;
			case 4:
				result -= (prefixes & 2);
				/* fallthrough */
			case 5:
				return result + (x & 15);
			case 6: /* Group 3. */
				if (p[1] & 0x38) {
					x = 2;
				} else if ((prefixes & 2) && (x == 0x66)) {
					x = 4;
				}
				goto mrm;
			case 7: /* VEX c4/c5. */
#ifdef IR_TARGET_X86
				if (p[1] < 0xc0) {
					x = 2;
					goto mrm;
				}
#endif
				if (x == 0x70) {
					x = *++p & 0x1f;
					result++;
					if (x >= 2) {
						p += 2;
						result += 2;
						goto mrm;
					}
				}
				p++;
				result++;
				x = map_op2[*++p];
				break;
			case 8:
				result -= (prefixes & 2);
				/* fallthrough */
			case 9:
mrm:
				/* ModR/M and possibly SIB. */
				result += (x & 15);
				x = *++p;
				switch (x >> 6) {
					case 0:
						if ((x & 7) == 5) {
							return result + 4;
						}
						break;
					case 1:
						result++;
						break;
					case 2:
						result += 4;
						break;
					case 3:
						return result;
				}
				if ((x & 7) == 4) {
					result++;
					if (x < 0x40 && (p[1] & 7) == 5) {
						result += 4;
					}
				}
				return result;
		}
	}
}

typedef IR_SET_ALIGNED(1, uint16_t unaligned_uint16_t);
typedef IR_SET_ALIGNED(1, int32_t unaligned_int32_t);

static int ir_patch_code(const void *code, size_t size, const void *from_addr, const void *to_addr)
{
	int ret = 0;
	uint8_t *p, *end;

	p = (uint8_t*)code;
	end = p + size - 5;
	while (p < end) {
		if ((*(unaligned_uint16_t*)p & 0xf0ff) == 0x800f && p + *(unaligned_int32_t*)(p+2) == (uint8_t*)from_addr - 6) {
			*(unaligned_int32_t*)(p+2) = ((uint8_t*)to_addr - (p + 6));
			ret++;
		} else if (*p == 0xe9 && p + *(unaligned_int32_t*)(p+1) == (uint8_t*)from_addr - 5) {
			*(unaligned_int32_t*)(p+1) = ((uint8_t*)to_addr - (p + 5));
			ret++;
		}
		p += _asm_x86_inslen(p);
	}
	if (ret) {
		ir_mem_flush((void*)code, size);
	}
	return ret;
}

#elif defined(IR_TARGET_AARCH64)

static int ir_patch_code(const void *code, size_t size, const void *from_addr, const void *to_addr)
{
	int ret = 0;
	uint8_t *p, *end;
	const void *veneer = NULL;
	ptrdiff_t delta;

	end = (uint8_t*)code;
	p = end + size;
	while (p > end) {
		uint32_t *ins_ptr;
		uint32_t ins;

		p -= 4;
		ins_ptr = (uint32_t*)p;
		ins = *ins_ptr;
		if ((ins & 0xfc000000u) == 0x14000000u) {
			// B (imm26:0..25)
			delta = (uint32_t*)from_addr - ins_ptr;
			if (((ins ^ (uint32_t)delta) & 0x01ffffffu) == 0) {
				delta = (uint32_t*)to_addr - ins_ptr;
				if (((delta + 0x02000000) >> 26) != 0) {
					abort(); // branch target out of range
				}
				*ins_ptr = (ins & 0xfc000000u) | ((uint32_t)delta & 0x03ffffffu);
				ret++;
				if (!veneer) {
					veneer = p;
				}
			}
		} else if ((ins & 0xff000000u) == 0x54000000u ||
		           (ins & 0x7e000000u) == 0x34000000u) {
			// B.cond, CBZ, CBNZ (imm19:5..23)
			delta = (uint32_t*)from_addr - ins_ptr;
			if (((ins ^ ((uint32_t)delta << 5)) & 0x00ffffe0u) == 0) {
				delta = (uint32_t*)to_addr - ins_ptr;
				if (((delta + 0x40000) >> 19) != 0) {
					if (veneer) {
						delta = (uint32_t*)veneer - ins_ptr;
						if (((delta + 0x40000) >> 19) != 0) {
							abort(); // branch target out of range
						}
					} else {
						abort(); // branch target out of range
					}
				}
				*ins_ptr = (ins & 0xff00001fu) | (((uint32_t)delta & 0x7ffffu) << 5);
				ret++;
			}
		} else if ((ins & 0x7e000000u) == 0x36000000u) {
			// TBZ, TBNZ (imm14:5..18)
			delta = (uint32_t*)from_addr - ins_ptr;
			if (((ins ^ ((uint32_t)delta << 5)) & 0x0007ffe0u) == 0) {
				delta = (uint32_t*)to_addr - ins_ptr;
				if (((delta + 0x2000) >> 14) != 0) {
					if (veneer) {
						delta = (uint32_t*)veneer - ins_ptr;
						if (((delta + 0x2000) >> 14) != 0) {
							abort(); // branch target out of range
						}
					} else {
						abort(); // branch target out of range
					}
				}
				*ins_ptr = (ins & 0xfff8001fu) | (((uint32_t)delta & 0x3fffu) << 5);
				ret++;
			}
		}
	}

	if (ret) {
		ir_mem_flush((void*)code, size);
	}

	return ret;
}
#endif

int ir_patch(const void *code, size_t size, uint32_t jmp_table_size, const void *from_addr, const void *to_addr)
{
	int ret = 0;

	if (jmp_table_size) {
		const void **jmp_slot = (const void **)((char*)code + IR_ALIGNED_SIZE(size, sizeof(void*)));

		do {
			if (*jmp_slot == from_addr) {
				*jmp_slot = to_addr;
				ret++;
			}
			jmp_slot++;
		} while (--jmp_table_size);
	}

	ret += ir_patch_code(code, size, from_addr, to_addr);

	return ret;
}
