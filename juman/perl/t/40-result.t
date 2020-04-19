# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 7 }

my $result = <<'__sample__';
形態素 けいたいそ 形態素 名詞 6 普通名詞 1 * 0 * 0
解析 かいせき 解析 名詞 6 サ変名詞 2 * 0 * 0
の の の 助詞 9 接続助詞 3 * 0 * 0
実行 じっこう 実行 名詞 6 サ変名詞 2 * 0 * 0
例 れい 例 名詞 6 普通名詞 1 * 0 * 0
@ 例 たとえ 例 名詞 6 普通名詞 1 * 0 * 0
@ 例 ためし 例 名詞 6 普通名詞 1 * 0 * 0
EOS
__sample__

use Juman::Result;

my $x = Juman::Result->new( $result );
ok( defined $x );
ok( $x->mrph == 5 );
ok( "形態素解析の実行例" eq join( "", map( $_->midasi, $x->mrph_list ) ) );

$x = undef;
$x = Juman::Result->new( [ map("$_\n",split(/\n/,$result)) ] );
ok( defined $x );
ok( $x->mrph == 5 );

$x = undef;
$x = Juman::Result->new( result => $result, pattern => q/^EOS$/ );
ok( defined $x );
ok( $x->mrph == 5 );
