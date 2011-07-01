# $Id$
package KNP;
require 5.004_04; # For base pragma.
use Carp;
use English qw/ $LIST_SEPARATOR /;
use Juman;
use KNP::Result;
use strict;
use base qw/ KNP::Obsolete Juman::Process /;
use vars qw/ $VERSION %DEFAULT $ENCODING /;

=head1 NAME

KNP - 構文解析を行うモジュール

=head1 SYNOPSIS

 use KNP;
 $knp = new KNP;
 $result = $knp->parse( "この文を構文解析してください．" );
 print $result->all;

=head1 DESCRIPTION

C<KNP> は，KNP を用いて構文解析を行うモジュールである．

単純に構文解析を行うだけならば、C<KNP::Simple> が利用できる．
C<KNP::Simple> は，C<KNP> モジュールのラッパーであり，より簡単に構文解
析器を利用できるように設計されている．

=head1 CONSTRUCTOR

C<KNP> オブジェクトを生成するコンストラクタは，以下の引数を受け付ける．

=head2 Synopsis

    $knp = new KNP
             [ -Server        => string,]
             [ -Port          => integer,]
             [ -Command       => string,]
             [ -Timeout       => integer,]
             [ -Option        => string,]
             [ -Rcfile        => filename,]
             [ -IgnorePattern => string,]
             [ -JumanServer   => string,]
             [ -JumanPort     => integer,]
             [ -JumanCommand  => string,]
             [ -JumanOption   => string,]

=head2 Options

=over 4

=item -Server

KNP サーバーのホスト名．省略された場合は，環境変数 C<KNPSERVER> で指定
されたサーバーが利用される．環境変数も指定されていない場合は，KNP を子
プロセスとして呼び出す．

=item -Port

KNP サーバーのポート番号．

=item -Command

KNP の実行ファイル名．KNP サーバーを利用しない場合に参照される．

=item -Timeout

サーバーまたは子プロセスと通信する時の待ち時間．

=item -Option

KNP を実行する際のコマンドライン引数．省略した場合は，
C<$KNP::DEFAULT{option}> の値が用いられる．

ただし，設定ファイルを指定する C<-r> オプションと，KNP によって無視さ
れる行頭パターンを指定する C<-i> オプションについては，それぞれ個別に 
C<-Rcfile>, C<-IgnorePattern> によって指定するべきである．

=item -Rcfile

KNP の設定ファイルを指定するオプション．

このオプションと，KNP サーバーの利用は両立しないことが多い．特に，サー
バーが利用している辞書と違う辞書を指定している設定ファイルは，意図した
通りには動作しない．

=item -IgnorePattern

KNP によって無視される行頭パターン．

=item -JumanServer

=item -JumanPort

=item -JumanCommand

=item -JumanOption

=item -JumanRcfile

Juman を呼び出す時のオプションを明示的に指定するためのオプション．

=back

=head1 METHODS

=over 4

=item knp( OBJ )

=item parse( OBJ )

文字列または形態素列オブジェクト OBJ を対象として構文解析を行い，構文
解析結果オブジェクトを返す．

ただし，文字列が空文字列であったり，文字列の先頭の文字が C<#> であった
りした場合には，文字列は無視され undef が返り値となる．

また，構文解析中に致命的なエラーが発生した場合も undef を返す．この場
合は，その直後に C<error> メソッドを用いることによって，実際に発生した
エラーを知ることができる．

したがって，C<parse> メソッドの返り値を，完全かつ安全に処理するために
は，以下のようなプログラムが必要である．

  Example:

    $result = $knp->parse( $str );
    if( $result ){
        # 構文解析が成功した場合
        if( $result->error() ){
            # ただし，構文解析中に何らかのエラーメッセージが
            # 出力された場合
        }
        else {
            # 安全に構文解析が終了した場合
        }
    } else {
        if( $knp->error() ){
            # 構文解析中に致命的なエラーが発生した場合
        }
        else {
            # 対象となる文字列が無視され，処理が行われなかった場合
        }
    }

一般的には以下のようなプログラムで十分だろう．

  Example:

    $result = $knp->parse( $str );
    if( $result ){
        # 構文解析が成功した場合
    }

=item parse_string( STRING )

文字列を対象として構文解析を行い，構文解析オブジェクトを返す．

=item parse_mlist( MLIST )

形態素列オブジェクトを対象として構文解析を行い，構文解析結果オブジェク
トを返す．

=item result

直前の構文解析結果オブジェクトを返す．

=item error

直前の致命的なエラーを返す．

=item detail( [TYPE] )

C<-detail> オプションを指定した場合に限り有効となるメソッド．

=item juman( STRING )

文字列を形態素解析し，形態素解析結果オブジェクトを返す．

=back

=head1 ENVIRONMENT

=over 4

=item KNPSERVER

環境変数 C<KNPSERVER> が設定されている場合は，指定されているホストを 
KNP サーバーとして利用する．

=back

=head1 SEE ALSO

=over 4

=item *

L<KNP::Simple>

=item *

L<KNP::Result>

=back

=head1 HISTORY

