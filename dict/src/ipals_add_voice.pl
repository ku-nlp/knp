#! /usr/local/bin/jperl -- -*-Perl-*-

# def/sa-you.def 参照

@name = ("ＩＤ", "読み", "表記", "意味", "述素", 
	 "格１", "格２", "格３", "格４", "格５", 
	 "意１", "意２", "意３", "意４", "意５",
	 "例１", "例２", "例３", "例４", "例５",
	 "態１", "態２", "態３", "態４", "態５", "態６", "態７");

$IPALV_FIELD_NUM = 27;

while( 1 ) {
 
    $WO = 0;
    for ($i = 0; $i < $IPALV_FIELD_NUM; $i++) {

	if (!($_ = <STDIN>)) {exit;}

	chop;
	($field_name, $data[$i]) = split;
	if ($field_name =~ /^格/ && $data[$i] =~ /ヲ/) {
	    $WO = 1;
	}

	if ($i == 20) {
	    if ($WO) {
		print "$name[$i] ニ使役\n";
	    } else {
		print "$name[$i] ヲ使役，ニ使役\n";
	    }
	} elsif ($i == 21) {
	    if ($WO) {
		print "$name[$i] 直受，間受\n";
	    } else {
		print "$name[$i] 間受\n";
	    }
	} elsif ($i == 22) {
	    if ($WO) {
		print "$name[$i] ヲ\n";
	    } else {
		print "$name[$i] nil\n";
	    }
	} elsif ($i == 23) {
	    if ($WO) {
		print "$name[$i] ニ，ニヨッテ\n";
	    } else {
		print "$name[$i] nil\n";
	    }
	} else {
	    print "$name[$i] $data[$i]\n";
	}
    }
}




	 
while(<STDIN>) {

    chop;
    ($type, $data) = split(/\t/);

    
    if ($type eq "Ｆサ変用法Ｓ" &&
	substr($data, 40, 4) ne "Ｚ  ") {

	if (!$map{substr($data, 8, 12)}) {
	    printf STDERR "$_\n"; 
	}

	($yomi, $hyouki, $imi) = split(/ /, $map{substr($data, 8, 12)});
	(@item) = split(/ /, substr($data, 54));
	$num = 0;

	# ID
	print "$name[$num++] $code2\n";
	# 読み
	print "$name[$num++] $yomi\n";
	# 表記
	print "$name[$num++] $hyouki\n";
	# 意味
	print "$name[$num++] $imi\n";
	# 述語素 (サ変名詞はない)
	print "$name[$num++] nil\n";
	# 格フレーム
	for ($i = 0; $i < 3; $i++) {     # "ノNP"をスキップ
	    for ($j = 0; $j < 4; $j++) { # サ変名詞は最大４要素
		$item[6+$i+$j*4] =~ s/^[^=]+=\"|\"$//g;
		if (!$item[6+$i+$j*4]) {
		    $item[6+$i+$j*4] = "nil";
		}
		printf "$name[$num++] $item[6+$i+$j*4]\n";
	    }
	    printf "$name[$num++] nil\n";
	}
	# 態 (サ変名詞はない)
	for ($i = 0; $i < 7; $i++) {
	    printf "$name[$num++] nil\n";
	}
    }
}

close(TMP);

system("rm tmp.tmp");
