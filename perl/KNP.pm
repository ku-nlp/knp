# -*- perl -*-
#
# KNP を perl から呼び出すライブラリ
#
#	Masatoshi Tsuchiya (tsuchiya@pine.kuee.kyoto-u.ac.jp)
#	Sadao Kurohasi (kuro@i.kyoto-u.ac.jp)

# このモジュールを使用する方法は、
#
#     perldoc KNP
#
# というコマンドで参照することができます。

# KNP オブジェクトは、以下に説明されているような要素を持つハッシュです。
# また、ここに記述した以外に、動作時に必要な情報を保持する要素が含まれ
# ます。ハッシュの内容を不用意に書き換えると誤動作しますので、なるべく
# メソッド経由で情報を取り出すようにしてください。

#     $this->{ALL}          : 解析結果そのまま
#
#     $this->{COMMENT}      : 解析結果の一行目(#ではじまるコメント行)
#
#     $this->{MRPH_NUM}     : 形態素数
#
#     $this->{MRPH}         : 形態素列
#                  [i]      : i番目の形態素
#                     {midasi}      : i番目の形態素の見出し
#                     {yomi}        : i番目の形態素の読み
#                     {genkei}      : i番目の形態素の原型
#                     {hinsi}       : i番目の形態素の品詞
#                     {hinsi_id}    : i番目の形態素の品詞番号
#                     {bunrui}      : i番目の形態素の細分類
#                     {bunrui_id}   : i番目の形態素の細分類番号
#                     {katuyou1}    : i番目の形態素の活用型
#                     {katuyou1_id} : i番目の形態素の活用型番号
#                     {katuyou2}    : i番目の形態素の活用形
#                     {katuyou2_id} : i番目の形態素の活用形番号
#                     {imis}        : i番目の形態素の意味
#                     {fstring}     : i番目の形態素の全てのfeature
#                     {feature}[j]  : i番目の形態素のj番目のfeature
#
#     $this->{BNST_NUM}     : 文節数
#
#     $this->{BNST}         : 文節列
#                  [i]      : i番目の文節
#                     {start}       : i番目の文節のはじめの形態素番号
#                     {end}         : i番目の文節のさいごの形態素番号
#                     {parent}      : i番目の文節の係り先
#                     {dpndtype}    : i番目の文節の係りのタイプ(D,P,I,A)
#                     {child}       : i番目の文節に係っている文節リスト
#                     {fstring}     : i番目の文節の全てのfeature
#                     {feature}[j]  : i番目の文節のj番目のfeature


package KNP;
require 5.000;
use Juman;
use strict;
use vars qw( $COMMAND $KNP_OPTION $JUMAN_OPTION $JUMAN_SERVER $JUMAN $VERBOSE $HOST $VERSION $MRPH_TYPE $BNST_TYPE );


# プログラム内部で利用される大域変数
$COMMAND      = "/share/tool/knp/system/knp";
$HOST         = "grape";		# KNP サーバーのホスト名 ( サーバーを利用しない場合は空にしておく )
$KNP_OPTION   = "-case2 -tab";		# KNP に渡されるオプション
$JUMAN_OPTION = "-e";			# Juman に渡されるオプション
$JUMAN_SERVER = "grape:1";		# Juman サーバー
$VERBOSE      = 0;			# エラーなどが発生した場合に警告させるためには 1 を設定する
$JUMAN        = 0;
$VERSION      = sprintf("%d.%02d", q$Revision$ =~ /(\d+)\.(\d+)/);

$MRPH_TYPE    = '^(?:midasi|yomi|genkei|hinsi|hinsi_id|bunrui|bunrui_id|katuyou1|katuyou1_id|katuyou2|katuyou2_id|imis|fstring|feature)$';
$BNST_TYPE    = '^(?:start|end|parent|dpndtype|child|fstring|feature)$';



sub Version { $VERSION; }


