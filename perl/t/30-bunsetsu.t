# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 19 }

use KNP::Bunsetsu;

my $b = KNP::Bunsetsu->new( <<'__bunsetsu__', 3 );
* -1D <BGH:解析><SM:解析:711006601***71100650****><文頭><文末><サ変><体言><用言:動><体言止><レベル:C><区切:5-5><ID:（文末）><RID:110><提題受:30><抽象>
__bunsetsu__

ok( defined $b );
ok( $b->id == 3 );
ok( $b->dpndtype eq "D" );
ok( $b->mrph == 0 );
ok( $b->tag == 0 );

my $pstring = "構文木の追加文字列";
ok( $b->pstring( $pstring ) eq $pstring );
ok( $b->pstring eq $pstring );
ok( ! $b->pstring( "" ) );
ok( ! $b->pstring );

ok( $b->fstring );
ok( $b->feature == 14 );
ok( ! $b->fstring( "" ) );
ok( ! $b->fstring );
ok( $b->feature == 0 );

my $m = KNP::Morpheme->new( <<'__koubun_mrph__' );
構文 こうぶん 構文 名詞 6 普通名詞 1 * 0 * 0 NIL <文頭><漢字><かな漢字><自立><名詞相当語>
__koubun_mrph__
ok( defined $m );
$b->push_mrph( $m );
ok( scalar($b->mrph) == 1 );

$m = KNP::Morpheme->new( <<'__kaiseki_mrph__' );
解析 かいせき 解析 名詞 6 サ変名詞 2 * 0 * 0 NIL <文末><表現文末><漢字><かな漢字><サ変><自立><←複合><名詞相当語>
__kaiseki_mrph__
ok( defined $m );
$b->push_mrph( $m );
ok( scalar($b->mrph) == 2 );

ok( join('',map($_->midasi,$b->mrph)) eq '構文解析' );
