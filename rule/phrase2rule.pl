#!/usr/bin/env perl

use Getopt::Long;
use encoding 'euc-jp';
binmode STDERR, ':encoding(euc-jp)';

######################################################################
#
#		 KNPの形態素，文節ルールのtranslator
#
#					99/09/10 by kuro@i.kyoto-u.ac.jp
######################################################################
#
# 各行のnotation
# ==============
#	[前の文脈]自分自身[後の文脈]\t+FEATURE列
#		※ FEATURE列の前は必ずTABで区切る
#		※ 前後の文脈，自分自身の中は文節ごとに空白で区切る
#						(文節ルールの場合)
#		※ FEATURE列はFEATUREごとに空白で区切る
#
#		※ 文節ルールの場合は前の文脈の前と後の文脈の後に
#		   任意の文節列を許す(?*が自動挿入される)
#		※ 形態素ルールの場合は前の文脈の前と後の文脈の後に
#		   任意の形態素列を許す(?*が自動挿入される)
#
# 形態素，文節のnotation
# ======================
#	^....		文節先頭からマッチ (文節ルールの場合)
#	<<...>>		文節のfeature
#	<...>		直前の形態素のfeature (〜<...>とすればfeatureだけの指定)
#
#	(...)		0回以上の出現 (形態素レベルと文節レベルそれぞれ)
#	{WORD1|WORD2|..}WORD1 or WORD2 or ...
#			  (扱えるのは品詞が同一の場合だけ)
#
#	{\品詞}		その品詞
#	{\品詞:細分類}	その細分類
#
#	‥		任意の形態素列 (※ ……は記号一語で使っている)
#	〜		任意の形態素 (形態素解析のために挿入，ルールでは削除)
#
#	WORD_活用語G	任意の活用語 (任意の活用形)
#	WORD_非活用語G	任意の非活用語
#
#	WORD_活用語g	品詞,細分類:*，活用型:以下の一般化，活用形:そのまま，語:*
#			  動詞タイプ 例) 書くg
#                         --------------------
#				母音動詞 子音動詞＊行 カ変動詞 サ変動詞 ザ変動詞
#				動詞性接尾辞ます型 動詞性接尾辞うる型 動詞性接尾辞得る型
#				無活用型
#			  イ形容詞タイプ 例) 美しいg
#                         -------------------------- 
#			 	イ形容詞＊段
#				助動詞ぬ型 助動詞く型
#			  ナ形容詞タイプ 例) 静かだg
#                         --------------------------
#				ナ形容詞 ナ形容詞特殊 ナノ形容詞 判定詞
#				助動詞ぬ型 助動詞だろう型 助動詞そうだ型
#				(「特別の」にすればダ列特殊連体となる)
#				(※ 「助動詞ぬ型」はイ形容詞とナ形容詞両方)
#			  タル形容詞タイプ 例) 堂々たるg
#                         ------------------------------
#				タル形容詞
#
#	WORD_非活用語g	名詞，名詞性名詞接尾辞，名詞性述語接尾辞
#
#	WORD12345	品詞，細分類，活用型，活用形，語について数字指定の部分だけのこす
#			  例) する1235 --> 活用形だけ何でもよい
#
#	※例外事項※	・"#define WORD 般化予約語"とした場合はWORDgと解釈
#			・句読点，括弧はgなしでも語の一般化を行う
#			・名詞，副詞，助詞は常に細分類を一般化

# 活用型の対応付け
# -----------------------------------------
# 無活用型		動詞
# 助動詞ぬ型		イ形容詞 ナ形容詞
# 助動詞く型		イ形容詞
# 動詞性接尾辞ます型	動詞
# 動詞性接尾辞うる型	動詞
# 動詞性接尾辞得る型	動詞

