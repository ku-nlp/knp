# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 21 }

use KNP::Morpheme;

my $spec = "構文 こうぶん 構文 名詞 6 普通名詞 1 * 0 * 0 NIL <漢字><かな漢字><自立><←複合><名詞相当語>\n";
my $mrph = KNP::Morpheme->new( $spec );

ok(defined $mrph);
ok($mrph->midasi eq '構文');
ok($mrph->yomi eq 'こうぶん');
ok($mrph->genkei eq '構文');
ok($mrph->hinsi eq '名詞');
ok($mrph->hinsi_id == 6);
ok($mrph->bunrui eq '普通名詞');
ok($mrph->bunrui_id == 1);
ok($mrph->katuyou1 eq '*');
ok($mrph->katuyou1_id == 0);
ok($mrph->katuyou2 eq '*');
ok($mrph->katuyou2_id == 0);
ok($mrph->imis eq 'NIL');
ok($mrph->fstring eq '<漢字><かな漢字><自立><←複合><名詞相当語>');
ok($mrph->feature == 5);
ok($mrph->spec eq $spec);

ok($mrph->push_feature('TEST') == 6);
ok($mrph->fstring =~ /<TEST>/);

ok( ! $mrph->fstring( "" ) );
ok( ! $mrph->fstring );
ok( $mrph->feature == 0 );
