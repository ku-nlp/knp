# $Id$
package Juman::Fork;
require 5.004_04; # For base pragma.
use English qw/ $PERL_VERSION /;
use IO::Handle;
use IO::Pipe;
use POSIX;
use Time::HiRes;
use strict;
use base qw/ Exporter /;
use vars qw/ @EXPORT_OK $TIMEOUT /;
@EXPORT_OK = qw/ $TIMEOUT /;

=head1 NAME

Juman::Fork - 非同期に実行される子プロセスを生成する

=head1 SYNOPSIS

 use Juman::Fork;
 $p = new Juman::Fork( "sort" );
 $p->print( "abc\n", "def\n", "ace\n" );
 $p->close;
 while( $_ = $p->getline ){
     print;
 }

=head1 DESCRIPTION

C<Juman::Fork> は，指定されたコマンドを fork して子プロセスとして実行
し，その標準入力への書き込みと，標準出力及び標準エラー出力からの読み出
しを行うためのモジュールです．

=head1 CONSTRUCTOR

=over 4

=item new ( COMMAND [,ARGV] )

C<Juman::Fork> オブジェクトを生成します．子プロセスとして実行するコマ
ンドを第1引数に指定し，第2引数以降にそのコマンドに対するコマンドライン
オプションを指定します．

Example:

   $p = new Juman::Fork( "cat" "-n" );

=back

=head1 METHODS

=over 4

=item print( [STR,] )

引数によって指定された文字列を子プロセスの標準入力に渡すメソッドです．

=item printf( FORMAT [,ARG] )

第1引数によって指定された書式に従って，指定された文字列を子プロセスの
標準入力に渡すメソッドです．

=item getline()

子プロセスの標準出力及び標準エラー出力から1行分のデータを取り出すメソッ
ドです．C<timeout> によって設定された時間以内に読み出されなければ，
C<undef> を返します．

=item timeout( VAL )

子プロセスの出力を C<getline> メソッドによって取り出す場合のタイムアウ
ト時間を設定するメソッドです．タイムアウト時間の初期値には変数 
C<$Juman::Fork::TIMEOUT> の値が使われます．

=item alive()

子プロセスが残っているか調べるメソッドです．

=item pid()

子プロセスの PID を返すメソッドです．

=item close()

子プロセスの標準入力と連結されているパイプを閉じるメソッドです．

=item kill()

子プロセスを強制終了するメソッドです．

=back

=head1 MEMO

Perl-5.8 以降の場合，子プロセスとの通信には， C<encoding> プラグマで指
定された文字コードが使われます．

=cut
BEGIN {
    if( $PERL_VERSION > 5.008 ){
	require Juman::Encode;
	Juman::Encode->import( qw/ set_encoding / );
    } else {
	*{Juman::Fork::set_encoding} = sub { undef; };
    }
}

=head1 AUTHOR

TSUCHIYA Masatoshi <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=cut

# デフォルトのタイムアウト時間
$TIMEOUT = 60;

# 指定されたコマンドを子プロセスとして fork する
sub new {
    my( $this, @argv ) = @_;
    ( @argv >= 1 ) || die 'Usage: $p = new Juman::Fork( command, [arguments] )';

    my $read  = new IO::Pipe;
    my $write = new IO::Pipe;

  FORK: {
	if( my $pid = fork ){
	    # 親プロセス側の処理
	    $read->reader;
	    $write->writer;
#	    &set_encoding( $read );
#	    &set_encoding( $write );
	    $this = {
		     PID     => $pid,
		     READ    => $read,
		     WRITE   => $write,
		     TIMEOUT => $TIMEOUT,
		    };
	    bless $this;
	    return $this;
	} elsif( defined $pid ){
	    # 子プロセス側の処理
	    $write->reader;
	    $read->writer;
	    STDOUT->fdopen( $read, "w" );
	    STDERR->fdopen( $read, "w" );
	    STDIN->fdopen( $write, "r" );
	    exec join( " ", @argv );
	    exit 0;
	} elsif( $! =~ /No more process/ ){
	    sleep 5;
	    redo FORK;
	} else {
	    die "Can't fork: $!\n";
	}
    }
}


# 子プロセスの標準入力に文字列を書き込む
sub print {
    my $this = shift;
    $this->{WRITE}->print( @_ );
    $this->{WRITE}->flush;		# 明示的にフラッシュする
    1;
}


# 子プロセスの標準入力に対する書式付き出力
sub printf {
    my $this = shift;
    my $fmt  = shift;
    $this->{WRITE}->print( sprintf( $fmt, @_ ) );
    $this->{WRITE}->flush;		# 明示的にフラッシュする
    1;
}


# 子プロセスの標準入力を閉じる関数
sub close {
    my( $this ) = @_;
    if( $this and $this->{WRITE} ){
	$this->{WRITE}->print( "\004" ); # 先に Ctrl-D を送っておく
	$this->{WRITE}->close;
    }
}


# タイムアウトの時間を設定する関数
sub timeout {
    my( $this, $timeout ) = @_;
    $this->{TIMEOUT} = eval $timeout;
}


# 子プロセスの標準出力と標準エラー出力からタイムアウトつきで読み出す
sub getline {
    my( $this ) = @_;
    my $buf = "";
    local $SIG{ALRM} = sub { die "SIGALRM is received\n"; };
    eval {
	alarm $this->{TIMEOUT};
	$buf = $this->{READ}->getline;
	alarm 0;
    };
    if( $@ =~ /SIGALRM is received/ ){
	return undef;
    }
    $buf;
}


# 子プロセスの PID を返す関数
sub pid {
    my( $this ) = @_;
    $this->{PID};
}


# 子プロセスがまだ生きているか調べる関数
sub alive {
    my( $this ) = @_;
    ( waitpid( $this->{PID},&POSIX::WNOHANG ) == 0 ) && ( $? == -1 );
}


# 子プロセスを強制終了する関数
sub kill {
    my( $this ) = @_;
    $this->close;
#    sleep 1;
    kill 15, $this->{PID};
    Time::HiRes::sleep 0.01;
    kill 9, $this->{PID};
    $this->alive();			# To avoid zombie.
    $this->{PID} = 0;
    1;
}

1;
__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
