#! /usr/local/bin/jperl -- -*-Perl-*-

# def/sa-you.def »²¾È

$id_head = "Ì¾";

@name = ("£É£Ä", "ÆÉ¤ß", "É½µ­", "°ÕÌ£", "½ÒÁÇ", 
	 "³Ê£±", "³Ê£²", "³Ê£³", "³Ê£´", "³Ê£µ", 
	 "°Õ£±", "°Õ£²", "°Õ£³", "°Õ£´", "°Õ£µ",
	 "Îã£±", "Îã£²", "Îã£³", "Îã£´", "Îã£µ",
	 "ÂÖ£±", "ÂÖ£²", "ÂÖ£³", "ÂÖ£´", "ÂÖ£µ", "ÂÖ£¶", "ÂÖ£·");

open(TMP, "> tmp.tmp");

while(<STDIN>) {

    print TMP;

    chop;
    ($type, $data) = split(/\t/);

    if ($type eq "£±¶èÊ¬£Ó") {
	$code2 = substr($data, 8, 12); 
	$yomi = substr($data, 20, 20); 
	$yomi =~ s/ //g;
	(@item) = split(/ /, substr($data, 54));
	$item[2] =~ s/^[^=]+=\"|\"$//g;
	$item[2] =~ s/¡Ê[^¡Ë]+¡Ë//g;
	$item[2] =~ s/¡Î|¡Ï//g;
	$hyouki = $item[2];
	$item[3] =~ s/^[^=]+=\"|\"$//g;
	$imi = $item[3];
	$map{$code2} = "$yomi $hyouki $imi"; 
	# print "$map{$code2}\n";
    }
} 

close(TMP);

open(TMP, "tmp.tmp");

while(<TMP>) {

    chop;
    ($type, $data) = split(/\t/);

    if ($type eq "£Æ¥µÊÑÍÑË¡£Ó" &&
	substr($data, 40, 4) ne "£Ú  ") {

	if (!$map{substr($data, 8, 12)}) {
	    printf STDERR "$_\n"; 
	}

	($yomi, $hyouki, $imi) = split(/ /, $map{substr($data, 8, 12)});
	(@item) = split(/ /, substr($data, 54));
	$num = 0;

	# ID
	print "$name[$num++] $id_head$code2\n";
	# ÆÉ¤ß
	print "$name[$num++] $yomi\n";
	# É½µ­
	print "$name[$num++] $hyouki\n";
	# °ÕÌ£
	print "$name[$num++] $imi\n";
	# ½Ò¸ìÁÇ (¥µÊÑÌ¾»ì¤Ï¤Ê¤¤)
	print "$name[$num++] nil\n";
	# ³Ê¥Õ¥ì¡¼¥à
	for ($i = 0; $i < 3; $i++) {     # "¥ÎNP"¤ò¥¹¥­¥Ã¥×
	    for ($j = 0; $j < 4; $j++) { # ¥µÊÑÌ¾»ì¤ÏºÇÂç£´Í×ÁÇ
		$item[6+$i+$j*4] =~ s/^[^=]+=\"|\"$//g;
		if (!$item[6+$i+$j*4]) {
		    $item[6+$i+$j*4] = "nil";
		}
		printf "$name[$num++] $item[6+$i+$j*4]\n";
	    }
	    printf "$name[$num++] nil\n";
	}
	# ÂÖ (¥µÊÑÌ¾»ì¤Ï¤Ê¤¤)
	for ($i = 0; $i < 7; $i++) {
	    printf "$name[$num++] nil\n";
	}
    }
}

close(TMP);

system("rm tmp.tmp");
