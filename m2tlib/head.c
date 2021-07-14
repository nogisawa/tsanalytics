#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m2tlib.h"

/* 
 * ここではパケットのヘッダとパース処理のみを行う。
 * 各パケットを構造体に入れる処理は、別ファイルに分離する
 * そうしないと長すぎてわけわかめ
 */


/*
 
  == パケットのパースについて ==
  TSパケットのほとんどはおおむね同じ構造をしてるので、同じように処理する

   [TSヘッダ][ペイロード１]   [TSヘッダ][ペイロード2] [TSヘッダ][ペイロード3] ...

   ヘッダを取り除いて結合させる
   →  [ペイロード1,2,3]

   面倒なパターン

   先頭ペイロードがない。
   [TSヘッダ][ペイロード2] [TSヘッダ][ペイロード3] ... [TSヘッダ][ペイロード1]
   → この場合はパケットを一旦リセットする(ゼロで埋める)
   　この時にfreeを使うとアドレスが変わってしまう。変更後のアドレスは呼び出し元に
   　伝わらない為おかしなことになるので絶対freeは使ってはいけない

   CRCエラー
   →結合したがCRC不一致
   →とりあえず上と同じくリセット(ゼロで埋める)

*/

// =========================================================================================TS ヘッダ
void st_head( TS_HEAD *ptr, unsigned char *buf ){

	uint64_t gomi;
	uint64_t boff = 0;
	ptr->sync_byte				= getBit( buf, &boff, 8 );

	// sync?byteが0x47じゃない場合、全ての値をゼロ埋めして
	// sync_byteのみを返す
	if( ptr->sync_byte != 0x47  ){
		uint16_t tmp =  ptr->sync_byte;
		memset( ptr, 0, sizeof(ptr) );
		ptr->sync_byte = tmp;
		return;
	}

	ptr->transport_error_indicator		= getBit( buf, &boff, 1 );
	ptr->payload_unit_start_indicator	= getBit( buf, &boff, 1 );
	ptr->transport_priority			= getBit( buf, &boff, 1 );
	ptr->pid				= getBit( buf, &boff, 13);
	ptr->transport_scrambling_control	= getBit( buf, &boff, 2 );
	ptr->adaptation_field_control		= getBit( buf, &boff, 2 );
	ptr->continuity_counter			= getBit( buf, &boff, 4 );


	// ==================================== adaptation_field_controlについて
	//  00(0x0) ... 将来使う
	//  01(0x1) ... adaptation_fieldは存在しない
	//  10(0x2) ... adaptation_fieldのみでペイロードが存在しない
	//  11(0x3) ... adaptation_fieldが存在して、その後にペイロードがある
	if( ptr->adaptation_field_control == 0x1 ){
		ptr->adaptation_field_length    = 0;
		ptr->payload_start_pos		= 4;

	}else if( ptr->adaptation_field_control == 0x2 ){
		ptr->adaptation_field_length    = getBit( buf, &boff, 8 );
		ptr->payload_start_pos  	= 0; // ペイロードは存在しない

	}else if( ptr->adaptation_field_control == 0x3 ){
		ptr->adaptation_field_length	= getBit( buf, &boff, 8 );
		ptr->payload_start_pos 		= 4 + ptr->adaptation_field_length;

	}else{
		ptr->payload_start_pos  = 4; // 未定義なので、とりあえず4を入れておく
		ptr->adaptation_field_length    = 0;
	}

	// 初回パケットはseparate_fieldが1バイトあるので1つ加算しておく
	if( ptr->payload_unit_start_indicator == 1 ){ ptr->payload_start_pos++;  }

	// adaptation_field関係
	if(ptr->adaptation_field_control & 2){
		extract_adaptation_field(&ptr->adapt, buf+4);
	}else{
		memset(&ptr->adapt, 0, sizeof(ADAPTATION_FIELD));
	}


	// デバッグ用
	// printf("SYNC=%02X TEI=%1X PID=0x%04X SCRAMBLE=%1X ADAP_CTL=%1X ADAP_LEN=%X, PAYL_POS=%d\n", 
	// 		ptr->sync_byte, ptr->transport_error_indicator ,ptr->pid, ptr->transport_scrambling_control, ptr->adaptation_field_control, ptr->adaptation_field_length, ptr->payload_start_pos );

	//return(ptr);
}


// ts_headのお掃除
void free_head( TS_HEAD *ptr ){
	free(ptr);
}

