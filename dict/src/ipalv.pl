#! /usr/local/bin/jperl -- -*-Perl-*-

# dousi1/files.doc 参照

$id_head = "動";

@name = ("ＩＤ", "読み", "表記", "意味", "述素", 
	 "格１", "格２", "格３", "格４", "格５", 
	 "意１", "意２", "意３", "意４", "意５",
	 "例１", "例２", "例３", "例４", "例５",
	 "態１", "態２", "態３", "態４", "態５", "態６", "態７");

while(<STDIN>) {

    chop;
    ~ s/\"\"/nil/g;
    ~ s/\"//g;
    (@item) = split(/,/);

    $item[0] =~ s/　//g;
    $item[14] =~ s/（[^）]+）//g;

    $num = 0;

    # ID
    print "$name[$num++] $id_head$item[0]$item[1]$item[2]\n";
    # 読み
    print "$name[$num++] $item[0]\n";
    # 表記
    print "$name[$num++] $item[14]\n";
    # 意味
    print "$name[$num++] $item[5]\n";
    # 述語素
    print "$name[$num++] $item[31]\n";
    # 格フレーム
    for ($i = 0; $i < 3; $i++) {
	for ($j = 0; $j < 5; $j++) {
	    printf "$name[$num++] $item[32+$i+$j*3]\n";
	}
    }
    # 態
    for ($i = 0; $i < 7; $i++) {
	printf "$name[$num++] $item[47+$i]\n";
    }
}
