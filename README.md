# 概要
録画したTSファイルが正しく録画されているかをチェックするための参考になりそうな情報をJSON形式で出力するツールです。せっかくファイル全体を舐めるのでついでにMD5とSHA-1のハッシュも取得します。

# ライセンス
Apacheライセンスを採用しています。

# コンパイル方法
OpenSSLの開発ツールが必要です。
```
$ make
```
コンパイルしたら./tsanalyticsを/usr/local/binなどPATHが通っている所にコピーします。

# 使い方
下記のような感じで使います。-oは前でも後ろでも大丈夫です。これを省略した場合標準出力に出力されます。
./tsanalytics 入力ファイル名 -o 出力ファイル名
./tsanalytics -o 出力ファイル名 入力ファイル名
./tsanakytics 入力ファイル名

# 対応入力ファイル
地デジ、BS/110度CSなどでお馴染みの188バイトなMPEG2-TSに対応しています。

# 出力の見方

## tsanalytics
ツールそのものに関する情報を出力します。現時点ではバージョンを表示するのみです。

## FileInfo
TSに限らないファイル一般的な情報を出力します。

### size
ファイルサイズをバイトで出力します

### md5
ファイルのMD5ハッシュ値

### sha1
ファイルのSHA-1ハッシュ値

### sha256
ファイルのSHA-256ハッシュ値

### sha512
ファイルのSHA-521ハッシュ値

## TSInfo
TSパケットから読み取った情報を主に表示します。
なお、先頭がsync_byte(0x47)となっていいないパケットが大量に続く場合このファイルはTSではないとみなし、TSInfoそのものが表示されなくなります。(FileInfoはTSファイル以外を指定しても表示されます)

### sync_error
TSパケットの先頭は0x47となっているのですが、先頭が0x47となっていない異常なパケットがあった場合、ここにカウントされます。

### packet_total
TSパケットの数を表示しています。TSパケットは188バイトの固定長なので、ここの数字を188倍するとファイルサイズになります。

### first_tot
TSファイルにTOT(日付時刻情報)が含まれている場合、一番最初のTOTのタイムスタンプを表示します。録画が開始された時刻が何となくわかりますが、放送局が出しているTOT情報はおおむね5秒おき程度なので、実際のファイルの先頭はこれより早い可能性があります。ちなみにTSファイル内にTOTが存在しない場合、unknownと表示されます。

### last_tot
first_totとは逆に一番最後のTOT情報です。録画が終了した時刻が何となくわかります。

### transport_stream_id
物理チャンネルごとに1つのIDが割り当てられていて、その数字が入ります。例えばEテレはEテレ１、２、３、携帯が全て同じIDです。BSで言えばBS日テレ・BSフジなどのように同一物理CHで送信されるチャンネルは同じIDになっています。ちなみにBSは4から始まり、CS1・CS2は6,7から始まるのでTSが地デジ/BS/CS1/CS2のどれなのかを判別する時に結構役立ちます。

### programs
TSファイルから検出されたプログラムを一覧しています。現段階では先頭のみしかチェックしておらず、今後表示の仕様が変わる可能性があります。

### drop_pid
```
"drop_pid":{
   "0x0110" : {"total":"156", "drop":"70", "error":"78", "scrambling":"74"}
}
```

上の例ではPID=0x0110のTSパケットが合計で156個あり、そのうちdropが156件、errorが78件、スクランブル未解除が74件発見された事を意味します。ちなみにdrop,error,scramblingの数字は両方にかかっている事もあるので、drop,erooの両方でカウントされるものも含みます。

### drop_percent
PIDごとのドロップ状況では、ファイルの先頭でドロップしているのか、最後なのか、それとも全体的なのかがわかりません。そこでファイルのおおむね何％ぐらいの位置でドロップしているかをここでは表示しています。

```
"drop_percent":{
   "55" : {"drop":"1071", "error":"2146", "scrambling":"1955"},
}
```

上の例ではファイル先頭から55%以上～56%未満でドロップが発生しています。55%未満でのドロップはありません。このようにドロップの位置がファイルの先頭付近なのか、真ん中なのか、最後なのかが何となくわかります。


### drop_tot
TSファイルのTOT情報が含まれている場合、どの時刻でドロップが発生したのかを特定できます。
```
"drop_tot":{
   "2020-01-26 15:36:09" : {"drop":"822", "error":"1676", "scrambling":"1529"}
}
```
上の例では、2020-01-26 15:36:09の後にドロップが発生しています。逆にこの時刻よりも前はドロップはない事を意味しています。TOTはおおむね5秒おきなので、15:36:09～15:36:14の間でドロップが発生しています。(放送局によっても変わります)

```
"drop_tot":{
   "unknown" : {"drop":"5", "error":"1", "scrambling":"0"}
   "2020-01-26 15:36:09" : {"drop":"822", "error":"1676", "scrambling":"1529"}
}
```
上の例ではファイルの先頭から一番最初のTOTである2020-01-26 15:36:09までの間にdropがあった事を示します。TOT不確定の所はunknownとしてまとめられます。

また、TOT情報が含まれていないTSの場合、全てがunknownにまとめられて表示さ:れます。


### 動作サンプル

