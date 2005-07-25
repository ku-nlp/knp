# $Id$
package KNP::Fstring;
require 5.000;
use Carp;
use strict;

=head1 NAME

KNP::Fstring - 特徴文字列を参照する

=head1 SYNOPSIS

このクラスをミキシングして使用する．

=head1 DESCRIPTION

C<KNP::Fstring> クラスは，特徴文字列を参照するメソッドを提供するクラス
である．

=head1 CONSTRUCTOR

このクラスはミキシングして使用するように設計されているため，特別なコン
ストラクタは定義されていない．

=head1 METHODS

=over 4

=item fstring

=item fstring [ STRING ]

特徴文字列を返す．引数が指定された場合は，指定された文字列を特徴文字列
として代入する．

=cut
sub fstring {
    my $this = shift;
    if( @_ ){
	&set_fstring( $this, @_ );
    } elsif( defined $this->{fstring} ){
	$this->{fstring};
    } else {
	undef;
    }
}

=item feature

=item feature [ STRING... ]

全ての特徴のリストを返す．引数が指定された場合は，指定された特徴のリス
トを代入する．

ただし，空リストを引数として指定することはできないので，特徴を全て消去
するためには使えない．

=cut
sub feature {
    my $this = shift;
    if( @_ ){
	&set_feature( $this, @_ );
    } elsif( defined $this->{feature} ){
	@{$this->{feature}};
    } else {
	wantarray ? () : 0;
    }
}

=item set_fstring [ STRING ]

特徴文字列を設定する．設定された文字列を返す．

=cut
sub set_fstring {
    my( $this, $str ) = @_;
    unless( defined $str ){
	$this->{feature} = [];
	$this->{fstring} = undef;
    } else {
	$str =~ s/\A\s*//;
	$str =~ s/\s*\Z//;
	unless( $str =~ m/\A(<[^<>]*>)*\Z/ ){
	    carp "Illegal feature string: $str";
	    return undef;
	}
	$this->{fstring} = $str;
	$str =~ s/\A<//;
	$str =~ s/>\Z//;
	$this->{feature} = [ split( /></, $str ) ];
	$this->{fstring};
    }
}

=item set_feature [ STRING... ]

特徴のリストを設定する．設定された特徴のリストを返す．

=cut
sub set_feature {
    my $this = shift;
    if( grep( /[<>]/, @_ ) ){
	# <> を含むような特徴文字列は追加できない．
	carp "Illegal feature string: @_";
	return ( wantarray ? () : 0 );
    }
    $this->{feature} = [ @_ ];
    $this->{fstring} = join( '', map( sprintf( '<%s>', $_ ), @_ ) );
    @{$this->{feature}};
}

=item push_feature ( FEATURES )

指定された特徴を追加する．追加後の特徴の数を返す．

=cut
sub push_feature {
    my( $this, @feature ) = @_;
    scalar( $this->set_feature( $this->feature(), @feature ) );
}

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
