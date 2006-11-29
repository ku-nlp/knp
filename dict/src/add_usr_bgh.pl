#!/usr/bin/env perl

# $Id$

use strict;
use encoding 'euc-jp';
binmode STDERR, ':encoding(euc-jp)';
binmode DB::OUT, ':encoding(euc-jp)';
use KNP;

# 分類語彙表にエントリを追加するスクリプト

# usage:
# ./add_usr_bgh.pl bgh.dat < usr_bgh.dat
 
# usr_bgh.datの例
# アスパラ=きゅうり

my $knp = new KNP;

my %word2code;

open F, "<:encoding(euc-jp)", $ARGV[0] or die;

# bgh.datの読み込み
while (<F>) {
    print;

    chomp;

    my ($word, $code) = split;

    $word2code{$word} .= defined $word2code{$word} ? "/$code" : $code;
}
close F;

# usr_bgh.datの読み込み
while (<STDIN>) {
    chomp;

    next if /\#/ || $_ eq "";

    my ($newword, $word);

    if (/^(\S+)=(\S+)$/) {
	($newword, $word) = ($1, $2);
    }
    else {
	print STDERR "Format Error: $_\n";
	next;
    }

    my ($new_repname, $repname);
    $new_repname = &get_repname($newword);
    $repname = &get_repname($word);

    # すでにエントリがある
    if (defined $word2code{$new_repname}) {
	print STDERR "!!$newword is already registered\n";
	next;
    }
    
    if (defined $word2code{$repname}) {
	foreach (split(/\//, $word2code{$repname})) {
	    print "$new_repname $_\n";
	}
    }
    else {
	print STDERR "!!Not Found: $word($repname)\n";
    }
}

sub get_repname {
    my ($word) = @_;

    my $result = $knp->parse($word);

    if ($result && $result->bnst == 1) {
	# 曖昧性がある場合(?で連結されている場合)、一番最初だけをとる
	return (split(/\?/, ($result->bnst)[0]->repname))[0];
    }
    else {
	print STDERR "Error!! Can't get the repname of $word\n";
	return;
    }
}