sub BEGIN {
    unless( $HOST ){
	# KNP を子プロセスとして実行している場合、標準出力のバッファリ
	# ングによって出力が二重にならないようにするためのおまじない
	require FileHandle or die;
	STDOUT->autoflush(1);
    }
}


sub DESTORY {
    my( $this ) = @_;
    &kill_knp( $this ) if $this->{KNP};
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
	$JUMAN = new Juman( $JUMAN_OPTION, $JUMAN_SERVER ) || die "KNP.pm: Can't make JUMAN object\n";
    }

    if( $HOST ){
	require IO::Socket::INET or die "KNP.pm: Can't load module: IO::Socket::INET\n";
    } else {
	require Fork or die "KNP.pm: Can't load module: Fork\n";
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
		    $this->{BNST}[$bnst_num]{fstring} = $f_string;
		    $f_string =~ s/^\<|\>$//g;
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
	    $this->{MRPH}[$mrph_num]{fstring} = $f_string;
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
	$this->{KNP}->print( pack("c",0x0b)."\n" );
	$this->{KNP}->close;
    } else {
	$this->{KNP}->alive && $this->{KNP}->kill;
    }
    delete $this->{KNP};
    1;
}


sub all {
    my( $this ) = @_;
    $this->{ALL};
}

sub comment {
    my( $this ) = @_;
    $this->{COMMENT};
}

sub mrph_num {
    my( $this ) = @_;
    $this->{MRPH_NUM};
}

