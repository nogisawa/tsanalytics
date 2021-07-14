#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "m2tlib.h"

// TOT構造体に値を突っ込む
// この関数では構造体にパケットを埋め込む以外はやらない
void st_tot( TS_TOT *out, unsigned char *buf ){

	uint64_t gomi;
	uint64_t boff = 0;
	out->table_id = getBit( buf, &boff, 8 );
	gomi          = getBit( buf, &boff, 1 );
	gomi          = getBit( buf, &boff, 1 );
	gomi          = getBit( buf, &boff, 2 );
	gomi          = getBit( buf, &boff, 12);

	// 40ビットからtm構造体を作る
	time_to_tm( &out->tm, buf+(boff/8) );

	// デバッグ用
	//packetdump( buf, 188);
	//printf("TOT: ");
	//dump_tm(&out->tm);

}

// TOTは構造が単純なのでfreeするだけ。一応関数にしておく
void free_tot(TS_TOT *out){
	// 特にする事はないけど念のためゼロ埋めしておく
	memset( out,  0 ,sizeof(out));
}
