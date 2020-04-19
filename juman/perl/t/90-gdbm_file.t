# -*- perl -*-

use utf8;
use strict;
use English qw/ $PERL_VERSION /;
use Test;

sub BEGIN {
    plan tests => 3;
    if( $PERL_VERSION > 5.008 ){
	require encoding;
	encoding->import("euc-jp");
    }
}

if( eval { require GDBM_File; } ){
    require Juman::GDBM_File;
    Juman::GDBM_File->import();
} else {
    print STDERR "GDBM_File is missing.  Skip all tests.\n";
    for( 1 .. 3 ){
	print "ok $_\n";
    }
    exit 0;
}

my $dbname;
sub INIT { unlink( $dbname = "$0.db" ); }
sub END  { unlink $dbname; }

my %hash;
my $tie = tie %hash, 'Juman::GDBM_File', $dbname, &O_CREAT;
ok( $tie );

my $soeji = "添字";
my $atai = "値";

$hash{$soeji} = $atai;
ok( (keys %hash)[0] eq $soeji );
ok( $hash{$soeji} eq $atai );
