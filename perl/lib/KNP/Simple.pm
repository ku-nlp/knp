# $Id$
package KNP::Simple;
require 5.004_04; # For base pragma.
use KNP;
use strict;
use base qw/ Exporter /;
use vars qw/ @EXPORT /;
@EXPORT = qw/ knp /;

=head1 NAME

KNP::Simple - 構文解析を行うモジュール

=head1 DESCRIPTION

C<KNP::Simple> は，KNP を用いて構文解析を行う関数 C<knp> を定義するモ
ジュールである．

このモジュールを使うと，C<KNP> モジュールを簡単に，しかし制限された形
で利用することができる．例えば，このモジュールは，最初に作成した 
C<KNP> オブジェクトを再利用するので，オプションの途中変更などはできな
い．より高度な設定で構文解析を行う必要がある場合は，C<KNP> モジュール
を直接呼び出すこと．

=head1 FUNCTION

=over 4

=item knp ($str)

指定された文字列を対象として構文解析を行う関数．C<KNP::Result> オブジェ
クトを返す．

  Example:

    use KNP::Simple;
    $result = &knp( "この文を構文解析してください．" );
    print $result->all();

構文解析のオプションを変更する場合は，C<use> の時点で指定しておく．

  Example:

    use KNP::Simple -Option => "-tab -case2";
    $result = &knp( "この文を構文解析してください．" );
    print $result->all();

オプションには，C<KNP::new> の受け付けるオプションと同じものが指定でき
る．

=cut
my @OPTION;
my $KNP;

sub import {
    my $class = shift;
    @OPTION = @_;
    $class->export_to_level( 1 );
}

sub knp {
    my( $str ) = @_;
    $KNP ||= KNP->new( @OPTION );
    $KNP->parse( $str );
}

1;

=back

=head1 SEE ALSO

=over 4

=item *

L<KNP>

=item *

L<KNP::Result>

=back

=head1 AUTHOR

=over 4

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=cut

__END__
# Local Variables:
# mode: perl
# coding: euc-japan
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