sub mrph {
    my $this = shift;
    unless( @_  ){
	$this->{MRPH};
    } else {
	my $i = shift;
	( $i =~ /[^0-9]/ )
	    and warn( "KNP.pm (mrph): Integer is required: arg=$i\n" ), return undef;
	( $i >= ( $this->{MRPH_NUM} + $[ ) )
	    and warn( "KNP.pm (mrph): Argument overflow: arg=$i\n" ), return undef;
	unless( @_ ){
	    $this->{MRPH}[$i];
	} else {
	    my $x = shift;
	    unless( @_ ){
		( $x =~ /$MRPH_TYPE/o )
		    or warn( "KNP.pm (mrph): Unknown type is specified: arg=$i, type=$x\n" ), return undef;
		$this->{MRPH}[$i]{$x};
	    } else {
		my $j = shift;
		( $j =~ /[^0-9]/ )
		    and warn "KNP.pm (mrph): Integer is required: arg=$i, type=$x, suffix=$j\n", return undef;
		( $x eq 'feature' )
		    or warn "KNP.pm (mrph): Illegal type is specified: arg=$i, type=$x, suffix=$j\n", return undef;
		$this->{MRPH}[$i]{$x}[$j];
	    }
	}
    }
}
    
sub bnst_num {
    my( $this ) = @_;
    $this->{BNST_NUM};
}

sub bnst {
    my $this = shift;
    unless( @_  ){
	$this->{BNST};
    } else {
	my $i = shift;
	( $i =~ /[^0-9]/ )
	    and warn( "KNP.pm (bnst): Integer is required: arg=$i\n" ), return undef;
	( $i >= ( $this->{BNST_NUM} + $[ ) )
	    and warn( "KNP.pm (bnst): Argument overflow: arg=$i\n" ), return undef;
	unless( @_ ){
	    $this->{BNST}[$i];
	} else {
	    my $x = shift;
	    unless( @_ ){
		( $x =~ /$BNST_TYPE/o )
		    or warn( "KNP.pm (bnst): Unknown type is specified: arg=$i, type=$x\n" ), return undef;
		$this->{BNST}[$i]{$x};
	    } else {
		my $j = shift;
		( $j =~ /[^0-9]/ )
		    and warn "KNP.pm (bnst): Integer is required: arg=$i, type=$x, suffix=$j\n", return undef;
		( $x eq 'feature' )
		    or warn "KNP.pm (bnst): Illegal type is specified: arg=$i, type=$x, suffix=$j\n", return undef;
		$this->{BNST}[$i]{$x}[$j];
	    }
	}
    }
}

1;


__END__

=head1 NAME

KNP - 構文解析を行うモジュール

=head1 SYNOPSIS

 use KNP;
 $knp = new KNP;
 $knp->parse( "この文を構文解析してください。" );
 print $knp->all;

=head1 DESCRIPTION

C<KNP> は、構文解析器 knp を Perl から利用するためのモジュールです。

=head1 CONSTRUCTOR

=over 4

=item new ( [OPTION] )

C<KNP> オブジェクトを生成します。引数に指定された文字列を knp を実行す
る場合のオプションとして利用します。

Examples:

   $knp = new KNP;
   $knp = new KNP( "" );
   $knp = new KNP( "-case2 -tab -helpsys" );

引数が省略された場合は、"-case2 -tab" をオプションとして knp を実行し
ます。

=back

=head1 METHODS

=over 4

=item parse( STRING )

STRING の構文解析を行います。

=item all()

knp が出力した構文解析結果そのままの文字列を返すメソッドです。

=item comment()

knp が出力した構文解析結果の1行目に含まれるコメントを返すメソッドです。

=item mrph_num()

形態素数を返すメソッドです。

=item mrph( [ARG,TYPE,SUFFIX] )

構文解析結果の形態素情報にアクセスするためのメソッドです。

Examples:

   $knp->mrph;
   # 引数が省略された場合は、形態素情報のリストに対
   # するリファレンスを返す。

   $knp->mrph( 1 );
   # ARG によって、何番目の形態素の情報を返すかを指
   # 定する。この場合は、1つ目の形態素情報のハッシュ
   # に対するリファレンスを返す。

   $knp->mrph( 2, 'fstring' );
   # TYPE によって必要な形態素情報を指定する。この場
   # 合、2つ目の形態素の全ての feature の文字列を返
   # す。

   $knp->mrph( 3, 'feature', 4 );
   # 3つ目の形態素の4個目の feature を返す。

TYPE として指定することができる文字列は次の通りです。

   midasi
   yomi
   genkei
   hinsi
   hinsi_id
   bunrui
   bunrui_id
   katuyou1
   katuyou1_id
   katuyou2
   katuyou2_id
   imis
   fstring
   feature

第3引数 SUFFIX を取ることができるのは TYPE として feature を指定した場
合に限られます。

=item bnst_num()

文節数を返すメソッドです。

=item bnst( [ARG,TYPE,SUFFIX] )

構文解析結果の文節に関する情報を取り出すメソッドです。

Examples:

   $knp->bnst;
   # 引数が省略された場合は、文節情報のリストに対す
   # るリファレンスを返す。

   $knp->bnst( 1 );
   # ARG によって、何番目の文節の情報を返すかを指定
   # する。この場合は、1つ目の文節情報のハッシュに対
   # するリファレンスを返す。

   $knp->bnst( 2, 'fstring' );
   # TYPE によって必要な文節情報を指定する。この場合、
   # 2つ目の文節の全ての feature の文字列を返す。

   $knp->bnst( 3, 'feature', 4 );
   # 3つ目の文節の4個目の feature を返す。

TYPE として指定することができる文字列は次の通りです。

   start
   end
   parent
   dpndtype
   child
   fstring
   feature

第3引数 SUFFIX を取ることができるのは TYPE として feature を指定した場
合に限られます。

=back

=head1 NOTE

C<KNP> オブジェクトを直接参照することによって形態素情報や文節情報を得
ることもできます。その方法については、source を参照してください。しか
し、誤動作などを避けるため、出来るだけメソッド経由で情報を取り出してく
ださい。

=head1 AUTHORS

=over 4

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=item
黒橋 禎夫 <kuro@pine.kuee.kyoto-u.ac.jp>

=cut
