# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 38 }

use Juman;
use Juman::Hinsi ":all";
use Juman::Morpheme;

my $juman = new Juman;
ok( defined $juman );
ok( $juman->get_hinsi_id( '名詞' ) == 6 );
ok( &get_hinsi( 6 ) eq '名詞' );

for( <DATA> ){
    if( $_ !~ m/\A\#/ and $_ =~ /\S/ ){
	my $m = Juman::Morpheme->new( $_, 0 );
	ok( defined $m );
	ok( $m->hinsi eq &get_hinsi($m->hinsi_id) );
	ok( $m->hinsi_id == &get_hinsi_id($m->hinsi) );
	ok( $m->bunrui eq &get_bunrui($m->hinsi_id,$m->bunrui_id) );
	ok( $m->bunrui_id == &get_bunrui_id($m->hinsi,$m->bunrui) );
	ok( $m->katuyou1 eq &get_type($m->katuyou1_id) );
	ok( $m->katuyou1_id == &get_type_id($m->katuyou1) );
#	ok( $m->katuyou2 eq &get_form($m->katuyou1_id,$m->katuyou2_id) );
#	ok( $m->katuyou2_id == &get_form_id($m->katuyou1,$m->katuyou2) );
    }
}

__DATA__
赤い あかい 赤い 形容詞 3 * 0 イ形容詞アウオ段 18 基本形 2 NIL
花 はな 花 名詞 6 普通名詞 1 * 0 * 0 NIL
が が が 助詞 9 格助詞 1 * 0 * 0 NIL
咲いた さいた 咲く 動詞 2 * 0 子音動詞カ行 2 タ形 8 NIL
のだ のだ のだ 助動詞 5 * 0 ナ形容詞 21 基本形 2 NIL
