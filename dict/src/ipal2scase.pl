#! /usr/local/bin/jperl -- -*-Perl-*-

# Conversion from "ipal?.dat" to "scase.dat"

# ¢§ ¥¬-¥¬¹½Ê¸¤ò°·¤¦¤¿¤á$frame¤ò11¥±¥¿¤ËÊÑ¹¹

$kaku_num{"¥¬"} = 1;
$kaku_num{"¥ò"} = 2;
$kaku_num{"¥Ë"} = 3;
$kaku_num{"¥Ç"} = 4;
$kaku_num{"¥«¥é"} = 5;
$kaku_num{"¥È"} = 6;
$kaku_num{"¥è¥ê"} = 7;
$kaku_num{"¥Ø"} = 8;
$kaku_num{"¥Þ¥Ç"} = 9;
$kaku_num{"¥Î"} = 10;
$kaku_num{"¥¬£²"} = 11;

$IPALV_FIELD_NUM = 27;

while (1) {

    $frame = "00000000000";
    @frame = split(//, $frame);
    for ($i = 0; $i < $IPALV_FIELD_NUM; $i++) {

	if (!($_ = <STDIN>)) {exit;}

	chop;
	($field_name, $data[$i]) = split;
	if ($field_name =~ /^³Ê/ && $data[$i] !~ /^nil$/) {
	    $data[$i] =~ s/¡ö//g;
	    foreach $item (split(/¡¿/, $data[$i])) {
		# ¥¬-¥¬¹½Ê¸¤Î°·¤¤
		if ($item eq "¥¬" && $frame[$kaku_num{"¥¬"}-1] == '1') {
		    $frame[$kaku_num{"¥¬£²"}-1] = '1';
		}

		if ($kaku_num{$item}) {
		    $frame[$kaku_num{$item}-1] = '1';
		}
	    }
	}
    }
    $frame = join('', @frame);

    printf "$data[1] $frame\n";			# ÆÉ¤ß¤ËÂÐ¤¹¤ë½ÐÎÏ
    foreach $item (split(/¡¤/, $data[2])) {
	printf "$item $frame\n";		# É½µ­¤ËÂÐ¤¹¤ë½ÐÎÏ
    }
}
