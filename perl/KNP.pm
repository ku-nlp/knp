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
use vars qw( $COMMAND $KNP_OPTION $JUMAN_OPTION $JUMAN $VERBOSE $HOST );


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
$HOST         = "grape";



sub BEGIN {
    # 出力が二重にならないようにするためのおまじない
    STDOUT->autoflush(1);
}


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

    if( $HOST ){
	require IO::Socket::INET or die;
    } else {
	require Fork or die;
    }

    $this = { ALL      => "",
	      COMMENT  => "",
	      ERROR    => "",
	      MRPH_NUM => 0,
	      MRPH     => [],
	      BNST_NUM => 0,
	      BNST     => [],
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
    &kill_knp( $this ) if( $this->{KNP}->alive );
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
    # knp を fork する。
    if( $HOST ){
	unless( $this->{KNP} ){
	    my $sock = new IO::Socket::INET( PeerAddr => $HOST, PeerPort => 31000, Proto => 'tcp' )
		|| die "KNP.pm: Can't connect server: host=$HOST\n";
	    $sock->timeout( 60 );
	    my $tmp = $sock->getline;
	    ( $tmp =~ /^200/ ) || die "KNP.pm: Illegal message: host=$HOST, msg=$tmp\n";
	    $sock->print( sprintf( "RUN %s\n", $this->{OPTION} ) );
	    $tmp = $sock->getline;
	    ( $tmp =~ /^200/ ) || die "KNP.pm: Configuration error: host=$HOST, msg=$tmp\n";
	    $this->{KNP} = $sock;
	}
    } else {
	unless( $this->{KNP} && $this->{KNP}->alive ){
	    $this->{KNP} = new Fork( $COMMAND, $this->{OPTION} ) || die "KNP.pm: Can't fork: command=$COMMAND\n";
	    $this->{KNP}->timeout( 60 );
	}
    }
    my( @juman ) = $JUMAN->parse( $input );
    $this->{KNP}->print( @juman );
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
    while( $_ = $this->{KNP}->getline ){
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
    if( $HOST ){
	$this->{KNP}->close;
    } else {
	$this->{KNP}->kill;
    }
    delete $this->{KNP};
    1;
}


1;
