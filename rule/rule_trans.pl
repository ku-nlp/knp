#! /usr/local/bin/jperl -- -*-Perl-*-

######################################################################
# KNPの文節featture付与ルールのtranslator	(99/09/10 kuro)
######################################################################

# 本当は全体に書いておく方がよいが，とりあえず使われるものだけ

$pos_cond{"形式名詞"} = " [名詞 形式名詞]";
$pos_repr{"形式名詞"} = "こと";
$pos_cond{"副詞的名詞"} = " [名詞 副詞的名詞]";
$pos_repr{"副詞的名詞"} = "ため";

$pos_cond{"接尾辞"} = " [接尾辞]";
$pos_repr{"接尾辞"} = "個";
$pos_cond{"名詞性名詞助数辞"} = " [接尾辞 名詞性名詞助数辞]";
$pos_repr{"名詞性名詞助数辞"} = "個";
$pos_cond{"名詞性特殊接尾辞"} = " [接尾辞 名詞性特殊接尾辞]";
$pos_repr{"名詞性特殊接尾辞"} = "以内";

$pos_cond{"助詞"} = " [助詞]";
$pos_repr{"助詞"} = "だけ";
$pos_cond{"格助詞"} = " [助詞 格助詞]";
$pos_repr{"格助詞"} = "に";
$pos_cond{"副助詞"} = " [助詞 副助詞]";
$pos_repr{"副助詞"} = "だけ";

$pos_cond{"副詞形態指示詞"} = " [指示詞 副詞形態指示詞]";
$pos_repr{"副詞形態指示詞"} = "こう";
$pos_cond{"連体詞"} = " [連体詞]";
$pos_repr{"連体詞"} = "ほんの";
$pos_cond{"連体詞形態指示詞"} = " [指示詞 連体詞形態指示詞]";
$pos_repr{"連体詞形態指示詞"} = "この";
$pos_cond{"接続詞"} = " [接続詞]";
$pos_repr{"接続詞"} = "そして";
$pos_cond{"感動詞"} = " [感動詞]";
$pos_repr{"感動詞"} = "あっ";

$pos_cond{"句点"} = " [特殊 句点]";
$pos_repr{"句点"} = "．";

######################################################################
use Juman
$juman = new Juman("-e -B"); 

######################################################################
$num = 0;
while ( <STDIN> ) {
    
    chomp;
    $num ++;
    next if (/^[\s\t]*\;/ || length($_) == 0);

    if (/^([^\t]+)[\s\t]+([^\;\t]+)[\s\t]*(\;.+)$/) {
	$pattern = $1; $feature = $2; $comment = $3;
    } elsif (/^([^\t]+)[\s\t]+([^\;\t]+)[\s\t]*$/) {
	$pattern = $1; $feature = $2; $comment = "";
    } else {
	print STDERR  "line $num is invalid; $_\n";
	next;
    }
    print "; $pattern\n";

    $pattern =~ s/^\s+//;	# 念のため
    $pattern =~ s/\s+$//;	# 念のため

    $pattern =~ /^(\[[^\[\]]+\])?([^\[\]]+)(\[[^\[\]]+\])?$/;
    $tmp1 = $1; $self = $2; $tmp2 = $3;
    $tmp1 =~ s/^\[|\]$//g;
    @pres = split(/ /, $tmp1);
    $tmp2 =~ s/^\[|\]$//g;
    @poss = split(/ /, $tmp2);

    # print " pre  @pres\n self $self\n post @poss\n\n";

    # まず代表句をつくる
    undef @pres_str;
    undef @poss_str;
    foreach (@pres) {
	push (@pres_str, print_bnst_cond(0, $_));
    }
    $self_str = print_bnst_cond(0, $self);
    foreach (@poss) {
	push (@poss_str, print_bnst_cond(0, $_));
    }

    print "(\n";
    # 前の文節列の条件
    if (@pres) {
	print "( ?*";
	for ($i = 0; $i < @pres; $i++) {
	    if ($i == 0) {
		$l_context = "";
	    } else {
		$l_context = $pres_str[$i-1];
	    }
	    if ($i == (@pres - 1)) {
		$r_context = $self_str;
	    } else {
		$r_context = $pres_str[$i+1];;
	    }
	    print_bnst_cond(1, $pres[$i], $l_context, $r_context);
	}
	print " )\n";
    } else {
	print "( ?* )\n";
    }

    # 自分の条件
    print "( ";

    if (@pres) {
	$l_context = $pres_str[@pres_str-1];
    } else {
	$l_context = "";
    }
    if (@poss) {
	$r_context = $poss_str[0];
    } else {
	$r_context = "";
    }
    print_bnst_cond(1, $self, $l_context, $r_context);

    print " )\n";

    # 後の文節列の条件
    if (@poss) {
	print "( ";
	for ($i = 0; $i < @poss; $i++) {
	    if ($i == 0) {
		$l_context = $self_str;
	    } else {
		$l_context = $poss_str[$i-1];
	    }
	    if ($i == (@poss - 1)) {
		$r_context = "";
	    } else {
		$r_context = $poss_str[$i+1];;
	    }
	    print_bnst_cond(1, $poss[$i], $l_context, $r_context);
	}
	print " ?* )\n";
    } else {
	print "( ?* )\n";
    }

    print "\t$feature\n";
    print ")\n";

    print "$comment\n" if $comment;
    print "\n";
}
$juman->close;

