# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 14 }

use KNP;

my $knp = new KNP;
my $result = $knp->parse( "「 」を含む文" );
ok( $result );
ok( ( $result->mrph )[1]->midasi eq '\ ' );

$result = $knp->parse( "「\\」を含む文" );
ok( $result );
ok( ( $result->mrph )[1]->midasi eq '\\' );

$result = $knp->parse( "赤い花が咲いた。" );
ok( $result );
ok( scalar($result->bnst) == 3 );
ok( ( $result->bnst )[0]->parent->id == 1 );
ok( ( $result->bnst )[1]->child == 1 );
ok( ( ( $result->bnst )[1]->child )[0]->id == 0 );
ok( ( $result->bnst )[1]->parent->id == 2 );
ok( ! defined( ( $result->bnst )[2]->parent ) );
ok( join( '', map( $_->midasi, ( $result->bnst )[0]->mrph ) ) eq '赤い' );
ok( join( '', map( $_->midasi, ( $result->bnst )[1]->mrph ) ) eq '花が' );
ok( join( '', map( $_->midasi, ( $result->bnst )[2]->mrph ) ) eq '咲いた。' );
