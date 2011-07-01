# -*- perl -*-

package MyBunsetsu;
use Exporter;
use Juman::MList;
use KNP::Bunsetsu;
use strict;
use vars qw/ @ISA /;
@ISA = qw/ KNP::Bunsetsu Exporter /;

# 文節を自立語列と付属語列に分割するメソッド
sub split {
    my( $this ) = @_;
    my @mrph = $this->mrph;
    my @buf;
    while ( @mrph and $mrph[0]->fstring !~ /<付属>/ ) {
	push( @buf, shift @mrph );
    }
    ( new Juman::MList( @buf ), new Juman::MList( @mrph ) );
}

package main;
use strict;
use Test;

BEGIN { plan tests => 3 }

use KNP;

my $knp = new KNP( bclass => 'MyBunsetsu' );
my $result = $knp->parse( "赤い花が咲いた。" );
ok( $result );
if( ($result->bnst)[1]->can('split') ){
    my( $jiritsu, $huzoku ) = ( $result->bnst )[1]->split();
    ok( join( '', map( $_->midasi, $jiritsu->mrph ) ) eq "花" );
    ok( join( '', map( $_->midasi, $huzoku->mrph ) ) eq "が" );
} else {
    ok(0);
    ok(0);
}
