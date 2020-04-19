# 日本語構文・格・照応解析システム KNP

KNPは日本語文の構文・格解析を行うシステムです。形態素解析システムJUMANの解析結果(形態素列)を入力とし、文節および基本句間の係り受け関係、格関係を出力します。係り受け関係、格関係は、Webから自動構築した大規模格フレームに基づく確率的構文・格解析により決定します。

以下では、KNPのインストール方法について説明します。KNPの使い方などは doc/manual.pdf をご覧ください。


## インストール方法

以下のものが必要ですので、あらかじめインストールしておいてください。

- [zlibライブラリ](http://zlib.net/) (※ 多くのOSに標準でインストールされています)
- gitからビルドする場合: libtool, automake, autoconf

次の手順でKNPをインストールしてください。

1. (gitからビルドする場合) `./autogen.sh`を実行してください。
1. 次のコマンドを実行して、KNP辞書をダウンロード、展開、配置してください。
```bash
$ wget http://lotus.kuee.kyoto-u.ac.jp/nl-resource/knp/dict/latest/knp-dict-latest-bin.zip # ビルド済み辞書(2.6GB)
$ unzip knp-dict-latest-bin.zip
$ cp -ars `pwd`/dict-bin/* ./dict
```
    - macOSなどで"cp: illegal option -- s"というエラーが出たら、`cp -ars`の代わりに`mv`を使うなどしてください。
1. `./configure`を実行してください。
1. `make`を実行してください。
1. `sudo make install`を実行してください。


## Pythonからの利用

[pyknp](https://github.com/ku-nlp/pyknp)を使ってください。


## Perlからの利用

次の手順を順番に実行し、KNPのPerlモジュールをインストールしてください。

```bash
$ cd perl
$ perl Makefile.PL
$ make
$ sudo make install
```
