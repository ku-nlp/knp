# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 14 }

use KNP::TList;

my $x = new KNP::TList();
ok(defined $x);
ok($x->tag == 0);
ok($x->mrph == 0);

my $m = KNP::Morpheme->new( <<'__koubun_mrph__' );
¹½Ê¸ ¤³¤¦¤Ö¤ó ¹½Ê¸ Ì¾»ì 6 ÉáÄÌÌ¾»ì 1 * 0 * 0 NIL <Ê¸Æ¬><´Á»ú><¤«¤Ê´Á»ú><¼«Î©><Ì¾»ìÁêÅö¸ì>
__koubun_mrph__
ok( defined $m );
$x->push_mrph( $m );
ok( $x->tag == 0 );
ok( $x->mrph == 0 );

my $t = KNP::Tag->new( <<'__tag__', 1 );
+ 2D <SM:ÉÙ»Ô:2110********><Ê¸ÀáÆâ><¿ÍÌ¾><ÂÎ¸À><·¸:ÎÙÀÜ><ÍçÌ¾»ì><¶èÀÚ:0-0><RID:1352><¥¿¥¤¥×-¼çÂÎ><²òÀÏ³Ê-¥¬><²òÀÏ³Ê-¥ò><²òÀÏ³Ê-¥Ë><²òÀÏÏ¢³Ê-¥¬><²òÀÏÏ¢³Ê-³°¤Î´Ø·¸><²òÀÏÏ¢³Ê-¥¬£²>
__tag__
ok( defined $t );
$x->push_tag( $t );
ok( $x->tag == 1 );
ok( $x->mrph == 0 );
ok( $t->mrph == 0 );

$m = KNP::Morpheme->new( <<'__kaiseki_mrph__' );
²òÀÏ ¤«¤¤¤»¤­ ²òÀÏ Ì¾»ì 6 ¥µÊÑÌ¾»ì 2 * 0 * 0 NIL <Ê¸Ëö><É½¸½Ê¸Ëö><´Á»ú><¤«¤Ê´Á»ú><¥µÊÑ><¼«Î©><¢«Ê£¹ç><Ì¾»ìÁêÅö¸ì>
__kaiseki_mrph__
ok( defined $m );
$x->push_mrph( $m );
ok( $x->tag == 1 );
ok( $x->mrph == 1 );
ok( $t->mrph == 1 );
