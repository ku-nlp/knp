#! /usr/local/bin/jperl -- -*-Perl-*-

# Conversion from "JWD.DIC" (version 1.0) to "scase.dat"

# ▼ ガ-ガ構文を扱うため$frameを11ケタに変更

# １レコードの情報 
#      レコード番号 \t 単語見出し \t 不変化部-連接属性対 \t 
#       かな表記 \t 発音 \t 品詞 \t 構文木 \t 活用情報 \t 表層格情報 \t
#       相情報 \t 機能語情報 \t 概念識別子 \t 英語概念見出し \t
#       日本語概念見出し \t 英語概念説明 \t 日本語概念説明 \t
#       用法 \t 頻度 \t 管理情報 \n

<STDIN>;			# skip header ("Copyright ... ")

while(<STDIN>) {

    (@record) = split(/\t/);

    if ($record[8] !~ /^""$/) {

	if ($record[5] =~ /JAP|JMP|JPR/) {
	    next;
	} elsif ($record[5] !~ /JVE|JAJ|JAM/) {
	    # printf STDERR "$record[1] $record[5] $record[8]\n";
	    next;
	} 

	$record[1] =~ /^(.*)\[(.*)\]$/;
	$hyouki = $1;
	$yomi = $2;
	$yomi =~ s/・//g;
	$yomi =~ tr/ァ-ン/ぁ-ん/;

	$frame = "00000000000";
	@frame = split(//, $frame);
	if ($record[8] =~ /JK01/) {$frame[0] = '1';}
	if ($record[8] =~ /JK02/) {$frame[1] = '1';}
	if ($record[8] =~ /JK03/) {$frame[2] = '1';}
	if ($record[8] =~ /JK04/) {$frame[3] = '1';}
	if ($record[8] =~ /JK05/) {$frame[4] = '1';}
	if ($record[8] =~ /JK06/) {$frame[5] = '1';}
	if ($record[8] =~ /JK07/) {$frame[6] = '1';}
	if ($record[8] =~ /JK08/) {$frame[7] = '1';}
	if ($record[8] =~ /JK09/) {$frame[8] = '1';}
	if ($record[8] =~ /JK10/) {$frame[9] = '1';}
	$frame = join('', @frame);

	$yomi =~ s/する$//;	# サ変名詞は末尾の「する」を削除
	$hyouki =~ s/する$//;

	print "$yomi $frame\n";
	print "$hyouki $frame\n";
    }
}

