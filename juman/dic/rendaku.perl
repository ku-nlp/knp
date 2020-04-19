#!/usr/bin/env perl

# 連濁により濁音化した語のエントリを作成するプログラム
# すでに濁音化した語が辞書に登録されているかをチェックするため
# 辞書を逆順にソートしたものを入力とする
# Usage: tac ContentW.dic | perl rendaku.perl | tac > Rendaku.dic

# by Ryohei Sasano <sasano@pi.titech.ac.jp>

use strict;
use utf8;
binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";

my %daku2sei = ("が" => "か", "ガ" => "カ", "ぎ" => "き", "ギ" => "キ", "ぐ" => "く",
	     "グ" => "ク", "げ" => "け", "ゲ" => "ケ", "ご" => "こ", "ゴ" => "コ",
	     "ざ" => "さ", "ザ" => "サ", "じ" => "し", "ジ" => "シ", "ず" => "す",
	     "ズ" => "ス", "ぜ" => "せ", "ゼ" => "セ", "ぞ" => "そ", "ゾ" => "ソ",
	     "だ" => "た", "ダ" => "タ", "ぢ" => "ち", "ヂ" => "チ", "づ" => "つ",
	     "ヅ" => "ツ", "で" => "て", "デ" => "テ", "ど" => "と", "ド" => "ト",
	     "ば" => "は", "バ" => "ハ", "び" => "ひ", "ビ" => "ヒ", "ぶ" => "ふ",
	     "ブ" => "フ", "べ" => "へ", "ベ" => "ヘ", "ぼ" => "ほ", "ボ" => "ホ");
my %sei2daku = reverse(%daku2sei);
my %check;

while (<STDIN>) {
    chomp;
    # (動詞 ((読み かびる)(見出し語 黴びる かびる)(活用型 母音動詞)(意味情報 "代表表記:黴びる/かびる")))
    # (名詞 (普通名詞 ((読み かびん)(見出し語 花瓶 (花びん 1.6) (かびん 1.6))(意味情報 "代表表記:花瓶/かびん カテゴリ:人工物-その他 ドメイン:文化・芸術;家庭・暮らし"))))
    # (形容詞 ((読み かびんだ)(見出し語 過敏だ (過びんだ 1.6) (かびんだ 1.6))(活用型 ナ形容詞)(意味情報 "代表表記:過敏だ/かびんだ")))
    if (my ($prefix, $words, $suffix) = (/(\(\S+ .*\(見出し語 )(.*?)(\)\(.*)/)) {
	my ($yomi) = ($prefix =~ /\(読み (.*?)\)/);
    my ($pos) = ($prefix =~ /\(([^ ]*) /);
	$check{$yomi."/".$pos} = 1;
	my $tmp_yomi = $yomi;
	$tmp_yomi =~ s/だ$// if ($prefix =~ /\(形容詞/);
	next if ($suffix !~ /代表表記/ || $suffix =~ /代表表記:\p{InKatakana}/ || length($yomi) == 1);
	next if ($tmp_yomi =~ /^(.)/ && !$sei2daku{$1});
	next if ($suffix !~ /濁音可/ && grep($daku2sei{$_}, split(//, $tmp_yomi)));
	next if ($suffix !~ /濁音可/ && $prefix !~ /^\([動名形]/);
	next if ($suffix =~ /代表表記:来る\/くる/ || $suffix =~ /代表表記:する\/する/);
	my ($first, $rest) = ($yomi =~ /^(.)(.*)/);
    # 濁音化した読みが check に登録されていたら生成しない
	next if ($check{$sei2daku{$first} . $rest."/".$pos});


	$words =~ s/ (\d)/-$1/g;
	my @words = split(' ', $words);
	my @voiced;
	for my $word (@words) {
	    ($word =~ /^\(/) ? ($word =~ s/^\((.*)\)$/$1/) : ($word .= "-1.0");
	    my ($first, $rest, $cost) = ($word =~ /^(.)([^\-]+)-([\d\.]+)$/);

	    if ($sei2daku{$first}) {
		#define VERB_VOICED_COST       7  /* "が"から始まる動詞を除く動詞の連濁化のコスト */
		#define VERB_GA_VOICED_COST    9  /* "が"から始まる動詞の連濁化のコスト */
		#define NOUN_VOICED_COST       8  /* "が"から始まる名詞を除く名詞の連濁化のコスト */
		#define NOUN_GA_VOICED_COST   11  /* "が"から始まる名詞の連濁化のコスト */
		#define ADJECTIVE_VOICED_COST  9  /* 形容詞の連濁化のコスト */
		#define OTHER_VOICED_COST      5  /* 上記以外の連濁化のコスト */		
		$cost += 
		    ($prefix =~ /\(動詞/ && $yomi !~ /^か/) ? 0.7 :
		    ($prefix =~ /\(動詞/ && $yomi =~ /^か/) ? 0.9 :
		    ($prefix =~ /\(名詞/ && $yomi !~ /^か/) ? 0.8 :
		    ($prefix =~ /\(名詞/ && $yomi =~ /^か/) ? 1.1 :
		    ($prefix =~ /\(形容詞/) ? 0.9 : 0.5;
		push(@voiced, "($sei2daku{$first}$rest $cost)");
	    }
	}

	if (@voiced) {
	    $prefix =~ s/(\(\(読み )(.)/$1$sei2daku{$2}/;
	    $suffix =~ s/\"\)\)\)/ 濁音化D\")))/;
	    print "$prefix" . join(' ', @voiced) . "$suffix\n";
	}
    }
}
