# $Id$
package Juman::Process;
require 5.000;
use English qw/ $PERL_VERSION /;
use IO::Socket::INET;
use Juman::Fork;
use strict;

=head1 NAME

Juman::Process - プロセスオブジェクト

=head1 METHODS

=over 4

=item setup( OPTION, DEFAULT )

ユーザーの指定するオプションのハッシュに対するリファレンスと，デフォル
ト値のハッシュに対するリファレンスを引数として呼び出すと，インスタンス
変数を適切に設定する．

=item open

子プロセスを生成し，そのプロセスと通信するソケットを返す．

=item close

子プロセスとの通信ソケットを閉じる．

=item pattern

形態素解析結果/構文解析結果の終端を検出するための正規表現パターンを取
り出す．

=head1 STRUCTURE

以下の内部変数が、メンバとしてハッシュに格納されている。

    $this->{OPTION}     オプションへのハッシュ
    $this->{SOCKET}     JUMANと通信するソケットへのハッシュ
    $this->{PATTERN}    解析結果の終了を検出するための正規表現

=cut

# インスタンス変数を設定するメソッド
sub setup {
    my( $this, $option, $default ) = @_;

    # ユーザーによって指定されたオプションを対象として，以下の正規化を行う
    #   (1) 文字列先頭の - を取り除く
    #   (2) 全て小文字に統一する
    my %opt;
    while( my( $key, $value ) = each %$option ){
	$key =~ s/^-+//;
	$opt{lc($key)} = $value;
    }

    # ユーザーによって指定されたオプションと，デフォルト値を混合して，
    # 実際のオプションの連想配列を作成する．この時，デフォルト値の存在
    # しないオプション(= 不正なオプション)は，単に無視される．
    while( my( $key, $value ) = each %$default ){
	if( defined $opt{$key} ){
	    $this->{OPTION}->{$key} = $opt{$key};
	} elsif( $value ){
	    $this->{OPTION}->{$key} = $value;
	}
    }

    # -Command オプションが指定された場合は -Server オプションを無視する．
    if( $opt{command} ){
	delete $this->{OPTION}->{server};
    } elsif( $this->{OPTION}->{server} ){
	delete $this->{OPTION}->{command};
    }
    if( $opt{jumancommand} ){
	delete $this->{OPTION}->{jumanserver};
    } elsif( $this->{OPTION}->{jumanserver} ){
	delete $this->{OPTION}->{jumancommand};
    }

    if( my $argv = $this->{OPTION}->{option} ){
	# 設定ファイルをコマンドラインオプションとして指定した場合
	if( $argv =~ s/\-r\s+(\S+)\s*// ){
	    die "Conflicted option." if defined $this->{OPTION}->{rcfile};
	    $this->{OPTION}->{rcfile} = ( $opt{rcfile} = $1 );
	}
	# Juman が解析時に無視する行のパターンをコマンドラインオプショ
	# ンとして指定した場合
	if( $argv =~ s/\-i\s+(\S+)\s*// ){
	    die "Conflicted option." if defined $this->{OPTION}->{ignorepattern};
	    $this->{OPTION}->{ignorepattern} = ( $opt{ignorepattern} = $1 );
	}
	$this->{OPTION}->{option} = $argv;
    }

    my $rcfile = $this->{OPTION}->{rcfile};
    unless( $rcfile and -r $rcfile ){
	die "Can't read initialize file($rcfile): $!\n" if $opt{rcfile};
	delete $this->{OPTION}->{rcfile};
    }

    $rcfile = $this->{OPTION}->{jumanrcfile};
    unless( $rcfile and -r $rcfile ){
	die "Can't read initialize file($rcfile): $!\n" if $opt{jumanrcfile};
	delete $this->{OPTION}->{jumanrcfile};
    }

    if( defined $this->{OPTION}->{ignorepattern} ){
	$this->{PATTERN}
	    = sprintf( '(?:^EOS$|^%s)', quotemeta $this->{OPTION}->{ignorepattern} );
    } else {
	$this->{PATTERN} = '^EOS$';
    }
}

