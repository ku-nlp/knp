# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 16 }

unless( eval { require KULM::Juman::M; } ){
    print STDERR "KULM::Juman::M is missing.  Skip all tests.\n";
    for( 1 .. 16 ){
	print "ok $_\n";
    }
    exit 0;
}
use Juman::Morpheme;

my $spec = "であり であり だ 判定詞 4 * 0 判定詞 25 デアル列基本連用形 18 NIL\n";
my $x = Juman::Morpheme->new( $spec );
my $y = KULM::Juman::M->new( string => $spec );

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

ok( join("", $x->gets('H1', 'G')) eq join("", $y->gets('H1', 'G')) );
ok( $x->string eq $y->string );
