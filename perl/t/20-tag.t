# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 16 }

use KNP::Tag;

my $t = KNP::Tag->new( <<'__tag__', 1 );
+ 2D <SM:富市:2110********><文節内><人名><体言><係:隣接><裸名詞><区切:0-0><RID:1352><タイプ-主体><解析格-ガ><解析格-ヲ><解析格-ニ><解析連格-ガ><解析連格-外の関係><解析連格-ガ２>
__tag__

ok( defined $t );
ok( $t->id == 1 );
ok( $t->dpndtype eq "D" );
ok( $t->mrph == 0 );

ok( $t->parent_id == 2 );
ok( ! $t->parent_id( undef ) );

ok( $t->fstring );
ok( $t->feature == 15 );
ok( ! $t->fstring( "" ) );
ok( ! $t->fstring );
ok( $t->feature == 0 );

my $m = KNP::Morpheme->new( <<'__koubun_mrph__' );
構文 こうぶん 構文 名詞 6 普通名詞 1 * 0 * 0 NIL <文頭><漢字><かな漢字><自立><名詞相当語>
__koubun_mrph__
ok( defined $m );
$t->push_mrph( $m );
ok( scalar($t->mrph) == 1 );

$m = KNP::Morpheme->new( <<'__kaiseki_mrph__' );
解析 かいせき 解析 名詞 6 サ変名詞 2 * 0 * 0 NIL <文末><表現文末><漢字><かな漢字><サ変><自立><←複合><名詞相当語>
__kaiseki_mrph__
ok( defined $m );
$t->push_mrph( $m );
ok( scalar($t->mrph) == 2 );

ok( join('',map($_->midasi,$t->mrph)) eq '構文解析' );
