# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 18 }

use Juman::Morpheme;

my $spec = "であり であり だ 判定詞 4 * 0 判定詞 25 デアル列基本連用形 18\n";
my $mrph = Juman::Morpheme->new( $spec );

ok(defined $mrph);
ok($mrph->midasi eq 'であり');
ok($mrph->yomi eq 'であり');
ok($mrph->genkei eq 'だ');
ok($mrph->hinsi eq '判定詞');
ok($mrph->hinsi_id == 4);
ok($mrph->bunrui eq '*');
ok($mrph->bunrui_id == 0);
ok($mrph->katuyou1 eq '判定詞');
ok($mrph->katuyou1_id == 25);
ok($mrph->katuyou2 eq 'デアル列基本連用形');
ok($mrph->katuyou2_id == 18);
ok($mrph->spec eq $spec );

$spec = "であり であり だ 判定詞 4 * 0 判定詞 25 デアル列基本連用形 18 NIL\n";
$mrph = Juman::Morpheme->new( $spec );
ok(defined $mrph);
ok($mrph->imis eq "NIL");
ok($mrph->spec eq $spec);

$spec = "@ @ @ 未定義語 15 その他 1 * 0 * 0";
$mrph = Juman::Morpheme->new( $spec );
ok(defined $mrph);
ok($mrph->midasi eq '@');
