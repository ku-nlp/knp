# -*- perl -*-

use strict;
use English qw/ $WARNING /;
use Test;

BEGIN { plan tests => 6 }

use Juman::Morpheme;

my $mrph = Juman::Morpheme->new( "動か うごか 動く 動詞 2 * 0 子音動詞カ行 2 未然形 3\n" );
ok(defined $mrph);

my $new;
{
    local $WARNING = 0;
    $new = $mrph->change_katuyou2( '存在しない活用形' );
}
ok(!defined $new);

$new = $mrph->change_katuyou2( '命令形' );
ok(defined $new);
ok($new->midasi() eq '動け' );
#ok($new->katuyou2_id == 5);

$new = $mrph->kihonkei();
ok(defined $new);
ok($new->midasi() eq '動く' );
#ok($new->katuyou2_id == 2);
