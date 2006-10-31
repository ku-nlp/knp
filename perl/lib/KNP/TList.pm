# $Id$
package KNP::TList;
require 5.004_04; # For base pragma.
use KNP::Tag;
use strict;
use base qw/ KNP::DrawTree KNP::KULM::TList Juman::KULM::MList /;

=head1 NAME

KNP::TList - タグ列オブジェクト

=head1 SYNOPSIS

  $result = new KNP::TList();

=head1 DESCRIPTION

KNP による格解析の単位であるタグのリストを保持するオブジェクト．

=head1 CONSTRUCTOR

=over 4

=item new( @TAG )

指定されたタグのリストを保持するオブジェクトを生成する．引数が省略され
た場合は，空のリストを保持するオブジェクトを生成する．

=cut
sub new {
    my $new = bless( {}, shift );
    if( @_ ){
	$new->push_tag( @_ );
    }
    $new;
}

=back

=head1 METHODS

=over 4

=item tag ( NUM )

第 I<NUM> 番目のタグを返す．

=item tag

全てのタグのリストを返す．

=begin comment

C<tag> メソッドの実体は，C<KNP::KULM::TList> クラスで定義されている．

=end comment

=item tag_list

全てのタグのリストを返す．

=cut
sub tag_list {
    my( $this ) = @_;
    if( defined $this->{tag} ){
	@{$this->{tag}};
    } else {
	wantarray ? () : 0;
    }
}

=item push_tag( @TAG )

指定されたタグをタグ列に追加する．

=cut
sub push_tag {
    my( $this, @tag ) = @_;
    if( grep( ! $_->isa('KNP::Tag'), @tag ) ){
	die "Illegal type of argument.";
    } elsif( $this->{TLIST_READONLY} ){
	die;
    } else {
	push( @{ $this->{tag} ||= [] }, @tag );
    }
}

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
    map( $_->mrph_list, shift->tag_list );
}

=item push_mrph( @MRPH )

指定された形態素を文末に追加し，そのタグの形態素列としての長さを返す．
追加対象となるタグが存在しない(= タグ列が空である)場合は，追加は行われ
ない．

=cut
sub push_mrph {
    my( $this, @mrph ) = @_;
    if( $this->tag_list ){
	( $this->tag_list )[-1]->push_mrph( @mrph );
    } else {
	0;
    }
}

=item set_readonly

タグ列に対する書き込みを不許可に設定する．

=cut
sub set_readonly {
    my( $this ) = @_;
    for my $tag ( $this->tag_list ){
	$tag->set_readonly();
    }
    $this->{TLIST_READONLY} = 1;
}

=item spec

タグ列を文字列に変換する．

=cut
sub spec {
    my( $this ) = @_;
    join( '', map( $_->spec, $this->tag_list ) );
}

=item draw_tree

=item draw_tag_tree

タグ列の依存関係を木構造として表現して出力する．

=cut
sub draw_tag_tree {
    shift->draw_tree( @_ );
}

# draw_tree メソッドとの通信用のメソッド．
sub draw_tree_leaves {
    shift->tag_list( @_ );
}

sub set_nodestroy {
    shift->{TLIST_NODESTROY} = 1;
}

=back

=head1 DESTRUCTOR

タグオブジェクト間に環状のリファレンスが作成されると，通常の Garbage
Collection によってはメモリが回収されなくなる．この問題を避けるために，
明示的にリファレンスを破壊する destructor を定義している．

=cut
sub DESTROY {
    my( $this ) = @_;
    unless( $this->{TLIST_NODESTROY} ){
	grep( ref $_ && $_->isa('KNP::Tag') && $_->DESTROY, $this->tag_list );
    }
}

=head1 SEE ALSO

=over 4

=item *

L<KNP::Tag>

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
# coding: euc-japan
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
