# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 17 }

use KNP::File;

my $file = 't/tag-sample.knp';
my $dbfile = sprintf('%s.%d', $file, $$ );

my $x = new KNP::File( $file );
ok(defined $x);
ok($x->name eq $file);

my( $i, $y, $z );
for( $i = 0; $y = $x->each(); $i++ ){ $z = $y; }
ok( $i == 9 );

$z = $x->each();
ok( join( "", map( $_->midasi, $z->mrph ) ) eq "村山富市首相は年頭にあたり首相官邸で内閣記者会と二十八日会見し、社会党の新民主連合所属議員の離党問題について「政権に影響を及ぼすことにはならない。離党者がいても、その範囲にとどまると思う」と述べ、大量離党には至らないとの見通しを示した。" );
ok( $z->bnst == 27 );
ok( $z->tag == 38 );

$y = ( $z->tag )[0];
ok( ref $y eq 'KNP::Tag' );
ok( ref $y->parent eq 'KNP::Tag' );
ok( $y->parent->id == 1 );

$y = ( $z->tag )[1];
ok( ref $y eq 'KNP::Tag' );
ok( ref $y->parent eq 'KNP::Tag' );
ok( $y->parent->id == 37 );

$y = ( $z->tag )[36];
ok( ref $y eq 'KNP::Tag' );
ok( ref $y->parent eq 'KNP::Tag' );
ok( $y->parent->id == 37 );

$y = ( $z->tag )[-1];
ok( ref $y eq 'KNP::Tag' );
ok( ! $y->parent );
