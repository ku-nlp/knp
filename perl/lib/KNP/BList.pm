# $Id$
package KNP::BList;
require 5.004_04; # For base pragma.
use KNP::Bunsetsu;
use KNP::TList;
use strict;
use base qw/ KNP::DrawTree KNP::KULM::BList KNP::KULM::TList Juman::KULM::MList /;

=head1 NAME

KNP::BList - 文節列オブジェクト

=head1 SYNOPSIS

  $result = new KNP::BList();

=head1 DESCRIPTION

文節列を保持するオブジェクト．

=head1 CONSTRUCTOR

=over 4

=item new( @BNST )

指定された文節のリストを保持するオブジェクトを生成する．引数が省略され
た場合は，空文節列を保持するオブジェクトを生成する．

=cut
sub new {
    my $new = bless( {}, shift );
    if( @_ ){
	$new->push_bnst( @_ );
    }
    $new;
}

=back

=head1 METHODS

=over 4

=item bnst ( NUM )

第 I<NUM> 番目の文節を返す．

=item bnst

全ての文節のリストを返す．

=begin comment

C<bnst> メソッドの実体は，C<KNP::KULM::BList> クラスで定義されている．

=end comment

=item bnst_list

全ての文節のリストを返す．

=cut
sub bnst_list {
    my( $this ) = @_;
    if( defined $this->{bnst} ){
	@{$this->{bnst}};
    } else {
	wantarray ? () : 0;
    }
}

=item push_bnst( @BNST )

指定された文節列を文末に追加する．

=cut
sub push_bnst {
    my( $this, @bnst ) = @_;
    if( grep( ! $_->isa('KNP::Bunsetsu'), @bnst ) ){
	die "Illegal type of argument.";
    } elsif( $this->{BLIST_READONLY} ){
	die;
    } else {
	push( @{ $this->{bnst} ||= [] }, @bnst );
    }
}

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
    map( $_->tag_list, shift->bnst_list );
}

=item push_tag( @TAG )

指定されたタグを文末に追加し，その文節のタグ列としての長さを返す．追加
対象となる文節が存在しない(= 文節列が空である)場合は，追加は行われない．

=cut
sub push_tag {
    my $this = shift;
    if( $this->bnst_list ){
	( $this->bnst_list )[-1]->push_tag( @_ );
    } else {
	0;
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
    map( $_->mrph_list, shift->bnst_list );
}

=item push_mrph( @MRPH )

指定された形態素を文末に追加し，その文節の形態素列としての長さを返す．
追加対象となる文節が存在しない(= 文節列が空である)場合は，追加は行われ
ない．

=cut
sub push_mrph {
    my $this = shift;
    if( $this->bnst_list ){
	( $this->bnst_list )[-1]->push_mrph( @_ );
    } else {
	0;
    }
}

=item set_readonly

文節列に対する書き込みを不許可に設定する．

=cut
sub set_readonly {
    my( $this ) = @_;
    for my $bnst ( $this->bnst_list ){
	$bnst->set_readonly();
    }
    $this->{BLIST_READONLY} = 1;
}

=item spec

文節列を文字列に変換する．

=cut
sub spec {
    my( $this ) = @_;
    join( '', map( $_->spec, $this->bnst_list ) );
}

=item draw_tree

=item draw_bnst_tree

文節列の依存関係を木構造として表現して出力する．

=cut
sub draw_bnst_tree {
    shift->draw_tree( @_ );
}

=item draw_tag_tree

タグ列の依存関係を木構造として表現して出力する．

=cut
sub draw_tag_tree {
    my $tlist = KNP::TList->new( shift->tag_list );
    $tlist->set_nodestroy();
    $tlist->draw_tree( @_ );
}

# draw_tree メソッドとの通信用のメソッド．
sub draw_tree_leaves {
    shift->bnst_list( @_ );
}

sub set_nodestroy {
    shift->{BLIST_NODESTROY} = 1;
}

=back

=head1 DESTRUCTOR

文節オブジェクト間に環状のリファレンスが作成されると，通常の Garbage
Collection によってはメモリが回収されなくなる．この問題を避けるために，
明示的にリファレンスを破壊する destructor を定義している．

=cut
sub DESTROY {
    my( $this ) = @_;
    unless( $this->{BLIST_NODESTROY} ){
	grep( ref $_ && $_->isa('KNP::Bunsetsu') && $_->DESTROY, $this->bnst_list );
    }
}

=head1 SEE ALSO

=over 4

=item *

L<KNP::Bunsetsu>

=back

=head1 AUTHOR

=over 4

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=back

=cut

1;
__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
