# $Id$
package KNP::Depend;
require 5.000;
use strict;

=head1 NAME

KNP::Depend - 依存関係を保持・参照する

=head1 SYNOPSIS

このクラスをミキシングして使用する．

=head1 DESCRIPTION

C<KNP::Depend> クラスは，解析単位(文節，タグ)間の依存関係を保持，操作
するためのメソッドを提供するクラスである．

=head1 CONSTRUCTOR

このクラスはミキシングして使用するように設計されているため，特別なコン
ストラクタは定義されていない．

=head1 METHODS

=over 4

=item parent

係り先を返す．

=item parent [ UNIT ]

係り先を設定する．

=cut
sub parent {
    my $this = shift;
    if( @_ ){
	$this->{parent} = shift;
    } elsif( defined $this->{parent} ){
	$this->{parent};
    } else {
	undef;
    }
}

=item child

係り元のリストを返す．

=item child [ UNIT... ]

係り元を設定する．

=cut
sub child {
    my $this = shift;
    if( @_ ){
	$this->{child} = [ @_ ];
	@{$this->{child}};
    } elsif( defined $this->{child} ){
	@{$this->{child}};
    } else {
	wantarray ? () : 0;
    }
}

=item dpndtype

依存関係の種類(D,P,I,A)を返す．

=item dpndtype [ STRING ]

依存関係の種類を設定する．

=cut
sub dpndtype {
    my $this = shift;
    if( @_ ){
	$this->{dpndtype} = shift;
    } elsif( defined $this->{dpndtype} ){
	$this->{dpndtype};
    } else {
	undef;
    }
}

=item id

解析単位の ID を返す．無指定の場合は -1 を返す．

=item id [ STRING ]

解析単位の ID を設定する．

=cut
sub id {
    my $this = shift;
    if( @_ ){
	$this->{id} = shift;
    } elsif( defined $this->{id} ){
	$this->{id};
    } else {
	-1;
    }
}

=item pstring

=item pstring [ STRING ]

解析単位の I<pstring> 属性の値を得る．引数が指定された場合は，その引数
を代入する．この属性は，C<KNP::DrawTree::draw_tree> メソッドから参照さ
れる．

=cut
sub pstring {
    my $this = shift;
    if( @_ ){
	$this->{_pstring} = shift;
    } elsif( defined $this->{_pstring} ){
	$this->{_pstring};
    } else {
	undef;
    }
}

=back

=head1 INTERNAL METHODS

以下のメソッドは，解析単位のリストを保持するオブジェクト
(C<KNP::BList>, C<KNP::TList>)のコンストラクタから呼び出されることを想
定しているメソッドである．一般の利用は推奨されない．

=over 4

=item parent_id

係り先単位の ID を返す．

=item parent_id [ STRING ]

係り先単位の ID を設定する．

=cut
sub parent_id {
    my $this = shift;
    if( @_ ){
	my $value = shift;
	if( my $parent = $this->parent() ){
	    $parent->id( $value );
	} elsif( defined $value ){
	    $this->{_parent_id} = $value;
	} else {
	    # 未定義値が指定された場合は，ハッシュから値を取り除く．
	    delete $this->{_parent_id};
	    $value;
	}
    } elsif( my $parent = $this->parent() ){
	$parent->id();
    } elsif( defined $this->{_parent_id} ){
	$this->{_parent_id};
    } else {
	-1;
    }
}

=item make_reference( LISTREF )

解析単位の係り先が正しくリファレンスとして参照されるように，オブジェク
トの内部情報を修正する．係り先単位が含まれるリストに対するリファレンス
を引数として呼び出す．

=cut
sub make_reference {
    my( $this, $list ) = @_;
    if( my $parent_id = $this->parent_id() ){
	$this->parent_id( undef );
	if( $parent_id >= 0 ){
	    my $parent = $list->[ $parent_id + $[ ];
	    $this->{parent} = $parent;
	    push( @{$parent->{child}}, $this );
	}
    }
}

=back

=head1 DESTRUCTOR

C<make_reference> メソッドによって環状のリファレンスが作成されると，通
常の Garbage Collection によっては，メモリが回収されなくなる．この問題
を避けるために，明示的にリファレンスを破壊する destructor を定義してい
る．

=cut
sub DESTROY {
    my( $this ) = @_;
    delete $this->{parent};
    delete $this->{child};
}

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
