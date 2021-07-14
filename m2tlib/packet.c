#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m2tlib.h"

#define DEBUG 0

// パケットを格納する関数。
// 途中ならスキップ
int st_packet( PACKET_LIST *packet_list, char *buf ){

	// ヘッダを取得してパケットの先頭を割り出す
	// ペイロードのoffsetを取得するのに必要
	TS_HEAD head; memset( &head,   0 ,sizeof(head));
	st_head( &head, buf );

#if DEBUG
	printf("packet.c: In st_packet: PID=0x%04X\n", head.pid);
#endif

	// ただし、エラーについてはそもそも除外されてるので不要
	// コピー元・コピー先を格納するポインタ変数
	char *copy_src, *copy_dst;
	uint64_t copy_size;

	// 最初？途中？
	if( head.payload_unit_start_indicator == 1 ){

		// 初回なはずなのに既に使われているなら初期化を行う
		// この際下記の項目はどうせ初期化されるのでここでは特にいじらない
		//   - packet_list[head.pid].payload
		//   - packet_list[head.pid].copybytes
		//   - packet_list[head.pid].loading
		if( packet_list[head.pid].copybytes > 0 ){ free(packet_list[head.pid].payload); }

		// 初回なのでcopybytesは初期化する
		packet_list[head.pid].copybytes = 0;

		// section_lengthを取り出す
		// 初回のみの動き
		uint64_t boff = 12 + ( head.payload_start_pos * 8 );
		uint16_t section_length = getBit( buf, &boff, 12);

		// パケットのサイズはsection_lengthより3バイト多い
		packet_list[head.pid].size = section_length+3;

#if DEBUG
		printf("packet.c: INITIAL: PID=0x%04X SecLen=%d PacketSize=%d\n", head.pid, section_length, section_length+3 );
#endif
		// 領域の確保を行う
		packet_list[head.pid].payload = (char *)calloc(1, packet_list[head.pid].size+188);

		// 領域の確保を行う
		packet_list[head.pid].payload = (char *)calloc(1, packet_list[head.pid].size+188);

		// memcpy用のパラメータ生成
		copy_dst  = packet_list[head.pid].payload;
		copy_src  = buf+head.payload_start_pos;
		copy_size = 188-head.payload_start_pos;


	}else{

		// 特にファイルの最初の方で
		// 初回をすっ飛ばしてここにきてしまう事がある。
		// その場合は何もせずにスルーさせる
		if( packet_list[head.pid].loading == 0 ){ goto TS_PACKET_FAIL; }

#if DEBUG
		printf("packet.c: CONTINUE: PID=0x%04X\n", head.pid );
#endif

		// memcpy用のパラメータ生成
		copy_dst  = packet_list[head.pid].payload+packet_list[head.pid].copybytes;
		copy_src  = buf+head.payload_start_pos;
		copy_size = 188-head.payload_start_pos;

	}

	// copy_sizeは基本的に188を超える事はないので
	// それを超えたら異常とみなして読み込みを中止する
	if(copy_size > 188 ){
	        goto TS_PACKET_FAIL;
	}

#if DEBUG
	printf("packet.c: MEMCPY DST=0x%X SRC=0x%X SIZE=%u\n", copy_dst, copy_src, copy_size);
#endif

	// memcpyでデータをコピーし、読み込み済みバイト数に加える
	memcpy( copy_dst, copy_src, copy_size );
	packet_list[head.pid].copybytes += copy_size;

#if DEBUG
	printf("packet.c: MEMCPY DONE\n");
#endif

	// 完了しているかどうかチェック
	if( packet_list[head.pid].copybytes > packet_list[head.pid].size ){

		// 読み込み完了記念にloadingをゼロにリセット
		packet_list[head.pid].loading = 0;

		// SecLenがないものは無視
		if( packet_list[head.pid].size < 7 ){ goto TS_PACKET_FAIL; }

		// PATは元々CRCがないのでチェックせずに成功とする
		if( head.pid == 0x00 ) { goto TS_PACKET_SUCCESS; }

		// CRCのチェック結果を代入
		packet_list[head.pid].crc_check = crc32_check(packet_list[head.pid].payload);
		goto TS_PACKET_SUCCESS;

	}

	// まだ続くのでloadingフラグを立てる
	packet_list[head.pid].loading = 1;
	goto TS_PACKET_FAIL;

TS_PACKET_SUCCESS:
	return(1);

TS_PACKET_FAIL:
	return(0);


}

// packet_listのpayloadを開放して全部ゼロ埋めする
void free_packet_list(PACKET_LIST *ptr){
	// 開放してゼロ埋め
	// なお、ゼロ埋めの際にenableをゼロにすると以降読み込まれなくなって
	// しまうので、それ以外の項目を初期化するにとどめる。
	if( ptr->payload != NULL ){ free(ptr->payload); }
	ptr->payload   = NULL;
	ptr->size      = 0;
	ptr->copybytes = 0;
	ptr->crc_check = 0;
	ptr->loading   = 0;
}


// 全PIDのpacket_listを初期化する
void free_packet_list_all(PACKET_LIST *ptr){
	int i;
	for(i=0;i<8192;i++){
		free_packet_list(&ptr[i]);
	}
}
