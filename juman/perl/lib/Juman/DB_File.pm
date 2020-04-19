# $Id$
package Juman::DB_File;
require 5.004_04; # For base pragma.
use English qw/ $PERL_VERSION /;
use POSIX qw/ O_CREAT O_RDONLY O_RDWR O_WRONLY /;
use strict;
use base qw/ DB_File /;
use vars qw/ @EXPORT /;
@EXPORT = qw/ O_CREAT O_RDONLY O_RDWR O_WRONLY /;

=head1 NAME

Juman::DB_File - Wrapper class of DB_File

=head1 SYNOPSIS

 use Juman::DB_File;
 use encoding "euc-jp";
 tie( %hash, 'Juman::DB_File', $dbfile, &O_CREAT ) or die;
 $hash{"添字"} = "値";
 while( my( $key, $value ) = each %hash ){
     print "$key:$value\n";
 }

=head1 DESCRIPTION

Perl-5.8.x は内部文字コードとして Unicode を採用している．そのため，日
本語 EUC で記述されたデータベースファイルを参照する場合には，添字や値
を書き込んだり，読み出したりする前に，常に明示的に encode/decode を行
う必要がある．

この C<Juman::DB_File> クラスは，特定の文字コードで保存されているデー
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
	*{Juman::DB_File::encode} = sub { $_[0]; };
	*{Juman::DB_File::decode} = sub { $_[0]; };
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

1;

=head1 SEE ALSO

=over 4

=item *

L<DB_File>

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
