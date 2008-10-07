# $Id$
package KNP::Morpheme;
require 5.004_04; # For base pragma.
use strict;
use base qw/ KNP::Fstring KNP::KULM::Morpheme Juman::Morpheme /;
use vars qw/ @ATTRS /;
use Juman::Hinsi qw/ get_hinsi get_bunrui get_type get_form /;
use Encode;

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

sub _alt2spec {
    my( $str ) = @_;
    my( $midasi, $yomi, $genkei, $hinsi_id, $bunrui_id, $katuyou1_id, $katuyou2_id, $imis ) = split( '-', $str , 8);
    my $hinsi = &get_hinsi( $hinsi_id );
    my $bunrui = &get_bunrui( $hinsi_id, $bunrui_id );
    my $katuyou1 = &get_type( $katuyou1_id );
    my $katuyou2 = &get_form( $katuyou1_id, $katuyou2_id );
    if( utf8::is_utf8( $str ) ){
	$hinsi = decode('euc-jp', $hinsi);
	$bunrui = decode('euc-jp', $bunrui);
	$katuyou1 = decode('euc-jp', $katuyou1);
	$katuyou2 = decode('euc-jp', $katuyou2);
    }

    return join( ' ', $midasi, $yomi, $genkei, $hinsi, $hinsi_id, $bunrui, $bunrui_id, 
		 $katuyou1, $katuyou1_id, $katuyou2, $katuyou2_id, $imis );
}

sub new {
    my( $class, $spec, $id ) = @_;
    my $this = { id => $id };

    # ALTは標準のJUMAN形式に変換する
    if ($spec =~ /^ALT-(.+)/){
	$spec = _alt2spec($1);
    }

    my @value;
    my( @keys ) = @Juman::Morpheme::ATTRS;
    push( @keys, @ATTRS );
    $spec =~ s/\s*$//;
    if( $spec =~ s/^\\ \\ \\ 特殊 1 空白 6 // ){
	@value = ( '\ ', '\ ', '\ ', '特殊', '1', '空白', '6' );
	push( @value, split( / /, $spec, scalar(@keys) - 7 ) );
    } else {
#	@value = split( / /, $spec, scalar(@keys) );

	# 意味情報は""でくくられている
	
	# 以下のような場合に対応するために正規表現を修正

	# あった あった あう 動詞 2 * 0 子音動詞ワ行 12 タ形 8 "代表表記:会う" <代表表記:会う><品曖><ALT-あった-あった-あう-2-0-12-8-"付属動詞候補（基本） 代表表記:合う"><ALT-あった-あった-ある-2-0-10-8-"補文ト 代表表記:有る"><原形曖昧><品曖-動詞><品曖-その他><付属動詞候補基本><かな漢字><ひらがな><活用語><独立語><自立><タグ単位始><文節始>

	while ($spec =~ s/\"([^\"\s]+)(\s)([^\"]+)\"/\"$1\@\@$3\"/) {
	    ;
	}
	@value = split( / /, $spec);
	$value[11] =~ s/\@\@/ /g;
	$value[12] =~ s/\@\@/ /g;

#	@value = &quotewords(" ", 1, $spec);
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

=item repname

形態素の代表表記を返す．

=cut

sub repname {
    my ( $this ) = @_;

    my $result = $this->Juman::Morpheme::repname;
    return $result if ( defined $result );

    my $pat = '(疑似)代表表記';
    if( utf8::is_utf8( $this->midasi ) ){
	$pat = decode('euc-jp', $pat);
    }

    if ( defined $this->{fstring} ){
	if ($this->{fstring} =~ /<$pat:([^\>]+)>/){
	    return $2;
	}
    }
    return undef;
}

=back

=item spec

形態素の全ての諸元を指示する文字列を生成する．KNP の出力の1行に相当す
る．

=cut

sub spec {
    my( $this ) = @_;
    sprintf( "%s\n", join( ' ', map( $this->{$_}, ( @Juman::Morpheme::ATTRS, @ATTRS ) ) ) );
}

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
