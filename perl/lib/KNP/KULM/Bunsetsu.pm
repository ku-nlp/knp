# $Id$
package KNP::KULM::Bunsetsu;
require 5.000;
use Carp;
use strict;

=head1 NAME

KNP::KULM::Bunsetsu - KULM 互換 API

=head1 SYNOPSIS

このクラスをミキシングして使用する．

=head1 DESCRIPTION

C<KULM::KNP::B> 互換のメソッドを C<KNP::Bunsetsu> クラスに追加する．

=head1 METHODS

=over 4

=item get ($attr)

指定された属性を返す．

=cut
sub get {
    my $this = shift;
    my $attr = shift;

    # 互換性を保つため, $m->get( [ F => $j ] ) という形式の指定も受け
    # 付けるようにしている．しかし，この仕様は KULM::KNP::M の仕様とも
    # 整合していないので，バグの可能性が高い．
    if( ref $attr eq 'ARRAY' ){
	( $attr, @_ ) = @{$attr};
    }

    if( $attr eq "ID" ){
	$this->id;
    } elsif( $attr eq "P" ){
	$this->parent;
    } elsif( $attr eq "D" ){
	$this->dpndtype;
    } elsif( $attr eq "C" ){
	[ $this->child ];
    } elsif( $attr eq "ML" ){
	[ $this->mrph_list ];
    } elsif( $attr eq "FS" ){
	$this->fstring;
    } elsif( $attr eq "F" ){
	if( @_ ){
	    ( $this->feature )[ shift ];
	} else {
	    [ $this->feature ];
	}
    } elsif( $attr eq "string" ){
	join( "", map( $_->midasi, $this->mrph_list ) );
    } elsif( $attr eq "p_id" ){
	$this->parent ? $this->parent->id : -1;
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
	map( $this->get($_), qw/ ID p_id D string FS / );
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
    join( $delimiter || " ", grep( defined($_), $this->gets( @_ ? @_ : "string" ) ) );
}

1;

=back

=head1 SEE ALSO

=over 4

=item *

L<KNP::Bunsetsu>

=item *

L<KULM::KNP::B>

=back

=head1 AUTHOR

=over 4

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=cut

__END__
# Local Variables:
# mode: perl
# coding: euc-japan
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
