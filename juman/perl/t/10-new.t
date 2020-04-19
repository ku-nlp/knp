# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 1 }

use Juman;

my $x = new Juman();
ok(defined $x);
