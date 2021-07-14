/* tsanalytics本体 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include "common.h"

#define DEBUG 0

void tsanalytics(ARGS_TSANALYTICS *params ){

	char *from_path = params->from_path;
	char *out_path  = params->out_path;
	uint16_t service_id = params->service_id;
	char *tmp_tm;
	char str_unknown[8] = "unknown";

	FILE *fp_in, *fp_out;  // 入力ファイルのファイルポインタ

	// 入力ファイルのファイルサイズを取得する
	// なおこの関数はファイルが読み込めない場合は-1を返すので
	// 存在チェックも同時に行う
	int64_t filesize = getFileSize(from_path);
	if( filesize < 0 ){
		fprintf(stderr, "Cannot open input file: %s\n", from_path);
		exit(1);
	}

	// 入力ファイルの存在確認
	// オープンして存在しなかったら終了する
	fp_in = fopen(from_path, "rb");
	if( fp_in == NULL ){
		fprintf(stderr, "Cannot open input file: %s\n", from_path);
		exit(1);
	}

	// 出力ファイル未指定の場合、標準出力にする
	if( out_path == NULL ){
		fp_out = stdout;
	}else{
		printf("OUTPUT\n");
		// 出力ファイルの存在確認
		// オープンして存在しなかったら終了する
		fp_out = fopen(out_path, "wb");
		if( fp_out == NULL ){
			fprintf(stderr, "Cannot open output file: %s\n", out_path);
			exit(1);
		}
	}

	// 188分割した時のパケットの個数
	uint64_t packet_count = filesize/188;

	// 188を超えた場合のパケットの残バイト。
	// TSじゃないファイルに対応するため
	uint64_t last_bytes = filesize - (packet_count*188);

	uint64_t p_cnt,i;
	char buf[188];

	// この段階でservice_idは確定しているので、そのEITを取得する
	fseek(fp_in, 0L, SEEK_SET);

	// パケット一時保管用スペースを確保する
	PACKET_LIST packet_list[8192]; memset( packet_list, 0, sizeof(packet_list));
	// 無条件で読み込むPIDを指定する
	packet_list[0x0].enable = 1; //TOT
	packet_list[0x14].enable = 1; //TOT

	// drop格納用配列を確保する
	TS_DROP_COUNT ts_drop_count[8192]; memset( ts_drop_count, 0, sizeof(ts_drop_count));

	// TOTごとのドロップリスト。
	// TOTの方の初期値はUnknown
	char current_tot[32];
	strcpy( current_tot, "unknown");
	DROP_LIST *drop_list_tot = NULL;
	DROP_LIST *drop_list_per = NULL;

	// パーセントごとのドロップリスト
	// おおまかにファイルのどの辺がドロップしてるのかわかる。
	char current_per[32];

	// 開始・終了時刻用TM構造体
	struct tm first_tot; memset( &first_tot, 0, sizeof(first_tot));
	struct tm last_tot;   memset( &last_tot,   0, sizeof(last_tot));

	// 先頭が0x47じゃないパケットをカウントする変数
	uint64_t sync_error = 0;

	// MD5計算用
	MD5_CTX md5_ctx;
	unsigned char md[MD5_DIGEST_LENGTH];
	char mdString[(MD5_DIGEST_LENGTH*2)+1];
	int md5_r = MD5_Init(&md5_ctx);;

	// SHA計算用
	SHA_CTX sha_ctx;
	unsigned char sha_md[SHA_DIGEST_LENGTH];
	char sha_mdString[(SHA_DIGEST_LENGTH*2)+1];
	int sha_r = SHA1_Init(&sha_ctx);;

	// SHA256計算用
	SHA256_CTX sha256_ctx;
	unsigned char sha256_md[SHA256_DIGEST_LENGTH];
	char sha256_mdString[(SHA256_DIGEST_LENGTH*2)+1];
	int sha256_r = SHA256_Init(&sha256_ctx);;

	// SHA512計算用
	SHA512_CTX sha512_ctx;
	unsigned char sha512_md[SHA512_DIGEST_LENGTH];
	char sha512_mdString[(SHA512_DIGEST_LENGTH*2)+1];
	int sha512_r = SHA512_Init(&sha512_ctx);;

	// TSじゃないと検知した場合ハッシュのみ計算に切り替える
	// その為のグラグ変数
	int this_is_not_TS = 0;
	// SyncErrorが一定数繰り返されたらTSじゃない認定する
	int sync_err_repeat = 0;
	// SyncErrorの一定数ってのはここで定義。
	// とりあえず100で。
	int sync_err_repeat_limit = 100;

#if DEBUG
	printf("packet_count=%d sync_limit=%d\n", packet_count, sync_err_repeat_limit);
#endif

	// PAT保存用
	TS_PAT pat;

	for( p_cnt=0; p_cnt<packet_count; p_cnt++){

		// 1パケット読み込み
		fread(buf, 1, sizeof(buf), fp_in);

		// 現在何％の位置にいるかの数字を更新する
		sprintf(current_per, "%d", (p_cnt*100/packet_count) );

		// MD5・SHA計算
		md5_r = MD5_Update(&md5_ctx, buf, 188);
		sha_r = SHA1_Update(&sha_ctx, buf, 188);
		sha256_r = SHA256_Update(&sha256_ctx, buf, 188);
		sha512_r = SHA512_Update(&sha512_ctx, buf, 188);

		// TSじゃない認定された場合ここで中断する
		if( this_is_not_TS == 1 ){
			continue;
		}

		// ヘッダは毎回初期化をする
		TS_HEAD head;
		st_head( &head, buf );

		// sync_byteが不正ならスキップ
		if( head.sync_byte != 0x47 ){
			sync_error++;
			if( sync_err_repeat > sync_err_repeat_limit){
				this_is_not_TS = 1;
			}
			sync_err_repeat++;
			continue;
		}else{
			// SyncErrがなかったらrepeatを0にリセット
			sync_err_repeat = 0;
		}

		// Totalの更新
		ts_drop_count[head.pid].total++;

		// ドロップをチェックする
		if( drop_check_and_count( ts_drop_count , buf , p_cnt )==0 ){
			// ドロップ時の動作
#if DEBUG
			printf("DROP: %s\n", current_tot);
#endif
			add_drop_list( &drop_list_tot, current_tot, &ts_drop_count[head.pid] );
			add_drop_list( &drop_list_per, current_per, &ts_drop_count[head.pid] );
		}

		// エラーパケットとペイロード無しパケットのスキップ
		if( head.transport_error_indicator == 0x1 || head.adaptation_field_control == 0x2 ){ continue; }

		// 興味があるPIDの読み込みが完了したらこの先に進める
		// それまではここから先へは進まない
		if( ! (packet_list[head.pid].enable && st_packet( packet_list, buf )) ){ continue; }

		if( head.pid == 0x00  ){
			// PATは最初のみチェック(とりあえず)
			st_pat(&pat, packet_list[head.pid].payload);
			packet_list[head.pid].enable = 0;

		}else if( head.pid == 0x14 ){
			// TOTを見つけたらcurrentTOTを更新する
			TS_TOT tot;
			st_tot(&tot, packet_list[head.pid].payload);

			// TOTからcurrent_totを更新
			tmp_tm = tm_to_text(&tot.tm);
			strcpy( current_tot, tmp_tm);
			free(tmp_tm);

			// first_totが空だったら代入する・
			// 日付に0日はないので、それで判定する
			if( first_tot.tm_mday == 0 ){ first_tot = tot.tm; }
			// 終了日時は常に書く事で実現する
			last_tot = tot.tm;

#if DEBUG
			printf("TOT: %s\n", current_tot);
#endif
			free_tot(&tot);

		}

	}

	// drop_countの最終PIDをチェックする
	mark_last_drop_count(ts_drop_count);

	// 残パケットがある場合、これを含めてハッシュ計算する
	if( last_bytes > 0 ){
		fread(buf, 1, last_bytes, fp_in);
		md5_r = MD5_Update( &md5_ctx, buf, last_bytes);
                sha_r = SHA1_Update(&sha_ctx, buf, last_bytes);
                sha256_r = SHA256_Update(&sha256_ctx, buf, last_bytes);
                sha512_r = SHA512_Update(&sha512_ctx, buf, last_bytes);
	}

	// MD5・SHAの計算を閉じる
	md5_r = MD5_Final (md    , &md5_ctx);
	sha_r = SHA1_Final(sha_md, &sha_ctx);
	sha256_r = SHA256_Final(sha256_md, &sha256_ctx);
	sha512_r = SHA512_Final(sha512_md, &sha512_ctx);
	
	// MD5の文字列を取り出す
	for(i = 0; i < MD5_DIGEST_LENGTH; i++)
		sprintf(&mdString[i * 2], "%02x", (unsigned int)md[i]);

	// SHAの文字列を取り出す
	for(i = 0; i < SHA_DIGEST_LENGTH; i++)
		sprintf(&sha_mdString[i * 2], "%02x", (unsigned int)sha_md[i]);

	// SHAの文字列を取り出す
	for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
		sprintf(&sha256_mdString[i * 2], "%02x", (unsigned int)sha256_md[i]);

	// SHAの文字列を取り出す
	for(i = 0; i < SHA512_DIGEST_LENGTH; i++)
		sprintf(&sha512_mdString[i * 2], "%02x", (unsigned int)sha512_md[i]);

	// パケットリストの掃除
	free_packet_list_all(packet_list);
	fclose(fp_in);

	// ==================================== ここから表示部分

	fprintf(fp_out, "{\n");

	fprintf(fp_out,"  \"tsanalytics\":{\n");
	fprintf(fp_out,"     \"version\":\"%s\"\n",VERSION);
	fprintf(fp_out,"  },\n");

	// =================================== FileInfo
	fprintf(fp_out,"  \"FileInfo\":{\n");
	fprintf(fp_out,"     \"size\":\"%ld\",\n",filesize);
	fprintf(fp_out,"     \"md5\":\"%s\",\n",mdString);
	fprintf(fp_out,"     \"sha1\":\"%s\",\n",sha_mdString);
	fprintf(fp_out,"     \"sha256\":\"%s\",\n",sha256_mdString);
	fprintf(fp_out,"     \"sha512\":\"%s\"\n",sha512_mdString);
	fprintf(fp_out,"  }");

	// TSじゃない認定された場合TSInfoの表示を見送る
	if( this_is_not_TS == 0 ){

		fprintf(fp_out,",\n");

		// =================================== TSInfo
		fprintf(fp_out,"  \"TSInfo\":{\n");
	
		fprintf(fp_out,"      \"sync_error\":\"%d\",\n",sync_error);
		fprintf(fp_out,"      \"packet_total\":\"%d\",\n",packet_count);

		// TOTの開始・終了時間情報
		if( first_tot.tm_mday == 0 ){ tmp_tm = str_unknown;      }
					else { tmp_tm = tm_to_text(&first_tot); }
					
		fprintf(fp_out,"      \"first_tot\":\"%s\",\n",tmp_tm);
	
		if( last_tot.tm_mday == 0   ){ tmp_tm = str_unknown;       }
					else { tmp_tm = tm_to_text(&last_tot);   }
	
		fprintf(fp_out,"      \"last_tot\":\"%s\",\n",tmp_tm);

		fprintf(fp_out,"      \"transport_stream_id\":\"0x%X\",\n",pat.transport_stream_id);

		fprintf(fp_out,"      \"programs\":[");
		TS_PAT_PIDS *pat_p = pat.pids;
		for(i=0; i<pat.num_of_pids;i++){
			if( pat_p->program_number != 0x0){
				fprintf(fp_out,"\"0x%04X\"", pat_p->program_number);
				if(i!=(pat.num_of_pids-1)){ fprintf(fp_out,","); }
			}
			pat_p=pat_p->next;
		}
		fprintf(fp_out,"],\n");
	
		// ========================== drop_pid
		
		conv_json_drop_pid(fp_out, ts_drop_count, 6);
		fprintf(fp_out, ",\n");
	
		// ========================== drop_percent
		conv_json_drop_key(fp_out, drop_list_per, "percent" , 6);
		fprintf(fp_out, ",\n");
	
		// ========================== drop_tot
		conv_json_drop_key(fp_out, drop_list_tot, "tot" , 6);
		fprintf(fp_out, "\n");
	
		// === 終わり
		fprintf(fp_out, "   }");

	}
	fprintf(fp_out, "\n}\n");

	//dump_drop_list( drop_list_tot );
	//dump_drop_list( drop_list_per );

	free_drop_list( drop_list_tot );
	free_drop_list( drop_list_per );

	fclose(fp_out);

	//dump_drop_count( ts_drop_count );


}
