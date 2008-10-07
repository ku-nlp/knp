# $Id$
package KNP::Tag;
require 5.004_04; # For base pragma.
use KNP::Morpheme;
use strict;
use base qw/ KNP::Depend KNP::Fstring Juman::MList /;
use Encode;

=head1 NAME

KNP::Tag - タグオブジェクト in KNP

=head1 SYNOPSIS

  $b = new KNP::Tag( "+ 1D <解析格-ニ>", 0 );

=head1 DESCRIPTION

格解析の単位となるタグの各種情報を保持するオブジェクト．

=head1 CONSTRUCTOR

=over 4

=item new ( SPEC, ID )

第1引数 C<SPEC> に KNP の出力を代入して呼び出すと，その行の内容を解析
し，相当するタグオブジェクトを生成する．

=cut
sub new {
    my( $class, $spec, $id ) = @_;
    my $new = bless( {}, $class );

    $spec =~ s/\s*$//;
    if( $spec eq '+' ){
	$new->id( $id );
    } elsif( my( $parent_id, $dpndtype, $fstring ) = ( $spec =~ m/^\+ (-?\d+)(\w)(.*)$/ ) ){
	$new->id( $id );
	$new->dpndtype( $dpndtype );
	$new->parent_id( $parent_id );
	$new->fstring( $fstring );
    } else {
	die "KNP::Tag::new(): Illegal spec = $spec\n";
    }
    $new;
}

=back

=head1 METHODS

1つのタグは，複数の形態素からなる．したがって，タグオブジェクトは，形
態素列オブジェクト C<Juman::MList> を継承するように実装され，形態素列
を取り出すための C<mrph> メソッドが利用可能である．

タグ間の依存関係に関する情報を保持・操作するために，C<KNP::Depend> ク
ラスを継承している．したがって，以下のメソッドが利用可能である．

=over 4

=item parent

係り先タグを返す．

=item child

このタグに係っているタグのリストを返す．

=item id

コンストラクタを呼び出すときに指定された ID を返す．無指定の場合は -1 
を返す．

=back

KNP によって割り当てられた特徴文字列を保持・参照するために，
C<KNP::Fstring> クラスを継承している．したがって，以下のメソッドが利用
可能である．

=over 4

=item fstring

特徴文字列を返す．

=item feature

特徴のリストを返す．

=item push_feature

特徴を追加する．

=back

加えて，以下のメソッドが定義されている．

=over 4

=item spec

タグオブジェクトを文字列に変換する．

=cut
sub spec {
    my( $this ) = @_;
    sprintf( "+ %d%s %s\n%s",
	     $this->parent() ? $this->parent->id() : -1,
	     $this->dpndtype(),
	     $this->fstring(),
	     $this->SUPER::spec() );
}

=item synnodes

SynNodesの情報を返す．

=cut
sub synnodes {
    my ( $this ) = @_;

    if( defined $this->{synnodes} ){
	@{$this->{synnodes}};
    } else {
	wantarray ? () : 0;
    }
}

=back

=head1 SEE ALSO

=over 4

=item *

L<KNP::Depend>

=item *

L<KNP::Fstring>

=item *

L<Juman::MList>

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
# coding: euc-japan
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
