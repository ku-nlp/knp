#
# KNPをperlから呼び出すライブラリ (Ver 0.1)
#
#		Masatoshi Tsuchiya (tsuchiya@pine.kuee.kyoto-u.ac.jp)
#		Sadao Kurohasi (kuro@i.kyoto-u.ac.jp)
#

# $this->{ALL}		: 解析結果そのまま
#
# $this->{COMMENT}	: 解析結果の一行目(#ではじまるコメント行)
#
# $this->{MRPH_NUM}	: 形態素数
#
# $this->{MRPH}		: 形態素列
#              [i]	: i番目の形態素
#                 {midasi}	: i番目の形態素の見出し
#                 {yomi}	: i番目の形態素の読み
#                 {genkei}	: i番目の形態素の原型
#                 {hinsi}	: i番目の形態素の品詞
#                 {hinsi_id}	: i番目の形態素の品詞番号
#                 {bunrui}	: i番目の形態素の細分類
#                 {bunrui_id}	: i番目の形態素の細分類番号
#                 {katuyou1}	: i番目の形態素の活用型
#                 {katuyou1_id}	: i番目の形態素の活用型番号
#                 {katuyou2}	: i番目の形態素の活用形
#                 {katuyou2_id}	: i番目の形態素の活用形番号
#                 {imis}	: i番目の形態素の意味
#                 {feature}[j]	: i番目の形態素のj番目のfeature
#
# $this->{BNST_NUM}	: 文節数
#
# $this->{BNST}		: 文節列
#              [i]	: i番目の文節
#                 {start}	: i番目の文節のはじめの形態素番号
#                 {end}		: i番目の文節のさいごの形態素番号
#                 {parent}	: i番目の文節の係り先
#                 {dpndtype}	: i番目の文節の係りのタイプ(D,P,I,A)
#                 {child}	: i番目の文節に係っている文節リスト
#                 {feature}[j]	: i番目の文節のj番目のfeature
#
# 使用例
#
# use KNP;
# $knp = new KNP();
# while ( <STDIN> ) {
#    chomp;
#    $knp->parse($_);
# }


package KNP;
require 5.000;
use FileHandle;
use Juman;
use strict;
use vars qw( $COMMAND $KNP_OPTION $JUMAN_OPTION $JUMAN $VERBOSE );
no strict 'refs';


# プログラム内部で利用される大域変数
$COMMAND = "";				# KNP のパス名
if( $ENV{OS_TYPE} eq "Solaris" ){
    $COMMAND = "/share/tool/knp/system/knp";
} else {
    die "Only Solaris is supported now !";
}
$KNP_OPTION   = "-case2 -tab";		# KNP に渡されるオプション
$JUMAN_OPTION = "-e";			# Juman に渡されるオプション
$VERBOSE      = 0;			# エラーなどが発生した場合に警告させるためには 1 を設定する
$JUMAN        = 0;
my $FH        = "KNP00000";
my $TIMEOUT   = 300;



sub new {
    my( $this, $option );
    if( @_ == 2 ){
	# 引数によって指定されたオプションを利用して KNP を実行する場合
	( $this, $option ) = @_;
    } else {
	# デフォルトのオプションを利用して KNP を実行する場合
	$this   = shift;
	$option = $KNP_OPTION;
    }

    unless( $JUMAN ){
	$JUMAN = new Juman( $JUMAN_OPTION, "grape:1" ) or die;
    }

    $this = { ALL      => "",
	      COMMENT  => "",
	      ERROR    => "",
	      MRPH_NUM => 0,
	      MRPH     => [],
	      BNST_NUM => 0,
	      BNST     => [],
	      PID      => 0,
	      OPTION   => $option,
	      PREVIOUS => [] };

    # -i オプションの対応
    if( $option =~ /\-i +(\S)+/ ){
	my $pat = $1;
	$this->{PATTERN} = "\Q$pat\E";
    }

    bless $this;
    $this;
}


# 呼び出した KNP を終了するメソッド
sub DESTORY {
    my( $this ) = @_;
    &kill_knp( $this ) if( $this->{PID} );
}


