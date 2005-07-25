# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 20 }

use KNP::BList;

my $x = new KNP::BList();
ok(defined $x);
ok( $x->bnst == 0 );
ok( $x->tag == 0 );
ok( $x->mrph == 0 );

my $b = KNP::Bunsetsu->new( <<'__bunsetsu__', 3 );
* -1D <BGH:解析><SM:解析:711006601***71100650****><文頭><文末><サ変><体言><用言:動><体言止><レベル:C><区切:5-5><ID:（文末）><RID:110><提題受:30><抽象>
__bunsetsu__
ok( defined $b );
$x->push_bnst( $b );
ok( $x->bnst == 1 );
ok( $x->tag  == 0 );
ok( $x->mrph == 0 );

my $t = KNP::Tag->new( <<'__tag__', 0 );
+ 1D <ダミー1>
__tag__
ok( defined $t );
$x->push_tag( $t );
ok( $x->bnst == 1 );
ok( $x->tag  == 1 );
ok( $x->mrph == 0 );

my $m = KNP::Morpheme->new( <<'__koubun_mrph__' );
構文 こうぶん 構文 名詞 6 普通名詞 1 * 0 * 0 NIL <文頭><漢字><かな漢字><自立><名詞相当語>
__koubun_mrph__
ok( defined $m );
$x->push_mrph( $m );
ok( $x->bnst == 1 );
ok( $x->tag  == 1 );
ok( $x->mrph == 1 );
ok( $t->mrph == 1 );

$m = KNP::Morpheme->new( <<'__kaiseki_mrph__' );
解析 かいせき 解析 名詞 6 サ変名詞 2 * 0 * 0 NIL <文末><表現文末><漢字><かな漢字><サ変><自立><←複合><名詞相当語>
__kaiseki_mrph__
ok( defined $m );
$x->push_mrph( $m );
ok( $b->mrph == 2 );
ok( $t->mrph == 2 );
