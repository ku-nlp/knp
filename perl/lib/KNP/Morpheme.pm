# $Id$
package KNP::Morpheme;
require 5.000;
use Exporter;
use Juman::Morpheme;
use KNP::Fstring;
use strict;
use vars qw/ @ISA @EXPORT_OK @ATTRS /;
@ISA = qw/ KNP::Fstring Juman::Morpheme Exporter /;
@EXPORT_OK = qw/ @ATTRS /;

=head1 NAME

KNP::Morpheme - 形態素オブジェクト in KNP

=head1 SYNOPSIS

  $m = new KNP::Morpheme( "解析 かいせき 解析 名詞 6 サ変名詞 2 * 0 * 0 NIL <文頭>", 1 );

=head1 DESCRIPTION

形態素の各種情報を保持するオブジェクト．

=head1 CONSTRUCTOR

=over 4

=item new ( SPEC, ID )

第1引数 C<SPEC> に KNP の出力を代入して呼び出すと，その行の内容を解析
し，相当する形態素オブジェクトを生成する．

=cut

@ATTRS = ( 'fstring' );

sub new {
    my( $class, $spec, $id ) = @_;
    my $this = { id => $id };

    my @value;
    my( @keys ) = @Juman::Morpheme::ATTRS;
    push( @keys, @ATTRS );
    $spec =~ s/\s*$//;
    if( $spec =~ s/^\\ \\ \\ 特殊 1 空白 6 // ){
	@value = ( '\ ', '\ ', '\ ', '特殊', '1', '空白', '6' );
	push( @value, split( / /, $spec, scalar(@keys) - 7 ) );
    } else {
	@value = split( / /, $spec, scalar(@keys) );
    }
    while( @keys and @value ){
	my $key = shift @keys;
	$this->{$key} = shift @value;
    }

    &KNP::Fstring::fstring( $this, $this->{fstring} );
    bless $this, $class;
}

=back

=head1 METHODS

L<Juman::Morpheme> の各メソッドに加えて，KNP によって割り当てられた特
徴文字列を参照するためのメソッドが利用可能である．

=over 4

=item fstring

特徴文字列を返す．

=item feature

特徴のリストを返す．

=item push_feature

特徴を追加する．

=back

これらのメソッドの詳細については，L<KNP::Fstring> を参照のこと．更に，
以下のメソッドが利用可能である．

=over 4

=item spec

形態素の全ての諸元を指示する文字列を生成する．KNP の出力の1行に相当す
る．

=cut

sub spec {
    my( $this ) = @_;
    sprintf( "%s\n", join( ' ', map( $this->{$_}, ( @Juman::Morpheme::ATTRS, @ATTRS ) ) ) );
}

=back

=head1 SEE ALSO

=over 4

=item *

L<KNP::Fstring>

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
# coding: euc-japan
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
