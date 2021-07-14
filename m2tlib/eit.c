#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "m2tlib.h"

#define DEBUG 0

// EIT関連
// EVENT_LISTは分離しました。→ eit_event_list.c

int st_eit( TS_EIT *out, unsigned char *buf ){

	// 一時的にしか使わない変数
	uint64_t desc_loop_len, desc_len;
	uint64_t i,j;
	uint64_t loaded,loaded_desc;
	TS_EIT_EVENTS *curr_ev, *before_ev;
	TS_DESCRIPTOR *curr_desc, *before_desc;

        uint64_t gomi;

        uint64_t boff = 0;
        out->table_id			 = getBit( buf, &boff, 8 );
	out->section_syntax_indicator	 = getBit( buf, &boff, 1 );
	gomi				 = getBit( buf, &boff, 1 );
	gomi				 = getBit( buf, &boff, 2 );
	out->section_length		 = getBit( buf, &boff, 12);
	out->service_id			 = getBit( buf, &boff, 16);
	gomi				 = getBit( buf, &boff, 2 );
	out->version_number		 = getBit( buf, &boff, 5 );
	out->current_next_indicator	 = getBit( buf, &boff, 1 );
	out->section_number		 = getBit( buf, &boff, 8 );
	out->last_section_number	 = getBit( buf, &boff, 8 );
	out->transport_stream_id	 = getBit( buf, &boff, 16);
	out->original_network_id	 = getBit( buf, &boff, 16);
	out->segment_last_section_number = getBit( buf, &boff, 8 );
	out->last_table_id		 = getBit( buf, &boff, 8 );
	out->events			 = NULL;

	//packetdump(buf,out->section_length+3); // dumpする時CRCを加える

	// 後でSecLenと比較して完了を検知するための読み込みバイト数
	// 初期値はSecLen～ヘッダの最後までのバイト数
	//  + 最後のCRC32(5バイト)
	loaded = 11 + 5;


	// 無効なEITは無視する
	uint8_t eit_ok = 0;
	if( out->table_id == 0x4E || ( 0x50 <= out->table_id && out->table_id <= 0x5F ) ){
		if( out->section_syntax_indicator == 1 &&  out->section_length <= 4093){ eit_ok = 1; }
	}
	if( ! eit_ok ){
#if DEBUG
		printf("InvalidEIT: table_id=0x%X Syntax=%X SecLen=%d Num=%X SID=0x%04X\n", out->table_id, out->section_syntax_indicator, out->section_length, out->section_number, out->service_id );
#endif 
		return(0);
	}


#if DEBUG
	printf("EIT: table_id=0x%X SecLen=%d Num=%X SID=0x%04X\n", out->table_id, out->section_length, out->section_number, out->service_id );
#endif

	// eventの処理
	for( i=0; i<100; i++){

		// SecLenを超えていたらループを抜ける
		if( loaded >= out->section_length ){ break; }

		// メモリの確保
		curr_ev = (TS_EIT_EVENTS *)calloc(1,sizeof(TS_EIT_EVENTS));

		// descriptor loop lengthだけとりあえず取得する
		curr_ev->event_id		 = getBit( buf, &boff, 16 );

		// start_timeだけはビット列なのでmemspyする。
		// 整合性を取るためにboffは手動加算
		memcpy( curr_ev->start_time, buf+(boff/8), 5);
		boff += 40;

		// 開始日時はtm構造体にも入れる(ビット列残す必要あるだろうか)
		time_to_tm( &curr_ev->start_time_tm, curr_ev->start_time );

		// 情報取得の続き
		curr_ev->duration		 = getBit( buf, &boff, 24 );

		// durationは秒に変換しておく
		boff -= 24;
		curr_ev->duration_sec = hex_to_digi(getBit( buf, &boff, 8 ))*3600
				      + hex_to_digi(getBit( buf, &boff, 8 ))*60
				      + hex_to_digi(getBit( buf, &boff, 8 ));

		// 残りの情報を取得
		curr_ev->running_status		 = getBit( buf, &boff,  3 );
		curr_ev->free_CA_mode   	 = getBit( buf, &boff,  1 );
		curr_ev->descriotors_loop_length = getBit( buf, &boff, 12 );
		curr_ev->desc			 = NULL;

#if DEBUG
		// デバッグ用(イベントの読み込み)
		if( curr_ev->descriotors_loop_length > out->section_length ){ printf("Invalid(LoopLen=%d SecLen=%d)",out->section_length, curr_ev->descriotors_loop_length);}
		printf("EIT: table_id=0x%X SecLen=%d Num=%X SID=0x%04X", out->table_id, out->section_length, out->section_number, out->service_id );
		printf(" %02d: event_id=0x%04X Sta=%s Dur=%06X LoopLen=%d\n", i,curr_ev->event_id, tm_to_text(&curr_ev->start_time_tm), curr_ev->duration, curr_ev->descriotors_loop_length);
#endif

		// section_lengthがloop_lengthを越えている場合、異常なのでループを抜ける
		if( curr_ev->descriotors_loop_length > out->section_length ){ break; }

		// descriptor関係の処理
		loaded_desc = 0;
		for( j=0; j<100; j++){

			if( loaded_desc >= curr_ev->descriotors_loop_length ){break;}

			// desc構造体自体は固定長なのでとりあえず確保する
			curr_desc = (TS_DESCRIPTOR *)calloc(1,sizeof(TS_DESCRIPTOR));
			curr_desc->descriptor_tag    = getBit( buf, &boff, 8 );
			curr_desc->descriptor_length = getBit( buf, &boff, 8 );

			// 多分これは間違ってるんだけど、event_idが0x00になったらやめる
			//if( curr_desc->descriptor_tag == 0x00 ){ break; }

#if DEBUG
			// デバッグ用(デスクリプタ読み込み)
			printf("EIT: table_id=0x%X SecLen=%d Num=%X SID=0x%04X", out->table_id, out->section_length, out->section_number, out->service_id );
			printf(" %02d: event_id=0x%04X Sta=%s Dur=%06X LoopLen=%d", i,curr_ev->event_id, tm_to_text(&curr_ev->start_time_tm), curr_ev->duration, curr_ev->descriotors_loop_length);
			printf(" %02d: descTag=0x%02X descLen=%d\n",         j,curr_desc->descriptor_tag, curr_desc->descriptor_length);
#endif
			// descLen以降のデスクリプタの中身をメモリ領域を確保して書き出す
			curr_desc->payload = (char *)calloc(1,curr_desc->descriptor_length);
			memcpy( curr_desc->payload, buf+(boff/8), curr_desc->descriptor_length );

			// とりあえずdescLen分boffを進める
			boff += curr_desc->descriptor_length * 8;
			loaded_desc += curr_desc->descriptor_length + 2;

			// ポインタを付け替えて連結リスト化
			if( curr_ev->desc == NULL ){ curr_ev->desc     = curr_desc; }
					       else{ before_desc->next = curr_desc; }

			before_desc = curr_desc;

		}

		// ポインタを接続する
		if( out->events == NULL ){ out->events = curr_ev; 	}
				     else{ before_ev->next = curr_ev;	}
		before_ev = curr_ev;

		// 色々インクリメント
		//boff += curr_ev->descriotors_loop_length*8;
		loaded += 12 + curr_ev->descriotors_loop_length;

	}

	return(1);

}


