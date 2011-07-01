# $Id$
package KNP::SynNode;
require 5.004_04; # For base pragma.
use strict;
use vars qw/ $ENCODING /;
use Encode;

=head1 NAME

KNP::SynNode - SynNode in KNP

=head1 DESCRIPTION

SynNodeの各種情報を保持するオブジェクト．

=cut

$ENCODING = $KNP::ENCODING ? $KNP::ENCODING : 'utf8';

sub new {
    my( $class, $str ) = @_;
    my $this = {};

    my $score_pat = 'スコア';
    if( utf8::is_utf8( $str ) ){
	$score_pat = decode($ENCODING, $score_pat);
    }

    # ! 1 <SYNID:近い/ちかい><スコア:1>
    # ! 1 <SYNID:s199:親しい/したしい><スコア:0.99>
    # ! 1 <SYNID:s1201:所在/しょざい><スコア:0.693><上位語><下位語数:323>
    my ($tagid, $string) = (split(' ', $str))[1,2];
    my @tagids = split(',', $tagid);

    my ($synid, $score, @features);
    while ($string =~ /(<.+?>)/g) {
	my $s = $1;
	# SYNID
	if ($s =~ /<SYNID:(.+?)>/) {
	    $synid = $1;
	}
	elsif ($s =~ /<$score_pat:(.+?)>/) {
	    $score = $1;
	}
	# その他はfeature
	else {
	    push @features, $s;
	}
    }

    $this->{synid} = $synid;
    $this->{tagid} = $tagid;
    $this->{tagids} = \@tagids;
    $this->{score} = $score;
    $this->{feature} = join('', @features);

    bless $this, $class;
}

=head1 METHODS

以下のメソッドが利用可能である。

=over 4

=item tagid

対応する基本句IDを返す。

=cut
sub tagid {
    my ($this) = @_;
    $this->{tagid};
}

=item tagids

対応する基本句ID(配列)を返す。

=cut
sub tagids {
    my ($this) = @_;
    @{$this->{tagids}};
}

=item synid

SynIDを返す。

=cut
sub synid {
    my ($this) = @_;
    $this->{synid};
}

=item score

スコアを返す。

=cut
sub score {
    my ($this) = @_;
    $this->{score};
}

=item feature

文法素性を返す。

=cut
sub feature {
    my ($this) = @_;
    $this->{feature};
}

=back

=head1 AUTHOR

=over 4

=item
柴田 知秀 <shibata@nlp.kuee.kyoto-u.ac.jp>

=cut

1;
__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