```
{
  "tsanalytics":{
     "version":"0.03"
  },
  "FileInfo":{
     "size":"105614640",
     "md5":"458ff1439ebe6b1f7ade465e7b3a3737",
     "sha1":"b1185df4e332e9d6afa11878c4c204f9407bd66a"
     "sha256":"2dacb1fbd7b468e32514ef2200a2e1547a813cbb25420ab50fb6f4142eed4ea0"
     "sha512":"ad9a733c57f91b418bce34ee263b9d7521af2549b41b01fd1685867962ac0a40d6b79cc6173225dbe728ee91eb986e9b75effee66d92aef16dc00373e8207dde"
  },
  "TSInfo":{
      "sync_error":"0",
      "packet_total":"561780",
      "first_tot":"2020-01-26 15:35:49",
      "last_tot":"2020-01-26 15:36:39",
      "transport_stream_id":"0x7FE1",
      "programs":["0x0408","0x0409","0x040A","0x0588"],
      "drop_pid":{
         "0x0000" : {"total":"1844", "drop":"382", "error":"403", "scrambling":"342"},
         "0x0001" : {"total":"40", "drop":"15", "error":"15", "scrambling":"13"},
         "0x0010" : {"total":"156", "drop":"26", "error":"24", "scrambling":"22"},
         "0x0011" : {"total":"76", "drop":"12", "error":"10", "scrambling":"9"},
         "0x0012" : {"total":"7680", "drop":"58", "error":"72", "scrambling":"27"},
         "0x0013" : {"total":"30", "drop":"13", "error":"15", "scrambling":"12"},
         "0x0014" : {"total":"40", "drop":"8", "error":"10", "scrambling":"10"},
         "0x0023" : {"total":"114", "drop":"6", "error":"6", "scrambling":"3"},
         "0x0024" : {"total":"128", "drop":"12", "error":"10", "scrambling":"5"},
         "0x0028" : {"total":"16", "drop":"3", "error":"3", "scrambling":"3"},
         "0x0029" : {"total":"24", "drop":"9", "error":"12", "scrambling":"11"},
         "0x0100" : {"total":"953414", "drop":"4231", "error":"8925", "scrambling":"8165"},
         "0x0110" : {"total":"20306", "drop":"192", "error":"288", "scrambling":"256"},
         "0x0130" : {"total":"138", "drop":"10", "error":"9", "scrambling":"8"},
         "0x0138" : {"total":"228", "drop":"58", "error":"63", "scrambling":"46"},
         "0x0140" : {"total":"40692", "drop":"217", "error":"397", "scrambling":"353"},
         "0x0160" : {"total":"20522", "drop":"144", "error":"234", "scrambling":"216"},
         "0x0161" : {"total":"2910", "drop":"65", "error":"68", "scrambling":"61"},
         "0x0162" : {"total":"16738", "drop":"130", "error":"200", "scrambling":"183"},
         "0x0163" : {"total":"52830", "drop":"301", "error":"530", "scrambling":"480"},
         "0x0164" : {"total":"262", "drop":"27", "error":"27", "scrambling":"24"},
         "0x0165" : {"total":"262", "drop":"27", "error":"28", "scrambling":"18"},
         "0x01F0" : {"total":"2214", "drop":"58", "error":"66", "scrambling":"43"},
         "0x01FF" : {"total":"1814", "drop":"29", "error":"35", "scrambling":"12"},
         "0x0901" : {"total":"1082", "drop":"17", "error":"22", "scrambling":"13"}
      },
      "drop_percent":{
         "55" : {"drop":"1071", "error":"2146", "scrambling":"1955"},
         "56" : {"drop":"3", "error":"0", "scrambling":"0"},
         "57" : {"drop":"10", "error":"62", "scrambling":"60"},
         "58" : {"drop":"1819", "error":"3301", "scrambling":"2965"},
         "59" : {"drop":"707", "error":"1166", "scrambling":"1004"},
         "60" : {"drop":"445", "error":"689", "scrambling":"595"},
         "61" : {"drop":"743", "error":"1432", "scrambling":"1286"},
         "62" : {"drop":"1223", "error":"2348", "scrambling":"2146"},
         "63" : {"drop":"18", "error":"200", "scrambling":"197"},
         "64" : {"drop":"2", "error":"0", "scrambling":"0"},
         "65" : {"drop":"9", "error":"114", "scrambling":"113"},
         "66" : {"drop":"0", "error":"14", "scrambling":"14"}
      },
      "drop_tot":{
         "2020-01-26 15:36:09" : {"drop":"822", "error":"1676", "scrambling":"1529"},
         "2020-01-26 15:36:14" : {"drop":"5217", "error":"9668", "scrambling":"8679"},
         "2020-01-26 15:36:24" : {"drop":"11", "error":"128", "scrambling":"127"}
      }
   }
}
```

# 謝辞
このツール制作にあたってはtsselectやepgdumpなどを参考にしている部分があります。これらの作者様には感謝です。 

# 変更履歴

## 2020-03-01 version 0.05
sync_errorの連続エラーのしきい値を大幅に減らして100にしました。
TSじゃないファイルを指定した時になるべく早く諦めるようになります。
また、TS以外のファイルを指定した時packet.cのmemcpyで異常なsizeが指定されてセグるのを抑止するため
memcpyでコピーする際のsizeは上限188としました。

## 2020-03-01 version 0.04
TOTごとのドロップ表示が1000件を超えるとJSONの書式が崩れる(最後の要素に,が残る)バグを修正
なお、TOTごとのドロップ表示は20000件までしか対応しておらず、それ以降は表示を諦めます。
(この件数は1日以上連続で録画をして、かつdrop/error/scrambleが常時出ない限り該当する事はないですが…。)

## 2020-02-26 version 0.03
transport_stream_idを加えました。
0.02でJSONの書式が崩れるという盛大なバグがあり修正。

## 2020-02-26 version 0.02
TSじゃないファイルを指定してもハッシュ計算ぐらいはできるようにした。
ハッシュの種類をMD5/SHA1の他にSHA256/512を追加。
sync_byte(0x47)が先頭になっていないパケットが一定数続くとTSじゃないとみなしてTSInfoの表示を見送る動きを追加

## 2020-01-28 version 0.01
初版。