// 使い終わったEITを削除して開放する
void free_eit( TS_EIT *out ){

	// 一時的にしか使わない変数
	uint64_t desc_loop_len, desc_len;
	uint64_t i,j;
	uint64_t loaded,loaded_desc;
	TS_EIT_EVENTS *curr_ev, *next_ev;
	TS_DESCRIPTOR *curr_desc, *next_desc;
	curr_ev = out->events;

	for( i=0; i<100; i++ ){
		if( curr_ev == NULL ){ break; }
		// starttime表示用
		curr_desc = curr_ev->desc;
		for( j=0;j<100;j++){
			if( curr_desc == NULL ){break;}
			next_desc = curr_desc->next;

			// デスクリプタを各種開放
			free(curr_desc->payload);
			free(curr_desc);

			// 次へ進める
			curr_desc = next_desc;
		}
		// 開放して次へ進める
		next_ev	= curr_ev->next;
		free(curr_ev);
		curr_ev=next_ev;
	}

	//最後に構造体をゼロ埋めする
	 memset( out,  0 ,sizeof(out));
}

// デバッグ用にEITの取得結果を表示する関数。引数はTS_EITのアドレス
void dump_eit( TS_EIT *out ){

        // 一時的にしか使わない変数
        uint64_t desc_loop_len, desc_len;
        uint64_t i,j;
        uint64_t loaded,loaded_desc;
        TS_EIT_EVENTS *curr_ev, *before_ev;
        TS_DESCRIPTOR *curr_desc, *before_desc;
        curr_ev = out->events;

        printf("EIT: table_id=0x%X SecLen=%d\n", out->table_id, out->section_length);
        for( i=0; i<100; i++ ){
                if( curr_ev == NULL ){ break; }

                // starttime表示用
                char dtmp[20];
                strftime(dtmp, 20, "%Y-%m-%d %H:%M:%S", &curr_ev->start_time_tm );

                printf("        %02d: event_id=0x%04X Start=%s Dur=%06X LoopLen=%d\n",
                                i,curr_ev->event_id, dtmp, curr_ev->duration, curr_ev->descriotors_loop_length);
                curr_desc = curr_ev->desc;
                for( j=0;j<100;j++){
                        if( curr_desc == NULL ){break;}
                        printf("                %02d: descTag=0x%02X descLen=%d\n",
                                        j,curr_desc->descriptor_tag, curr_desc->descriptor_length);
                        curr_desc = curr_desc->next;
                }
                curr_ev = curr_ev->next;
        }

}

