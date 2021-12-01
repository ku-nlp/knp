# 日本語構文・格・照応解析システム KNP

KNPは日本語文の構文・格・照応解析を行うシステムです。形態素解析システムJUMANもしくはJuman++の解析結果(形態素列)を入力とし、文節および基本句間の係り受け関係、格関係、照応関係を出力します。これらの関係の同定には、Webから自動構築した大規模格フレームを用いています。

以下では、KNPのインストール方法について説明します。KNPの使い方などは doc/manual.pdf をご覧ください。


## インストール方法

以下のものが必要ですので、あらかじめインストールしておいてください。

- [zlibライブラリ](http://zlib.net/) (※ 多くのOSに標準でインストールされています)
- gitからビルドする場合: [libtool](https://www.gnu.org/software/libtool/), [automake](https://www.gnu.org/software/automake/), [autoconf](https://www.gnu.org/software/autoconf/)
  - Mac の場合、`libtoolize` が `glibtoolize` の形でインストールされていることがあります。その場合、 `ln -s /usr/local/bin/glibtoolize /usr/local/bin/libtoolize` などを実行し、`libtoolize` にパスを通しておいてください。

次の手順でKNPをビルドし、インストールしてください。

1. (gitからビルドする場合) `./autogen.sh`を実行してください。
2. 次のコマンドを実行して、KNP辞書をダウンロード、展開、配置してください。
    ```bash
    $ wget http://lotus.kuee.kyoto-u.ac.jp/nl-resource/knp/dict/latest/knp-dict-latest-bin.zip # ビルド済み辞書(2.6GB)
    $ unzip knp-dict-latest-bin.zip
    $ cp -ars `pwd`/dict-bin/* ./dict
    ```
3. `./configure`を実行してください。
4. `make`を実行してください。
5. `sudo make install`を実行してください。

`cp -ars`実行時にmacOSなどで"cp: illegal option -- s"というエラーが出たら、`cp -ars`の代わりに`mv`を使うなどしてください。


## Docker経由での利用
KNPのインストールに失敗する場合、Dockerを利用してコンテナ内のビルド済みKNPを使用することができます。
Dockerがインストールされた環境で以下のようにエイリアスを設定してください。

```
alias knp='docker run -i --rm --platform linux/amd64 kunlp/jumanpp-knp knp'
```

## Pythonからの利用

[pyknp](https://github.com/ku-nlp/pyknp)を使ってください。


## Perlからの利用

次の手順を実行し、KNPのPerlモジュールをインストールしてください。

```bash
$ cd perl
$ perl Makefile.PL
$ make
$ sudo make install
```
