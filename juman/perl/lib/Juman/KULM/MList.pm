# $Id$
package Juman::KULM::MList;
require 5.000;
use strict;

=head1 NAME

Juman::KULM::MList - KULM 互換 API

=head1 SYNOPSIS

このクラスをミキシングして使用する．

=head1 DESCRIPTION

C<KULM::Juman::MLMixin> 互換のメソッドを C<Juman::MList> クラスに追加
する．

=head1 METHODS

=over 4

=item mrph ( NUM )

第 I<NUM> 番目の形態素を返す．

=item mrph

全ての形態素のリストを返す．

=cut
sub mrph {
    my $this = shift;
    if( @_ ){
	( $this->mrph_list )[ @_ ];
    } else {
	$this->mrph_list;
    }
}

=item mrph_num

形態素列の長さを返す．

=cut
sub mrph_num {
    scalar( shift->mrph_list );
}

1;

=back

=head1 SEE ALSO

=over 4

=item *

L<Juman::MList>

=item *

L<KULM::Juman::MLMixin>

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
