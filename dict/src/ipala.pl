#! /usr/local/bin/jperl -- -*-Perl-*-

# keiyou/files.doc 参照

$id_head = "形";

@name = ("ＩＤ", "読み", "表記", "意味", "述素", 
	 "格１", "格２", "格３", "格４", "格５", 
	 "意１", "意２", "意３", "意４", "意５",
	 "例１", "例２", "例３", "例４", "例５",
	 "態１", "態２", "態３", "態４", "態５", "態６", "態７");

open(TABLE, $ARGV[0]) || die;

while(<TABLE>) {
    ~ s/\"//g;
    (@item) = split(/,/);
    $item[6] =~ s/（[^）]+）//g;
    $item[6] =~ s/［|］//g;
    $hyouki{substr($item[0], 0, 10)} = $item[6];
    $meaning{substr($item[0], 0, 10)} = $item[24];
    # print substr($item[0], 0, 10);
}

close(TABLE);

open(DATA, $ARGV[1]) || die;

while(<DATA>) {

    chop;
    ~ s/\"\"/nil/g;
    ~ s/\"//g;
    (@item) = split(/,/);

    $num = 0;

    # ID
    print "$name[$num++] $id_head$item[0]\n";
    # 読み
    print "$name[$num++] $item[1]\n";
    # 表記
    print "$name[$num++] $hyouki{substr($item[0], 0, 10)}\n";
    # 意味
    print "$name[$num++] $meaning{substr($item[0],0,10)}\n";
    # 述語素
    print "$name[$num++] $item[7]\n";
    # 格フレーム
    for ($i = 1; $i < 4; $i++) {     # "ＮＰの添字"をスキップ
	for ($j = 0; $j < 3; $j++) { # 形容詞は最大３要素
	    printf "$name[$num++] $item[8+$i+$j*4]\n";
	}
	printf "$name[$num++] nil\n";
	printf "$name[$num++] nil\n";
    }
    # 態 (形容詞はない)
    for ($i = 0; $i < 7; $i++) {
	printf "$name[$num++] nil\n";
    }
}