# 構文解析を行うメソッド
sub parse {
    my( $this, $input ) = @_;

    $this->{ALL}      = "";
    $this->{COMMENT}  = "";
    $this->{ERROR}    = "";
    $this->{MRPH_NUM} = 0;
    $this->{MRPH}     = [];
    $this->{BNST_NUM} = 0;
    $this->{BNST}     = [];

    my $counter = 0;
  PARSE:
    unless( $this->{PID} ){
	# knp を fork する。
	my( $pid, $write, $read ) = &fork( "$COMMAND $this->{OPTION}" );
	$this->{PID}   = $pid;
	$this->{WRITE} = $write;
	$this->{READ}  = $read;
    }
    $this->{WRITE}->print( $JUMAN->parse( $input ) );
    $counter++;

    # Parse ERROR などが発生した場合に原因を調べるため、構文解析した文
    # の履歴を保存しておく。
    unshift( @{$this->{PREVIOUS}}, $input );
    splice( @{$this->{PREVIOUS}}, 10 );

    # 構文解析結果を読み出す。
    if( $input =~ /^\#/ ){
	# "#" で始まる入力文の場合は解析結果を読み込まずに、単に1を返す。
	$this->{ALL} = $input;
	return 1;
    }
    my $buf = "";
    while( $_ = &read( $this->{READ}, $TIMEOUT ) ){
	$buf .= $_;
	last if( $buf =~ /\nEOS$/ || $this->{PATTERN} && /^$this->{PATTERN}/ );
    }
    $this->{ALL} = $buf;

    # 構文解析結果の最後に EOS のみの行が無い場合は、読み出し中にタイ
    # ムアウトが発生している。
    unless( $buf =~ /\nEOS$/ || $this->{PATTERN} && $buf =~ /\n$this->{PATTERN}/ ){
 	if(( $counter==1 )&&( $VERBOSE )){
 	    print STDERR ";; TIMEOUT is occured.\n";
 	    for( my $i=$[; $this->{PREVIOUS}[$i]; $i++ ){
 		print STDERR sprintf( ";; TIMEOUT:%02d:%s\n", $i, $this->{PREVIOUS}[$i] );
 	    }
 	}
	&kill_knp( $this );
	goto PARSE if( $counter <= 1 );
	return 0;
    }

    # "Cannot detect consistent CS scopes." というエラーの場合は、KNP 
    # のバグである可能性があるので、一旦 KNP を再起動する。
    if( $buf =~ /;; Cannot detect consistent CS scopes.\n/ ){
 	if(( $counter==1 )&&( $VERBOSE )){
 	    print STDERR ";; Cannot detect consistent CS scopes.\n";
 	    for( my $i=$[; $this->{PREVIOUS}[$i]; $i++ ){
 		print STDERR sprintf( ";; CS:%02d:%s\n", $i, $this->{PREVIOUS}[$i] );
 	    }
 	}
 	&kill_knp( $this );
 	goto PARSE if( $counter <= 1 );
    }

    # -tab オプションがない場合は、解析結果を処理しない。
    return 1 if $this->{OPTION} !~ /\-tab/;

    # 構文解析結果を処理する。
    my( $mrph_num, $bnst_num, $f_string );
    ( $this->{COMMENT} ) = ( $buf =~ s/^(\#[^\n]*?)\n// );     # 最初のコメント( # S-ID:1 )を取り除く
    for( split( /\n/,$buf ) ){
	chomp;

	if ( /^EOS$/ || $this->{PATTERN} && /^$this->{PATTERN}/) {
	    $this->{BNST}[$bnst_num - 1]{end} = $mrph_num - 1 if $bnst_num > $[;
	    last;
	}
	elsif (/^;;/) {
	    $this->{ERROR} .= $_;
	}
	elsif (/^\*/) {
	    if ($bnst_num != 0) {
		$this->{BNST}[$bnst_num - 1]{end} = $mrph_num - 1;
	    }
	    $this->{BNST}[$bnst_num]{start} = $mrph_num;
	    if( s/^\* ([\-0-9]+)([DPIA])// ){
		$this->{BNST}[$bnst_num]{parent} = $1;
		$this->{BNST}[$bnst_num]{dpndtype} = $2;
		if( ( $f_string ) = /^ (.+)$/ ){
		    # 文節のfeature
		    $f_string =~ s/^\<//;
		    $f_string =~ s/\>$//g;
		    @{$this->{BNST}[$bnst_num]{feature}} = split(/\>\</, $f_string);
		} else {
		    @{$this->{BNST}[$bnst_num]{feature}} = ();
		}
	    } else {
		$this->{ALL}    = ";; KNP.pm : Illegal output of knp : output=$_\n" . $this->{ALL};
		$this->{ERROR} .= ";; KNP.pm : Illegal output of knp : output=$_\n"
	    }
	    $bnst_num++;
	}
	else {
	    # @{$this->{MRPH}[$mrph_num]} = split;
	    ( $this->{MRPH}[$mrph_num]{midasi},
	      $this->{MRPH}[$mrph_num]{yomi},
	      $this->{MRPH}[$mrph_num]{genkei},
	      $this->{MRPH}[$mrph_num]{hinsi},
	      $this->{MRPH}[$mrph_num]{hinsi_id},
	      $this->{MRPH}[$mrph_num]{bunrui},
	      $this->{MRPH}[$mrph_num]{bunrui_id},
	      $this->{MRPH}[$mrph_num]{katuyou1},
	      $this->{MRPH}[$mrph_num]{katuyou1_id},
	      $this->{MRPH}[$mrph_num]{katuyou2},
	      $this->{MRPH}[$mrph_num]{katuyou2_id},
	      $this->{MRPH}[$mrph_num]{imis},
	      $f_string ) = split;
	    $f_string =~ s/^\<|\>$//g;
	    @{$this->{MRPH}[$mrph_num]{feature}} = split(/\>\</, $f_string);
	    $mrph_num++;
	}
    }

    $this->{MRPH_NUM} = $mrph_num;
    $this->{BNST_NUM} = $bnst_num;
    for my $i ( 0 .. ($this->{BNST_NUM}-2) ) {
	push(@{$this->{BNST}[$this->{BNST}[$i]{parent}]{child}}, $i);
    }

    # 構文解析結果に、;; で始まるエラーメッセージが含まれていた場合
    return 0 if $this->{ERROR};

    return 1;
}


# parse 関数から呼び出される KNP を終了させるためのサブルーチン
sub kill_knp {
    my( $this ) = @_;
    $this->{PREVIOUS} = [];
    $this->{WRITE}->close;
    $this->{READ}->close;
    sleep 1;
    kill 15, $this->{PID};
    sleep 1;
    kill 9, $this->{PID};
    $this->{PID} = 0;
    1;
}


# 指定されたコマンドを fork して、
#     (1) そのコマンドの PID
#     (2) そのコマンドの標準入力のファイルハンドル
#     (3) そのコマンドの標準出力および標準エラー出力のファイルハンドル
# という3つの要素からなる配列を返す関数
sub fork {
    my $command = shift;

    my $parent_read  = ++$FH;
    my $child_write  = ++$FH;
    pipe $parent_read, $child_write;

    my $parent_write = ++$FH;
    my $child_read   = ++$FH;
    pipe $child_read, $parent_write;

  FORK: {
	if( my $pid = fork ){
	    # 親プロセス側の処理
	    close $child_read;
	    close $child_write;
	    $parent_write->autoflush(1);
	    return ( $pid, $parent_write, $parent_read );
	} elsif( defined $pid ){
	    # 子プロセス側の処理
	    close $parent_write;
	    open \*STDIN, "<&$child_read";
	    close $child_read;
	    close $parent_read;
	    open \*STDOUT, ">&$child_write";
	    open \*STDERR, ">&$child_write";
	    close $child_write;
	    exec "$command";
	    exit 0;
	} elsif( $! =~ /No more process/ ){
	    sleep 5;
	    redo FORK;
	} else {
	    die "Can't fork: $!\n";
	}
    }
}


# 指定されたファイルハンドルから、タイムアウトつきで読み込みを行う関数
sub read {
    my( $fh, $timeout ) = @_;
    my $buf;
    $SIG{ALRM} = sub { die "SIGALRM is received\n"; };
    eval {
	alarm $timeout;
	$buf = <$fh>;
	alarm 0;
    };
    if( $@ =~ /SIGALRM is received/ ){
	return undef;
    }
    $buf;
}


1;
