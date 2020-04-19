# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 4 }

use Juman::Simple;

my $str = "この文を解析してください。";
my $result = &juman( $str );
ok( defined $result );

ok( $result->mrph == 7 );
ok( join('',map($_->midasi,$result->mrph)) eq $str );

my @buf = split( "\n", $result->spec() );
ok( @buf >= 7 );
