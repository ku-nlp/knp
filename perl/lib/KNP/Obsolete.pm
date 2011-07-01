# $Id$
package KNP::Obsolete;
require 5.003_09; # For UNIVERSAL->can().
use Carp;
use English qw/ $LIST_SEPARATOR /;
use strict;

=head1 NAME

KNP::Obsolete - 後方互換のメソッドを定義する

=head1 SYNOPSIS

このクラスをミキシングして使用する．

=head1 DESCRIPTION

C<KNP::Obsolete> クラスは，C<KNP> モジュールに以前(2001年8月28日版)と
同じ API を追加するクラスである．

=head1 CONSTRUCTOR

このクラスはミキシングして使用するように設計されているため，特別なコン
ストラクタは定義されていない．

=head1 METHODS

=over 4

=item all()

KNP が出力した構文解析結果そのままの文字列を返すメソッド。

=cut
sub all {
    shift->{ALL};
}

=item comment()

KNP が出力した構文解析結果の先頭に含まれるコメントを返すメソッド。

=cut
sub comment {
    shift->{COMMENT};
}

=item mrph_num()

形態素数を返すメソッド。

=cut
sub mrph_num {
    scalar( @{shift->{MRPH}} );
}

=item mrph( [ARG,TYPE,SUFFIX] )

構文解析結果の形態素情報にアクセスするためのメソッド。

Examples:

   $knp->mrph;
   # 引数が省略された場合は、形態素情報のリストに対
   # するリファレンスを返す。

   $knp->mrph( 1 );
   # ARG によって、何番目の形態素の情報を返すかを指
   # 定する。この場合は、1つ目の形態素情報のハッシュ
   # に対するリファレンスを返す。

   $knp->mrph( 2, 'fstring' );
   # TYPE によって必要な形態素情報を指定する。この場
   # 合、2つ目の形態素の全ての feature の文字列を返
   # す。

   $knp->mrph( 3, 'feature', 4 );
   # 3つ目の形態素の4個目の feature を返す。

TYPE として指定することができる文字列は次の通りである。

   midasi
   yomi
   genkei
   hinsi
   hinsi_id
   bunrui
   bunrui_id
   katuyou1
   katuyou1_id
   katuyou2
   katuyou2_id
   imis
   fstring
   feature

第3引数 SUFFIX を取ることができるのは TYPE として feature を指定した場
合に限られる。

=cut
sub mrph {
    my $this = shift;
    unless( @_ ){
	$this->{MRPH};
    }
    else {
	my $i = shift;
	unless( my $mrph = $this->{MRPH}->[$i] ){
	    carp "Suffix ($i) is out of range";
	    undef;
	}
	else {
	    unless( @_ ){
		$mrph;
	    }
	    else {
		my $type = shift;
		if( @_ == 1 and $type eq "feature" ){
		    ( $mrph->feature )[ shift ];
		}
		elsif( @_ ){
		    local $LIST_SEPARATOR = ", ";
		    carp "Too many arguments ($i, $type, @_)";
		    undef;
		}
		elsif( $mrph->can($type) ){
		    $mrph->$type();
		}
		else {
		    carp "Unknown type ($type)";
		    undef;
		}
	    }
	}
    }
}

=item bnst_num()

文節数を返すメソッド。

=cut
sub bnst_num {
    scalar( @{shift->{BNST}} );
}

=item bnst( [ARG,TYPE,SUFFIX] )

構文解析結果の文節に関する情報を取り出すメソッド。

Examples:

   $knp->bnst;
   # 引数が省略された場合は、文節情報のリストに対す
   # るリファレンスを返す。

   $knp->bnst( 1 );
   # ARG によって、何番目の文節の情報を返すかを指定
   # する。この場合は、1つ目の文節情報のハッシュに対
   # するリファレンスを返す。

   $knp->bnst( 2, 'fstring' );
   # TYPE によって必要な文節情報を指定する。この場合、
   # 2つ目の文節の全ての feature の文字列を返す。

   $knp->bnst( 3, 'feature', 4 );
   # 3つ目の文節の4個目の feature を返す。

TYPE として指定することができる文字列は次の通りである。

   start
   end
   parent
   parent_id
   dpndtype
   child
   child_id
   fstring
   feature

第3引数 SUFFIX を取ることができるのは TYPE として feature を指定した場
合に限られる。

=cut
sub bnst {
    my $this = shift;
    unless( @_ ){
	$this->{BNST};
    } else {
	my $i = shift;
	unless( my $bnst = $this->{BNST}->[$i] ){
	    carp "Suffix ($i) is out of range";
	    undef;
	}
	else {
	    unless( @_ ){
		$bnst;
	    }
	    else {
		my $type = shift;
		if( @_ == 1 and $type eq "feature" ){
		    ( $bnst->feature )[ shift ];
		}
		elsif( @_ ){
		    local $LIST_SEPARATOR = ", ";
		    carp "Too many arguments ($i, $type, @_)";
		    undef;
		}
		elsif( $bnst->can($type) ){
		    $bnst->$type();
		}
		elsif ( $type eq 'start' ) {
		    ( $bnst->mrph )[0]->id;
		}
		elsif ( $type eq 'end' ) {
		    ( $bnst->mrph )[-1]->id;
		}
		elsif ( $type eq 'parent_id' ) {
		    if ( my $parent = $bnst->parent ) {
			$parent->id;
		    } else {
			-1;
		    }
		}
		elsif ( $type eq 'child_id' ) {
		    map( $_->id, $bnst->child );
		}
		else {
		    carp "Unknown method ($type)";
		    undef;
		}
	    }
	}
    }
}

sub draw_tree {
    my $blist = KNP::BList->new( @{shift->{BNST}} );
    $blist->set_nodestroy();
    $blist->draw_tree( @_ );
}

=back

=head1 SEE ALSO

=over 4

=item *

L<KNP>

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
