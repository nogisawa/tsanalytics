/* ほぼ呼び出ししかしない部分 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

void show_usage()
{
	fprintf(stderr, "tsanalytics ver.%s\n", VERSION);
	fprintf(stderr, "Usage: ./tsanalytics XXXX.ts -o XXXX.json\n");
}

// パラメーターチェック。
// 引数にオプションが設定されているかどうかに加えて
// 次の引数が存在するかどうかも含めてチェックする
int check_param( char *str, char *option, int i, int argc ){
	if( strcmp(str, option)==0 ){
		if( i < argc-1 ){
			return 1;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{

	unsigned char *from_path = NULL;
	unsigned char *out_path = NULL;

	int i;
	uint16_t service_id = 0x0; // 初期値はゼロ。異常時も0。
	char *argstr = NULL;
	// 引数の処理
	// ここでは下記を取得する
	// FROM, TO, SID
	for(i=1;i<argc;i++){

		// -sが来たらSIDとして処理
		if( check_param( argv[i], "-s" , i, argc) ){
			// 16進数かどうかで処理を分ける
			if( strncmp(argv[i+1], "0x", 2)==0 ){
				sscanf(argv[i+1],"0x%x", &service_id);
			}else{
				sscanf(argv[i+1],"%d", &service_id);
			}
			i++; // 1文字先を読んだのでインクリメントを忘れずに

		// -oが来たら出力先として処理
		// (使わないけどとりあえず)
		}else if( check_param( argv[i], "-o" , i, argc) ){
			out_path = argv[i+1];
			i++;

		// いずれにも該当しない場合はfromとみなす
		}else{
			from_path = argv[i];
		}

	}

	
	// =================異常な引数チェック
	// fromがNULLだったら引数不足
	if( from_path == NULL ){
		show_usage();
		exit(1);
	}

	ARGS_TSANALYTICS params;
	params.service_id = service_id;
	params.from_path  = from_path;
	params.out_path   = out_path;

	tsanalytics(&params);

	return 0;
}
