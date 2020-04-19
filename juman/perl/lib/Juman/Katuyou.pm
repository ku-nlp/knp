# $Id$
package Juman::Katuyou;
require 5.000;
use Juman::Grammar qw/ $FORM /;
use Juman::Hinsi qw/ get_form_id /;
use Encode;
use strict;
use vars qw/ $ENCODING /;

=head1 NAME

Juman::Katuyou - 形態素オブジェクトの活用形を操作する

=head1 DESCRIPTION

形態素オブジェクト L<Juman::Morpheme> の活用形を操作するメソッドを追加
する．

=head1 FILE

活用形辞書 F<JUMAN.katuyou.db> を参照する．この辞書は，Juman 本体に付
属している辞書 F<JUMAN.katuyou> から機械的に生成され，F<Katuyou.pm> と
同じディレクトリにインストールされているはずである．

なお，活用形辞書は BerkeleyDB 形式で作成され，L<DB_File> モジュールを
通じてアクセスされる．juman-perl のインストール時に L<DB_File> モジュー
ルが存在しないと，辞書の作成は行われないので，本モジュールで提供される
メソッドは利用できない．

=head1 METHODS

=over 4

=item kihonkei

形態素の基本形を返す．

=cut

$ENCODING = $JUMAN::ENCODING ? $JUMAN::ENCODING : 'utf8';

sub kihonkei {
    my( $this ) = @_;
    $this->change_katuyou2( '基本形' );
}

=item change_katuyou2 ( FORM )

指定された活用形 I<FORM> (基本形，命令形など)を持つ新たな形態素オブジェ
クトを返す．指定された活用形が存在しない場合は未定義値を返す．

=cut
sub change_katuyou2 {
    my( $this, $org_form ) = @_;
    my $form;
    if( utf8::is_utf8( $org_form ) ){
	$form = encode( $ENCODING, $org_form ); # 元のエンコーディング(従来はeuc-jp)にもどす
    }
    else{
	$form = $org_form;
    }

    my $type = $this->katuyou1;
    if( utf8::is_utf8( $type ) ){
	$type = encode( $ENCODING, $type ); # 元のエンコーディング(従来はeuc-jp)にもどす
    }

    my $id = &get_form_id( $type, $form );
    if( defined $id and $id > 0 ){
	# 変更先活用形が存在する場合
	my $new = &_dup( $this );
	my @oldgobi = @{ $FORM->{$type}->[$this->katuyou2_id] }; # 元のエンコーディング(従来はeuc-jp)でやりとり
	my @newgobi = @{ $FORM->{$type}->[$id] };
	if ( utf8::is_utf8( $this->midasi ) ){
	    map( { $_ = decode( $ENCODING, $_ ) } @oldgobi ); # encodeされてるならdecode
	    map( { $_ = decode( $ENCODING, $_ ) } @newgobi );
	}
	$new->{midasi} = &_change_gobi( $this->midasi, $oldgobi[1], $newgobi[1] );
	$new->{yomi}   = &_change_gobi( $this->yomi,
					( $oldgobi[2] || $oldgobi[1] ),
					( $newgobi[2] || $newgobi[1] ) );
	$new->{katuyou2} = $org_form; # もとのencodingで格納
	$new->{katuyou2_id} = $id;
	$new;
    } else {
	# 変更先活用形が存在しない場合
	undef;
    }
}

# 語尾を変化させる内部関数
sub _change_gobi {
    my( $str, $cut, $add ) = @_;

    unless( $cut eq '*' ){
	$str =~ s/$cut\Z//;
    }
    unless( $add eq '*' ){
	$str .= $add;
    }
    $str;
}

# 形態素オブジェクトを複製する内部関数
sub _dup {
    my( $this ) = @_;
    my $new = {};
    while( my( $key, $value ) = each %$this ){
	$new->{$key} = $value;
    }
    bless $new, ref $this;
}

1;

=back

=head1 SEE ALSO

=over 4

=item *

L<Juman::Morpheme>

=back

=head1 HISTORY

このモジュールは，L<KULM::Juman::Katuyou> モジュールを原形として作成さ
れた．

=head1 AUTHORS

=over 4

=item
佐藤 理史 <sato@i.kyoto-u.ac.jp>

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=cut

__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
