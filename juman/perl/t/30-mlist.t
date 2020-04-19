# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 6 }

use Juman::Morpheme;
use Juman::MList;

my $mlist = Juman::MList->new();
ok( defined $mlist );

$mlist->push_mrph( Juman::Morpheme->new( "構文 こうぶん 構文 名詞 6 普通名詞 1 * 0 * 0\n") );
$mlist->push_mrph( Juman::Morpheme->new( "解析 かいせき 解析 名詞 6 サ変名詞 2 * 0 * 0\n" ) );

ok( $mlist->mrph == 2 );
ok( $mlist->mrph(0)->midasi eq '構文' );
ok( $mlist->mrph(-1)->midasi eq '解析' );
ok( join('',map($_->midasi,$mlist->mrph(0,1))) eq '構文解析' );
ok( join('',map($_->midasi,$mlist->mrph_list)) eq '構文解析' );