This module is the completely rewritten version of the original module
written by Sadao Kurohashi <kuro@i.kyoto-u.ac.jp>.

=head1 AUTHOR

=over 4

=item
TSUCHIYA Masatoshi <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=back

=head1 COPYRIGHT

利用及び再配布については GPL2 または Artistic License に従ってください。

=cut


### バージョン表示
$VERSION = '0.4.9';

# KNPコマンドの入出力エンコーディング
$ENCODING = 'utf8';

# カスタマイズ用変数
%DEFAULT =
    ( command => &Juman::Process::which_command('knp'),
      server  => $ENV{KNPSERVER} || '',		# KNP サーバーのホスト名
      port    => 31000,				# KNP サーバーのポート番号
      timeout => 60,				# KNP サーバーの応答の待ち時間
      option  => '-tab',			# KNP に渡されるオプション
      rcfile  => (exists($ENV{HOME}) ? $ENV{HOME} : '') . '/.knprc',
      bclass  => $KNP::Result::DEFAULT{bclass},
      mclass  => $KNP::Result::DEFAULT{mclass},
      tclass  => $KNP::Result::DEFAULT{tclass}, );
while( my( $key, $value ) = each %Juman::DEFAULT ){
    $DEFAULT{"juman$key"} = $value;
}



#----------------------------------------------------------------------
#		Constructor
#----------------------------------------------------------------------

# KNP を子プロセスとして実行している場合，標準出力のバッファリングによっ
# て出力が二重にならないようにするためのおまじない
sub BEGIN {
    unless( $DEFAULT{server} ){
	require FileHandle or die;
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
    } else {
	# 新しい形式で呼び出された場合の処理
	my( %option ) = @_;
	$this->setup( \%option, \%DEFAULT );
    }

    if( $this->{OPTION}->{rcfile} and $this->{OPTION}->{server} ){
	carp "Rcfile option may not work with KNP server";
    }

    $this;
}

sub close {
    my( $this ) = @_;
    $this->{PREVIOUS} = [];
    $this->Juman::Process::close();
}



#----------------------------------------------------------------------
#		構文解析を行うメソッド
#----------------------------------------------------------------------
sub knp { &parse(@_); }
sub parse {
    my( $this, $object ) = @_;
    if( ref($object) and $object->isa('Juman::MList') ){
	&parse_mlist( $this, $object );
    } else {
	&parse_string( $this, $object );
    }
}

# 文字列を対象として，構文解析を行うメソッド
sub parse_string {
    my( $this, $str ) = @_;
    
    # 空白と改行のみからなる入力文は無視される
    return &_set_error( $this, undef ) if $str =~ m/^\s*$/s;

    # "#" で始まる入力文は無視される
    return &_set_error( $this, undef ) if $str =~ /^\#/;

    &_real_parse( $this,
		  &juman_lines( $this, $str ),
		  $str );
}

# 形態素列オブジェクトを対象として，構文解析を行うメソッド
sub parse_mlist {
    my( $this, $mlist ) = @_;
    &_real_parse( $this,
		  [ $mlist->Juman::MList::spec(), "EOS\n" ],
		  join( '', map( $_->midasi(), $mlist->mrph ) ) );
}

