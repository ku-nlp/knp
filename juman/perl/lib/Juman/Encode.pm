# $Id$
package Juman::Encode;
require 5.008;
use strict;
use base qw/ Exporter /;
our @EXPORT_OK = qw/ encode decode set_encoding /;

=head1 NAME

Juman::Encode - character encoding functions

=head1 DESCRIPTION

Perl-5.8.x は内部文字コードとして Unicode を採用している．そのため，日
本語 EUC を使っているプロセスとの入出力を行う場合や，日本語 EUC で記述
されたデータを参照する場合には，常に明示的に encode/decode を行う必要
がある．

このライブラリでは，そのための関数を定義している．

=head1 FUNCTIONS

=over 4

=item $octets = encode ( $string )

文字列を，C<encoding> プラグマで指定されている文字コードで encode して，
バイト列を得る．

=cut
sub encode {
    my( $string ) = @_;
    if( $string and ${^ENCODING} ){
	${^ENCODING}->encode( $string );
    } else {
	$string;
    }
}

=item $string = decode ( $octets )

バイト列を，C<encoding> プラグマで指定されている文字コードで decode し
て，文字列を得る．

=cut
sub decode {
    my( $string ) = @_;
    if( $string and ${^ENCODING} ){
	${^ENCODING}->decode( $string );
    } else {
	$string;
    }
}

=item set_encoding ( $handle )

指定されたファイルハンドルとの通信に，C<encoding> プラグマで指定されて
いる文字コードを使うように設定する．

=cut
sub set_encoding {
    my( $fh ) = @_;
    if( ${^ENCODING} ){
	my $name = ${^ENCODING}->name();
	binmode( $fh, ":encoding($name)" );
    }
}

1;

=back

いずれの関数も，C<encoding> プラグマが指定されていない場合には，何もし
ない．

=head1 MEMO

このライブラリは，変数 C<${^ENCODING}> を参照する必要のある関数の定義
を，一ヶ所にまとめるために導入した．なぜならば，変数 C<${^ENCODING}> 
を参照しようとするコードは，Jperl-5.005 では syntax error となるためで
ある．

  Sample Script:
    use English qw/ $PERL_VERSION /;
    if( $PERL_VERSION > 5.008 ){
        ${^ENCODING}->encode( ... );
    }

  Error Message:
    syntax error at sample.perl line 3, near "{^"

=head1 SEE ALSO

=over 4

=item *

L<encoding>

=item *

L<Encode>

=back

=head1 AUTHOR

=over 4

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=cut

__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