######################################################################
sub print_bnst_cond
{
    my ($flag, $input, $l_context, $r_context) = @_;
    my ($ast_flag, $input1, $input2);

    if ($input =~ /^\((.+)\)$/) {
	$input = $1;
	$ast_flag = 1;
    } else {
	$ast_flag = 0;
    }

    if ($input =~ /^\<([^\<\>]+)\>$/) {
	$input = $1;
	if ($flag) {
	    printf "< (?*) %s >", feature2str($input);
	} else {
	    return "？";
	}
    } elsif ($input =~ /^(.+)\<([^\<\>]+)\>$/) {
	$input1 = $1; $input2 = $2;
	if ($flag) {
	    print "< (";
	    print_normal_cond(1, $input1, $l_context, $r_context);
	    printf ") %s >", feature2str($input2);
	} else {
	    return print_normal_cond(0, $input1);
	}
    } else {
	if ($flag) {
	    print "< (";
	    print_normal_cond(1, $input, $l_context, $r_context);
	    print ") >";
	} else {
	    return print_normal_cond(0, $input);
	}
    }
    print "*" if $ast_flag;
    print " ";
}

######################################################################
sub feature2str
{
    my ($input) = @_;

    $data = "(";
    foreach (split(/\|\|/, $input)) {
	s/\&\&/ /g;
	$data .= "($_)";
    }
    $data .= ")";
    return $data;
}

