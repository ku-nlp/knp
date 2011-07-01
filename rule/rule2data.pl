#!/usr/bin/env perl

# KNPルールファイルの括弧([, ], <, >)を変換

for ($i = 0; $i < @ARGV; $i++) {

    open(INPUT, $ARGV[$i]);
    $ARGV[$i] =~ s/rule$/data/;

    print "make $ARGV[$i] ... \n";

    open(OUTPUT, "> $ARGV[$i]") || die;

    while (<INPUT>) {

	s/\[/\(/g;
	s/\]/\)/g;
	s/\</\(/g;
	s/\>/\)/g;

	print OUTPUT;
    }

    close INPUT;
    close OUTPUT;
}
