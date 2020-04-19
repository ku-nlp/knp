# 	$Id$	

package Juman;
require 5.004_04; # For base pragma.
use Carp;
use Juman::Result;
use strict;
use vars qw/ $VERSION %DEFAULT $ENCODING /;
use base qw/ Juman::Process Juman::Hinsi /;

=head1 NAME

Juman -	形態素解析を行うモジュール

=head1 SYNOPSIS

 use Juman;
 $juman = new Juman;
 $result = $juman->analysis( "この文を形態素解析してください．" );
 print $result->all();

=head1 DESCRIPTION

C<Juman> は，形態素解析器 JUMAN を Perl から利用するためのモジュールで
ある．

単純に形態素解析を行うだけならば、C<Juman::Simple> が利用できる．
C<Juman::Simple> は，C<Juman> モジュールのラッパーであり，より簡単に形
態素解析器を利用できるように設計されている．

=head1 CONSTRUCTOR

C<Juman> オブジェクトを生成するコンストラクタは，以下の引数を受け付け
る．

=head2 Synopsis

    $juman = new Juman
               [ -Server        => string,]
               [ -Port          => integer,]
               [ -Command       => string,]
               [ -Timeout       => integer,]
               [ -Option        => string,]
               [ -Rcfile        => filename,]
               [ -IgnorePattern => string,]

=head2 Options

=over 4

各引数の意味は次の通り．

=item -Server

JUMAN サーバーのホスト名．省略された場合は，環境変数 C<JUMANSERVER> で
指定されたサーバーが利用される．環境変数も指定されていない場合は，
Juman を子プロセスとして呼び出す．

=item -Port

サーバーのポート番号．

=item -Command

Juman の実行ファイル名．Juman サーバーを利用しない場合に参照される．

=item -Timeout

サーバーまたは子プロセスと通信する時の待ち時間．

=item -Option

JUMAN を実行する際のコマンドライン引数．省略した場合は，
C<$Juman::DEFAULT{option}> の値が用いられる．

ただし，設定ファイルを指定する C<-r> オプションと，KNP によって無視さ
れる行頭パターンを指定する C<-i> オプションについては，それぞれ個別に 
C<-Rcfile>, C<-IgnorePattern> によって指定するべきである．

=item -Rcfile

JUMAN の設定ファイルを指定するオプション．

このオプションと，Juman サーバーの利用は両立しないことが多い．特に，サー
バーが利用している辞書と違う辞書を指定している設定ファイルは，意図した
通りには動作しない．

=item -IgnorePattern

JUMAN によって無視される行頭パターン．

=back

=head1 METHODS

=over 4

=item analysis( STR )

指定された文字列 STR を形態素解析し，その結果を C<Juman::Result> オブ
ジェクトとして返す．

=item juman ( STR )

C<analysis> の別名．

=back

=head1 ENVIRONMENT

=over 4

=item JUMANSERVER

環境変数 C<JUMANSERVER> が設定されている場合は，指定されているホストを 
Juman サーバーとして利用する．

=back

=head1 SEE ALSO

=over 4

=item *

L<Juman::Result>

=item *

L<Juman::Simple>

=back

=head1 HISTORY

This module is the completely rewritten version of the original module
written by Taku Kudoh <taku-ku@is.aist-nara.ac.jp>.

=head1 AUTHOR

=over 4

=item
TSUCHIYA Masatoshi <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=back

=head1 COPYRIGHT

利用及び再配布については GPL2 または Artistic License に従ってください。

=cut

# バージョン表示
$VERSION = '0.5.7';

# JUMANコマンドの入出力エンコーディング
$ENCODING = 'utf8';

# カスタマイズ用変数
%DEFAULT =
    ( command => &Juman::Process::which_command('juman'),
      server  => $ENV{JUMANSERVER} || '',	# Juman サーバーのホスト名
      port    => 32000,				# Juman サーバーのポート番号
      timeout => 30,				# Juman サーバーの応答の待ち時間
      option  => '',
      rcfile  => (exists($ENV{HOME}) ? $ENV{HOME} : '') . '/.jumanrc',
      mclass  => $Juman::Result::DEFAULT{mclass},
      ignorepattern => '',
    );

# Juman を子プロセスとして実行する場合，標準出力のバッファリングによっ
# て出力が二重にならないようにするためのおまじない
sub BEGIN {
    unless( $DEFAULT{server} ){
	require FileHandle or die "Juman.pm (BEGIN): Can't load module: FileHandle\n";
	STDOUT->autoflush(1);
    }
}

sub new {
    my $class = shift @_;
    my $this = {};
    bless $this, $class;

    if( @_ == 1 ){
	# 旧バージョンの形式で呼び出された場合の処理
	my( $argv ) = @_;
	$this->setup( $argv, \%DEFAULT );
#	$this->setup( { 'option' => $argv }, \%DEFAULT );
    } else {
	# 新しい形式で呼び出された場合の処理
	my( %option ) = @_;
	$this->setup( \%option, \%DEFAULT );
    }

    if( $this->{OPTION}->{rcfile} and $this->{OPTION}->{server} ){
	carp "Rcfile option may not work with Juman server";
    }

    # for jumanpp
    if ($this->{OPTION}->{command} =~ /jumanpp/) {
	delete $this->{OPTION}->{rcfile};
    }
    
    $this;
}

# EUC-JPの3バイトコードを〓に変換
sub conv_3bytecode_to_geta {
    my ($buf) = @_;
    my ($ret_buf);

    while ($buf =~ /([^\x80-\xfe]|[\x80-\x8e\x90-\xfe][\x80-\xfe]|\x8f[\x80-\xfe][\x80-\xfe])/g) {
	my $chr = $1;
	if ($chr =~ /^\x8f/) { # 3byte code (JISX0212)
	    $ret_buf .= '〓';
	}
	else {
	    $ret_buf .= $chr;
	}
    }

    return $ret_buf;
}

sub juman_lines {
    my( $this, $str ) = @_;
    my $socket  = $this->open();
    my $pattern = $this->pattern();
    my @buf;

    # UTFフラグをチェックする
    if (utf8::is_utf8($str)) {
	require Encode;
	if ($ENCODING eq 'euc-jp') {
	    # euc-jpにない文字とJISX0212補助漢字(3バイト)は〓に変換
	    $str = &conv_3bytecode_to_geta(Encode::encode($ENCODING, $str, sub {'〓'}));
	}
	else { # UTF-8のはず
	    $str = Encode::encode($ENCODING, $str);
	}
	$this->{input_is_utf8} = 1;
    }
    else {
	$this->{input_is_utf8} = 0;
    }

    # プロセスに文を送信する
    $str =~ s/[\r\n\f\t]*$/\n/s;
    $socket->print( $str );
    # 解析結果を読み出す
    while( defined( $str = $socket->getline ) ){
	if ($this->{input_is_utf8}) {
	    $str = Encode::decode($ENCODING, $str);
	}
	push( @buf, $str );
	last if $str =~ /$pattern/;
    }
    \@buf;
}
    
# 形態素解析を行うメソッド
sub analysis { &juman(@_); }
sub juman {
    my( $this, $str ) = @_;
    new Juman::Result( result  => &juman_lines( $this, $str ),
		       pattern => $this->pattern(),
		       mclass  => $this->{OPTION}->{mclass} );
}

1;
__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
