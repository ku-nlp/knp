# $Id$
package KNP::KULM::TList;
require 5.000;
use strict;

=head1 NAME

KNP::KULM::TList - KULM 互換 API

=head1 SYNOPSIS

このクラスをミキシングして使用する．

=head1 DESCRIPTION

C<KULM> 互換のメソッドを C<KNP::TList> クラスに追加する．

=head1 METHODS

=over 4

=item tag ( NUM )

第 I<NUM> 番目のタグを返す．

=item tag

全てのタグのリストを返す．

=cut
sub tag {
    my $this = shift;
    if( @_ ){
	( $this->tag_list )[ @_ ];
    } else {
	$this->tag_list;
    }
}

=item tag_num

タグ列の長さを返す．

=cut
sub tag_num {
    scalar( shift->tag_list );
}

1;

=back

=head1 SEE ALSO

=over 4

=item *

L<KNP::TList>

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
