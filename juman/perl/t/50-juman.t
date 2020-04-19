# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 11 }

use Juman;

my $str = "この文を解析してください。";
my $juman = new Juman;
ok( defined $juman );

my $result = $juman->analysis( $str );
ok( defined $result );

ok( $result->mrph == 7 );
ok( join('',map($_->midasi,$result->mrph)) eq $str );

my @buf = split( "\n", $result->spec() );
ok( @buf >= 7 );

$result = $juman->analysis( "「 」を含む文" );
ok( defined $result );
ok( ( $result->mrph )[1]->midasi eq '\ ' );

$result = $juman->analysis( "「\\」を含む文" );
ok( defined $result );
ok( ( $result->mrph )[1]->midasi eq '\\' );

$result = $juman->analysis( "「@」を含む文" );
ok( defined $result );
ok( ( $result->mrph )[1]->midasi eq '@' );
