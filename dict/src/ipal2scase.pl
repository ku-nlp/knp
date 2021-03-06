#! /usr/local/bin/jperl -- -*-Perl-*-

# Conversion from "ipal?.dat" to "scase.dat"

# ▼ ガ-ガ構文を扱うため$frameを11ケタに変更

$kaku_num{"ガ"} = 1;
$kaku_num{"ヲ"} = 2;
$kaku_num{"ニ"} = 3;
$kaku_num{"デ"} = 4;
$kaku_num{"カラ"} = 5;
$kaku_num{"ト"} = 6;
$kaku_num{"ヨリ"} = 7;
$kaku_num{"ヘ"} = 8;
$kaku_num{"マデ"} = 9;
$kaku_num{"ノ"} = 10;
$kaku_num{"ガ２"} = 11;

$IPALV_FIELD_NUM = 27;

while (1) {

    $frame = "00000000000";
    @frame = split(//, $frame);
    for ($i = 0; $i < $IPALV_FIELD_NUM; $i++) {

	if (!($_ = <STDIN>)) {exit;}

	chop;
	($field_name, $data[$i]) = split;
	if ($field_name =~ /^格/ && $data[$i] !~ /^nil$/) {
	    $data[$i] =~ s/＊//g;
	    foreach $item (split(/／/, $data[$i])) {
		# ガ-ガ構文の扱い
		if ($item eq "ガ" && $frame[$kaku_num{"ガ"}-1] == '1') {
		    $frame[$kaku_num{"ガ２"}-1] = '1';
		}

		if ($kaku_num{$item}) {
		    $frame[$kaku_num{$item}-1] = '1';
		}
	    }
	}
    }
    $frame = join('', @frame);

    printf "$data[1] $frame\n";			# 読みに対する出力
    foreach $item (split(/，/, $data[2])) {
	printf "$item $frame\n";		# 表記に対する出力
    }
}
