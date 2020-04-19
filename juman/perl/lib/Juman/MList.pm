# $Id$
package Juman::MList;
require 5.003_07; # For UNIVERSAL->isa().
use strict;
use base qw/ Juman::KULM::MList /;
use Encode;

=head1 NAME

Juman::MList - 形態素列オブジェクト

=head1 SYNOPSIS

  $result = new Juman::MList();

=head1 DESCRIPTION

形態素列を保持するオブジェクト．

=head1 CONSTRUCTOR

=over 4

=item new ( [MRPHS] )

指定された形態素列を保持するオブジェクトを生成する．省略された場合は，
空形態素列を初期値として用いる．

=cut
sub new {
    my $new = bless( {}, shift );
    if( @_ ){
	$new->push_mrph( @_ );
    }
    $new;
}

=back

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

=cut
sub mrph_list {
    my( $this ) = @_;
    if( defined $this->{mrph} ){
	@{$this->{mrph}};
    } else {
	wantarray ? () : 0;
    }
}

=item push_mrph ( @MRPH )

指定された形態素列を文末に追加する．

=cut
sub push_mrph {
    my( $this, @mrph ) = @_;
    $this->{MLIST_READONLY} and die;
    grep( ! $_->isa('Juman::Morpheme'), @mrph ) and die;
    push( @{$this->{mrph}}, @mrph );
}

=item set_readonly

形態素列に対する書き込みを不許可に設定する．

=cut
sub set_readonly {
    my( $this ) = @_;
    $this->{MLIST_READONLY} = 1;
}

# 後方互換性を維持するための別名．
sub set_mlist_readonly {
    shift->set_readonly();
}

=item spec

形態素列の全文字列を返す．Juman による出力と同じ形式の結果が得られる．

=cut
sub spec {
    my( $this ) = @_;
    my $str;
    for my $mrph ( $this->mrph_list() ){
	$str .= $mrph->spec();

	for my $doukei ( $mrph->doukei() ){
	    $str .= '@ ' . $doukei->spec();
	}
    }
    $str;
}

=back

=head1 SEE ALSO

=over 4

=item *

L<Juman::Result>

=item *

L<Juman::Morpheme>

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
