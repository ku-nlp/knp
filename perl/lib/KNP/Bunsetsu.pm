# $Id$
package KNP::Bunsetsu;
require 5.004_04; # For base pragma.
use Carp;
use KNP::Morpheme;
use strict;
use base qw/ KNP::Depend KNP::Fstring KNP::TList KNP::KULM::Bunsetsu KNP::MList /;

=head1 NAME

KNP::Bunsetsu - 文節オブジェクト in KNP

=head1 SYNOPSIS

  $b = new KNP::Bunsetsu( "* -1D <BGH:解析>" );

=head1 DESCRIPTION

KNP による係り受け解析の単位である文節の各種情報を保持するオブジェクト．

=head1 CONSTRUCTOR

=over 4

=item new ( SPEC, ID )

第1引数 C<SPEC> に KNP の出力を代入して呼び出すと，その行の内容を解析
し，相当する文節オブジェクトを生成する．

=cut
sub new {
    my( $class, $spec, $id ) = @_;
    my $new = bless( {}, $class );

    $spec =~ s/\s*$//;
    if( $spec eq '*' ){
	$new->id( $id );
    } elsif( my( $parent_id, $dpndtype, $fstring ) = ( $spec =~ m/^\* (-?\d+)([DPIA])(.*)$/ ) ){
	$new->id( $id );
	$new->parent_id( $parent_id );
	$new->dpndtype( $dpndtype );
	$new->fstring( $fstring );
    } else {
	die "KNP::Bunsetsu::new(): Illegal spec = $spec\n";
    }
    $new;
}

=back

=head1 METHODS

1つの文節は複数の形態素からなるため，文節オブジェクトは形態素列オブジェ
クト C<KNP::MList> を継承している．したがって，形態素列を取り出すた
めの C<mrph> メソッドが利用可能である．

また，格解析を行った場合は，1つの文節が複数のタグからなる場合もある．
そのため，文節オブジェクトはタグ列オブジェクト C<KNP::TList> を継承し，
タグのリストを取り出すための C<tag> メソッドが利用可能である．

=over 4

=item mrph_list

文節に含まれる全ての形態素を返す．

=cut
sub mrph_list {
    my $this = shift;
    if( $this->tag ){
	$this->KNP::TList::mrph_list( @_ );
    } else {
	$this->KNP::MList::mrph_list( @_ );
    }
}

=item push_mrph ( @MRPH )

指定された形態素を文節に追加する．

=cut
sub push_mrph {
    my $this = shift;
    if( $this->tag ){
	$this->KNP::TList::push_mrph( @_ );
    } else {
	$this->KNP::MList::push_mrph( @_ );
    }
}

=item push_tag ( @TAG )

指定されたタグを文節に追加する．

=cut
sub push_tag {
    my $this = shift;
    if( $this->KNP::MList::mrph_list ){
	# この文節には，タグに含まれていない形態素が既に存在しているの
	# で，データの矛盾を引き起こす可能性がある．
	carp "Unsafe addition of tags";
    }
    $this->KNP::TList::push_tag( @_ );
}

=back

文節間の依存関係に関する情報を保持・操作するために，C<KNP::Depend> ク
ラスを継承している．したがって，以下のメソッドが利用可能である．

=over 4

=item parent

係り先文節を返す．

=item child

この文節に係っている文節のリストを返す．

=item dpndtype

係り受け関係の種類(D,P,I,A)を返す．

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

文節オブジェクトを文字列に変換する．

=cut
sub spec {
    my( $this ) = @_;
    sprintf( "* %d%s %s\n%s",
	     $this->parent() ? $this->parent->id() : -1,
	     $this->dpndtype(),
	     $this->fstring(),
	     ( $this->tag ? $this->KNP::TList::spec() : $this->KNP::MList::spec() ) );
}

=back

=head1 DESTRUCTOR

文節オブジェクトは，デストラクタを定義している2種類のオブジェクト 
C<KNP::Depend>, C<KNP::TList> を継承している．両方のデストラクタをきち
んと呼び出さないと，メモリリークの原因となる．

=cut
sub DESTROY {
    my( $this ) = @_;
    $this->KNP::TList::DESTROY();
    $this->KNP::Depend::DESTROY();
}

=head1 SEE ALSO

=over 4

=item *

L<KNP::Depend>

=item *

L<KNP::Fstring>

=item *

L<KNP::TList>

=item *

L<KNP::MList>

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
