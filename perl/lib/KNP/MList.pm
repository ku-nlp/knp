# $Id$
package KNP::MList;
require 5.003_07; # For UNIVERSAL->isa().
use strict;
use base qw/ Juman::MList /;
use vars qw/ $ENCODING /;
use Encode;

=head1 NAME

KNP::MList - 形態素列オブジェクト

=head1 SYNOPSIS

  $result = new KNP::MList();

=head1 DESCRIPTION

形態素列を保持するオブジェクト．

=head1 CONSTRUCTOR

=over 4

=item new ( [MRPHS] )

指定された形態素列を保持するオブジェクトを生成する．省略された場合は，
空形態素列を初期値として用いる．

=head1 METHODS

=over 4

=item mrph ( NUM )

第 I<NUM> 番目の形態素を返す．

=item mrph

全ての形態素のリストを返す．

=begin comment

C<mrph> メソッドの実体は C<Juman::KULM::MList> で定義されている．

=end comment

=item mrph_list

全ての形態素のリストを返す．

=item push_mrph ( @MRPH )

指定された形態素列を文末に追加する．

=item set_readonly

形態素列に対する書き込みを不許可に設定する．

=item spec

形態素列の全文字列を返す．KNP による出力と同じ形式の結果が得られる．

=cut

$ENCODING = $KNP::ENCODING ? $KNP::ENCODING : 'utf8';

sub spec {
    my( $this ) = @_;
    my $str;
    for my $mrph ( $this->mrph_list() ){
	$str .= $mrph->spec();

	# KNP::Morpheme は fstring に同形が埋め込まれているので
	# 特別な処理は行なわない
    }
    $str;
}

=item repname

形態素列の代表表記を返す．

=cut
sub repname {
    my ( $this ) = @_;
    my $pat = '正規化代表表記';
    if( utf8::is_utf8( $this->fstring ) ){
	$pat = decode($ENCODING, $pat);
    }

    if ( defined $this->{fstring} ){
	if ($this->{fstring} =~ /<$pat:([^\>]+)>/){
	    return $1;
	}
    }
    return undef;
}

=back

=head1 SEE ALSO

=over 4

=item *

L<KNP::Result>

=item *

L<KNP::Morpheme>

=back

=head1 AUTHOR

=over 4

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=cut

1;
__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
