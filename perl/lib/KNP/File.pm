package KNP::File;
require 5.000;
use English qw/ $PERL_VERSION /;
use IO::File;
use KNP::Result;
use POSIX qw/ SEEK_SET O_RDONLY O_CREAT /;
use strict;

=head1 NAME

KNP::File - 構文解析結果の格納されているファイルを操作するモジュール

=head1 SYNOPSIS

 $knp = new KNP::File( $file ) or die;
 while( $result = $knp->each() ){
     print $result->spec;
 }

=head1 CONSTRUCTOR

=over 4

=item new ( FILE )

構文解析結果の格納されているファイルを指定して，オブジェクトを初期化す
る．

=item new ( OPTIONS )

拡張オプションを指定してオブジェクトを初期化する．例えば，構文解析結果
ファイルに含まれている解析結果の文 ID のデータベースファイルを指定する
必要がある場合には，以下のように指定することができる．

  Example:

    $knp = new KNP::File( file => 'path_to_file',
                          dbfile => 'path_to_dbfile' );

=cut
sub new {
    my $class = shift;
    my %opt;
    if( @_ == 1 ){
	$opt{file} = shift;
    } else {
	while( @_ ){
	    my $key = shift;
	    my $val = shift;
	    $key =~ s/^-+//;
	    $opt{lc($key)} = $val;
	}
    }

    my $fh = new IO::File;
    if( ($opt{file} and $fh->open( $opt{file}, "r" )) or 
        (!$opt{file} and $fh->fdopen(fileno(STDIN), "r"))){ # read from STDIN
	if( $opt{encoding} ){
	    $fh->binmode( ":encoding($opt{encoding})" );
	} else {
	    &set_encoding( $fh );
	}
	my $new = { name    => $opt{file},
		    dbname  => $opt{dbfile}  || $opt{file}.'.db',
		    pattern => $opt{pattern} || $KNP::Result::DEFAULT{pattern},
		    bclass  => $opt{bclass}  || $KNP::Result::DEFAULT{bclass},
		    mclass  => $opt{mclass}  || $KNP::Result::DEFAULT{mclass},
		    tclass  => $opt{tclass}  || $KNP::Result::DEFAULT{tclass},
		    _file_handle => $fh };
	bless $new, $class;
    } else {
	undef;
    }
}

=back

=head1 METHODS

=over 4

=item name

参照しているファイルのファイル名を返す．

=cut
sub name {
    my( $this ) = @_;
    $this->{name};
}

=item each

格納されている構文解析結果を文を単位として順に返す．

=cut
sub each {
    my( $this ) = @_;
    my $pattern = $this->{pattern};
    my $fh = $this->{_file_handle};
    $this->setpos( 0 )
	unless $this->{_each_pos} and ( $this->getpos == $this->{_each_pos} );
    my @buf;
    while( <$fh> ){
	push( @buf, $_ );
	if( m!$pattern! ){
	    $this->{_each_pos} = $this->getpos;
	    return &_result( $this, \@buf );
	}
    }
    $this->{_each_pos} = 0;
    undef;
}

sub _result {
    my( $this, $spec ) = @_;
    KNP::Result->new( result  => $spec,
		      pattern => $this->{pattern},
		      bclass  => $this->{bclass},
		      mclass  => $this->{mclass},
		      tclass  => $this->{tclass} );
}

=item look

文 ID を指定して，構文解析結果を取り出す．

=cut
sub look {
    my( $this, $sid ) = @_;
    unless( $this->{_db} ){
	my %db;
	if( -f $this->dbname ){
	    require Juman::DB_File;
	    tie( %db, 'Juman::DB_File', $this->dbname, O_RDONLY ) or return undef;
	} else {
	    &_make_hash( $this, \%db );
	}
	$this->{_db} = \%db;
    }
    if( my $spec = $this->{_db}->{$sid} ){
	my( $pos, $len ) = split( /,/, $spec );
	$this->setpos( $pos );
	read( $this->{_file_handle}, $spec, $len );
	&_result( $this, $spec );
    } else {
	undef;
    }
}

=item makedb

ファイルに含まれている構文解析結果の文 ID のデータベースを作成する．

=cut
sub makedb {
    my( $this ) = @_;

    my %db;
    require Juman::DB_File;
    tie( %db, 'Juman::DB_File', $this->dbname, O_CREAT ) or return 0;
    &_make_hash( $this, \%db ) or return 0;
    untie %db;
    1;
}

# 文 ID の連想配列を作成する内部関数
sub _make_hash {
    my( $this, $hash ) = @_;

    %$hash = ();			# 連想配列を初期化
    $this->setpos( 0 ) or return 0;
    my $pos = 0;
    my $pattern = $this->{pattern};
    my $fh = $this->{_file_handle};

  OUTER:
    while (1) {
	my $len = 0;
	my $id;
	while( <$fh> ){
	    $len += length;
	    if( m!^# S-ID:([-A-z0-9]+)! ){
		$id = $1;
	    }elsif( m!$pattern! ){
		$id and $hash->{ $id } = sprintf( "%d,%d", $pos, $len );
		$pos = $this->getpos;
		next OUTER;
	    }
	}
	$this->{_each_pos} = 0;
	last;
    }
    1;
}

=item dbname

文 ID データベースのファイル名を返す．

=cut
sub dbname {
    my( $this ) = @_;
    $this->{dbname};
}

=item getpos

開いているファイルの現在のファイルポインタの位置を返す．

=cut
sub getpos {
    my( $this ) = @_;
    my $fh = $this->{_file_handle};
    $fh->tell;
}

=item setpos ( POS )

開いているファイルのファイルポインタの位置を C<POS> に移動する．成功時
には 1 を，失敗時には 0 を返す．

=cut
sub setpos {
    my( $this, $pos ) = @_;
    my $fh = $this->{_file_handle};
    $fh->seek( $pos, SEEK_SET );
}

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

=head1 SEE ALSO

=over 4

=item *

L<KNP::Result>

=back

=head1 AUTHOR

=over 4

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=cut

1;
