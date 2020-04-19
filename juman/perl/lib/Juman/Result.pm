# $Id$
package Juman::Result;
require 5.004_04; # For base pragma.
use Juman::Morpheme;
use strict;
use base qw/ Juman::MList /;
use vars qw/ %DEFAULT /;

=head1 NAME

Juman::Result - 形態素解析結果オブジェクト in Juman

=head1 SYNOPSIS

  $result = new Juman::Result( "解析...\n...\nEOS\n" );

=head1 DESCRIPTION

Juman による形態素解析の結果を保持するオブジェクト．

=head1 CONSTRUCTOR

=over 4

=item new ( RESULT )

Juman の出力を行を単位として格納されたリストに対するリファレンス RESULT
を引数として呼び出すと，その形態素解析結果を表すオブジェクトを生成する．

=item new ( OPTIONS )

以下の拡張オプションを指定してコンストラクタを呼び出す．

=over 4

=item result => RESULT

Juman の出力文字列，または，その文字列を行を単位として格納されたリスト
に対するリファレンスを指定する．

=item pattern => STRING

解析結果を終端するためのパターンを指定する．

=item mclass => NAME

形態素オブジェクトを指定する．無指定の場合は，C<Juman::Morpheme> を用
いる．

=cut
%DEFAULT = ( pattern => '^EOS$',
	     mclass  => 'Juman::Morpheme' );

sub new {
    my $class = shift;

    my( %opt ) = %DEFAULT;
    if( @_ == 1 ){
	$opt{result} = shift;
    } else {
	while( @_ ){
	    my $key = shift;
	    my $val = shift;
	    $key =~ s/\A-+//;
	    $opt{lc($key)} = $val;
	}
    }
    my $result  = $opt{result};
    my $pattern = $opt{pattern};
    my $mclass  = $opt{mclass};
    return undef unless( $result and $pattern and $mclass );

    # 文字列が直接指定された場合
    $result = [ map( "$_\n", split( /\n/, $result ) ) ] unless ref $result;

    my $this = {};
    bless $this, $class;

    my( $str );
    while ( defined( $str = shift @$result ) ) {
	if ( $str =~ m!$pattern! and @$result == 0 ) {
	    $this->{_eos} = $str;
	    last;
	} elsif ( $str =~ m!\A\@ \@ \@ [^\@]! ){
	    # 「@」のみからなる未定義語を処理する
	    $this->push_mrph( $mclass->new( $str, scalar($this->mrph) ) );
	} elsif ( $str =~ s!\A\@ !! ) {
	    # 「@」が先頭にあれば同形語
	    ( $this->mrph )[-1]->push_doukei( $mclass->new( $str, scalar($this->mrph) ) );
	} else {
	    $this->push_mrph( $mclass->new( $str, scalar($this->mrph) ) );
	}
    }
    $this->set_readonly();

    $this;
}

=back

=head1 METHODS

本オブジェクトは，形態素列オブジェクト C<Juman::MList> を継承するよう
実装されている．したがって，形態素列オブジェクトに対して有効な以下のメ
ソッドが利用可能である．

=over 4

=item mrph

=cut

=item spec

形態素列の全文字列を返す．Juman による出力と同じ形式の結果が得られる．

=cut
sub spec {
    my( $this ) = @_;
    sprintf( "%s%s",
	     $this->Juman::MList::spec(),
	     $this->{_eos} );
}


=back

=head1 SEE ALSO

=over 4

=item *

L<Juman::Morpheme>

=item *

L<Juman::MList>

=back

=head1 AUTHOR

=over 4

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=cut

# 後方互換性を保つためのメソッド
sub all {
    my( $this ) = @_;
    $this->spec();
}

1;
__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
