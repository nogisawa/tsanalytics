#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "m2tlib.h"

// tsselectから持ってきたadaptation_field関係の関数

void extract_adaptation_field(ADAPTATION_FIELD *dst, unsigned char *data)
{
	int n;
	unsigned char *p;
	unsigned char *tail;

	p = data;

	memset(dst, 0, sizeof(ADAPTATION_FIELD));
	if( (p[0] == 0) || (p[0] > 183) ){
		return;
	}

	dst->adaptation_field_length = p[0];
	p += 1;
	tail = p + dst->adaptation_field_length;
	if( (p+1) > tail ){
		memset(dst, 0, sizeof(ADAPTATION_FIELD));
		return;
	}

	dst->discontinuity_counter = (p[0] >> 7) & 1;
	dst->random_access_indicator = (p[0] >> 6) & 1;
	dst->elementary_stream_priority_indicator = (p[0] >> 5) & 1;
	dst->pcr_flag = (p[0] >> 4) & 1;
	dst->opcr_flag = (p[0] >> 3) & 1;
	dst->splicing_point_flag = (p[0] >> 2) & 1;
	dst->transport_private_data_flag = (p[0] >> 1) & 1;
	dst->adaptation_field_extension_flag = p[0] & 1;

	p += 1;

	if(dst->pcr_flag != 0){
		if( (p+6) > tail ){
			memset(dst, 0, sizeof(ADAPTATION_FIELD));
			return;
		}
		dst->program_clock_reference = ((p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3]);
		dst->program_clock_reference <<= 10;
		dst->program_clock_reference |= (((p[4]&0x80)<<2)|((p[4]&1)<<1)|p[5]);
		p += 6;
	}

	if(dst->opcr_flag != 0){
		if( (p+6) > tail ){
			memset(dst, 0, sizeof(ADAPTATION_FIELD));
			return;
		}
		dst->original_program_clock_reference = ((p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3]);
		dst->original_program_clock_reference <<= 10;
		dst->original_program_clock_reference |= (((p[4]&0x80)<<2)|((p[4]&1)<<1)|p[5]);
		p += 6;
	}

	if(dst->splicing_point_flag != 0){
		if( (p+1) > tail ){
			memset(dst, 0, sizeof(ADAPTATION_FIELD));
			return;
		}
		dst->splice_countdown = p[0];
		p += 1;
	}

	if(dst->transport_private_data_flag != 0){
		if( (p+1) > tail ){
			memset(dst, 0, sizeof(ADAPTATION_FIELD));
			return;
		}
		n = p[0];
		dst->transport_private_data_length = n;
		p += (1+n);
		if( p > tail ){
			memset(dst, 0, sizeof(ADAPTATION_FIELD));
			return;
		}
	}

	if(dst->adaptation_field_extension_flag != 0){
		if( (p+2) > tail ){
			memset(dst, 0, sizeof(ADAPTATION_FIELD));
			return;
		}
		n = p[0];
		dst->adaptation_field_extension_length = n;
		p += 1;
		if( (p+n) > tail ){
			memset(dst, 0, sizeof(ADAPTATION_FIELD));
			return;
		}
		dst->ltw_flag = (p[0] >> 7) & 1;
		dst->piecewise_rate_flag = (p[0] >> 6) & 1;
		dst->seamless_splice_flag = (p[0] >> 5) & 1;
		p += 1;
		n -= 1;
		if(dst->ltw_flag != 0){
			if(n < 2){
				memset(dst, 0, sizeof(ADAPTATION_FIELD));
				return;
			}
			dst->ltw_valid_flag = (p[0] >> 7) & 1;
			dst->ltw_offset = (((p[0] & 0x7f)<<8) | p[1]);
			p += 2;
			n -= 2;
		}
		if(dst->piecewise_rate_flag != 0){
			if(n < 3){
				memset(dst, 0, sizeof(ADAPTATION_FIELD));
				return;
			}
			dst->piecewise_rate = (((p[0] & 0x3f)<<16)|(p[1]<<8)|p[2]);
			p += 3;
			n -= 3;
		}
		if(dst->seamless_splice_flag != 0){
			if(n < 5){
				memset(dst, 0, sizeof(ADAPTATION_FIELD));
				return;
			}
			dst->splice_type = (p[0] >> 4) & 0x0f;
			dst->dts_next_au = (((p[0]&0x0e)<<14)|(p[1]<<7)|((p[2]>>1)&0x7f));
			dst->dts_next_au <<= 15;
			dst->dts_next_au |= ((p[3]<<7)|((p[4]>>1)&0x7f));
			p += 5;
			n -= 5;
		}
		p += n;
	}

}