# 引数を生成する内部関数
sub generate_option {
    my( $this, $remote ) = @_;
    my $option = $this->{OPTION}->{option};
    # Juman が解析時に無視する行のパターンを引数に追加する
    if( defined $this->{OPTION}->{ignorepattern} ){
	$option .= sprintf( ' -i %s', $this->{OPTION}->{ignorepattern} );
    }
    # プロセスをローカルのマシンで実行する場合は、設定ファイルを引数で
    # 指定する必要がある
    unless( $remote ){
	if( my $rcfile = $this->{OPTION}->{rcfile} ){
	    $option .= sprintf( ' -r %s', $rcfile  );
	}
    }
    $option;
}

# ネットワーク上のサーバーとの通信を開始する内部関数
sub open_remote_socket {
    my( $this ) = @_;

    my $host = $this->{OPTION}->{server};
    return undef unless $host;

    my $port = $this->{OPTION}->{port};
    my $sock = new IO::Socket::INET( PeerAddr => $host,
				     PeerPort => $port,
				     Proto => 'tcp' )
	or die "Can't connect server: host=$host, port=$port\n";
    $sock->timeout( $this->{OPTION}->{timeout} );
#    &set_encoding( $sock );

    # サーバーの greeting message を確認する
    my $res;
    ( $res = $sock->getline ) =~ /^200/
	or die "Illegal response: host=$host, port=$port, response=$res\n";

    # 設定ファイルを送信する
    if( my $rcfile = $this->{OPTION}->{rcfile} ){
	open( RC, "< $rcfile" )
	    or die "Can't open initialize file($rcfile): $!\n";
	$sock->print( "RC\n", <RC>, "\n", pack("c",0x0b), "\n" );
	close RC;
	( $res = $sock->getline ) =~ /^200/
	    or die "Configuration error: rcfile=$rcfile, response=$res\n";
    }

    # サーバーにコマンドラインオプションを渡す
    my $option = $this->generate_option( 'remote' );
    $sock->print( "RUN $option\n" );
    ( $res = $sock->getline ) =~ /^200/
	or die "Configuration error: option=$option, response=$res\n";

    # 生成されたソケットを記録しておく
    $this->{SOCKET}->{REMOTE} = $sock;
}

# ローカルマシン上で子プロセスを実行する内部関数
sub open_local_socket {
    my( $this ) = @_;

    # juman/knp が server-client mode で動作しないようにしている．
    local %ENV = %ENV;
    delete $ENV{JUMANSERVER};
    delete $ENV{KNPSERVER};

    my $command = $this->{OPTION}->{command};
    my $option = $this->generate_option();
    my $sock = new Juman::Fork( $command, $option )
	or die "Can't fork: command=$command, option=$option\n";
    $sock->timeout( $this->{OPTION}->{timeout} );
    $this->{SOCKET}->{LOCAL} = $sock;
}

# ソケットを生成するメソッド
sub open {
    my( $this ) = @_;
    $this->{SOCKET}->{REMOTE}
	or $this->{SOCKET}->{LOCAL}
	    or $this->open_remote_socket()
		or $this->open_local_socket();
}

# ソケットを閉じるメソッド
sub close {
    my( $this ) = @_;
    my $fh;
    if( $fh = $this->{SOCKET}->{REMOTE} ){
	$fh->print( pack("c",0x0b) . "\nQUIT\n" );
	$fh->close;
    } elsif( $fh = $this->{SOCKET}->{LOCAL} ){
	if( $fh->alive ){
	    $fh->close;
	    if ( $fh->alive ) {
		# Call waitpid() to avoid zombie.
		$fh->kill;
	    }
	}
    }
    delete $this->{SOCKET};
    1;
}

sub DESTROY {
    my( $this ) = @_;
    $this->close();
}

sub pattern {
    my( $this ) = @_;
    $this->{PATTERN} || undef;
}

sub which_command {
    my( $bin ) = @_;
    for my $p ( split( /:/, $ENV{PATH} ) ){
	return "$p/$bin" if -x "$p/$bin" and -f "$p/$bin";
    }
}

=head1 MEMO

Perl-5.8 以降の場合，ネットワーク上のサーバーとの通信には， 
C<encoding> プラグマで指定された文字コードが使われます．

=cut
BEGIN {
    if( $PERL_VERSION > 5.008 ){
	require Juman::Encode;
	Juman::Encode->import( qw/ set_encoding / );
    } else {
	*{Juman::Process::set_encoding} = sub { undef; };
    }
}

1;
__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
