# $Id$
package KNP::KULM::BList;
require 5.000;
use strict;

=head1 NAME

KNP::KULM::BList - KULM 互換 API

=head1 SYNOPSIS

このクラスをミキシングして使用する．

=head1 DESCRIPTION

C<KULM::KNP::Result> 互換のメソッドを C<KNP::BList> クラスに追加する．

=head1 METHODS

=over 4

=item bnst ( NUM )

第 I<NUM> 番目の文節を返す．

=item bnst

全ての文節のリストを返す．

=cut
sub bnst {
    my $this = shift;
    if( @_ ){
	( $this->bnst_list )[ @_ ];
    } else {
	$this->bnst_list;
    }
}

=item bnst_num

文節列の長さを返す．

=cut
sub bnst_num {
    scalar( shift->bnst_list );
}

1;

=back

=head1 SEE ALSO

=over 4

=item *

L<KNP::BList>

=item *

L<KULM::KNP::Result>

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
