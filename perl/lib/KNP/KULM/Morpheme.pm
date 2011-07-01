# $Id$
package KNP::KULM::Morpheme;
require 5.000;
use strict;
use base qw/ Juman::KULM::Morpheme /;

=head1 NAME

KNP::KULM::Morpheme - KULM 互換 API

=head1 SYNOPSIS

このクラスをミキシングして使用する．

=head1 DESCRIPTION

C<KULM::KNP::M> 互換のメソッドを C<KNP::Morpheme> クラスに追加する．

=head1 METHODS

=over 4

=item get ($attr)

指定された属性を返す．

=cut
sub get {
    my $this = shift;
    my $attr = shift;
    if( $attr eq "FS" ){
	$this->fstring;
    } elsif( $attr eq "F" ){
	if( @_ ){
	    ( $this->feature )[ shift ];
	} else {
	    [ $this->feature ];
	}
    } else {
	$this->SUPER::get( $attr, @_ );
    }
}

=item gets (@attr)

指定された属性のリストを返す．C<all> という指定が可能である．

=cut
sub gets {
    my( $this, @attr ) = @_;
    if( $attr[0] eq "all" ){
	map( $this->$_(), @Juman::Morpheme::ATTRS, @KNP::Morpheme::ATTRS );
    } else {
	map( $this->get($_), @attr );
    }
}

1;

=back

=head1 SEE ALSO

=over 4

=item *

L<KNP::Morpheme>

=item *

L<Juman::Morpheme::KULM>

=item *

L<KULM::KNP::M>

=back

=head1 AUTHOR

=over 4

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=cut

__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