$conj_type[0] = "母音動詞 子音動詞カ行 子音動詞カ行促音便形 子音動詞ガ行 子音動詞サ行 子音動詞タ行 子音動詞ナ行 子音動詞バ行 子音動詞マ行 子音動詞ラ行 子音動詞ラ行イ形 子音動詞ワ行 子音動詞ワ行文語音便形 カ変動詞 カ変動詞来 サ変動詞 ザ変動詞 動詞性接尾辞ます型 動詞性接尾辞うる型 動詞性接尾辞得る型 無活用型";
$conj_type[1] = "イ形容詞アウオ段 イ形容詞イ段 イ形容詞イ段特殊 助動詞ぬ型 助動詞く型";
$conj_type[2] = "ナ形容詞 ナ形容詞特殊 ナノ形容詞 判定詞 助動詞ぬ型 助動詞だろう型 助動詞そうだ型";
$conj_type[3] = "タル形容詞";

# 本当は全体に書いておく方がよいが，とりあえず使われるものだけ

$pos_repr{"\\名詞:形式名詞"} = "こと";
$pos_repr{"\\名詞:副詞的名詞"} = "ため";
$pos_repr{"\\接尾辞"} = "個";
$pos_repr{"\\接尾辞:名詞性名詞助数辞"} = "個";
$pos_repr{"\\接尾辞:名詞性特殊接尾辞"} = "以内";
$pos_repr{"\\助詞"} = "だけ";
$pos_repr{"\\助詞:格助詞"} = "に";
$pos_repr{"\\助詞:副助詞"} = "だけ";
$pos_repr{"\\指示詞:名詞形態指示詞"} = "これ";
$pos_repr{"\\指示詞:副詞形態指示詞"} = "こう";
$pos_repr{"\\連体詞"} = "ほんの";
$pos_repr{"\\指示詞:連体詞形態指示詞"} = "この";
$pos_repr{"\\接続詞"} = "そして";
$pos_repr{"\\感動詞"} = "あっ";
$pos_repr{"\\特殊:句点"} = "．";

my (%opt);
&GetOptions(\%opt, 'rid');
# --rid: RIDを付与

######################################################################
use KNP;
$knp = new KNP(-Option => "-bnst -tab");
######################################################################
$bnstrule_flag = 1;
$num = 0;

