#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "m2tlib.h"

#define DEBUG 0

// TOT構造体に値を突っ込む
// この関数では構造体にパケットを埋め込む以外はやらない
void st_tot( TS_TOT *out, unsigned char *buf ){

	uint64_t gomi;
	uint64_t boff = 0;
	out->table_id                 = getBit( buf, &boff, 8 );
	out->section_syntax_indicator = getBit( buf, &boff, 1 );
	gomi                          = getBit( buf, &boff, 1 );
	gomi                          = getBit( buf, &boff, 2 );
	out->section_length           = getBit( buf, &boff, 12);

	// TOTの有効性確認
	//  - tableIDが合ってない
	//  - syntax_indicator==1
	//  - out->section_lengthが1021より大きい　…場合は無効
	if( out->table_id != 0x73 
			|| out->section_syntax_indicator == 1
			|| out->section_length > 1021        )
	{
		out->disable = 1;
		return;
	}

	out->disable = 0;

	// 40ビットからtm構造体を作る
	time_to_tm( &out->tm, buf+(boff/8) );

#if	DEBUG
	// デバッグ用
	packetdump( buf, 188);
	printf("TOT: %s\n", tm_to_text(&out->tm));
#endif

}

// TOTは構造が単純なのでfreeするだけ。一応関数にしておく
void free_tot(TS_TOT *out){
	// 特にする事はないけど念のためゼロ埋めしておく
	memset( out,  0 ,sizeof(out));
}
