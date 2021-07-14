#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m2tlib.h"

// ドロップをカウントします。ドロップがない場合はTRUE、ドロップ発生時はFALSEを返す
uint8_t drop_check_and_count( TS_DROP_COUNT *ts_drop_count, char *buf, uint64_t pos){

	// ※メモ： tsselectのoffsetが何だかわからないけど、posから求められそうな気がする
	// 現状はパラーメータだけ設定していて、特に使われない

	// ヘッダを取得(今回のヘッダ、前回のヘッダ)
	TS_HEAD head, before_head;
	st_head( &head, buf );
	st_head( &before_head, ts_drop_count[head.pid].before_packet );

	// 前回の値が入ってる可能性が高いので消す
	ts_drop_count[head.pid].now_drop       = 0;
	ts_drop_count[head.pid].now_error      = 0;
	ts_drop_count[head.pid].now_scrambling = 0;

	// error,scramblingの処理は簡単なので先にやる
	if( head.transport_error_indicator    ){ ts_drop_count[head.pid].now_error = 1;      }
	if( head.transport_scrambling_control ){ ts_drop_count[head.pid].now_scrambling = 1; }
	ts_drop_count[head.pid].error 	   += ts_drop_count[head.pid].now_error;
	ts_drop_count[head.pid].scrambling += ts_drop_count[head.pid].now_scrambling;

	// 長いのでポインタによる省略
	// nowはポインタじゃなくてもよい気がするけど、統一させる
	uint8_t before = before_head.continuity_counter;
	uint8_t now    = head.continuity_counter;

	// 初期値はdropしてる事にして、ドロップしていない条件が合致したら
	// 以降のチェックをスキップする
	uint8_t have_drop = 0;
	
	// ※初回時の処理は最後にした

	// adapt.discontinuity_counterがよくわからないけど、とりあえずゼロじゃないとダメらしい
	if( ts_drop_count[head.pid].use && head.adapt.discontinuity_counter == 0 ){

		if( head.pid == 0x1FFF ){
			// Nullパケットは何もしない

		}else if( (head.adaptation_field_control & 0x01) == 0 ){
			// ペイロードがない場合の処理。同じカウンタが続くらしい
			if( before != now ){ have_drop=1; }

		}else if( before == now ){
			// 前回のパケットと比較して同じじゃなかったらドロップ認定
			if( memcmp(ts_drop_count[head.pid].before_packet, buf, 188) != 0){
				have_drop = 1;
			}else{
				// 同じパケットが続いて、かつ繰り返しが2回以上の場合ドロップ判定
				ts_drop_count[head.pid].repeat++;
				if( ts_drop_count[head.pid].repeat > 1 ){
					have_drop = 1;
				}
			}
		}else{
			// カウントアップして正しく増えていなかったらドロップ判定
			uint8_t m;
			m = ( before + 1) & 0x0F;
			if( m != head.continuity_counter){
				have_drop = 1;
			}
			ts_drop_count[head.pid].repeat = 0;
		}


	}


	// 最終的にドロップ認定できていた場合、ドロップカウンタを増やす
	// あと繰り返しカウンタもリセットする
	if( have_drop ){
		ts_drop_count[head.pid].now_drop = 1;
		ts_drop_count[head.pid].drop++;
		ts_drop_count[head.pid].repeat = 0;
	}


	// 初回時の操作は判定処理をスキップする。
	// useフラグを立てて前回のパケットをコピーして終わり
	if( ts_drop_count[head.pid].use == 0 ){
		ts_drop_count[head.pid].use = 1;
	}

	// 前回のヘッダの更新
	memcpy( ts_drop_count[head.pid].before_packet, buf, 188 );

	// パケット全体の数を加算
	ts_drop_count[head.pid].total++;

	// drop,error,scramblingのいずれかに異常があったら
	// result>0となるので、その場合はFALSEを返す
	int8_t result = ts_drop_count[head.pid].now_drop 
			+ ts_drop_count[head.pid].now_error 
			+ ts_drop_count[head.pid].now_scrambling;

	if( result > 0 ){ return(0); }

	// 異常なしはTRUEを返す
	return(1);

}

uint16_t mark_last_drop_count( TS_DROP_COUNT *ptr ){

	int i;
	uint16_t last_pid = 8191;
	for(i=0;i<8192;i++){

		// 一度全てをゼロにする
		ptr[i].last = 0;

		// 最後のPIDのを探す
		if(ptr[i].total>0){
			last_pid = i;
		}

	}

	// last_pidの更新
	// 見つからなかった場合0xFFFFが設定される(範囲外)
	ptr[last_pid].last = 1;
}

void dump_drop_count( TS_DROP_COUNT *ts_drop_count ){

	uint64_t i;
	for(i=0;i<8192;i++){
		TS_DROP_COUNT *p = &ts_drop_count[i];
		if( p->use ){
			printf("PID=0x%04X total=%d d=%d e=%d scrambling=%d\n", 
					i, p->total, p->drop, p->error, p->scrambling );
		}
	}

}
