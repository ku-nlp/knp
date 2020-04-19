# $Id$
package Juman::GDBM_File;
require 5.000;
use Carp qw/ croak /;
use English qw/ $PERL_VERSION /;
use GDBM_File qw/ GDBM_WRCREAT GDBM_READER GDBM_WRITER /;
use POSIX qw/ O_CREAT O_RDONLY O_RDWR O_WRONLY /;
use strict;
use vars qw/ @EXPORT @ISA /;
@ISA = qw/ GDBM_File /;
@EXPORT = qw/ O_CREAT O_RDONLY O_RDWR O_WRONLY GDBM_WRCREAT GDBM_READER GDBM_WRITER /;

=head1 NAME

Juman::GDBM_File - Wrapper class of GDBM_File

=head1 SYNOPSIS

 use Juman::GDBM_File;
 use encoding "euc-jp";
 tie( %hash, 'Juman::GDBM_File', $dbfile, &GDBM_WRCREAT, 0640 ) or die;
 $hash{"添字"} = "値";
 while( my( $key, $value ) = each %hash ){
     print "$key:$value\n";
 }

=head1 DESCRIPTION

Perl-5.8.x は内部文字コードとして Unicode を採用している．そのため，日
本語 EUC で記述されたデータベースファイルを参照する場合には，添字や値
を書き込んだり，読み出したりする前に，常に明示的に encode/decode を行
う必要がある．

この C<Juman::GDBM_File> クラスは，特定の文字コードで保存されているデー
タベースファイルを扱うために，透過的に encode/decode を行う．

=head1 ENCODING

このクラスを利用する時は，データベースとの入出力時に使う文字コードを，
C<encoding> プラグマで指定する．C<encoding> プラグマによる指定が存在し
ない場合は，まったく変換を行わない．

=cut
BEGIN {
    if( $PERL_VERSION > 5.008 ){
	require Juman::Encode;
	Juman::Encode->import( qw/ encode decode / );
    } else {
	*{Juman::GDBM_File::encode} = sub { $_[0]; };
	*{Juman::GDBM_File::decode} = sub { $_[0]; };
    }
}

# データベースにアクセスするメソッドを上書きしている．必要なメソッドの
# 詳細については，perldoc perltie を参照．
sub FETCH {
    my( $this, $key ) = @_;
    &decode( $this->SUPER::FETCH( &encode( $key ) ) );
}

sub STORE {
    my( $this, $key, $value ) = @_;
    $this->SUPER::STORE( &encode( $key ), &encode( $value ) );
}

sub DELETE {
    my( $this, $key ) = @_;
    $this->SUPER::DELETE( &encode( $key ) );
}

sub EXISTS {
    my( $this, $key ) = @_;
    $this->SUPER::EXISTS( &encode( $key ) );
}

sub FIRSTKEY {
    my( $this ) = @_;
    &decode( $this->SUPER::FIRSTKEY() );
}

sub NEXTKEY {
    my( $this, $lastkey ) = @_;
    &decode( $this->SUPER::NEXTKEY( &encode( $lastkey ) ) );
}

=head1 CONSTRUCTOR

C<GDBM_File> と同一の書式で，連想配列を作成できる．

    tie( %hash, 'Juman::GDBM_File', $dbfile, &GDBM_WRCREAT, 0640 );

第5引数には，データベースファイルを新規作成する場合のファイル属性が指
定されている．

第5引数を省略した場合は，C<DB_File> 互換の書式が使われていると見なされ
る．

    tie( %hash, 'Juman::GDBM_File', $dbfile, &O_CREAT );

この場合，データベースファイルを開くモードを指定している第4引数には，
C<O_CREAT>, C<O_RDWR> など C<DB_File> 形式のデータベースを開く時と同じ
指定を使う．データベースファイルを新規作成する場合のファイル属性は，
C<umask> の返り値から自動的に算出される．

=cut
sub TIEHASH {
    my $class = shift;
    my $name  = shift;
    my $mode  = shift;
    my $permission;
    if( @_ ){ # GDBM_File style
	$permission = shift;
    } else {  # DB_File style
	if ( $mode == &O_CREAT ) {
	    $mode = &GDBM_WRCREAT;
	} elsif ( $mode == &O_RDONLY ) {
	    $mode = &GDBM_READER;
	} elsif ( $mode == &O_RDWR ) {
	    $mode = &GDBM_READER | &GDBM_WRITER;
	} elsif ( $mode == &O_WRONLY ) {
	    $mode = &GDBM_WRITER;
	} else {
	    croak "$class (TIEHASH): Unknown mode $mode is specified";
	}
	$permission = 0666 & (~umask);
    }
    my $new = GDBM_File->TIEHASH( $name, $mode, $permission );
    bless $new, $class;
}

1;

=head1 SEE ALSO

=over 4

=item *

L<GDBM_File>

=item *

L<perltie>

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
