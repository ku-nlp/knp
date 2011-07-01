# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 21 }

unless( eval { require KULM::KNP::M; } ){
    print STDERR "KULM::KNP::M is missing.  Skip all tests.\n";
    for( 1 .. 21 ){
	print "ok $_\n";
    }
    exit 0;
}
use KNP::Morpheme;

my $spec = "構文 こうぶん 構文 名詞 6 普通名詞 1 * 0 * 0 NIL <漢字><かな漢字><自立><←複合><名詞相当語>\n";
my $x = KNP::Morpheme->new( $spec );
my $y = KULM::KNP::M->new( string => $spec );

ok( defined $x );
ok( defined $y );
ok( $x->get('M') eq $y->get('M') );
ok( $x->get('Y') eq $y->get('Y') );
ok( $x->get('G') eq $y->get('G') );
ok( $x->get('H1') eq $y->get('H1') );
ok( $x->get('H1_ID') == $y->get('H1_ID') );
ok( $x->get('H2') eq $y->get('H2') );
ok( $x->get('H2_ID') == $y->get('H2_ID') );
ok( $x->get('K1') eq $y->get('K1') );
ok( $x->get('K1_ID') == $y->get('K1_ID') );
ok( $x->get('K2') eq $y->get('K2') );
ok( $x->get('K2_ID') == $y->get('K2_ID') );
ok( $x->get('I') eq $y->get('I') );
ok( $x->get('FS') eq $y->get('FS') );

my $f = $x->get('F');
my $g = $y->get('F');
ok( ref $f eq "ARRAY" );
ok( ref $g eq "ARRAY" );
ok( @{$f} == @{$g} );
ok( $x->get(F => 1) eq $y->get(F => 1) );

ok( join("", $x->gets('H1', 'G')) eq join("", $y->gets('H1', 'G')) );
ok( $x->string eq $y->string );