# 実際の構文解析を行う内部関数
sub _real_parse {
    my( $this, $array, $str ) = @_;

    return &_set_error( $this, ";; TIMEOUT is occured when Juman was called.\n" )
	unless( @{$array} );

    # UTFフラグをチェックする
    if (utf8::is_utf8($str)) {
	require Encode;
	foreach my $str (@{$array}) {
	    $str = Encode::encode($ENCODING, $str);
	}
	$this->{input_is_utf8} = 1;
    }
    else {
	$this->{input_is_utf8} = 0;
    }

    # Parse ERROR などが発生した場合に原因を調べるため，構文解析した文
    # の履歴を保存しておく．
    unshift( @{$this->{PREVIOUS}}, $str );
    splice( @{$this->{PREVIOUS}}, 10 ) if @{$this->{PREVIOUS}} > 10;

    # 構文解析
    my @error;
    my $counter = 0;
    my $pattern = $this->pattern();
  PARSE:
    my $sock = $this->open();
    $sock->print( @$array );
    $counter++;

    # 構文解析結果を読み出す
    my( @buf );
    my $skip = ( $this->{OPTION}->{option} =~ /\-detail/ ) ? 1 : 0;
    while( defined( $str = $sock->getline ) ){
	if ($this->{input_is_utf8}) {
	    $str = Encode::decode($ENCODING, $str);
	}
	# 「;; Too many para ()!」「;; Invalid input」などのエラーメッセージ
	if( $str =~ /^;; (?:Too|Invalid) /) {
	    push( @error, $str);
	    $this->close();
	    return &_set_error( $this, join( '', @error ) );
	}

	push( @buf, $str );
	last if $str =~ /$pattern/ and ! $skip--;
    }
#    die "Mysterious error: KNP server or process gives no response" unless @buf;

    # 構文解析結果の最後に EOS のみの行が無い場合は，読み出し中にタイ
    # ムアウトが発生している．
    unless( @buf and $buf[$#buf] =~ /$pattern/ ){
 	if( $counter == 1 ){
 	    push( @error, ";; TIMEOUT is occured.\n" );
	    my $i = $[;
	    push( @error,
		  map( sprintf(";; TIMEOUT:%02d:%s\n",$i++,$_), @{$this->{PREVIOUS}} ) );
 	}
	$this->close();
	goto PARSE if( $counter <= 1 );
	return &_set_error( $this, join( '', @error ) );
    }

    # "Cannot detect consistent CS scopes." というエラーの場合は，KNP 
    # のバグである可能性があるので，一旦 KNP を再起動する．
    if( grep( /^;; Cannot detect consistent CS scopes./, @buf ) ){
 	if( $counter == 1 ){
 	    push( @error, ";; Cannot detect consistent CS scopes.\n" );
	    my $i = $[;
	    push( @error,
		  map( sprintf(";; CS:%02d:%s\n",$i++,$_), @{$this->{PREVIOUS}} ) );
 	}
 	$this->close();
 	goto PARSE if( $counter <= 1 );
    }

    # -detail オプションが指定されている場合
    if( $this->{OPTION}->{option} =~ /\-detail/ ){
	my( $str, @mrph, @bnst );
	while( defined( $str = shift @buf ) ){
	    push( @mrph, $str );
	    last if $str =~ /$pattern/;
	}
	while( defined( $str = shift @buf ) ){
	    if( $str =~ /^#/ ){
		unshift( @buf, $str );
		last;
	    }
	    push( @bnst, $str );
	}
	$this->{DETAIL} = { mrph   => join( '', @mrph ),
			    bnst   => join( '', @bnst ),
			    struct => join( '', @buf ) };
    }

    # 構文解析結果を処理する．
    unshift( @buf, @error );
    &_internal_analysis( $this, \@buf );
}



#----------------------------------------------------------------------
#		形態素解析を行うメソッド
#----------------------------------------------------------------------
sub _new_juman {
    my( $this ) = @_;
    unless( $this->{JUMAN} ){
	my %opt;
	while( my( $key, $value ) = each %{$this->{OPTION}} ){
	    $key =~ s/^juman// and $opt{$key} = $value;
	}
	$this->{JUMAN} = new Juman( %opt );
    }
}

sub juman_lines {
    my( $this, $str ) = @_;
    &_new_juman($this);
    $this->{JUMAN}->juman_lines( $str );
}

sub juman {
    my( $this, $str ) = @_;
    &_new_juman($this);
    $this->{JUMAN}->juman( $str );
}



#----------------------------------------------------------------------
#		構文解析結果を解析する関数
#----------------------------------------------------------------------
sub analysis {
    my( $this, @result ) = @_;
    &_internal_analysis( $this, \@result );
}

sub _internal_analysis {
    my( $this, $result ) = @_;

    my $pattern = $this->{OPTION}->{option} =~ /\-(?:(mrph)?tab|bnst)\b/ ? $this->pattern() : '';
    $result = new KNP::Result( result  => $result,
			       pattern => $pattern,
			       bclass  => $this->{OPTION}->{bclass},
			       mclass  => $this->{OPTION}->{mclass},
			       tclass  => $this->{OPTION}->{tclass} );

    # result メソッドから参照できるように保存
    $this->{RESULT} = $result;

    # NOTE: 内部のハッシュ構造を直接アクセスしているスクリプトの後方互
    # 換性のための小細工
    $this->{ALL}     = $result->all;
    $this->{COMMENT} = $result->comment;
    $this->{ERROR}   = $result->error;
    $this->{MRPH}    = [ $result->mrph ];
    $this->{BNST}    = [ $result->bnst ];

    delete $this->{_fatal_error};
    $result;
}

sub _set_error {
    my( $this, $error ) = @_;

    # 実行結果をリセット
    delete $this->{RESULT};

    # 後方互換性のためのハッシュを削除
    delete $this->{ALL};
    delete $this->{COMMENT};
    delete $this->{ERROR};
    delete $this->{MRPH};
    delete $this->{BNST};

    if( $error ){
	$this->{_fatal_error} = $error;
    } else {
	delete $this->{_fatal_error};
    }
    undef;
}



#----------------------------------------------------------------------
#		構文解析結果を取り出すメソッド
#----------------------------------------------------------------------
sub detail {
    if( @_ == 1 ){
	my( $this ) = @_;
	$this->{DETAIL};
    } elsif( @_ == 2 ){
	my( $this, $type ) = @_;
	if( defined $this->{DETAIL}{$type} ){
	    $this->{DETAIL}{$type};
	} else {
	    carp "Unknown type ($type)";
	    undef;
	}
    } else {
        local $LIST_SEPARATOR = ', ';
        carp "Too many arguments (@_)";
	undef;
    }
}

sub result {
    my( $this ) = @_;
    $this->{RESULT} || undef;
}

sub error {
    my( $this ) = @_;
    $this->{_fatal_error} || undef;
}

1;
__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
