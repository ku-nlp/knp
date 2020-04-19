# $Id$
package Juman::Simple;
require 5.000;
use Juman;
use strict;
use base qw/ Exporter /;
use vars qw/ @EXPORT /;
@EXPORT = qw/ juman /;

=head1 NAME

Juman::Simple - 形態素解析を行うモジュール

=head1 DESCRIPTION

C<Juman::Simple> は，Juman を用いて形態素解析を行う関数 C<juman> を定
義するモジュールである．

このモジュールを使うと，C<Juman> モジュールを簡単に，しかし制限された
形で利用することができる．例えば，このモジュールは，最初に作成した 
C<Juman> オブジェクトを再利用するので，オプションの途中変更などはでき
ない．より高度な設定で形態素解析を行う必要がある場合は，C<Juman> モジュー
ルを直接呼び出すこと．

=head1 FUNCTION

=over 4

=item juman ($str)

指定された文字列を対象として形態素解析を行う関数．C<Juman::Result> オ
ブジェクトを返す．

  Example:

    use Juman::Simple;
    $result = &juman( "この文を形態素解析してください．" );
    print $result->all();

形態素解析のオプションを変更する場合は，C<use> の時点で指定しておく．

  Example:

    use Juman::Simple -Option => "-B -e2";
    $result = &juman( "この文を形態素解析してください．" );
    print $result->all();

オプションには，C<Juman::new> の受け付けるオプションと同じものが指定で
きる．

=cut
my @OPTION;
my $JUMAN;

sub import {
    my $class = shift;
    @OPTION = @_;
    $class->export_to_level( 1 );
}

sub juman {
    my( $str ) = @_;
    $JUMAN ||= Juman->new( @OPTION );
    $JUMAN->analysis( $str );
}

1;

=back

=head1 SEE ALSO

=over 4

=item *

L<Juman>

=item *

L<Juman::Result>

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