while ( <STDIN> ) {
    
    chomp;
    $num ++;
    next if (/^[\s\t]*\;/ || length($_) == 0);

    if (/^\#define/) {
	@tmp = split;
	if ($tmp[1] eq "文節ルール") {
	    $bnstrule_flag = 1;
	} 
	elsif ($tmp[1] eq "形態素ルール") {
	    $bnstrule_flag = 0;
	} 
	elsif ($tmp[2] eq "般化予約語") {
	    $g_define_word{$tmp[1]} = 1;
	} 
	else {
	    die "Invalid define line ($_)!!\n" 
	}
	next;
    }


    # 表現とFEATURE列とコメントを分離
    # 	FEATURE列との間はtab，コメントは ; の後

    if (/^([^\t]+)[\s\t]+([^\;\t]+)[\s\t]*(\;.+)$/) {
	$pattern = $1; $feature = $2; $comment = $3;
    } elsif (/^([^\t]+)[\s\t]+([^\;\t]+)[\s\t]*$/) {
	$pattern = $1; $feature = $2; $comment = "";
    } else {
	print STDERR  "line $num is invalid; $_\n";
	next;
    }
    $pattern =~ s/^\s+//;	# 念のため
    $pattern =~ s/\s+$//;	# 念のため


    # 前後の文脈と自分自身を分離，前後の文脈は[...]

    $pattern =~ /^(\[[^\[\]]+\])?([^\[\]]+)(\[[^\[\]]+\])?$/;
    $tmp1 = $1; $tmp2 = $2; $tmp3 = $3;
    $tmp1 =~ s/^\[|\]$//g;
    @pres = split(/ /, $tmp1);
    $tmp2 =~ s/^\[|\]$//g;
    @self = split(/ /, $tmp2);
    $tmp3 =~ s/^\[|\]$//g;
    @poss = split(/ /, $tmp3);

    # print ">> pre  @pres\n self @self\n post @poss\n\n";

    # 前後の文脈の前後には空文字列を挿入(出力時は?*)

    if ($bnstrule_flag) {
	unshift(@pres, "") if ($pres[0] ne "‥");
	push(@poss, "") if ($poss[$#poss] ne "‥");
    } else {
	unshift(@pres, "") if (!@pres);
	push(@poss, "") if (!@poss);
    }	

    @all = (@pres, @self, @poss);

    # 代表句をつくる

    undef @repr_str;
    foreach (@all) {
	push(@repr_str, bnst_cond($_, 0));
    }

    # MAIN

    print "; $pattern\n";
    print "(\n(";
    # 前の文脈
    for ($i = 0; $i < @pres; $i++) {
	if ($bnstrule_flag) {
	    if ($i == 0) {
		print " ?*";
	    } else {
		bnst_cond($all[$i], 1, $repr_str[$i-1], $repr_str[$i+1]);
	    }
	}
	else {
	    print " ?*";
	    bnst_cond($all[$i], 1, "", $repr_str[$i+1]);
	}
    }	
    print " )\n(";
    # 自分
    for ($i = @pres; $i < @pres + @self; $i++) {
	bnst_cond($all[$i], 1, $repr_str[$i-1], $repr_str[$i+1]);
    } 
    print " )\n(";
    # 後の文脈
    for ($i = @pres + @self; $i < @all ; $i++) {
	if ($bnstrule_flag) {
	    if ($i == (@all - 1)) {
		print " ?*";
	    } else {
		bnst_cond($all[$i], 1, $repr_str[$i-1], $repr_str[$i+1]);
	    }
	}
	else {
	    bnst_cond($all[$i], 1, $repr_str[$i-1], "");
	    print " ?*";
	}
    }
    print " )\n\t$feature";
    print " RID:$num" if $opt{rid};
    print "\n)\n";
    print "$comment\n" if $comment;
    print "\n";
}

# $juman->close;
# $knp->close;

######################################################################
sub bnst_cond
{
    # 文節の条件を処理 (flag=1:通常の処理，flag=0:代表表現をかえす)

    my ($input, $flag, $l_context, $r_context) = @_;
    my ($ast_flag, $string, $feature);

    # (....) -> ....とast_flagに分離

    if ($bnstrule_flag && $input =~ /^\((.+)\)$/) {
	$input = $1;
	$ast_flag = 1;
    } else {
	$ast_flag = 0;
    }


    # 文節のFEATUREを分離

    if ($input =~ /^\<\<([^\<\>]+)\>\>$/) {
	$string = "‥"; $feature = $1;
    } elsif ($input =~ /^(.+)\<\<([^\<\>]+)\>\>$/) {
	$string = $1; $feature = $2;
    } else {
	$string = $input; $feature = "";
    }


    # 出力

    if ($flag) {
	if ($bnstrule_flag) {
	    print " < (";
	    bnst_cond2($string, 1, $l_context, $r_context);
	    printf " )%s >", feature2str($feature);
	    print "*" if $ast_flag;
	} else {
	    bnst_cond2($string, 1, $l_context, $r_context);
	}
    } else {
	return bnst_cond2($string, 0);
    }
}

######################################################################
sub feature2str
{
    my ($input) = @_;

    return "" unless ($input);

    $data = " (";
    foreach (split(/\|\|/, $input)) {
	s/\&\&/ /g;
	$data .= "($_)";
    }
    $data .= ")";
    return $data;
}

######################################################################
sub bnst_cond2
{
    # 文節の条件を処理 (flag=1:通常の処理，flag=0:代表表現をかえす)

    # (彼|彼女)に(だけ|すら|さえ)は
    #
    # part: [0][0]:彼   [1][0]:に [2][0]:だけ [3][0]:は
    #       [0][1]:彼女           [2][1]:すら
    #                             [2][1]:さえ
    # 
    # data[0][0]{phrase}: 彼にだけは
    # data[0][1]{phrase}: 彼女にだけは
    # data[2][1]{phrase}: 彼にすらは
    # data[2][2]{phrase}: 彼にさえは
    # 
    # ※ data[0][0]{phrase}はすべてのpartで0番目の語を集めたもの
    #    data[i][j]{phrase}はi番目のpartをj番目の語にしたもの

    my ($input, $flag, $l_context, $r_context) = @_;
    my (@data, @feature, @mrph);
    my ($i, $j, $k, $l, $error_flag, $any_prefix, $knp_input);

    # 先頭が ^ であれば先頭からの条件

    if ($input =~ s/^\^// || $bnstrule_flag == 0) {
	$any_prefix = 0;
    } else {
	$any_prefix = 1;
    }

    # 要素ごとに分割 --> @part
    # (括弧,‥の前後，>,G,g,数字の後に空白を入れる)

    $input =~ s/\(/ \(/g;
    $input =~ s/\)/\) /g;
    $input =~ s/\{/ \{/g;
    $input =~ s/\}/\} /g;
    $input =~ s/‥/ ‥ /g;
    $input =~ s/\>/\> /g;
    $input =~ s/G/G /g;
    $input =~ s/g/g /g;

    $input =~ s/(\d+)/\1 /g;
    $input =~ s/(\d) (\))/\1\2/g;
    $input =~ s/(\d) (\<)/\1\2/g;

    $input =~ s/^ +| +$//g;
    @part_str = split(/ +/, $input);
    $part_num = @part_str;

    # @part_strを整形し，@part(2次元配列)を作成

    for ($i = 0; $i < $part_num; $i++) {
	if ($part_str[$i] =~ /^\((.+)\)$/) {
	    $part_str[$i] =  $1;
	    $feature[$i]{ast} = 1;
	}
	elsif ($part_str[$i] =~ /^\{(.+)\}$/) {
	    $part_str[$i] =  $1;
	}

	# ORの場合は表層表現だけ
	if ($part_str[$i] !~ /^\<\</ && $part_str[$i] =~ /\|/) {
	    @{$part[$i]} = split(/\|/, $part_str[$i]);
	    $feature[$i]{or} = 1;
	}

	# 他は種々のメタ表現を考慮
	else {
	    @{$part[$i]} = ($part_str[$i]);

	    if ($part[$i][0] =~ /\<(.+)\>$/) {
		$feature[$i]{lastfeature} = feature2str($1);
		$part[$i][0] =~ s/\<.+\>$//;
	    }
	    if ($part[$i][0] eq "‥") {
		$feature[$i]{result} = " ?*";
		$part[$i][0] = "‥";
	    }
	    if ($part[$i][0] =~ /G$/) {
		$feature[$i]{lastGENERAL} = 1;
		$part[$i][0] =~ s/G$//;
	    }
	    if ($part[$i][0] =~ /g$/) {
		$feature[$i]{lastgeneral} = 1;
		$part[$i][0] =~ s/g$//;
	    }
	    if ($part[$i][0] =~ /([\d]+)$/) {
		$feature[$i]{lastnum} = $1;
		$part[$i][0] =~ s/[\d]+$//;
	    }
	    if ($part[$i][0] =~ /^\\/) {
		$feature[$i]{result} = " [$part[$i][0]]";
		$feature[$i]{result} =~ s/\\//;
		$feature[$i]{result} =~ s/\:/ /;
		$part[$i][0] = $pos_repr{$part[$i][0]};
	    }
	}
    }

    # $part[$i][0]を標準データとして$data[0][0]に

    for ($i = 0; $i < $part_num; $i++) {
	$data[0][0]{phrase} .= $part[$i][0];
	$data[0][0]{length}[$i] = length($part[$i][0]);
    }

    # 代表表現を返す場合

    return $data[0][0]{phrase} if ($flag == 0);

    # i番目のpartをj番目の語にしたものを$data[i][j]に

    for ($i = 0; $i < $part_num; $i++) {
	next unless ($feature[$i]{or});
	for ($j = 1; $j < @{$part[$i]}; $j++) {
	    for ($k = 0; $k < $part_num; $k++) {
		if ($k != $i) {
		    $data[$i][$j]{phrase} .= $part[$k][0];
		    $data[$i][$j]{length}[$k] = length($part[$k][0]);
		} else {
		    $data[$i][$j]{phrase} .= $part[$k][$j];
		    $data[$i][$j]{length}[$k] = length($part[$k][$j]);
		}
	    }
	}
    }

    # それぞれKNP(JUMAN)で処理

    for ($i = 0; $i < $part_num; $i++) {
	for ($j = 0; $j < @{$part[$i]}; $j++) {
	    next unless ($data[$i][$j]);

	    # 末尾が"，|．"なら"＝"，それ以外なら"，＝"を付与
	    # ※ 以下の問題を解決するため
	    #      1. 連体形の文節がそれだけでは正しくJUMANされない
	    #      2. 単に"＝"を付与すると"同じ"が語幹になる
	    #      2. "，，"はJUMANで未定義語
	    #      3. 末尾の"，"もJUMANで未定義語

	    $knp_input = $l_context . $data[$i][$j]{phrase} . $r_context;

	    if($bnstrule_flag){
		if ($knp_input =~ /(，|．|、|。)$/) {
		    $knp_input .= "＝";
		} else {
		    $knp_input .= "，＝";
		}
	    }

	    $knp->parse($knp_input); 
	    @knp_result = split(/\n/, $knp->{ALL});

	    # print "\n\n>>>>>$knp_input\n>>>@knp_result\n";

	    $k = 0;
	    foreach $item (@knp_result) { 
		next if ($item =~ /^EOS/);
		# next if ($item =~ /^\@/);
		next if ($item =~ /^(\*|\#|\;)/);
		($mrph[$i][$j][$k]{word}, 
		 $mrph[$i][$j][$k]{yomi}, 
		 $mrph[$i][$j][$k]{base}, 
		 $mrph[$i][$j][$k]{pos}, 
		 $mrph[$i][$j][$k]{d1}, 
		 $mrph[$i][$j][$k]{pos2}, 
		 $mrph[$i][$j][$k]{d2}, 
		 $mrph[$i][$j][$k]{conj}, 
		 $mrph[$i][$j][$k]{d3}, 
		 $mrph[$i][$j][$k]{conj2}, 
		 $mrph[$i][$j][$k]{d4}) = split(/ /, $item);
		$mrph[$i][$j][$k]{length} = length($mrph[$i][$j][$k]{word});
		$mrph[$i][$j][$k]{result} = 
		    " [$mrph[$i][$j][$k]{pos} $mrph[$i][$j][$k]{pos2} $mrph[$i][$j][$k]{conj} $mrph[$i][$j][$k]{conj2} $mrph[$i][$j][$k]{base}]";
		$k++;
	    }

	    # l_contextとr_contextが形態素として正しく区切れているかチェック
	    $begin_check = 0;
	    $end_check = 0;
	    $length = 0;
	    for ($k = 0; $mrph[$i][$j][$k]{word}; $k++) {
		if ($length == length($l_context)) {
		    $begin_check = 1;
		    $mrph_start_num = $k;
		} 
		$length += $mrph[$i][$j][$k]{length};
		if ($length == (length($l_context) + 
				   length($data[$i][$j]{phrase}))) {
		    $end_check = 1;
		}
	    }
	    if (!$begin_check || !$end_check) {
		print STDERR "CONTEXT ERROR ($pattern)\n";
		return;
	    }

	    # part と mrph の対応つけ 
	    # (場合によってORの文字数が違うので各$data[$i][$j]に必要)

	    $part_length = 0;
	    $mrph_length = 0;
	    $k = $mrph_start_num;
	    for ($l = 0; $l < $part_num; $l++) {
		$part_length += $data[$i][$j]{length}[$l];
		$data[$i][$j]{start}[$l] = $k;
		for (; $mrph_length < $part_length; $k++) {
		    $mrph_length += $mrph[$i][$j][$k]{length};
		}
		$data[$i][$j]{end}[$l] = $k - 1;
		# print "($data[$i][$j]{start}[$l] $data[$i][$j]{end}[$l])";
	    }
	}
    }

    # ORの部分のマージ

    $start_pos = 0;
    $error_flag = 0;
    for ($i = 0; $i < $part_num; $i++) {
	if ($feature[$i]{or}) {
	    for ($j = 1; $j < @{$part[$i]}; $j++) {

		# ORの前後の形態素数の一致， ORが一形態素
		if ($data[0][0]{start}[$i] != $data[$i][$j]{start}[$i] ||
		    $data[0][0]{end}[$i] != $data[$i][$j]{end}[$i] ||
		    $data[0][0]{start}[$i] != $data[0][0]{end}[$i] ||
		    $data[$i][$j]{start}[$i] != $data[$i][$j]{end}[$i] ||
		    @{$mrph[0][0]} != @{$mrph[$i][$j]}) {
		    $error_flag = 1;
		}
		# ORの前の形態素列の一致
		for ($k = 0; $k < $data[0][0]{start}[$i]; $k++) {
		    if ($mrph[0][0][$k]{result} ne $mrph[$i][$j][$k]{result}) {
			$error_flag = 1;
			last;
		    }
		}
		# ORの後の形態素列の一致
		for ($k = $data[0][0]{end}[$i]+1; $k < @{$mrph[0][0]}; $k++) {
		    if ($mrph[0][0][$k]{result} ne $mrph[$i][$j][$k]{result}) {
			$error_flag = 1;
			last;
		    }
		}

		# ORが同一品詞か
		if ($mrph[0][0][$data[0][0]{start}[$i]]{pos} ne $mrph[$i][$j][$data[$i][$j]{start}[$i]]{pos}) {
		    $error_flag = 1;
		    if ($mrph[0][0][$data[0][0]{start}[$i]]{conj} eq "*" && 
			$mrph[$i][$j][$data[$i][$j]{start}[$i]]{conj} ne "*") {

			# $data[0][0]を標準とするので，$data[0][0]が無活用，$data[i][j]が活用の場合
			# $data[i][j]のwordをbaseにコピーする

			$mrph[$i][$j][$data[$i][$j]{start}[$i]]{base} = $mrph[$i][$j][$data[$i][$j]{start}[$i]]{word}
		    }
		}

		$WORD = $mrph[0][0][$data[0][0]{start}[$i]]{base} if ($j == 1);
		$WORD .= " $mrph[$i][$j][$data[$i][$j]{start}[$i]]{base}";
	    }
	    # print "$WORD\n";
	    $mrph[0][0][$data[0][0]{start}[$i]]{base} = "($WORD)";
	}
	$start_pos += $data[0][0]{length}[$i];
    }

    # ORの条件を満たさなければエラー出力
    if ($error_flag) {
	for ($i = 0; $i < $part_num; $i++) {
	    for ($j = 0; $j < @{$part[$i]}; $j++) {
		next unless ($data[$i][$j]);
		print STDERR "ERROR($i,$j) ";
		for ($k = 0; $k < @{$mrph[$i][$j]}; $k++) {
		    print STDERR "$mrph[$i][$j][$k]{result}";
		}
		print STDERR "\n";
	    }
	}
	print STDERR "\n";
    }

    # 出力

    print " ?*" if ($any_prefix && $part[0][0] ne "‥");

    for ($i = 0; $i < $part_num; $i++) {
	if ($feature[$i]{result}) {
	    print $feature[$i]{result};
	    print "*" if ($feature[$i]{ast});
	}
	else {
	    for ($k = $data[0][0]{start}[$i]; $k <= $data[0][0]{end}[$i]; $k++) {
		if ($k == $data[0][0]{end}[$i] &&
		    $feature[$i]{lastGENERAL}) {
		    if ($mrph[0][0][$k]{conj} ne "*") {
			$mrph[0][0][$k]{result} = " [* * * * * ((活用語))]";
		    } else {
			$mrph[0][0][$k]{result} = " [* * * * * ((^活用語))]";
		    }
		}
		elsif (($k == $data[0][0]{end}[$i] &&
			$feature[$i]{lastgeneral}) ||
		       $g_define_word{$mrph[0][0][$k]{base}}) {
		    if ($mrph[0][0][$k]{conj} ne "*") {
			$conj_flag = 0;
			for ($m = 0; $m < @conj_type; $m++) {
			    if ($conj_type[$m] =~ /$mrph[0][0][$k]{conj}/) {
				$mrph[0][0][$k]{result} = " [* * ($conj_type[$m]) $mrph[0][0][$k]{conj2} *]";
				$conj_flag = 1;
				last;
			    }
			}
			if (!$conj_flag) {
			    print STDERR "Invalid conjugation type ($mrph[0][0][$k]{conj})!!\n";
			}
		    } else {
			$mrph[0][0][$k]{result} = " [* * * * * ((名詞相当語))]";
		    }
		}
		elsif ($k == $data[0][0]{end}[$i] &&
		       $feature[$i]{lastnum}) {
		    if ($feature[$i]{lastnum} =~ /1/) {
			$mrph[0][0][$k]{result} = " [$mrph[0][0][$k]{pos}";
		    } else {
			$mrph[0][0][$k]{result} = " [*";
		    }
		    if ($feature[$i]{lastnum} =~ /2/) {
			$mrph[0][0][$k]{result} .= " $mrph[0][0][$k]{pos2}";
		    } else {
			$mrph[0][0][$k]{result} .= " *";
		    }
		    if ($feature[$i]{lastnum} =~ /3/) {
			$mrph[0][0][$k]{result} .= " $mrph[0][0][$k]{conj}";
		    } else {
			$mrph[0][0][$k]{result} .= " *";
		    }
		    if ($feature[$i]{lastnum} =~ /4/) {
			$mrph[0][0][$k]{result} .= " $mrph[0][0][$k]{conj2}";
		    } else {
			$mrph[0][0][$k]{result} .= " *";
		    }
		    if ($feature[$i]{lastnum} =~ /5/) {
			$mrph[0][0][$k]{result} .= " $mrph[0][0][$k]{base}]";
		    } else {
			$mrph[0][0][$k]{result} .= " *]";
		    }
		}
		elsif ($mrph[0][0][$k]{base} eq "，") {
		    $mrph[0][0][$k]{result} = " [特殊 読点 * * *]";
		}
		elsif ($mrph[0][0][$k]{base} eq "．") {
		    $mrph[0][0][$k]{result} = " [特殊 句点 * * *]";
		}
		elsif ($mrph[0][0][$k]{base} eq "「") {
		    $mrph[0][0][$k]{result} = " [特殊 括弧始 * * *]";
		}
		elsif ($mrph[0][0][$k]{base} eq "」") {
		    $mrph[0][0][$k]{result} = " [特殊 括弧終 * * *]";
		}
		elsif ($mrph[0][0][$k]{base} eq "〜") {
		    $mrph[0][0][$k]{result} = "";
		}
		else {
		    $mrph[0][0][$k]{pos2} = "*" if ($mrph[0][0][$k]{pos} eq "名詞");
		    $mrph[0][0][$k]{pos2} = "*" if ($mrph[0][0][$k]{pos} eq "副詞");
		    $mrph[0][0][$k]{pos2} = "*" if ($mrph[0][0][$k]{pos} eq "助詞");
		    $mrph[0][0][$k]{result} =
			" [$mrph[0][0][$k]{pos} $mrph[0][0][$k]{pos2} $mrph[0][0][$k]{conj} $mrph[0][0][$k]{conj2} $mrph[0][0][$k]{base}]";
		}

		# 形態素のfeatureが指定されている場合
 		if ($k == $data[0][0]{end}[$i] &&
		    $feature[$i]{lastfeature}) {
		    if ($mrph[0][0][$k]{result}) {
			$mrph[0][0][$k]{result} =~ s/\]$/$feature[$i]{lastfeature}\]/;
		    }
		    else {
			$mrph[0][0][$k]{result} = " [* * * * *$feature[$i]{lastfeature}]";
		    }
		}

		print $mrph[0][0][$k]{result};
		print "*" if ($feature[$i]{ast});
	    }

	    if ($feature[$i]{ast} &&
		$data[0][0]{end}[$i] - $data[0][0]{start}[$i] > 0) {
		print STDERR "$input: *は各形態素につくだけ！！！\n";
	    }
	}
    }
}

######################################################################