######################################################################
sub print_normal_cond
{
    my ($flag, $input, $l_context, $r_context) = @_;
    my (@data, @feature, @mrph);
    my ($i, $j, $k, $l, $error_flag);
    my $from_any = 1;

    if ($input =~ /^\^/) {
	$input =~ s/^\^//;
	$from_any = 0;
    }

    # 要素ごとに分割
    
    $input =~ s/\(/ \(/g;
    $input =~ s/\)/\) /g;
    $input =~ s/\{/ \{/g;
    $input =~ s/\}/\} /g;
    # $input =~ s/\<\</ \<\</g;
    $input =~ s/\>\>/\>\> /g;
    $input =~ s/\*/\* /g;
    $input =~ s/\?\*/ \?\*/g;
    $input =~ s/^ +| +$//g;

    # 標準データを$data[0][0]に

    $i = 0;
    foreach $item (split(/ +/, $input)) {
	if ($item =~ /^\((.+)\)$/) {
	    $item =  $1;
	    $feature[$i]{ast} = 1;
	}
	elsif ($item =~ /^\{(.+)\}$/) {
	    $item =  $1;
	}

	if ($item !~ /^\<\</ && $item =~ /\|/) {

	    # ORの場合は句だけ

	    $feature[$i]{or} = 1;
	    @{$part[$i]} = split(/\|/, $item);

	} else {

	    # 他は種々の指定を考慮

	    @{$part[$i]} = ($item);

	    if (0 && $part[$i][0] =~ /\<\<(.+)\>\>$/) {
		$feature[$i]{result} = 
		    " [* * * * * " . feature2str($1) . "]";
		$part[$i][0] = "？";
	    }

	    if ($part[$i][0] =~ /\<\<(.+)\>\>$/) {
		$feature[$i]{lastfeature} = feature2str($1);
		$part[$i][0] =~ s/\<\<.+\>\>$//;
	    }
	    elsif ($part[$i][0] eq "?*") {
		$feature[$i]{result} = " ?*";
		$part[$i][0] = "？";
	    }
	    elsif ($part[$i][0] =~ /\*$/) {
		$feature[$i]{lastast} = 1;
		$part[$i][0] =~ s/\*$//;
	    }
	    elsif ($pos_cond{$part[$i][0]}) {
		$feature[$i]{result} = $pos_cond{$part[$i][0]};
		$part[$i][0] = $pos_repr{$part[$i][0]};
	    }
	}

	$data[0][0]{phrase} .= $part[$i][0];
	$data[0][0]{length}[$i] = length($part[$i][0]);
	$i++;
    }
    $part_num = $i;

    if ($flag == 0) {
	return $data[0][0]{phrase};
    }

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

    # それぞれJUMANで処理

    for ($i = 0; $i < $part_num; $i++) {
	for ($j = 0; $j < @{$part[$i]}; $j++) {
	    next unless ($data[$i][$j]);
	    # print ">> ($l_context)$data[$i][$j]{phrase}($r_context)(@{$data[$i][$j]{length}})\n";

	    if ($r_context) {
		@juman_result = $juman->parse($l_context . $data[$i][$j]{phrase} . $r_context); 
	    } elsif (!$r_context && $data[$i][$j]{phrase} =~ /(，|．)$/) {
		@juman_result = $juman->parse($l_context . $data[$i][$j]{phrase} . "？"); 
	    } else {
		@juman_result = $juman->parse($l_context . $data[$i][$j]{phrase} . "，？"); 
	    }
	    $k = 0;
	    $length = 0;
	    $begin_check = 0;
	    foreach $item (@juman_result) { 
		next if ($item =~ /^EOS/);
		next if ($item =~ /^\@/);
		@tmp_mrph = split(/ /, $item);

		if ($length < length($l_context)) {
		    $length += length($tmp_mrph[0]);
		    next;
		} elsif ($length < (length($l_context) + length($data[$i][$j]{phrase}))) {
		    if ($begin_check == 0) {
			if ($length != length($l_context)) {
			    print "CONTEXT ERROR @tmp_mrph\n";
			    return;
			} else {
			    $begin_check = 1;
			}
		    }

		    $length += length($tmp_mrph[0]);
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
		     $mrph[$i][$j][$k]{d4}) = @tmp_mrph;
		    $mrph[$i][$j][$k]{length} = length($mrph[$i][$j][$k]{word});

		    # ここで作るのはチェックのためだけ
		    $mrph[$i][$j][$k]{result} = 		    
			" [$mrph[$i][$j][$k]{pos} $mrph[$i][$j][$k]{pos2} $mrph[$i][$j][$k]{conj} $mrph[$i][$j][$k]{conj2} $mrph[$i][$j][$k]{base}]";
		    $k++;
		} else {
		    if ($length != (length($l_context) + length($data[$i][$j]{phrase}))) {
			print "CONTEXT ERROR @tmp_mrph\n";
			return;
		    }
		}
	    }

	    # part と mrph の対応つけ (場合によってORの文字数が違うので各$data[$i][$j]に必要)
	    $part_length = 0;
	    $mrph_length = 0;
	    $k = 0;
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
			break;
		    }
		}
		# ORの後の形態素列の一致
		for ($k = $data[0][0]{end}[$i]+1; $k < @{$mrph[0][0]}; $k++) {
		    if ($mrph[0][0][$k]{result} ne $mrph[$i][$j][$k]{result}) {
			$error_flag = 1;
			break;
		    }
		}
		
		# ORが同一品詞か
		if ($mrph[0][0][$data[0][0]{start}[$i]]{pos} ne $mrph[$i][$j][$data[$i][$j]{start}[$i]]{pos}) {
		    $error_flag = 1;
		    if ($mrph[0][0][$data[0][0]{start}[$i]]{conj} eq "*" && 
			$mrph[$i][$j][$data[$i][$j]{start}[$i]]{conj} ne "*") {
			# 活用語とそうでないものの間は若干吸収
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

    print " ?*" if ($from_any);

    for ($i = 0; $i < $part_num; $i++) {
	if ($feature[$i]{result}) {
	    print $feature[$i]{result};
	} else {
	    for ($k = $data[0][0]{start}[$i]; $k <= $data[0][0]{end}[$i]; $k++) {
		if (($mrph[0][0][$k]{base} eq "書く" || $mrph[0][0][$k]{base} eq "読む") && 
		    $k == $feature[$i]{end} &&
		    $feature[$i]{lastast}) {
		    $mrph[0][0][$k]{result} = " [* * * * * ((活用語))]";
		}
		elsif ($mrph[0][0][$k]{base} eq "書く" || $mrph[0][0][$k]{base} eq "読む") {
		    $mrph[0][0][$k]{result} = " [* * (母音動詞 子音動詞カ行 子音動詞カ行促音便形 子音動詞ガ行 子音動詞サ行 子音動詞タ行 子音動詞ナ行 子音動詞バ行 子音動詞マ行 子音動詞ラ行 子音動詞ラ行イ形 子音動詞ワ行 子音動詞ワ行文語音便形 カ変動詞 カ変動詞来 サ変動詞 ザ変動詞 動詞性接尾辞ます型 動詞性接尾辞うる型 動詞性接尾辞得る型 無活用型) $mrph[0][0][$k]{conj2} *]";
		}
		elsif ($mrph[0][0][$k]{base} eq "美しい") {
		    $mrph[0][0][$k]{result} = " [* * (イ形容詞アウオ段 イ形容詞イ段 イ形容詞イ段特殊 助動詞ぬ型 助動詞く型) $mrph[0][0][$k]{conj2} *]";
		}
		elsif ($mrph[0][0][$k]{base} eq "静かだ" || $mrph[0][0][$k]{base} eq "特別だ") {
		    $mrph[0][0][$k]{result} = " [* * (ナ形容詞 ナ形容詞特殊 ナノ形容詞 判定詞 助動詞ぬ型 助動詞だろう型 助動詞そうだ型) $mrph[0][0][$k]{conj2} *]";
		}
		elsif ($mrph[0][0][$k]{base} eq "堂々たる") {
		    $mrph[0][0][$k]{result} = " [* * (タル形容詞) $mrph[0][0][$k]{conj2} *]";
		}
		elsif ($mrph[0][0][$k]{base} eq "学生" &&
		       $k == $feature[$i]{end} &&
		       $feature[$i]{lastast}) {
		    $mrph[0][0][$k]{result} = " [* * * * * ((^活用語))]";
		}
		elsif ($mrph[0][0][$k]{base} eq "学生") {
		    $mrph[0][0][$k]{result} = " [* * * * * ((名詞相当語))]";
		}
		elsif ($mrph[0][0][$k]{base} eq "すこし") {
		    $mrph[0][0][$k]{result} = " [副詞 * * * *]";
		}
		elsif ($mrph[0][0][$k]{base} eq "昨日") {
		    $mrph[0][0][$k]{result} = " [名詞 時相名詞 * * *]";
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
		    $mrph[0][0][$k]{conj2} = "*" if ($feature[$i]{lastast});
		    $mrph[0][0][$k]{result} =
			" [$mrph[0][0][$k]{pos} $mrph[0][0][$k]{pos2} $mrph[0][0][$k]{conj} $mrph[0][0][$k]{conj2} $mrph[0][0][$k]{base}]";
		}

		# 形態素のfeatureが指定されている場合
		if ($feature[$i]{lastfeature}) {
		    if ($mrph[0][0][$k]{result}) {
			$mrph[0][0][$k]{result} =~ s/\]$/ $feature[$i]{lastfeature}]/;
		    }
		    else {
			$mrph[0][0][$k]{result} = " [* * * * * $feature[$i]{lastfeature}]";
		    }
		}

		print $mrph[0][0][$k]{result};
		print "*" if ($feature[$i]{ask});
	    }
	}

	if ($feature[$i]{ast}) {
	    print "*";
	}
    }
}

######################################################################
sub print_juman
{
    my ($input) = @_;
    my ($data, $item, @juman_result);
    $data = "";

    @juman_result = $juman->parse($input);

    foreach $item (@juman_result) {
        next if ($item =~ /^EOS/);
        next if ($item =~ /^\@/);
        ($word, $yomi, $base, $pos, $d1, $pos2, $d2, $conj, $d3, $conj2, $d4)
            = split(/ /, $item);
        $data .= "  [$pos $pos2 $conj $conj2 ($base)] \n";
    }
    print $data;
}
######################################################################
