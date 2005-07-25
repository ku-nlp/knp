# $Id$
package KNP::DrawTree;
require 5.000;
use Carp;
use strict;

=head1 NAME

KNP::DrawTree - 依存関係の木構造を表示する

=head1 SYNOPSIS

このクラスをミキシングして使用する．

=head1 DESCRIPTION

C<KNP::DrawTree> クラスは，解析単位(文節，タグ)間の依存関係を木構造と
して表示するためのメソッドを提供するクラスである．

=head1 CONSTRUCTOR

このクラスはミキシングして使用するように設計されているため，特別なコン
ストラクタは定義されていない．

=head1 METHODS

=over 4

=item draw_tree ( FILE_HANDLE )

構文木を指定された C<FILE_HANDLE> に出力する．指定を省略した場合は，標
準出力に出力される．

=cut
my %POS_MARK = 
    ( '特殊'     => '*',
      '動詞'     => 'v',
      '形容詞'   => 'j',
      '判定詞'   => 'c',
      '助動詞'   => 'x',
      '名詞'     => 'n',
      '固有名詞' => 'N',	# 特別
      '人名'     => 'J',	# 特別
      '地名'     => 'C',	# 特別
      '組織名'   => 'A',	# 特別
      '指示詞'   => 'd',
      '副詞'     => 'a',
      '助詞'     => 'p',
      '接続詞'   => 'c',
      '連体詞'   => 'm',
      '感動詞'   => '!',
      '接頭辞'   => 'p',
      '接尾辞'   => 's',
      '未定義語' => '?',
    );

sub _leaf_string {
    my( $obj ) = @_;
    my $string;
    for my $mrph ( $obj->mrph() ) {
	$string .= $mrph->midasi();
	if ( $mrph->bunrui() =~ /^(?:固有名詞|人名|地名)$/ ) {
	    $string .= $POS_MARK{$mrph->bunrui()};
	} else {
	    $string .= $POS_MARK{$mrph->hinsi()};
	}
    }
    $string;
}

sub draw_tree {
    my( $this, $fh ) = @_;

    no strict qw/refs/;
    $fh ||= 'STDOUT';			# 指定なしの場合は標準出力を用いる．

    my( $i, $j, $para_row, @item );

    my $limit = scalar($this->draw_tree_leaves);
    my( @active_column ) = 0 x $limit--;

    for $i ( 0 .. ( $limit - 1 ) ){
	$para_row = ( ( $this->draw_tree_leaves )[$i]->dpndtype() eq "P" )? 1 : 0;
	for $j ( ( $i + 1 ) .. $limit ){
	    if ( $j < ( $this->draw_tree_leaves )[$i]->parent->id() ) {
		if ( $active_column[$j] == 2 ) {
		    $item[$i][$j] = ( $para_row ? "╋" : "╂" );
		} elsif ( $active_column[$j] == 1 ) {
		    $item[$i][$j] = ( $para_row ? "┿" : "┼" );
		} else {
		    $item[$i][$j] = ( $para_row ? "━" : "─" );
		}
	    } elsif ( $j == ( $this->draw_tree_leaves )[$i]->parent->id() ) {
		if ( ( $this->draw_tree_leaves )[$i]->dpndtype() eq "P" ) {
		    $item[$i][$j] = "Ｐ";
		} elsif ( ( $this->draw_tree_leaves )[$i]->dpndtype() eq "I" ) {
		    $item[$i][$j] = "Ｉ";
		} elsif ( ( $this->draw_tree_leaves )[$i]->dpndtype() eq "A" ) {
		    $item[$i][$j] = "Ａ";
		} else {
		    if ( $active_column[$j] == 2 ) {
			$item[$i][$j] = "┨";
		    } elsif ( $active_column[$j] == 1 ) {
			$item[$i][$j] = "┤";
		    } else {
			$item[$i][$j] = "┐";
		    }
		}
		if ( $active_column[$j] == 2 ) {
		    ;		# すでにＰからの太線があればそのまま
		} elsif ( $para_row ) {
		    $active_column[$j] = 2;
		} else {
		    $active_column[$j] = 1;
		}
	    } else {
		if ( $active_column[$j] == 2 ) {
		    $item[$i][$j] = "┃";
		} elsif ( $active_column[$j] == 1 ) {
		    $item[$i][$j] = "│";
		} else {
		    $item[$i][$j] = "　";
		}
	    }
	}
    }

    my( @line ) = map( &_leaf_string($_), $this->draw_tree_leaves );
    for $i ( 0 .. $limit ){
	for $j ( ( $i + 1 ) .. $limit ){
	    $line[$i] .= $item[$i][$j];
	}
    }
    my $max_length = ( sort { $b <=> $a; } map( length, @line ) )[0];
    for $i ( 0 .. $limit ){
	my $diff = $max_length - length($line[$i]);
	print $fh ' ' x $diff;
	print $fh $line[$i], ($this->draw_tree_leaves)[$i]->pstring, "\n";
    }
}

=item draw_tree_leaves

木構造の葉となるオブジェクトのリストを返すメソッド．C<KNP::DrawTree> 
クラスを継承するクラスで定義する必要がある．

=cut
sub draw_tree_leaves {
    croak "Undefined method is called";
}

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
# coding: euc-japan
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
