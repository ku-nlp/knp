# $Id$
package Juman::KULM::Morpheme;
require 5.000;
use Carp;
use strict;

=head1 NAME

Juman::KULM::Morpheme - KULM 互換 API

=head1 SYNOPSIS

このクラスをミキシングして使用する．

=head1 DESCRIPTION

C<KULM::Juman::M> 互換のメソッドを C<Juman::Morpheme> クラスに追加する．

=head1 METHODS

=over 4

=item get ($attr)

指定された属性を返す．

=cut
my %KULM = ( M      => 'midasi',
	     Y      => 'yomi',
	     G      => 'genkei',
	     H1     => 'hinsi',
	     H1_ID  => 'hinsi_id',
	     H2     => 'bunrui',
	     H2_ID  => 'bunrui_id',
	     K1     => 'katuyou1',
	     K1_ID  => 'katuyou1_id',
	     K2     => 'katuyou2',
	     K2_ID  => 'katuyou2_id',
	     I      => 'imis',
	     Doukei => 'doukei' );

sub get {
    my( $this, $attr ) = @_;
    if( defined $KULM{$attr} ){
	$attr = $KULM{$attr};
	$this->$attr();
    } else {
	croak "Unknown attribute: $attr";
    }
}

=item gets (@attr)

指定された属性のリストを返す．C<all> という指定が可能である．

=cut
sub gets {
    my( $this, @attr ) = @_;
    if( $attr[0] eq "all" ){
	map( $this->$_(), @Juman::Morpheme::ATTRS );
    } else {
	map( $this->get($_), @attr );
    }
}

=item string ($delimiter, @attr)

指定された属性を C<$delimiter> で結合した文字列を返す．

=cut
sub string {
    my $this = shift;
    my $delimiter = shift;
    join( $delimiter || " ", grep( defined($_), $this->gets( @_ ? @_ : "all" ) ) );
}

1;

=back

=head1 SEE ALSO

=over 4

=item *

L<Juman::Morpheme>

=item *

L<KULM::Juman::M>

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
